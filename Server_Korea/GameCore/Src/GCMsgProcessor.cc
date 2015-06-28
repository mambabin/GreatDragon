#include "GCMsgProcessor.hpp"
#include "PlayerEntity.hpp"
#include "PlayerPool.hpp"
#include "Movement.hpp"
#include "Equipment.hpp"
#include "Fight.hpp"
#include "MapInfoManager.hpp"
#include "MapPool.hpp"
#include "ProfessionInfoManager.hpp"
#include "EquipmentInfoManager.hpp"
#include "GoodsInfoManager.hpp"
#include "SkillInfo.pb.h"
#include "SkillInfoManager.hpp"
#include "MissionInfo.hpp"
#include "MissionInfoManager.hpp"
#include "Mission.hpp"
#include "Time.hpp"
#include "GCAgent.hpp"
#include "MathUtil.hpp"
#include "NetProto.pb.h"
#include "DCProto.pb.h"
#include "PlayerInfo.pb.h"
#include "RoleAtt.hpp"
#include "ProfessionInfo.hpp"
#include "NPCEntity.hpp"
#include "NPCPool.hpp"
#include "Status.hpp"
#include "Debug.hpp"
#include "Item.hpp"
#include "BusinessInfoManager.hpp"
#include "AccountPool.hpp"
#include "ConnectionPool.hpp"
#include "GlobalMissionManager.hpp"
#include "AwardInfoManager.hpp"
#include "NPCInfoManager.hpp"
#include "Event.hpp"
#include "NetID.hpp"
#include "GMPool.hpp"
#include "FashionInfoManager.hpp"
#include "PlayOffManager.hpp"
#include "ScribeClient.hpp"
#include "FactionPool.hpp"
#include "DesignationInfoManager.hpp"
#include "GmSet.hpp"
#include <MD5.hpp>
#include <zmq.hpp>
#include <sys/types.h>
#include <string>
#include <map>
#include <set>
#include <string>
#include <climits>
#include <cstdio>
#include <cstring>

using namespace std;

static set<string> nameCache;

static bool ExtraDCMsgHeader(zmq::message_t *msg, u_int8_t *groupID, u_int8_t *unitID, void **data, size_t *size) {
	if (msg->size() < 2)
		return false;

	*groupID = *((u_int8_t *)msg->data());
	*unitID = *((u_int8_t *)msg->data() + 1);
	*data = (u_int8_t *)msg->data() + 2;
	*size = msg->size() - 2;

	return true;
}

void GCMsgProcessor_ProcessDCMsg(zmq::message_t *msg) {
	u_int8_t groupID = 0, unitID = 0;
	void *data = NULL;
	size_t size = 0;
	if (!ExtraDCMsgHeader(msg, &groupID, &unitID, &data, &size)) {
		DEBUG_LOGERROR("Failed to parse raw msg, len: %d", msg->size());
		return;
	}

	switch(groupID) {
		case DCProto_SaveRoleData::GROUPID:
			{
				switch(unitID) {
					case DCProto_DeleteRoleData::UNITID:
						{
							static DCProto_DeleteRoleData deleteRoleData;
							if (!deleteRoleData.ParseFromArray(data, size))
								return;

							PlayerInfo account;
							account.set_platform(deleteRoleData.platform());
							account.set_account(deleteRoleData.account());
							int32_t id = AccountPool_AccountToID(account);
							if (id == -1) {
								DEBUG_LOGERROR("Failed to get cached id and roleAtt, account: %s %s", account.platform().c_str(), account.account().c_str());
								break;
							}

							static NetProto_DeleteRole deleteRole;
							deleteRole.Clear();

							if (deleteRoleData.id() == -1) {
								DEBUG_LOGERROR("Failed to delete role, account: %s %s", account.platform().c_str(), account.account().c_str());
								deleteRole.set_id(-1);
								GCAgent_SendProtoToClients(&id, 1, &deleteRole);
								break;
							}

							DCProto_LoadRoleData *roles = AccountPool_AccountToRoleList(account);
							if (roles == NULL) {
								DEBUG_LOGERROR("Failed to get cached role list, account: %s %s", account.platform().c_str(), account.account().c_str());
								break;
							}

							for (int i = 0; i < roles->data_size(); i++) {
								if (roles->data(i).att().baseAtt().roleID() == deleteRoleData.id()) {
									if (i != roles->data_size() - 1) {
										for (int j = i; j < roles->data_size() - 1; j++) {
											roles->mutable_data()->SwapElements(j, j + 1);
										}
									}
									roles->mutable_data()->RemoveLast();

									deleteRole.set_id(i);
									GCAgent_SendProtoToClients(&id, 1, &deleteRole);
									break;
								}
							}
						}
						break;

					case DCProto_LoadRoleData::UNITID:
						{
							static DCProto_LoadRoleData loadRoleData;
							if (!loadRoleData.ParseFromArray(data, size))
								return;

							static NetProto_RoleList roleList;
							roleList.Clear();

							PlayerInfo account;
							account.set_account(loadRoleData.account());
							account.set_platform(loadRoleData.platform());
							int32_t id = AccountPool_AccountToID(account);
							if (id == -1) {
								DEBUG_LOGERROR("Failed to get cached id and roleAtt, account: %s", loadRoleData.account().c_str());
								break;
							}

							DCProto_LoadRoleData *cachedRoles = AccountPool_AccountToRoleList(account);
							if (cachedRoles == NULL || loadRoleData.data_size() == cachedRoles->data_size()) {
								AccountPool_AddRoleList(account, &loadRoleData);

								DEBUG_LOG("--------role list %d---------", loadRoleData.data_size());
								for (int i = 0; i < loadRoleData.data_size(); i++) {
									*roleList.add_roles() = loadRoleData.data(i);
									DEBUG_LOG("role %s", loadRoleData.data(i).att().baseAtt().name().c_str());
								}

								GCAgent_SendProtoToClients(&id, 1, &roleList);
							}
							else if (loadRoleData.data_size() == cachedRoles->data_size() + 1) {
								DEBUG_LOG("create role");
								// Create a new role.
								int index = loadRoleData.data_size() - 1;
								*roleList.add_roles() = loadRoleData.data(index);
								GCAgent_SendProtoToClients(&id, 1, &roleList);

								nameCache.erase(loadRoleData.data(index).att().baseAtt().name());
								DEBUG_LOG("erase name from cache");

								*cachedRoles = loadRoleData;
								PlayerPool_AddRoleInfoToRank(&loadRoleData.data(index));
							}
							else {
								// assert(0);
								DEBUG_LOGERROR("Failed to load role data, account: %s, cur: %d, total: %d", loadRoleData.account().c_str(), loadRoleData.data_size(), cachedRoles->data_size());
								break;
							}
						}
						break;

					case DCProto_CollectRole::UNITID:
						{
							DCProto_CollectRole collect;
							if (!collect.ParseFromArray(data, size))
								return;

							if (collect.cur() == -1) {
								DEBUG_LOGERROR("Failed to collect idle role");
								exit(EXIT_FAILURE);
							}

							DEBUG_LOGRECORD("end collect role");
							PlayerPool_SetIdles(collect.cur());
							PlayerPool_SetRestrictionRecords(&collect.Restriction());
							MapInfoManager_SetSingleRecord(&collect.singleRecord());
							PlayerPool_SynGodInfoTime(collect.GodInfoTime().arg1());
							PlayerPool_SetWinFactionRecord(&collect.winFactionInfo());
							PlayerPool_SetFactionRecords(&collect.factionInfo());
						}
						break;

					case DCProto_AddAccount::UNITID:
						{
							static DCProto_AddAccount aa;
							if (!aa.ParseFromArray(data, size))
								return;

							int32_t id = aa.id();

							// stress test
							/*
							   static NetProto_Register proto;
							   GCAgent_SendProtoToClients(&id, 1, &proto);
							   break;
							   */

							if (aa.res()) {
								static PlayerInfo playerInfo;
								playerInfo = aa.info();
								ScribeClient_SendMsg(ScribeClient_Format(&playerInfo, id, "RegistAccount"));
							}

							// only for local platform
							// if (aa.platform() != LOCAL_PLATFORM)
							// 	return;

							static NetProto_Register reg;
							reg.Clear();

							if (aa.res()) {
								reg.set_account(aa.info().account());
								reg.set_password(aa.info().password());
							}

							GCAgent_SendProtoToClients(&id, 1, &reg);
						}
						break;

					case DCProto_HasName::UNITID:
						{
							static DCProto_HasName hn;
							if (!hn.ParseFromArray(data, size))
								return;

							int32_t id = hn.id();
							if (hn.has()) {
								static NetProto_Error error;
								error.set_content(Config_Words(73));
								GCAgent_SendProtoToClients(&id, 1, &error);
								break;
							}

							const PlayerInfo *account = AccountPool_IDToAccount(id);
							if (account == NULL) {
								// DEBUG_LOGERROR("Failed to find cached account");
								break;
							}

							DCProto_LoadRoleData *roleList = AccountPool_AccountToRoleList(*account);
							if (roleList == NULL) {
								DEBUG_LOGERROR("Failed to find role list");
								break;
							}

							if (roleList->data_size() >= MAX_ROLE_PER_ACCOUNT)
								break;

							int cur = (int)Time_TimeStamp();

							static NetProto_ServerTime serverTime;
							serverTime.Clear();
							serverTime.set_time((int64_t)Time_ElapsedTime());
							serverTime.set_cur(cur);
							GCAgent_SendProtoToClients(&id, 1, &serverTime);

							const PlayerAtt *profession = ProfessionInfoManager_ProfessionInfo((ProfessionInfo::Type)hn.cr().type(), hn.cr().male());
							if (profession == NULL)
								break;

							PB_PlayerAtt roleAtt;
							profession->ToPB(&roleAtt);
							roleAtt.mutable_att()->mutable_baseAtt()->set_name(hn.cr().name().c_str());
							roleAtt.mutable_att()->mutable_baseAtt()->set_roleID(PlayerPool_GenRoleID());
							// roleAtt.mutable_att()->mutable_baseAtt()->set_passStory(true);
							const vector<MailGift> *mailGift = Config_MailGift(MailGift::CREATE_ROLE);
							if (mailGift != NULL) {
								for (size_t i = 0; i < mailGift->size(); i++) {
									PB_MailInfo mail = (*mailGift)[i].mail();
									if (mail.item().type() == PB_ItemInfo::EQUIPMENT) {
										const EquipmentInfo *equip = EquipmentInfoManager_EquipmentInfo(mail.item().id());
										assert(equip != NULL);
										int32_t newID = Item_AddEquip(roleAtt.mutable_itemPackage(), equip);
										if (newID == -1)
											continue;
										mail.mutable_item()->set_id(newID);
										mail.mutable_item()->set_count(1);
									}
									mail.set_time(cur);
									*roleAtt.mutable_mails(PlayerEntity_FindEmptyMail(&roleAtt)) = mail;
								}
							}
							roleAtt.set_firstLoginIP(NetID_IP(id));
							roleAtt.set_createTime(cur);
							roleAtt.set_lastLoginIP(NetID_IP(id));
							roleAtt.set_prevLogin(cur);
							roleAtt.set_prevLogout(cur);
							// playofftest
							/*
							   if (roleAtt.att().baseAtt().professionType() == PB_ProfessionInfo::KNIGHT) {
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(0, 244);
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(1, 245);
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(2, 246);
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(4, 247);
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(3, 248);
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(5, 249);
							   } else if (roleAtt.att().baseAtt().professionType() == PB_ProfessionInfo::RANGER) {
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(0, 250);
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(1, 251);
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(2, 252);
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(4, 253);
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(3, 254);
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(5, 255);
							   } else if (roleAtt.att().baseAtt().professionType() == PB_ProfessionInfo::MAGICIAN) {
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(0, 256);
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(1, 257);
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(2, 258);
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(4, 259);
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(3, 260);
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(5, 261);
							   }
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(6, 262);
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(7, 263);
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(8, 264);
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(9, 265);
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(10, 266);
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments(11, 267);
							   for (int i = 0; i < roleAtt.att().equipmentAtt().equipments_size(); i++) {
							   int64_t id = roleAtt.att().equipmentAtt().equipments(i);
							   if (id != -1) {
							   const EquipmentInfo *equip = EquipmentInfoManager_EquipmentInfo(id);
							   assert(equip != NULL);
							   id = Item_AddEquip(roleAtt.mutable_itemPackage(), equip);
							   roleAtt.mutable_att()->mutable_equipmentAtt()->set_equipments((PB_ProfessionInfo::Type)i, id);
							   }
							   }
							   */
							/*
							   roleAtt.mutable_att()->mutable_fightAtt()->set_level(100);
							   roleAtt.mutable_att()->mutable_fightAtt()->set_baseWingLevel(6);
							   roleAtt.mutable_att()->mutable_fightAtt()->set_baseWingDegree(12);
							   for (int i = 0; i < roleAtt.att().fightAtt().skills_size(); i++) {
							   int id = roleAtt.att().fightAtt().skills(i).id();
							   if (id != -1 && id != 0 && id != 8 && id != 16)
							   roleAtt.mutable_att()->mutable_fightAtt()->mutable_skills(i)->set_level(54);
							// roleAtt.mutable_att()->mutable_fightAtt()->mutable_skills(i)->set_level(1);
							}
							*/
							/*
							   for (int i = 0; i < roleAtt.passGuide_size(); i++)
							   roleAtt.set_passGuide(i, true);
							   */
							/*
							   roleAtt.mutable_itemPackage()->set_rmb(1000000);
							   roleAtt.mutable_itemPackage()->set_money(5000000);
							   roleAtt.mutable_itemPackage()->set_honor(100000);
							   */

							PB_ItemInfo itemInfo;
							itemInfo.set_type(PB_ItemInfo::GOODS);
							itemInfo.set_id(37);
							itemInfo.set_count(1);
							*roleAtt.mutable_itemPackage()->mutable_items(PB_ItemPackage::GOODS) = itemInfo;


							static DCProto_AddRole addRole;
							addRole.Clear();
							*addRole.mutable_info() = *account;
							*addRole.mutable_data() = roleAtt;
							GCAgent_SendProtoToDCAgent(&addRole);

							static DCProto_LoadRoleData loadRoleData;
							loadRoleData.set_account(account->account());
							loadRoleData.set_platform(account->platform());
							GCAgent_SendProtoToDCAgent(&loadRoleData);

							ScribeClient_SendMsg(ScribeClient_Format(account, id, "CreateRole"));

							DEBUG_LOG("insert name into cache");
						}
						break;

					case DCProto_Login::UNITID:
						{
							static DCProto_Login login;
							if (!login.ParseFromArray(data, size))
								break;

							static NetProto_LoginResult loginResult;
							loginResult.Clear();

							int32_t id = login.id();
							DEBUG_LOGERROR("gc recv dc login->%d, %d", id, login.res());
							if (login.res() < 0) {
								loginResult.set_id(login.res());
								GCAgent_SendProtoToClients(&id, 1, &loginResult);
								break;
							}

							if (NetID_FD(id) == -1)
							{
								DEBUG_LOGERROR("login invalid id->%d", id);
								break;
							}

							if (login.newUser() && login.beyond()) {
								PlayerPool_RoleFull(id);
								GCAgent_SendOrderToNetAgent(ORDER_CLOSE_CLIENT, id, 0);
								break;
							}

							if (login.login().platform() == "")
								login.mutable_login()->set_platform(LOCAL_PLATFORM);
							static PlayerInfo account;
							PlayerEntity_LoginToPlayerInfo(&login.login(), &account);

							int32_t prevID = AccountPool_AccountToID(account);
							if (prevID != -1) {
								int status = AccountPool_AccountStatus(account);
								if (status == ACCOUNT_ONLINE) {
									AccountPool_Logout(prevID);
								}
								else if (status == ACCOUNT_DROP) {
									AccountPool_Logout(prevID);
								}
								else {
									assert(0);
								}
							}

							loginResult.set_id(id);
							loginResult.set_serverOpenTime(Config_OpenTime(NULL));
							GCAgent_SendProtoToClients(&id, 1, &loginResult);
							DEBUG_LOG("send NetProto_LoginResult to client");

							AccountPool_AddAccountAndID(account, id);

							ScribeClient_SendMsg(ScribeClient_Format(&account, id, "Index"));

							if (login.useActivateKey()) {
								static char buf[CONFIG_FIXEDARRAY];
								SNPRINTF1(buf, "{\"key\":\"%s\"}", login.login().activateKey().c_str());
								ScribeClient_SendMsg(ScribeClient_Format(&account, id, "ActivateKey", buf));
							}

							/*
							static DCProto_RegistDeviceServer msg;
							msg.Clear();
							msg.set_deviceNum(account.deviceID());
							msg.set_id(id);
							msg.set_idfa(account.idfa());
							msg.set_noLine(false);
							GCAgent_SendProtoToDCAgent(&msg);
							*/

							static NetProto_ServerTime serverTime;
							serverTime.Clear();
							serverTime.set_time((int64_t)Time_ElapsedTime());
							serverTime.set_cur((int32_t)Time_TimeStamp());
							GCAgent_SendProtoToClients(&id, 1, &serverTime);
						}
						break;

					case DCProto_QueryRole::UNITID:
						{
							static DCProto_QueryRole queryRole;
							if (!queryRole.ParseFromArray(data, size))
								break;

							static NetProto_QueryPlayer queryPlayer;
							queryPlayer.Clear();

							int32_t id = queryRole.id();
							if (id >= -219 && id <= -200) {
								PlayerPool_ReservationQueryRole(&queryRole);
								break;
							}

							struct PlayerEntity *player = PlayerEntity_Player(id);
							if (player == NULL) {
								break;
							}
							DEBUG_LOG("TTTTTTTTTTTTTTTTTTTTT%lld, %s",queryRole.roleID(), queryRole.name().c_str());
							if (queryRole.roleID() == -1) {
								queryPlayer.set_roleID(-1);
								queryPlayer.mutable_att()->mutable_att()->mutable_baseAtt()->set_roleID(queryRole.att().att().baseAtt().roleID());
								GCAgent_SendProtoToClients(&id, 1, &queryPlayer);

								PlayerEntity_DelFriend(player, queryRole.att().att().baseAtt().roleID(), true);
								PlayerEntity_DelFriend(player, queryRole.att().att().baseAtt().roleID(), false);
							} else {
								queryPlayer.set_roleID(queryRole.roleID());
								PlayerEntity_ToSceneData(&queryRole.att(), queryPlayer.mutable_att());

								queryPlayer.set_online(false);
								GCAgent_SendProtoToClients(&id, 1, &queryPlayer);
							}
						}
						break;

					case DCProto_SendMail::UNITID:
						{
							static DCProto_SendMail dsm;
							if (!dsm.ParseFromArray(data, size))
								break;

							if (dsm.id() == -2) {
								PlayerPool_SendGodAward(false);
								break;
							} else if (dsm.id() == -3) {
								PlayerPool_SendWeekGodAward(false);
								break;
							} else if (dsm.id() == -4) {
								PlayerPool_SendActiveAward(NetProto_Rank::BLESSCOME, false);
								break;
							} else if (dsm.id() == -5) {
								PlayerPool_SendActiveAward(NetProto_Rank::LUCK, false);
								break;
							} else if (dsm.id() == -6) {
								PlayerPool_SendActiveAward(NetProto_Rank::CONSUME, false);
								break;
							} else if (dsm.id() == -7) {
								PlayerPool_SendPKAward(false);
								break;
							} else if (dsm.id() == -8) {
								PlayerPool_SendWeekPKAward(false);
								break;
							}

							struct PlayerEntity *entity = PlayerEntity_Player(dsm.id());
							if (entity == NULL) {
								break;
							}
							struct Component *component = PlayerEntity_Component(entity);

							static NetProto_SendMail sm;
							sm.Clear();
							sm = dsm.sm();
							if (sm.receiver() != -1) {
								Item_ModifyMoney(component->item, -Config_MailCost());
							}
							int32_t id = dsm.id();
							GCAgent_SendProtoToClients(&id, 1, &sm);
						}
						break;

					case DCProto_GetKeyGift::UNITID:
						{
							static DCProto_GetKeyGift dgkg;
							if (!dgkg.ParseFromArray(data, size))
								break;

							static NetProto_GetKeyGift gkg;
							gkg.Clear();
							gkg.set_key(dgkg.key());
							gkg.set_res(dgkg.res());

							int32_t id = dgkg.id();
							if (dgkg.res() >= 0) {
								struct PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);
								
								bool flag = true;
								for (int i = 0; i < dgkg.index_size(); i++) {
									if (component->playerAtt->fixedEvent(dgkg.index(i)) != dgkg.event(i)) {
										flag = false;
										break;
									}
								}

								if (flag) {
									break;
								}

								int group = dgkg.group();
								if(group < 0)
								{
									DEBUG_LOGERROR("get key gfit error group->%d", group);
									break;
								}
								int index = group / 32;
								if(index < 0 || index >= dgkg.index_size())
								{
									DEBUG_LOGERROR("get key gift error->%d, %d, %d", group, index, dgkg.index_size());
									break;
								}

								if(component->playerAtt->fixedEvent(dgkg.index(index)) & (1 << (group % 32)))
								{
									DEBUG_LOGERROR("get key gift geted key,  role:%lld", component->playerAtt->att().baseAtt().roleID());
									//GCAgent_SendProtoToClients(&id, 1, &gkg);
									break;
								}

								int32_t results[CONFIG_FIXEDARRAY];
								int count = Item_Lottery(component->item, dgkg.res(), results, CONFIG_FIXEDARRAY);
								if (count == -1) {
									gkg.set_res(-2);
								} else {
									for (int i = 0; i < dgkg.index_size(); i++)
										PlayerEntity_SetFixedEvent(entity, dgkg.index(i), dgkg.event(i));
									dgkg.set_done(true);
									GCAgent_SendProtoToDCAgent(&dgkg);

									/*
									   if (Config_ScribeHost() != NULL) {
									   static char buf[CONFIG_FIXEDARRAY];
									   SNPRINTF1(buf, "{\"key\":\"%s\",\"time\":\"%d\"", gkg.key().c_str(), (int)Time_TimeStamp());
									   ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "GetKeyGift", buf));
									   }
									   */
								}
							}

							GCAgent_SendProtoToClients(&id, 1, &gkg);

						}
						break;

					case DCProto_Recharge::UNITID:
						{
							static DCProto_Recharge dr;
							if (!dr.ParseFromArray(data, size))
								break;

							static NetProto_Recharge r;
							r.Clear();

							int32_t id = dr.id();
							struct PlayerEntity *entity = PlayerEntity_Player(id);
							if (entity == NULL)
								break;
							struct Component *component = PlayerEntity_Component(entity);

							if (component->baseAtt->roleID() != dr.roleID())
								break;

							if (dr.rmb() < 0) {
								// GCAgent_SendProtoToClients(&id, 1, &r);
								break;
							}

							int32_t v = 0;
							if (dr.info().platform() == "QQ"
									|| dr.info().platform() == "WX") {
								v = dr.rmb() - component->playerAtt->itemPackage().rmb();
								if (v <= 0)
									break;
								if (v > dr.recharge().v())
									v = dr.recharge().v();
							} else {
								v = dr.rmb();
								v *= Config_RMBToGem();
							}

							bool first = false;
							if (Config_DoubleRecharge(v) && !(component->playerAtt->fixedEvent(15) & (1 << Config_DoubleRechargeLevel(v)))) {
								first = true;
							}

							Item_ModifyRMB(component->item, v, true, -1, 0, 0);

							static NetProto_GetRes gr;
							gr.Clear();
							PB_ItemInfo *item = gr.add_items();
							item->set_type(PB_ItemInfo::RMB);
							item->set_count(v);
							if (first) {
								item = gr.add_items();
								item->set_type(PB_ItemInfo::SUBRMB);
								item->set_count(v);
							}
							GCAgent_SendProtoToClients(&id, 1, &gr);

							if (dr.info().platform() == "appstore"
									|| dr.info().platform() == "QQ"
									|| dr.info().platform() == "WX") {
								// nothing to do
							} else {
								//dr.set_over(true);
								//GCAgent_SendProtoToDCAgent(&dr);
							}

							ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "Payment"));
						}
						break;

					case DCProto_PlayerStatus::UNITID:
						{
							static DCProto_PlayerStatus ps;
							if (!ps.ParseFromArray(data, size))
								break;

							int32_t id = ps.id();
							struct PlayerEntity *entity = PlayerEntity_Player(id);
							if (entity == NULL)
								break;

							static NetProto_PlayerStatus playerStatus;
							playerStatus.Clear();
							playerStatus = ps.ps();
							GCAgent_SendProtoToClients(&id, 1, &playerStatus);
						}
						break;

					case DCProto_GodTarget::UNITID:
						{
							static DCProto_GodTarget gt;
							if (!gt.ParseFromArray(data, size))
								break;

							// TODO
							/*
							   int32_t id = gt.id();
							   struct PlayerEntity *entity = PlayerEntity_Player(id);
							   if (entity == NULL)
							   break;
							   struct Component *component = PlayerEntity_Component(entity);

							   if (!gt.res()) {
							   int range = Fight_PowerRank(component->fight);
							   if (range == -1)
							   break;

							   int64_t target = PlayerPool_RandomPower(range, PlayerEntity_RoleID(entity), Fight_Att(component->fight)->godTarget());
							   if (target == -1)
							   break;

							   gt.set_roleID(target);
							   GCAgent_SendProtoToDCAgent(&gt);
							   } else {
							   for (int i = 0; i < gt.att().att().fightAtt().skills_size(); i++) {
							   if (gt.att().att().fightAtt().skills(i).id() == -1) {
							   for (int j = 0; j < i / 2; j++) {
							   gt.mutable_att()->mutable_att()->mutable_fightAtt()->mutable_skills()->SwapElements(j, i - j - 1);
							   }
							   break;
							   }
							   }

							   Fight_SetGodTarget(component->fight, gt.roleID(), Fight_Power(gt.mutable_att()->mutable_att()->mutable_fightAtt()), gt.att().itemPackage().godScore());

							   static NetProto_RandomGodTarget rgt;
							   rgt.Clear();
							 *rgt.mutable_att() = gt.att();
							 rgt.set_event(gt.event());
							// playofftest
							rgt.mutable_att()->mutable_att()->mutable_fightAtt()->set_level(100);
							rgt.mutable_att()->mutable_att()->mutable_fightAtt()->set_properties(FightAtt::ATK, 50000);
							rgt.mutable_att()->mutable_att()->mutable_fightAtt()->set_properties(FightAtt::DEF, 30000);
							rgt.mutable_att()->mutable_att()->mutable_fightAtt()->set_properties(FightAtt::MAXHP, 1000000);
							rgt.mutable_att()->mutable_att()->mutable_fightAtt()->set_properties(FightAtt::ACCURACY, 10000);
							rgt.mutable_att()->mutable_att()->mutable_fightAtt()->set_properties(FightAtt::DODGE, 10000);
							rgt.mutable_att()->mutable_att()->mutable_fightAtt()->set_properties(FightAtt::CRIT, 5000);
							for (int i = 0; i < rgt.att().att().fightAtt().propertiesDelta_size(); i++)
							 *rgt.mutable_att()->mutable_att()->mutable_fightAtt()->mutable_propertiesDelta(i) = PB_FightPropertyDelta();
							 rgt.mutable_att()->mutable_att()->mutable_fightAtt()->set_hp(1000000);

							 GCAgent_SendProtoToClients(&id, 1, &rgt);
							 }
							 */
						}
						break;

					case DCProto_SaveGodRankInfoRecord::UNITID:
						{
							static DCProto_SaveGodRankInfoRecord proto;
							if (!proto.ParseFromArray(data, size))
								break;

							PlayerEntity_SaveOnlineGodRecord(&proto);
						}
						break;

					case DCProto_QueryGodRole::UNITID:
						{
							static DCProto_QueryGodRole proto;
							if (!proto.ParseFromArray(data, size))
								break;

							static NetProto_SelectGodRole queryPlayer;
							queryPlayer.Clear();

							int32_t id = proto.id();
							struct PlayerEntity *player = PlayerEntity_Player(id);
							if (player == NULL) {
								break;
							}

							if (proto.roleID() == -1) {
								queryPlayer.set_roleID(-1);
								GCAgent_SendProtoToClients(&id, 1, &queryPlayer);
							} else {
								queryPlayer.set_roleID(proto.roleID());
								PlayerEntity_ToSceneData(&proto.att(), queryPlayer.mutable_att());
								for (int i = 0; i < queryPlayer.att().att().equipmentAtt().equipments_size(); ++i) {
									DEBUG_LOG("DDDDDDDDDDDDDDDD%lld", queryPlayer.att().att().equipmentAtt().equipments(i));
								}
								for (int i = 0; i < queryPlayer.att().itemPackage().equips_size(); ++i) {
									DEBUG_LOG("WWWWWWWWWWWWWWWW%lld", queryPlayer.att().itemPackage().equips(i).mode());
								}
								GCAgent_SendProtoToClients(&id, 1, &queryPlayer);
							}

						}
						break;

					case DCProto_RandomRoles::UNITID:
						{
							static DCProto_RandomRoles proto;
							if (!proto.ParseFromArray(data, size))
								break;

							PlayerPool_UpdateFreeRoles(&proto.atts());
						}
						break;

					case DCProto_AddOutLineFriends::UNITID:
						{
							static DCProto_AddOutLineFriends proto;
							proto.Clear();
							if (!proto.ParseFromArray(data, size))
								break;

							PlayerEntity *player = PlayerEntity_PlayerByRoleID(proto.roleID1());
							if (player == NULL)
								break;

							PlayerEntity_AddOutLineFriend(player, proto.roleID2(), proto.name().c_str(), (ProfessionInfo::Type)proto.professionType());
						}
						break;

					case DCProto_LoadHireRoleDate::UNITID:
						{
							static DCProto_LoadHireRoleDate proto;
							proto.Clear();
							if (!proto.ParseFromArray(data, size))
								break;

							int id = proto.id();

							static NetProto_Hire hire;
							hire.Clear();

							struct PlayerEntity *entity = PlayerEntity_Player(id);
							if (entity == NULL)
								break;
							const PlayerAtt *roleAtt = PlayerEntity_Att(entity);
							if (roleAtt == NULL)
								break;

							static float percent[MAX_HIREPERSONNUMBER] = {1.1f, 1.05f, 1.01f, 0.95f, 0.9f};
							for (int i = 0; i < proto.atts_size() && i < MAX_HIREPERSONNUMBER; ++i) {
								int32_t power = Fight_Power(&proto.atts(i).att().fightAtt());
								if (!(power < proto.power() * 5 / 4 && power > proto.power() * 4 / 5 )) {
									for (int j = 0; j < roleAtt->att().fightAtt().properties_size(); j++) {
										proto.mutable_atts(i)->mutable_att()->mutable_fightAtt()->set_properties(j, (int)(roleAtt->att().fightAtt().properties(j) * percent[i]));
										PB_FightPropertyDelta *delta = proto.mutable_atts(i)->mutable_att()->mutable_fightAtt()->mutable_propertiesDelta(j);
										roleAtt->att().fightAtt().propertiesDelta(j).ToPB(delta);
										delta->set_delta((int)(delta->delta() * percent[i]));
									}
								}
								PlayerEntity_ToSceneData(&proto.atts(i), hire.add_att());
								power = Fight_Power(&proto.atts(i).att().fightAtt());
								PlayerEntity_SetHire(entity, i, power);
							}

							GCAgent_SendProtoToClients(&id, 1, &hire);
						}
						break;

					case DCProto_QueryRoleFaction::UNITID:
						{
							static DCProto_QueryRoleFaction proto;
							proto.Clear();
							if (!proto.ParseFromArray(data, size))
								break;

							PlayerPool_InitDevilRank(&proto);
						}
						break;

					case DCProto_FilterRecharge::UNITID:
						{
							static DCProto_FilterRecharge proto;
							if (!proto.ParseFromArray(data, size))
								break;

							int32_t id = proto.id();
							struct PlayerEntity *player = PlayerEntity_Player(id);
							if (player == NULL)
								break;
							struct Component *component = PlayerEntity_Component(player);

							if (component->baseAtt->roleID() != proto.roleID())
								break;

							const PlayerInfo *info = AccountPool_IDToAccount(id);
							if (info == NULL)
								break;

							static DCProto_Recharge dRecharge;
							dRecharge.Clear();
							dRecharge.set_id(id);
							dRecharge.set_roleID(component->baseAtt->roleID());
							dRecharge.set_over(false);
							*dRecharge.mutable_info() = *info;
							for (int i = 0; i < proto.recharge_size(); i++) {
								*dRecharge.mutable_recharge() = proto.recharge(i);
								GCAgent_SendProtoToDCAgent(&dRecharge);
							}
						}
						break;

					default:
						break;
				}
			}
			break;

		case DCProto_CollectEquipment::GROUPID:
			{
				switch(unitID) {
					default:
						break;
				}
			}
			break;

		case DCProto_GMLoadData::GROUPID:
			{
				switch(unitID) {
					case DCProto_GMLoadData::UNITID:
						{
							static DCProto_GMLoadData gmLoadData;
							gmLoadData.Clear();
							if (!gmLoadData.ParseFromArray(data, size)) {
								break;		
							}

							stForbid forbid;
							for (int i = 0; i < gmLoadData.gmData_size(); ++i) {
								forbid.id = gmLoadData.gmData(i).id();
								forbid.roleID = gmLoadData.gmData(i).roleID();
								forbid.name = gmLoadData.gmData(i).name();
								forbid.level = gmLoadData.gmData(i).level();
								forbid.profession = gmLoadData.gmData(i).profession();
								forbid.startTime = gmLoadData.gmData(i).startTime();
								forbid.endTime = gmLoadData.gmData(i).endTime();
								forbid.GM = gmLoadData.gmData(i).GM();
								forbid.flag = gmLoadData.gmData(i).flag();
								Event_AddGMInfo(forbid, false);
							}
						}
						break;

					case DCProto_GMPlayerQuery::UNITID:
						{
							static DCProto_GMPlayerQuery gmQuery;
							gmQuery.Clear();
							if (!gmQuery.ParseFromArray(data, size)) {
								break;
							}

							int id = GMPool_AccountIDByID(gmQuery.account());
							if (id == -1) {
								break;
							}
							static NetProto_GMPlayerQuery gmPlayerQuery;
							gmPlayerQuery.Clear();
							*gmPlayerQuery.mutable_att() = gmQuery.att();
							struct PlayerEntity *player = PlayerEntity_PlayerByRoleID(gmQuery.att().att().baseAtt().roleID());
							if (player != NULL) {
								gmPlayerQuery.set_online(true);
							}else {
								gmPlayerQuery.set_online(false);
							}
							GCAgent_SendProtoToClients(&id, 1, &gmPlayerQuery);
						}
						break;

					case DCProto_GMChatRecords::UNITID:
						{
							static DCProto_GMChatRecords proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							int id = proto.id();
							if (id == -1) {
								break;
							}

							GCAgent_SendProtoToClients(&id, 1, &proto.record());
						}
						break;

					case DCProto_GMRegistrCount::UNITID:
						{
							static DCProto_GMRegistrCount proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							int id = proto.id();
							if (id == -1) {
								break;
							}

							GCAgent_SendProtoToClients(&id, 1, &proto.record());
						}
						break;

					case DCProto_GMRoleCount::UNITID:
						{
							static DCProto_GMRoleCount proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							int id = proto.id();
							if (id == -1) {
								break;
							}

							DEBUG_LOGRECORD("recv DCProto_GMRoleCount, send NetProto_GMRoleCount to gm");
							GCAgent_SendProtoToClients(&id, 1, &proto.record());
						}
						break;

					case DCProto_GMLevelStatistics::UNITID:
						{
							static DCProto_GMLevelStatistics proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							int id = proto.id();
							if (id == -1) {
								break;
							}

							GCAgent_SendProtoToClients(&id, 1, &proto.record());
						}
						break;

					case DCProto_LoadAllDataFromGMDataTable::UNITID:
						{
							static DCProto_LoadAllDataFromGMDataTable  proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}
							Event_AddGMData(&proto, -1);
						}
						break;

					case DCProto_LoadRekooRole::UNITID:
						{
							static DCProto_LoadRekooRole  proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							for (int i = 0; i < proto.roleID_size(); ++i) {
								PlayerPool_AddRekooRole(proto.roleID(i));
							}
						}
						break;

					case DCProto_LoadInviteCode::UNITID:
						{
							static DCProto_LoadInviteCode  proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							PlayerPool_InitInviteCode(&proto);
						}
						break;

					case DCProto_QueryGMAccount::UNITID:
						{
							static DCProto_QueryGMAccount  proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							int id = proto.id();
							if(proto.gm().permission() == NetProto_GMLogin::CP || proto.gm().permission() == NetProto_GMLogin::YUNYING || proto.gm().permission() == NetProto_GMLogin::CUSTOM) {
								DCProto_LoadAllDataFromGMDataTable message;
								DCProto_GMInfo *gmInfo = message.add_info();
								gmInfo->set_key(-2);
								gmInfo->set_arg1((int)proto.gm().permission());
								gmInfo->set_str1(proto.gm().account().c_str());
								gmInfo->set_str2(proto.gm().passwd().c_str());

								Event_AddGMData(&message, 3);
								GMPool_Add(id, proto.gm().account());
							}else {
								proto.mutable_gm()->clear_account();
								proto.mutable_gm()->clear_passwd();
							}
							GCAgent_SendProtoToClients(&id, 1, &proto.gm());
						}
						break;

					default:
						break;
				}
			}
			break;

		case DCProto_RegistDeviceServer::GROUPID:
			{
				switch(unitID) {
					case DCProto_RegistDeviceServer::UNITID: 
						{
							static DCProto_RegistDeviceServer regist;
							regist.Clear();

							if (!regist.ParseFromArray(data, size)) {
								break;
							}

							int32_t id = regist.id();

							if (regist.noLine()) {
								static PlayerInfo playerInfo;
								playerInfo.set_deviceID(regist.deviceNum());
								playerInfo.set_idfa(regist.idfa());
								ScribeClient_SendMsg(ScribeClient_Format(&playerInfo, id, "RegistDevice"));
							} else {
								static PlayerInfo playerInfo;
								playerInfo.set_deviceID(regist.deviceNum());
								playerInfo.set_idfa(regist.idfa());
								ScribeClient_SendMsg(ScribeClient_Format(&playerInfo, id, "RegistDeviceServer"));
							}
						}
						break;

					default:
						break;
				}
				break;
			}

		case DCProto_FactionLoadData::GROUPID:
			{
				switch(unitID) {
					case DCProto_FactionLoadData::UNITID:
						{
							static DCProto_FactionLoadData proto;
							proto.Clear();
							if (!proto.ParseFromArray(data, size)) {
								break;		
							}
							FactionPool_Init(&proto);
						}
						break;

					default:
						break;
				}
			}
			break;

		case DCProto_InitRank::GROUPID:
			{
				switch(unitID) {
					case DCProto_InitRank::UNITID:
						{
							static DCProto_InitRank proto;
							proto.Clear();
							if (!proto.ParseFromArray(data, size)) {
								break;		
							}

							if (proto.type() == NetProto_Rank::WORLD_BOSS && proto.flag() == true) {
								PlayerPool_WorldBossRank(&proto);
							} else {
								PlayerPool_InitRank(&proto);
							}
						}
						break;

					case DCProto_PingPongAward::UNITID:
						{
							static DCProto_PingPongAward proto;
							if (!proto.ParseFromArray(data, size))
								break;
							if (proto.type() == NetProto_Rank::PVPSCORE) {
								PlayerPool_ResetPvpScore(false);
							}else {
								PlayerPool_SendActiveAward(proto.type(), false);
							}
						}
						break;

					case DCProto_FactionPower::UNITID:
						{
							static DCProto_FactionPower proto;
							proto.Clear();
							if (!proto.ParseFromArray(data, size)) {
								break;		
							}

							PlayerPool_FactionPower(&proto);

						}
						break;

					default:
						break;
				}
			}
			break;

		default:
			break;
	}
}

static bool ProcessLocalOrder(zmq::message_t *msg) {
	if (msg->size() < 1)
		return false;

	u_int8_t order = *(u_int8_t *)msg->data();
	switch(order) {
		case ORDER_CLOSE_CLIENT: // Close client
			{
				if (msg->size() != 5)
					break;

				int32_t id = *(int32_t *)((int8_t *)msg->data() + 1);
				ConnectionPool_Del(id);

				struct PlayerEntity *player = PlayerEntity_Player(id);
				if (player == NULL) {
					AccountPool_Logout(id);
					GMPool_Del(id);
				} else {
					// no drop
					/*
					   struct Component *component = PlayerEntity_Component(player);
					   if (Movement_InSingle(component->movement)) {
					   AccountPool_Drop(id);
					   return true;
					   }
					   */
					AccountPool_Logout(id);
				}
				return true;
			}
			break;

		case ORDER_NEW_CLIENT:
			{
				if (msg->size() != 5)
					break;

				int32_t id = *(int32_t *)((int8_t *)msg->data() + 1);
				ConnectionPool_Add(id);
				return true;
			}
			break;

		default:
			break;
	}

	return false;
}

static bool InSingle(int32_t id) {
	struct PlayerEntity *player = PlayerEntity_Player(id);
	if (player == NULL)
		return false;
	struct Component *component = PlayerEntity_Component(player);
	return Movement_InSingle(component->movement);
}

static bool ExtraNetMsgHeader(zmq::message_t *msg, int32_t *id, u_int8_t *groupID, u_int8_t *unitID, void **data, size_t *size) {
	if (msg->size() < 6)
		return false;

	*id = *(int32_t *)msg->data();
	*groupID = *((u_int8_t *)msg->data() + 4);
	*unitID = *((u_int8_t *)msg->data() + 5);
	*data = (u_int8_t *)msg->data() + 6;
	*size = msg->size() - 6;

	return true;
}

void GCMsgProcessor_ProcessNetMsg(zmq::message_t *msg) {
	if (ProcessLocalOrder(msg))
		return;

	int32_t id = -1;
	u_int8_t groupID = 0, unitID = 0;
	void *data = NULL;
	size_t size = 0;
	if (!ExtraNetMsgHeader(msg, &id, &groupID, &unitID, &data, &size)) {
		DEBUG_LOGERROR("Failed to parse raw msg, len: %d", msg->size());
		return;
	}

	if (Event_ForbidMessage(groupID, unitID)) {
		return;	
	}

	PlayerEntity *player = PlayerEntity_Player(id);
	if(player != NULL)
	{
		if(PlayerEntity_AddMsgSt(player, groupID, unitID) > 0)
			//DEBUG_LOGERROR("invalid msg->%d, %d, %ld", groupID, unitID, player->att.att().baseAtt().roleID());
			return;
	}

	switch(groupID) {
		case NetProto_ClientException::GROUPID:
			{
				switch(unitID) {
					case NetProto_ClientException::UNITID:
						{
							static NetProto_ClientException err;
							if (!err.ParseFromArray(data, size))
								return;

							DEBUG_LOGERROR("-----------ClientException----------------");
							DEBUG_LOGERROR("%s", err.type().c_str());
							DEBUG_LOGERROR("%s", err.output().c_str());
							DEBUG_LOGERROR("%s", err.stackTrace().c_str());
						}
						break;

					default:
						break;
				}
			}
			break;

		case NetProto_Heartbeat::GROUPID:
			{
				switch(unitID) {
					case NetProto_Heartbeat::UNITID:
						{
							static NetProto_Heartbeat heartbeat;
							if (!heartbeat.ParseFromArray(data, size))
								return;

							ConnectionPool_DoHeartbeat(id);
						}
						break;

					case NetProto_Login::UNITID:
						{
							static NetProto_Login login;
							if (!login.ParseFromArray(data, size))
								break;

							time_t cur = Time_TimeStamp();
							char *openTimeStr = NULL;
							if ((int)cur < Config_OpenTime(&openTimeStr)) {
								if (!Config_InWhiteList(login.account().c_str())) {
									static NetProto_Error error;
									static char buf[CONFIG_FIXEDARRAY];
									SNPRINTF1(buf, Config_Words(51), openTimeStr);
									error.set_content(buf);
									GCAgent_SendProtoToClients(&id, 1, &error);
									return;
								}
							}

							//if (login.version() != Config_Version()) {
							if (login.version() < Config_Version()) {
								const char * url = Config_ChannelURL(login.platform().c_str());
								if (url != NULL) {
									static NetProto_LoginURL loginurl;
									loginurl.set_url(url);
									loginurl.set_result(1);
									GCAgent_SendProtoToClients(&id, 1, &loginurl);
								} else {
									static NetProto_LoginURL loginurl;
									loginurl.set_result(0);
									GCAgent_SendProtoToClients(&id, 1, &loginurl);
								}

								static NetProto_Error error;
								error.set_content(Config_Words(71));
								GCAgent_SendProtoToClients(&id, 1, &error);
								break;
							} else if (login.version() > Config_Version()) {
								static NetProto_LoginURL loginurl;
								loginurl.set_result(2);
								GCAgent_SendProtoToClients(&id, 1, &loginurl);
								break;
							}

							if (login.platform().empty())
								login.set_platform(LOCAL_PLATFORM);
							if (!Config_HasPlatform(login.platform().c_str())) {
								static NetProto_Error error;
								static char buf[CONFIG_FIXEDARRAY];
								// sprintf(buf, Config_Words(43), login.platform().c_str());
								strcpy(buf, Config_Words(93));
								error.set_content(buf);
								GCAgent_SendProtoToClients(&id, 1, &error);
								break;
							}


							if (!login.loginLater() && PlayerPool_TotalCount() >= Config_MaxOnlinePlayers()) {
								static NetProto_LoginLaterTime loginLaterTime;
								loginLaterTime.Clear();
								loginLaterTime.set_time(PlayerPool_LoginLaterTime());
								GCAgent_SendProtoToClients(&id, 1, &loginLaterTime);
								GCAgent_SendOrderToNetAgent(ORDER_CLOSE_CLIENT, id, 0);
								break;
							}

							/*
							   static DCProto_RegistDeviceServer msg;
							   msg.Clear();
							   msg.set_deviceNum(login.deviceID());
							   msg.set_id(id);
							   msg.set_idfa(login.idfa());
							   msg.set_noLine(true);
							   msg.set_time((int)Time_TimeStamp() - Time_Random(20, 60));
							   GCAgent_SendProtoToDCAgent(&msg);
							   */

							static DCProto_Login dcLogin;
							dcLogin.Clear();
							*dcLogin.mutable_login() = login;
							dcLogin.set_id(id);
							dcLogin.set_ip(NetID_IP(id));
							dcLogin.set_beyond(PlayerPool_RoleFull(-1));
							GCAgent_SendProtoToDCAgent(&dcLogin);
							DEBUG_LOG("send DCProto_Login to DC");
							DEBUG_LOG("QQQQQQQQQQQQQ:::%s", login.platform().c_str());
							break;
						}
						break;

							case NetProto_Logout::UNITID:
						{
							static NetProto_Logout logout;
							if (!logout.ParseFromArray(data, size))
								break;

							DEBUG_LOG("Recv logout");
							AccountPool_Logout(id);
						}
						break;

							case NetProto_CreateRole::UNITID:
						{
							static NetProto_CreateRole createRole;
							if (!createRole.ParseFromArray(data, size))
								break;

							const char tokens[] = {',', ';', '.', ':', '\"', '\'', '\?', '/', '`', '!', '~'};
							size_t pos;
							bool flag = false;
							for (size_t i = 0; i < sizeof(tokens) / sizeof(char); ++i) {
								pos = createRole.name().find(tokens[i]);
								if (pos != std::string::npos) {
									static NetProto_Error error;
									error.Clear();
									static char buf[CONFIG_FIXEDARRAY];
									SNPRINTF1(buf, Config_Words(69), tokens[i]);
									error.set_content(buf);
									GCAgent_SendProtoToClients(&id, 1, &error);
									flag = true;
									break;
								}
							}

							if(flag) {
								static NetProto_Error error;
								error.set_content(Config_Words(74));
								GCAgent_SendProtoToClients(&id, 1, &error);
								break;
							}

							if (nameCache.find(createRole.name()) != nameCache.end()) {
								static NetProto_Error error;
								error.set_content(Config_Words(73));
								GCAgent_SendProtoToClients(&id, 1, &error);
								break;
							}
							nameCache.insert(createRole.name());

							static DCProto_HasName hn;
							hn.Clear();
							*hn.mutable_cr() = createRole;
							hn.set_id(id);
							GCAgent_SendProtoToDCAgent(&hn);
						}
						break;

							case NetProto_ServerTime::UNITID:
						{
							static NetProto_ServerTime proto;
							if (!proto.ParseFromArray(data, size))
								break;

							proto.set_time(Time_ElapsedTime());
							proto.set_cur((int32_t)Time_TimeStamp());
							GCAgent_SendProtoToClients(&id, 1, &proto);
						}
						break;

							case NetProto_SelectRole::UNITID:
						{
							static NetProto_SelectRole selectRole;
							if (!selectRole.ParseFromArray(data, size))
								break;
							const PlayerInfo *account = AccountPool_IDToAccount(id);
							if (account == NULL) {
								DEBUG_LOGERROR("Failed to get cached account");
								break;
							}

							DCProto_LoadRoleData *roles = AccountPool_AccountToRoleList(*account);
							if (roles == NULL) {
								// DEBUG_LOGERROR("Failed to get cached role list, account: %s", account->c_str());
								break;
							}

							DEBUG_LOG("%d  ---  %d ---- %d ", selectRole.id(), roles->data_size());
							if (selectRole.id() < 0 || selectRole.id() >= roles->data_size()) {
								DEBUG_LOGERROR("Failed to select role->%d, %d", selectRole.id(), roles->data_size());
								break;
							}

							static NetProto_AddPlayers addPlayers;
							addPlayers.Clear();
							addPlayers.add_id(id);
							PB_PlayerAtt *pb_roleAtt = addPlayers.add_att();
							*pb_roleAtt = roles->data(selectRole.id());

							{
								// faction
								FactionPool_Signin(pb_roleAtt->att().baseAtt().roleID(), true);
								//DEBUG_LOG("KKKKKKKK::%s, %s", pb_roleAtt->faction().c_str(), pb_roleAtt->att().baseAtt().name().c_str());
								int res = FactionPool_Check(pb_roleAtt->att().baseAtt().roleID(), pb_roleAtt->faction().c_str());
								if (res == 0) {
									//PlayerEntity_SysFaction(entity, "");
									pb_roleAtt->set_faction("");
								}
								//DEBUG_LOG("KKKKKKKK::%s, %s", pb_roleAtt->faction().c_str(), pb_roleAtt->att().baseAtt().name().c_str());
							}

							if (Event_IsFreeze(pb_roleAtt->att().baseAtt().roleID())) {
								static NetProto_Error error;
								error.set_content(Config_Words(40));
								GCAgent_SendProtoToClients(&id, 1, &error);
								break;
							}

							if (pb_roleAtt->att().movementAtt().mapID() != MAP_CREATE)
								pb_roleAtt->mutable_att()->mutable_movementAtt()->set_mapID(MAP_CHANGING);

							pb_roleAtt->mutable_att()->mutable_baseAtt()->set_passStory(true);
							pb_roleAtt->mutable_att()->mutable_movementAtt()->set_status(PB_MovementAtt::IDLE);
							pb_roleAtt->mutable_att()->mutable_fightAtt()->set_status(PB_FightAtt::IDLE);
							// pb_roleAtt->mutable_itemPackage()->set_rmb(10000000);
							// pb_roleAtt->mutable_itemPackage()->set_money(10000000);
							// pb_roleAtt->mutable_itemPackage()->set_honor(10000000);
							const PlayerAtt *profession = ProfessionInfoManager_ProfessionInfo((ProfessionInfo::Type)pb_roleAtt->att().baseAtt().professionType(), pb_roleAtt->att().baseAtt().male());
							assert(profession != NULL);
							pb_roleAtt->mutable_att()->mutable_fightAtt()->set_selfFaction(profession->att().fightAtt().selfFaction());
							pb_roleAtt->mutable_att()->mutable_fightAtt()->set_friendlyFaction(profession->att().fightAtt().friendlyFaction());
							pb_roleAtt->mutable_att()->mutable_baseAtt()->set_passStory(true);
							// playoff
							// pb_roleAtt->mutable_itemPackage()->set_vip(0);
							// for (int i = 0; i < pb_roleAtt->passGuide_size(); i++)
							// 	pb_roleAtt->set_passGuide(i, 0x7fffffff);

							struct PlayerEntity *entity = PlayerEntity_Create(id, pb_roleAtt);
							if (entity == NULL) {
								// DEBUG_LOGERROR("Failed to create player");
								break;
							}

							if (pb_roleAtt->selfcode() == "") {
								const char * invateCode = PlayerEntity_SelfInviteCode(pb_roleAtt->att().baseAtt().roleID());
								if (invateCode != NULL) {
									pb_roleAtt->set_selfcode(invateCode);
									PlayerPool_SysInvateCodeInfo(pb_roleAtt->att().baseAtt().roleID(), invateCode);
								}else {
									DEBUG_LOG("ERROR NULL");
								}
							}

							{
								PlayerPool_GroupPurchase(pb_roleAtt->att().baseAtt().roleID(), NULL);
							}

							{
								struct Component *component = PlayerEntity_Component(entity);
								if (account->platform() == "QQ"
										|| account->platform() == "WX") {
									static DCProto_Recharge dRecharge;
									dRecharge.Clear();
									dRecharge.mutable_recharge()->set_v(INT_MAX);
									dRecharge.set_id(id);
									dRecharge.set_roleID(component->baseAtt->roleID());
									dRecharge.set_over(false);
									*dRecharge.mutable_info() = *account;
									GCAgent_SendProtoToDCAgent(&dRecharge);
								} else {
									static DCProto_FilterRecharge proto;
									proto.set_id(id);
									*proto.mutable_info() = *account;
									proto.set_roleID(component->baseAtt->roleID());
									GCAgent_SendProtoToDCAgent(&proto);
								}
							}

							{
								//active 2
								const multimap<int, int64_t>* fixedMap = Config_fixedEventMap();
								assert(fixedMap != NULL);
								multimap<int, int64_t>::const_iterator begin = fixedMap->lower_bound(-2);
								if (begin == fixedMap->end()) {
									PlayerEntity_SetFixedEvent(entity, 33, 0);
								} else {
									struct Component *component = PlayerEntity_Component(entity);
									if (component->playerAtt->fixedEvent(38) < Time_TimeStamp()) {
										PlayerEntity_SetFixedEvent(entity, 33, 0);
									}
								}
							}

							{
								//active 3
								PlayerEntity_ActiveStrongeSolider(entity);
							}	

							PlayerEntity_ActiveStrongeSolider(entity);
							GlobalMissionManager_ApplyLoginMission(PlayerEntity_Component(entity)->movement);

							const PlayerAtt *roleAtt = PlayerEntity_Att(entity);
							roleAtt->ToPB(pb_roleAtt);

							int fashionID = roleAtt->att().equipmentAtt().fashion();
							Component* component = PlayerEntity_Component(entity);
							if (NULL != component) {
								if (Equipment_AddStatusToBody(component->equipment, fashionID, StatusInfo::ROOM_EXP)) {
									DEBUG_LOG("Add role status ok, fashionID =  %d", fashionID);
								}
							}

							{
								RecordInfo info;
								int64_t level = (int64_t)roleAtt->att().fightAtt().level();
								static PB_FightAtt fightAtt;
								roleAtt->att().fightAtt().ToPB(&fightAtt);
								int64_t power = (int64_t)Fight_Power(&fightAtt);
								int64_t value = (level << 32) + power; 
								info.set_arg1(value);
								info.mutable_role()->set_roleID(roleAtt->att().baseAtt().roleID());
								info.mutable_role()->set_name(roleAtt->att().baseAtt().name());
								info.mutable_role()->set_professionType((PB_ProfessionInfo::Type)roleAtt->att().baseAtt().professionType());

								PlayerPool_AddGodRank(roleAtt->att().baseAtt().roleID(), &info);
							}

							DEBUG_LOG("Select role, id: %lld, name: %s", roleAtt->att().baseAtt().roleID(), roleAtt->att().baseAtt().name());
							DEBUG_LOG("Total roles: %d", PlayerPool_TotalCount());

							GCAgent_SendProtoToClients(&id, 1, &addPlayers);

							int vip = component->playerAtt->itemPackage().vip();
							int rmb = component->playerAtt->itemPackage().rmb();
							if (vip > 0) {
								if (Config_ScribeHost() != NULL) {
									static char buf[CONFIG_FIXEDARRAY];
									memset(buf, 0, sizeof(buf) / sizeof(char));
									char *index = buf;

									SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)roleAtt->att().baseAtt().roleID());
									index += strlen(index);
									SNPRINTF2(buf, index, "\",\"roleName\":\"%s", roleAtt->att().baseAtt().name());
									index += strlen(index);
									SNPRINTF2(buf, index, "\",\"roleVipLevel\":\"%d", vip);
									index += strlen(index);
									SNPRINTF2(buf, index, "}");
									ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "RoleVipLevel", buf));
								}
							}
							if (rmb > 0) {
								if (Config_ScribeHost() != NULL) {
									static char buf[CONFIG_FIXEDARRAY];
									memset(buf, 0, sizeof(buf) / sizeof(char));
									char *index = buf;

									SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)roleAtt->att().baseAtt().roleID());
									index += strlen(index);
									SNPRINTF2(buf, index, "\",\"roleName\":\"%s", roleAtt->att().baseAtt().name());
									index += strlen(index);
									SNPRINTF2(buf, index, "\",\"roleRmb\":\"%d", rmb);
									index += strlen(index);
									SNPRINTF2(buf, index, "}");
									ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "RoleRmb", buf));
								}
							}
							if (roleAtt->att().movementAtt().mapID() == MAP_CREATE) {
								int32_t nextMap = MAP_FIRSTROOM;
								Vector2i nextCoord;
								bool res = MapInfoManager_EnterCoord(MapInfoManager_MapInfo(nextMap), 0, &nextCoord);
								assert(res);

								Movement_BeginChangeScene(PlayerEntity_Component(entity)->movement, nextMap, &nextCoord, 0);
							} else if (roleAtt->att().movementAtt().mapID() == MAP_CHANGING) {
								int32_t nextMap = roleAtt->att().movementAtt().prevNormalMap();
								if (nextMap < MAP_START) {
									DEBUG_LOGERROR("Map of role: %lld is wrong, cur map: %d", roleAtt->att().baseAtt().roleID(), nextMap);

									Vector2i nextCoord;
									nextMap = MapInfoManager_Next(MapPool_MapInfo(MAP_CREATE), 0, &nextCoord);
									assert(nextMap != -1);

									Movement_BeginChangeScene(PlayerEntity_Component(entity)->movement, nextMap, &nextCoord, 0);
								}
								else {
									Movement_BeginChangeScene(PlayerEntity_Component(entity)->movement, nextMap, &roleAtt->att().movementAtt().prevCoord());
								}
							} else {
								DEBUG_LOG("server logout");
								assert(0);
							}
						}
						break;

							case NetProto_RoleList::UNITID:
						{
							const PlayerInfo *account = AccountPool_IDToAccount(id);
							if (account == NULL) {
								DEBUG_LOGERROR("Failed to read cached account");
								break;
							}

							static DCProto_LoadRoleData loadRoleData;
							loadRoleData.set_account(account->account());
							loadRoleData.set_platform(account->platform());
							GCAgent_SendProtoToDCAgent(&loadRoleData);
							DEBUG_LOG("send DCProto_LoadRoleData to DC");
						}
						break;

						/*
						   case NetProto_DeleteRole::UNITID:
						   {
						   static NetProto_DeleteRole deleteRole;
						   if (!deleteRole.ParseFromArray(data, size))
						   break;

						   const PlayerInfo *account = AccountPool_IDToAccount(id);
						   if (account == NULL) {
						   DEBUG_LOGERROR("Failed to get cached account");
						   break;
						   }

						   DCProto_LoadRoleData *roles = AccountPool_AccountToRoleList(*account);
						   if (roles == NULL) {
						   DEBUG_LOGERROR("Failed to get cached role list, account: %s %s", account->platform().c_str(), account->account().c_str());
						   break;
						   }

						   if (deleteRole.id() < 0 || deleteRole.id() >= roles->data_size()) {
						   DEBUG_LOGERROR("Failed to delete role");
						   break;
						   }

						   static DCProto_DeleteRoleData deleteRoleData;
						   deleteRoleData.Clear();
						   deleteRoleData.set_account(account->account());
						   deleteRoleData.set_platform(account->platform());

						   const PB_PlayerAtt *att = &roles->data(deleteRole.id());
						   deleteRoleData.set_id(att->att().baseAtt().roleID());

						   deleteRoleData.clear_equipments();
						   for (int i = 0; i < att->att().equipmentAtt().equipments_size(); i++) {
						   int64_t equip = att->att().equipmentAtt().equipments(i);
						   if (equip != -1)
						   deleteRoleData.add_equipments(equip);
						   }
						   for (int i = (int)ItemPackage::EQUIPMENT; i < (int)ItemPackage::EQUIPMENT + att->itemPackage().validNumEquipment(); i++) {
						   int64_t equip = att->itemPackage().items(i).id();
						   if (equip != -1 && att->itemPackage().items(i).type() == PB_ItemInfo::EQUIPMENT)
						   deleteRoleData.add_equipments(equip);
						   }
						   for (int i = 0; i < att->mails_size(); i++) {
						   if (att->mails(i).item().type() == PB_ItemInfo::EQUIPMENT)
						   deleteRoleData.add_equipments(att->mails(i).item().id());
						   }

						   GCAgent_SendProtoToDCAgent(&deleteRoleData);
						   }
						   break;
						   */

						/*
						   case NetProto_ReLogin::UNITID:
						   {
						   static NetProto_ReLogin reLogin;
						   if (!reLogin.ParseFromArray(data, size))
						   break;

						   DEBUG_LOG("Account Relogin: %s", reLogin.account().c_str());

						   static NetProto_ReLoginResult reLoginResult;
						   reLoginResult.Clear();

						   PlayerInfo account;
						   if (reLogin.platform() == "")
						   reLogin.set_platform(LOCAL_PLATFORM);
						   account.set_platform(reLogin.platform());
						   account.set_account(reLogin.account());
						   int32_t prevID = AccountPool_AccountToID(account);
						   if (prevID == -1) {
						// DEBUG_LOGERROR("Account: %s has logged out", reLogin.account().c_str());
						reLoginResult.set_id(-1);
						GCAgent_SendProtoToClients(&id, 1, &reLoginResult);
						GCAgent_SendOrderToNetAgent(ORDER_CLOSE_CLIENT, id, -1);
						break;
						}

						int status = AccountPool_AccountStatus(account);
						if (status != ACCOUNT_DROP) {
						DEBUG_LOGERROR("Account: %s %s is not dropped, status = %d", account.platform().c_str(), account.account().c_str(), status);
						reLoginResult.set_id(-1);
						GCAgent_SendProtoToClients(&id, 1, &reLoginResult);
						GCAgent_SendOrderToNetAgent(ORDER_CLOSE_CLIENT, id, -1);
						break;
						}

						struct PlayerEntity *player = PlayerEntity_Player(prevID);
						if (player == NULL) {
						DEBUG_LOGERROR("Account: %s %s has no role", account.platform().c_str(), account.account().c_str());
						reLoginResult.set_id(-1);
						GCAgent_SendProtoToClients(&id, 1, &reLoginResult);
						GCAgent_SendOrderToNetAgent(ORDER_CLOSE_CLIENT, id, -1);
						break;
						}

						reLoginResult.set_id(prevID);
						GCAgent_SendProtoToClients(&id, 1, &reLoginResult);

						AccountPool_ReLogin(prevID);
						GCAgent_SendOrderToNetAgent(ORDER_EXCHANGE_ID, id, prevID);
						ConnectionPool_Del(id);
						ConnectionPool_Add(prevID);
						}
						break;
						*/

							case NetProto_Rank::UNITID:
						{
							if (InSingle(id))
								break;

							static NetProto_Rank rank;
							if (!rank.ParseFromArray(data, size))
								break;

							PlayerEntity *entity = PlayerEntity_Player(id);
							if (entity == NULL)
								break;
							struct Component *component = PlayerEntity_Component(entity);

							int32_t map = Movement_Att(component->movement)->mapID();
							const MapInfo *info = MapInfoManager_MapInfo(MapPool_MapInfo(map));
							if (info == NULL)
								break;
							if (info->mapType() != MapInfo::PEACE)
								break;

							int64_t res = PlayerPool_QueryRank(PlayerEntity_RoleID(entity), &rank);
							if (res == 0) {
								GCAgent_SendProtoToClients(&id, 1, &rank);

							}
						}
						break;

							case NetProto_SetUp::UNITID:
						{
							static NetProto_SetUp setUp;
							if (!setUp.ParseFromArray(data, size))
								break;

							PlayerEntity *entity = PlayerEntity_Player(id);
							if (entity == NULL)
								break;

							int event = 0;
							if (setUp.soundVolume() < 0 || setUp.soundVolume() > 100)
								setUp.set_soundVolume(100);
							event |= setUp.soundVolume();
							if (setUp.musicVolume() < 0 || setUp.musicVolume() > 100)
								setUp.set_musicVolume(100);
							event |= (setUp.musicVolume() << 8);
							event |= ((setUp.fixedCamera() ? 0 : 1) << 16);
							if (setUp.playerNum() < 0 || setUp.playerNum() > 2)
								setUp.set_playerNum(0);
							event |= (1 << (17 + setUp.playerNum()));
							event |= ((setUp.displayOtherEffect() ? 0 : 1) << 20);
							event |= ((setUp.displayMyEffect() ? 0 : 1) << 21);
							event |= ((setUp.displayOtherHurtNum() ? 0 : 1) << 22);

							PlayerEntity_SetFixedEvent(entity, 5, event);
						}
						break;

							case NetProto_Register::UNITID:
						{
							static NetProto_Register reg;
							if (!reg.ParseFromArray(data, size))
								break;

							// only for local platform
							if (!Config_HasPlatform(LOCAL_PLATFORM)) {
								static NetProto_Error error;
								error.Clear();
								static char buf[CONFIG_FIXEDARRAY];
								snprintf(buf, sizeof(buf), Config_Words(44), LOCAL_PLATFORM);
								error.set_content(buf);
								GCAgent_SendProtoToClients(&id, 1, &error);
								break;
							}

							if (AccountPool_IDToAccount(id) != NULL)
								break;

							static DCProto_AddAccount addAccount;
							addAccount.Clear();
							addAccount.set_id(id);
							PlayerInfo *info = addAccount.mutable_info();
							info->set_account(reg.account());
							info->set_password(reg.password());
							info->set_platform(LOCAL_PLATFORM);
							info->set_deviceID(reg.deviceID());
							info->set_idfa(reg.idfa());
							addAccount.set_ip(NetID_IP(id));
							GCAgent_SendProtoToDCAgent(&addAccount);
						}
						break;

							case NetProto_GetKeyGift::UNITID:
						{
							static NetProto_GetKeyGift gkg;
							if (!gkg.ParseFromArray(data, size))
								break;

							PlayerEntity *entity = PlayerEntity_Player(id);
							if (entity == NULL)
								break;
							struct Component *component = PlayerEntity_Component(entity);
							int64_t roleID = PlayerEntity_RoleID(entity);
							if(roleID <= 0)
								break;
							static DCProto_GetKeyGift dgkg;
							dgkg.Clear();
							dgkg.set_key(gkg.key());
							dgkg.set_id(id);
							dgkg.set_done(false);
							dgkg.set_roleID(roleID);
							int index[] = {16, 70, 71};
							for (size_t i = 0; i < sizeof(index) / sizeof(index[0]); i++) {
								dgkg.add_index(index[i]);
								dgkg.add_event(component->playerAtt->fixedEvent(index[i]));
							}
							GCAgent_SendProtoToDCAgent(&dgkg);
						}
						break;

							case NetProto_StatisticsUIBtn::UNITID:
						{
							static NetProto_StatisticsUIBtn proto;
							if (!proto.ParseFromArray(data, size))
								break;

							static char buf[CONFIG_FIXEDARRAY];
							SNPRINTF1(buf, "{\"ui\":\"%s\",\"btn\":\"%d\",\"param\":\"%s\"}", proto.ui().c_str(), proto.btn(), proto.param().c_str());
							ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "ClickBtn", buf));
						}
						break;

							case NetProto_StatisticsGuide::UNITID:
						{
							static NetProto_StatisticsGuide proto;
							if (!proto.ParseFromArray(data, size))
								break;

							static char buf[CONFIG_FIXEDARRAY];
							SNPRINTF1(buf, "{\"pass\":\"%d\",\"node\":\"%d\"}", proto.pass(), proto.node());
							ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "GuideNode", buf));
						}
						break;

							case NetProto_RechargeParam::UNITID:
						{
							static NetProto_RechargeParam proto;
							if (!proto.ParseFromArray(data, size))
								break;

							const PlayerInfo *info = AccountPool_IDToAccount(id);
							if (info == NULL)
								break;
							struct PlayerEntity *entity = PlayerEntity_Player(id);
							if (entity == NULL)
								break;
							struct Component *component = PlayerEntity_Component(entity);

							static char buf[CONFIG_FIXEDARRAY];
							SNPRINTF1(buf, "%s^%d^%lld^%s", info->account().c_str(), Config_Line(), (long long)component->baseAtt->roleID(), info->platform().c_str());
							proto.set_param(buf);
							GCAgent_SendProtoToClients(&id, 1, &proto);
						}
						break;

							default:
						break;
						}
				}
				break;

				case NetProto_AddPlayers::GROUPID:
				{
					switch(unitID) {
						case NetProto_LoadScene::UNITID:
							{
								static NetProto_LoadScene loadScene;
								if (!loadScene.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								int32_t mapID = Movement_Att(component->movement)->mapID();
								if (mapID < MAP_START) {
									// DEBUG_LOGERROR("Client has sent too many NetProto_LoadScene msgs");
									break;
								}

								const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(mapID));
								if (mapInfo == NULL) {
									// DEBUG_LOGERROR("Client has sent too many NetProto_LoadScene msgs");
									break;
								}

								{
									static char buf[CONFIG_FIXEDARRAY];
									char *index = buf;
									if (Config_ScribeHost() != NULL) {
										memset(buf, 0, sizeof(buf) / sizeof(char));
										SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)PlayerEntity_RoleID(entity));
										index += strlen(index);
										SNPRINTF2(buf, index, "\",\"mapID\":\"%d", mapInfo->id());
										index += strlen(index);
										SNPRINTF2(buf, index, "\",\"status\":begin");
										index += strlen(index);
										SNPRINTF2(buf, index, "\",\"time\":\"%d", (int)Time_TimeStamp());
										ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(PlayerEntity_ID(entity)), PlayerEntity_ID(entity), "GameMap", buf));
									}
								}


								if (loadScene.type() == NetProto_LoadScene::NORMAL) {
									static Vector2i nextCoord;
									int32_t nextMap = MapInfoManager_Next(MapPool_MapInfo(mapID), loadScene.id(), &nextCoord);
									if (nextMap == -1) {
										DEBUG_LOGERROR("Failed to jump scene.");
										return;
									}

									const MapInfo *info = MapInfoManager_MapInfo(nextMap);
									assert(info != NULL);
									if (info->mapType() == MapInfo::PEACE) {
										DEBUG_LOG("LoadScene, cur: %d, next: %d", Movement_Att(PlayerEntity_Component(entity)->movement)->mapID(), nextMap);
										Movement_BeginChangeScene(PlayerEntity_Component(entity)->movement, nextMap, &nextCoord, loadScene.id());
										/*
										   } else if (info->mapType() == MapInfo::SINGLE) {
										   Movement_BeginChangeScene(PlayerEntity_Component(entity)->movement, nextMap, &nextCoord, loadScene.id());
										   } else if (info->mapType() == MapInfo::ROOM) {
										   Movement_BeginChangeScene(PlayerEntity_Component(entity)->movement, nextMap, &nextCoord, loadScene.id());
										   } else if (info->mapType() == MapInfo::ONLY_ROOM) {
										   int32_t room = MapPool_Gen(nextMap);
										   if (room == -1) {
										   DEBUG_LOGERROR("Failed to gen room");
										   break;
										   }

										   Movement_BeginChangeScene(PlayerEntity_Component(entity)->movement, room, &nextCoord, loadScene.id());
										   */
									} else {
										break;
									}
								}
								else if (loadScene.type() == NetProto_LoadScene::JUMP) {
									int32_t real = MapPool_MapInfo(mapID);
									if (real == loadScene.id())
										break;

									bool nextMulTower = false;

									const MapInfo *info = MapInfoManager_MapInfo(real);
									if (info == NULL) {
										break;
									} else if (info->mapType() != MapInfo::PEACE) {
										if (info->id() >= Config_TowerBegin() && info->id() <= Config_TowerEnd()) {
											// pass
										} else if (Config_MulTowerType(info->id()) >= 0) {
											if (loadScene.id() == real + 1) {
												int type = Config_MulTowerType(info->id());
												if (type == 2) // end
													return;
												if (!MapPool_IsSingle(mapID)) {
													int count = MapPool_LinkersCount(mapID);
													if (count > 0) {
														static NetProto_Error proto;
														proto.set_content(Config_Words(66));
														GCAgent_SendProtoToClients(&id, 1, &proto);
														return;
													} else {
														nextMulTower = true;
													}
												}
											} else {
												return;
											}
										} else {
											break;
										}
									}

									info = MapInfoManager_MapInfo(loadScene.id());
									if (info == NULL)
										break;
									
									static Vector2i nextCoord;
									if (!MapInfoManager_EnterCoord(info, loadScene.pos(), &nextCoord)) {
										DEBUG_LOGERROR("Failed to get enter coord");
										break;
									}

									
									if (info->id() == Config_ReservationMap()) {
										int room = PlayerPool_ReservationMap();

										if (!MapInfoManager_EnterCoord(info, PlayerPool_Count(room, -1), &nextCoord)) {
											DEBUG_LOGERROR("Failed to get enter coord");
											break;
										}

										int64_t roleID = PlayerEntity_RoleID(entity);
										if (PlayerPool_ReservationEnterOrPower(roleID, 1)) {
											Movement_BeginChangeScene(PlayerEntity_Component(entity)->movement, room, &nextCoord);
										}
										break;
									}

									if (nextMulTower) {
										int32_t room = MapPool_Gen(info->id(), true);
										if (room == -1) {
											DEBUG_LOGERROR("Failed to gen room");
										} else {
											static vector<struct PlayerEntity *> players;
											players.clear();
											int count = PlayerPool_Players(mapID, &players);
											for (int i = 0; i < count; i++) {
												struct Component *component = PlayerEntity_Component(players[i]);
												if (component != NULL)
													Movement_BeginChangeScene(component->movement, room, &nextCoord);
											}
										}
										return;
									}

									if (info->id() >= Config_TowerBegin() && info->id() <= Config_TowerEnd()) {
										DEBUG_LOG("()=%d", Fight_Att(component->fight)->curTower());
										if (Fight_Att(component->fight)->curTower() + Config_TowerBegin() != info->id() + 1)
											break;
									}
									else if(info->id() >= Config_RoomBegin() && info->id() <= Config_RoomEnd())
									{
										/*
										int events[] = {78, 79, 80};
										int index = (info->id() - Config_RoomBegin()) / 10;
										if(index < (int)(sizeof(events) / sizeof(events[0])))
										{
											index = events[index];
											int bit = ((info->id() - Config_RoomBegin()) % 10) * 3;
											int count = (component->playerAtt->dayEvent(index) & (7 << bit)) >> bit;

											if(count >= Config_DayRoomCount())
											{
												return;
											}
										}
										else
										{
											DEBUG_LOGERROR("day room id is out of range");
											return;
										}
										*/
										int re = Event_DayEnterRoom(component, info->id());
										if(re < 0)
										{
											return;
										}
									}

									if (info->id() == Config_WorldBossMap()) {
										time_t cur = Time_TimeStamp();
										bool open = Config_IsWorldBossOpen(cur);
										if (!open) {
											static NetProto_Error error;
											error.set_content(Config_Words(32));
											GCAgent_SendProtoToClients(&id, 1, &error);
											break;
										}
										if (MapPool_WorldBoss() == -2) {
											static NetProto_Error error;
											error.set_content(Config_Words(33));
											GCAgent_SendProtoToClients(&id, 1, &error);
											break;
										}

										int32_t room = MapPool_WorldBoss();
										if (room == -1) {
											DEBUG_LOGERROR("Failed to get worldboss room");
											break;
										}
										Movement_BeginChangeScene(PlayerEntity_Component(entity)->movement, room, &nextCoord);
									} else if (info->id() == Config_FactionWarMap()) {
										if (strlen(component->playerAtt->faction()) <= 0)
											break;
										time_t cur = Time_TimeStamp();
										bool open = Config_IsFactionWarOpen(cur);
										if (!open) {
											static NetProto_Error error;
											error.set_content(Config_Words(32));
											GCAgent_SendProtoToClients(&id, 1, &error);
											break;
										}
										if (MapPool_FactionWar() == -2) {
											static NetProto_Error error;
											error.set_content(Config_Words(33));
											GCAgent_SendProtoToClients(&id, 1, &error);
											break;
										}

										int32_t room = MapPool_FactionWar();
										if (room == -1) {
											DEBUG_LOGERROR("Failed to get factionwar room");
											break;
										}
										Movement_BeginChangeScene(PlayerEntity_Component(entity)->movement, room, &nextCoord);
									} else if (info->id() == Config_FactionBossMap()) {
										if (strlen(component->playerAtt->faction()) <= 0)
											break;
										time_t cur = Time_TimeStamp();
										bool open = Config_IsFactionBossOpen(cur);
										if (!open) {
											static NetProto_Error error;
											error.set_content(Config_Words(32));
											GCAgent_SendProtoToClients(&id, 1, &error);
											// DEBUG_LOG("haha");
											break;
										}
										if (MapPool_FactionBoss(component->playerAtt->faction()) == -2) {
											static NetProto_Error error;
											error.set_content(Config_Words(33));
											GCAgent_SendProtoToClients(&id, 1, &error);
											break;
										}

										int32_t room = MapPool_FactionBoss(component->playerAtt->faction());
										if (room < 0) {
											room = MapPool_GenFactionBoss(component->playerAtt->faction());
										}
										if (room == -1) {
											DEBUG_LOGERROR("Failed to get factionboss room");
											break;
										}
										Movement_BeginChangeScene(PlayerEntity_Component(entity)->movement, room, &nextCoord);
									} else if (info->mapType() == MapInfo::PEACE) {
										// DEBUG_LOG("LoadScene, cur: %d, next: %d", Movement_Att(PlayerEntity_Component(entity)->movement)->mapID(), loadScene.id());
										Movement_BeginChangeScene(PlayerEntity_Component(entity)->movement, loadScene.id(), &nextCoord);
									} else if (info->mapType() == MapInfo::SINGLE
											// for debug
											|| info->id() == Config_GodMap()) {
										Movement_BeginChangeScene(PlayerEntity_Component(entity)->movement, loadScene.id(), &nextCoord);
									} else if (info->mapType() == MapInfo::ROOM) {
										// for debug
										//Movement_BeginChangeScene(PlayerEntity_Component(entity)->movement, loadScene.id(), &nextCoord);

										int32_t room = MapPool_Gen(loadScene.id(), true);
										if (room == -1) {
											DEBUG_LOGERROR("Failed to gen room");
											break;
										}

										Movement_BeginChangeScene(PlayerEntity_Component(entity)->movement, room, &nextCoord);

									} else if (info->mapType() == MapInfo::ONLY_ROOM) {
										int32_t room = MapPool_Gen(loadScene.id(), true);
										if (room == -1) {
											DEBUG_LOGERROR("Failed to gen room, mapid: %d", info->id());
											break;
										}
										Movement_BeginChangeScene(PlayerEntity_Component(entity)->movement, room, &nextCoord);
									} else if (info->mapType() == MapInfo::HELL) {
										/*
										   time_t cur = Time_TimeStamp();
										   bool open = Config_IsHellOpen(cur);
										   if (!open) {
										   static NetProto_Error error;
										   error.set_content(Config_Words(32));
										   GCAgent_SendProtoToClients(&id, 1, &error);
										   break;
										   }
										   */

										int32_t room = MapPool_Hell();
										if (room == -1)
											break;
										if (!MapInfoManager_EnterCoord(info, MapInfoManager_RandomEnterCoord(info), &nextCoord)) {
											DEBUG_LOGERROR("Failed to get enter coord");
											break;
										}
										Movement_BeginChangeScene(PlayerEntity_Component(entity)->movement, room, &nextCoord);

										static NetProto_Chat chat;
										chat.Clear();
										chat.set_channel(NetProto_Chat::SYSTEM);
										char buf[CONFIG_FIXEDARRAY];
										SNPRINTF1(buf, Config_Words(45), component->baseAtt->name());
										chat.set_content(buf);
										GCAgent_SendProtoToAllClients(&chat);
									} else {
										break;
									}
								}
								else if (loadScene.type() == NetProto_LoadScene::LEAVE) {
									/*
									   if (loadScene.gemRecover() > 0)
									   Item_ModifyRMB(component->item, -loadScene.gemRecover() * Config_RecoverGem(), false);
									   */

									if (mapInfo->id() == Config_FactionWarMap()) {
									} else if (mapInfo->mapType() == MapInfo::PRACTICE
											|| mapInfo->mapType() == MapInfo::PVP
											|| mapInfo->mapType() == MapInfo::HELL
											|| (mapInfo->id() == Config_GodMap())
											|| (mapInfo->id() == Config_EventMap1_Easy())
											|| (mapInfo->id() == Config_EventMap1_Normal())
											|| (mapInfo->id() == Config_EventMap1_Hard())
											|| (mapInfo->id() == Config_EventMap_ProtectCrystal_Easy())
											|| (mapInfo->id() == Config_EventMap_ProtectCrystal_Normal())
											|| (mapInfo->id() == Config_EventMap_ProtectCrystal_Hard())
											|| (mapInfo->id() == Config_EventMap_GoldPig_Easy())
											|| (mapInfo->id() == Config_EventMap_GoldPig_Normal())
											|| (mapInfo->id() == Config_EventMap_GoldPig_Hard())
											|| (mapInfo->id() == Config_EventMap_PetBattle_Easy())
											|| (mapInfo->id() == Config_EventMap_PetBattle_Normal())
											|| (mapInfo->id() == Config_EventMap_PetBattle_Hard())
											|| (mapInfo->id() == Config_EventMap_ObstacleFan_Easy())
											|| (mapInfo->id() == Config_EventMap_ObstacleFan_Normal())
											|| (mapInfo->id() == Config_EventMap_ObstacleFan_Hard())
											|| (mapInfo->id() == Config_EventMap_PetStone_Easy())
											|| (mapInfo->id() == Config_EventMap_PetStone_Normal())
											|| (mapInfo->id() == Config_EventMap_PetStone_Hard())
											|| (mapInfo->id() == Config_EventMap_RobRes_Easy())
											|| (mapInfo->id() == Config_EventMap_RobRes_Normal())
											|| (mapInfo->id() == Config_EventMap_RobRes_Hard())
											|| (mapInfo->id() >= Config_TowerBegin() && mapInfo->id() <= Config_TowerEnd())
											|| (mapInfo->id() >= Config_BossBegin() && mapInfo->id() <= Config_BossEnd())) {
												
												if (PlayerPool_ReservationMap() == mapID) {
													int64_t roleID = PlayerEntity_RoleID(entity);
													if (PlayerPool_ReservationEnterOrPower(roleID, 1)) {
														break;
													}
												}

												if (!Fight_HasClearedRoom(component->fight)) {
													Fight_ClearRoom(component->fight, false, 0);
													break;
												}
											} else if (mapInfo->id() == Config_SurviveMap()) {
												if (!Fight_HasClearedRoom(component->fight)) {
													static NetProto_ClearRoom clearRoom;
													clearRoom.Clear();
													Fight_ClearRoom(component->fight, true, loadScene.pos(), &clearRoom);
													clearRoom.set_totalTime(loadScene.pos());
													GCAgent_SendProtoToClients(&id, 1, &clearRoom);
													break;
												}
											}

									const MovementAtt *movementAtt = Movement_Att(component->movement);
									int32_t nextMap = movementAtt->prevNormalMap();
									if (nextMap < MAP_START) {
										DEBUG_LOGERROR("Map of role: %lld is wrong, cur map: %d", PlayerEntity_RoleID(entity), nextMap);

										Vector2i nextCoord;
										nextMap = MapInfoManager_Next(MapPool_MapInfo(MAP_CREATE), 0, &nextCoord);
										assert(nextMap != -1);

										Movement_BeginChangeScene(component->movement, nextMap, &nextCoord, 0);
									}
									else {
										Movement_BeginChangeScene(component->movement, nextMap, &movementAtt->prevCoord());
									}
								}
							}
							break;

						case NetProto_EndLoadScene::UNITID:
							{
								static NetProto_EndLoadScene endLoadScene;
								if (!endLoadScene.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;

								Movement_EndChangeScene(PlayerEntity_Component(entity)->movement);
							}
							break;

						case NetProto_ClearRoom::UNITID:
							{
								static NetProto_ClearRoom clearRoom;
								if (!clearRoom.ParseFromArray(data, size)) {
									DEBUG_LOGERROR("Failed to clearroom, 1");
									break;
								}

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL) {
									DEBUG_LOGERROR("Failed to clearroom, 3");
									break;
								}
								struct Component *component = PlayerEntity_Component(entity);

								if (!InSingle(id)) {
									DEBUG_LOGERROR("Failed to clearroom, 2");
									if (component != NULL) {
										int32_t map = Movement_Att(component->movement)->mapID();
										const MapInfo *info = MapInfoManager_MapInfo(MapPool_MapInfo(map));
										if (info != NULL) {
											DEBUG_LOGERROR("mapid: %d", info->id());
										}
									}
									break;
								}

								int ret = PlayerEntity_Check(entity, clearRoom.index(), clearRoom.time(), clearRoom.md5().c_str());
								if (ret != 0) {
									int64_t roleId = PlayerEntity_RoleID(entity);
									DEBUG_LOGERROR("Clear room check md5 failed, role id: %d, error num: %d.", roleId, ret);
									break;
								}

								int32_t map = Movement_Att(component->movement)->mapID();
								const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(map));
								if (mapInfo->id() == Config_GodMap())
								{
									ret = PlayerEntity_CheckCombatRecord(entity, clearRoom.combatRecord());
									if (ret != 0) {
										int64_t roleId = PlayerEntity_RoleID(entity);
										DEBUG_LOGERROR("Clear room check combat record failed, role id: %d, error num: %d.", roleId, ret);
										break;
									}
									ret = PlayerEntity_CheckCombatRecordDamage(entity, clearRoom.combatRecord(), true);
									if (ret != 0) {
										int64_t roleId = PlayerEntity_RoleID(entity);
										DEBUG_LOGERROR("Clear room check combat record damage failed, role id: %d, error num: %d.", roleId, ret);
										break;
									}
								}

								DEBUG_LOG("Clear Room begin");
								ret = Fight_ClearRoom(component->fight, true, clearRoom.totalTime(), &clearRoom);
								if (ret < 0) {
									DEBUG_LOGERROR("Failed to clearroom, clear, %d", ret);
									break;
								}

								DEBUG_LOG("Clear Room begin");
								if (mapInfo == NULL) {
									DEBUG_LOGERROR("Failed to clearroom, 4");
									break;
								}
								DEBUG_LOG("Clear Room begin");
								if ((mapInfo->id() >= Config_SingleBegin() && mapInfo->id() <= Config_SingleEnd())
										|| (mapInfo->id() >= Config_SingleEnhanceBegin() && mapInfo->id() <= Config_SingleEnhanceEnd())) {
									MapInfoManager_UpdateSingleRecord(mapInfo->id(), clearRoom.totalTime(), entity);
									DEBUG_LOG("Clear Room begin");
								}

								DEBUG_LOG("Clear Room end");
								GCAgent_SendProtoToClients(&id, 1, &clearRoom);
							}
							break;

						case NetProto_OpenRoomBox::UNITID:
							{
								static NetProto_OpenRoomBox openRoomBox;
								if (!openRoomBox.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								int32_t results[CONFIG_FIXEDARRAY];
								int count = Fight_OpenRoomBox(component->fight, true, results, CONFIG_FIXEDARRAY);
								if (count < 0) {
									// DEBUG_LOGERROR("Failed to open room box, res = %d", count);
									break;
								} else {
									for (int i = 0; i < count; i++) {
										openRoomBox.add_result(results[i]);
									}
								}

								GCAgent_SendProtoToClients(&id, 1, &openRoomBox);
							}
							break;

						case NetProto_BeginWaitRoom::UNITID:
							{
								static NetProto_BeginWaitRoom beginWaitRoom;
								if (!beginWaitRoom.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								if (Movement_BeginWaitRoom(component->movement, beginWaitRoom.map()) == -1)
									beginWaitRoom.set_map(-1);

								GCAgent_SendProtoToClients(&id, 1, &beginWaitRoom);
							}
							break;

						case NetProto_QuickFight::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_QuickFight quickFight;
								if (!quickFight.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								Fight_QuickFight(component->fight, quickFight.map(), quickFight.count());
							}
							break;

						case NetProto_Timeout::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_Timeout timeout;
								if (!timeout.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								Fight_Timeout(component->fight);
							}
							break;

						case NetProto_BeginWaitPVP::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_BeginWaitPVP beginWaitPVP;
								if (!beginWaitPVP.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								if (((component->playerAtt->dayEvent(30) & 0xff0000) >> 16) >= 20)
									break;

								if (beginWaitPVP.type() == NetProto_BeginWaitPVP::ONE_TO_ONE) {
									int res = Fight_BeginWaitPVP(component->fight);
									if (res == -1)
										beginWaitPVP.set_res(false);
									else if (res == 0)
										beginWaitPVP.set_res(true);
									else
										break;
									/*
									   } else if (beginWaitPVP.type() == NetProto_BeginWaitPVP::HELL) {
									   int res = Fight_BeginWaitHell(component->fight);
									   if (res == -1)
									   beginWaitPVP.set_res(false);
									   else if (res == 0)
									   beginWaitPVP.set_res(true);
									   else
									   break;
									   */
								} else {
									break;
								}

								GCAgent_SendProtoToClients(&id, 1, &beginWaitPVP);
							}
							break;

						case NetProto_Lottery::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_Lottery lottery;
								if (!lottery.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								int32_t map = Movement_Att(component->movement)->mapID();
								const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(map));
								if (mapInfo == NULL || mapInfo->mapType() != MapInfo::PEACE)
									break;

								int32_t res[CONFIG_FIXEDARRAY];
								int count = Item_Lottery(component->item, Config_LuckyBox(), res, CONFIG_FIXEDARRAY);
								if (count == -1) {
									lottery.set_res(-1);
									DEBUG_LOGERROR("Failed to lottery, -1");
								} else if (count == 1) {
									lottery.set_res(res[0]);
									Item_NoticeBox(component->item, Config_LuckyBox(), res, count, 1);
								} else {
									DEBUG_LOGERROR("Failed to lottery, %d", count);
									lottery.set_res(-1);
								}

								GCAgent_SendProtoToClients(&id, 1, &lottery);
							}
							break;

						case NetProto_GetGift::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_GetGift getGift;
								if (!getGift.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								int32_t map = Movement_Att(component->movement)->mapID();
								const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(map));
								if (mapInfo == NULL || mapInfo->mapType() != MapInfo::PEACE)
									break;

								if (Item_GetGift(component->item, getGift.type(), getGift.index(), getGift.arg()) == -1)
									getGift.set_index(-1);

								GCAgent_SendProtoToClients(&id, 1, &getGift);
							}
							break;

						case NetProto_SingleRecord::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_SingleRecord sr;
								if (!sr.ParseFromArray(data, size))
									break;

								const RecordInfo *record = MapInfoManager_SingleRecord(sr.mapID());
								if (record != NULL)
									*sr.mutable_record() = *record;
								else
									sr.clear_record();

								GCAgent_SendProtoToClients(&id, 1, &sr);
							}
							break;

						case NetProto_EndLoadModel::UNITID:
							{
								static NetProto_EndLoadModel elm;
								if (!elm.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								Movement_EndLoadModel(component->movement);
							}
							break;

						case NetProto_RoomWaitCount::UNITID:
							{
								static NetProto_RoomWaitCount rwc;
								if (!rwc.ParseFromArray(data, size))
									break;

								rwc.set_count(Movement_RoomWaitCount(rwc.room()));
								GCAgent_SendProtoToClients(&id, 1, &rwc);
							}
							break;

						case NetProto_PVPWaitCount::UNITID:
							{
								static NetProto_PVPWaitCount pwc;
								if (!pwc.ParseFromArray(data, size))
									break;

								if (pwc.type() == NetProto_PVPWaitCount::ONE_TO_ONE)
									pwc.set_count(Fight_PVPWaitCount());
								else if (pwc.type() == NetProto_PVPWaitCount::HELL)
									pwc.set_count(Fight_HellWaitCount());
								else
									break;
								GCAgent_SendProtoToClients(&id, 1, &pwc);
							}
							break;

						case NetProto_CancelWaitRoom::UNITID:
							{
								static NetProto_CancelWaitRoom cwr;
								if (!cwr.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								Movement_EndWaitRoom(component->movement);
								Movement_DestroyRoom(component->movement);
								Movement_LeaveRoom(component->movement);
								Fight_EndWaitPVP(component->fight);
								Fight_EndWaitHell(component->fight);
								GCAgent_SendProtoToClients(&id, 1, &cwr);
							}
							break;

						case NetProto_CancelWaitPVP::UNITID:
							{
								static NetProto_CancelWaitPVP cwp;
								if (!cwp.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								Fight_EndWaitPVP(component->fight);
								Fight_EndWaitHell(component->fight);
								Movement_EndWaitRoom(component->movement);
								Movement_DestroyRoom(component->movement);
								Movement_LeaveRoom(component->movement);
								GCAgent_SendProtoToClients(&id, 1, &cwp);
							}
							break;

						case NetProto_Invest::UNITID:
							{
								static NetProto_Invest invest;
								if (!invest.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								int investID = component->playerAtt->fixedEvent(11) & 0xffff;
								if (investID != 0)
									break;

								int cost = Config_InvestCost(invest.id());
								if (cost == -1)
									break;

								//	if (Item_Package(component->item)->rmb() < cost)
								if (!Item_HasRMB(component->item, cost, 1))
									break;

								Item_ModifyRMB(component->item, -cost, true, 4, invest.id(), 0);
								PlayerEntity_SetFixedEvent(entity, 11, invest.id());

								GCAgent_SendProtoToClients(&id, 1, &invest);
							}
							break;

						case NetProto_InvestAward::UNITID:
							{
								static NetProto_InvestAward ia;
								if (!ia.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								int investID = component->playerAtt->fixedEvent(11) & 0xffff;
								if (investID == 0 || investID != ia.id())
									break;

								const AwardInfo *award = AwardInfoManager_AwardInfo(Config_InvestAward(investID), ia.index());
								if (award == NULL)
									break;
								if (component->playerAtt->fixedEvent(65) & (1 << ia.index()))
									break;
								if (component->roleAtt->fightAtt().level() < award->arg())
									break;

								int32_t res[CONFIG_FIXEDARRAY];
								Item_Lottery(component->item, award->award(), res, CONFIG_FIXEDARRAY);
								PlayerEntity_SetFixedEvent(component->player, 65, component->playerAtt->fixedEvent(65) | (1 << ia.index()));
								GCAgent_SendProtoToClients(&id, 1, &ia);
							}
							break;

						case NetProto_CreateRoom::UNITID:
							{
								static NetProto_CreateRoom proto;
								if (!proto.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								Movement_CreateRoom(component->movement, proto.map(), proto.noPower());
							}
							break;

						case NetProto_JoinRoom::UNITID:
							{
								static NetProto_JoinRoom proto;
								if (!proto.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								Movement_JoinRoom(component->movement, proto.id(), proto.noPower());
							}
							break;

						case NetProto_LeaveRoom::UNITID:
							{
								static NetProto_LeaveRoom proto;
								if (!proto.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								Movement_LeaveRoom(component->movement);
							}
							break;

						case NetProto_DestroyRoom::UNITID:
							{
								static NetProto_DestroyRoom proto;
								if (!proto.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								Movement_DestroyRoom(component->movement);
							}
							break;

						case NetProto_EvictRole::UNITID:
							{
								static NetProto_EvictRole proto;
								if (!proto.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								Movement_EvictRole(component->movement, proto.pos());
							}
							break;

						case NetProto_RoomList::UNITID:
							{
								static NetProto_RoomList proto;
								if (!proto.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								Movement_RoomList(component->movement);
							}
							break;

						case NetProto_InviteFriend::UNITID:
							{
								static NetProto_InviteFriend proto;
								if (!proto.ParseFromArray(data, size))
									break;

								PlayerEntity *target = PlayerEntity_PlayerByRoleID(proto.roleID());
								if (target == NULL) {
									static NetProto_Error error;
									error.set_content(Config_Words(38));
									GCAgent_SendProtoToClients(&id, 1, &error);
									break;
								}
								int32_t targetID = PlayerEntity_ID(target);

								struct Component *component = PlayerEntity_Component(target);

								int32_t map = Movement_Att(component->movement)->mapID();
								const MapInfo *info = MapInfoManager_MapInfo(MapPool_MapInfo(map));
								if (info == NULL || info->mapType() != MapInfo::PEACE) {
									static NetProto_Error error;
									error.set_content(Config_Words(46));
									GCAgent_SendProtoToClients(&id, 1, &error);
								} else {
									map = proto.map();
									info = MapInfoManager_MapInfo(map);
									if (info == NULL)
										break;

									if(info->id() >= Config_RoomBegin() && info->id() <= Config_RoomEnd())
									{
										int re = Event_DayEnterRoom(component, info->id());
										if(re < 0)
										{
											static NetProto_Error error;
											error.set_content(Config_Words(46));
											GCAgent_SendProtoToClients(&id, 1, &error);
											return;
										}
									}

									RoomInfo *roomInfo = Movement_SearchRoomInfo(proto.id());
									if (roomInfo == NULL)
										break;
									if (Movement_MultiRoom(component->movement) != -1) {
										static NetProto_Error error;
										error.set_content(Config_Words(47));
										GCAgent_SendProtoToClients(&id, 1, &error);
									} else if (!Movement_CanEnterRoom(component->movement, map, true, roomInfo->noPower())) {
										static NetProto_Error error;
										error.set_content(Config_Words(63));
										GCAgent_SendProtoToClients(&id, 1, &error);
									} else {
										proto.set_flag(1);
										GCAgent_SendProtoToClients(&targetID, 1, &proto);
									}
								}
							}
							break;

						case NetProto_BeginMultiRoom::UNITID:
							{
								static NetProto_BeginMultiRoom proto;
								if (!proto.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								Movement_BeginMultiRoom(component->movement);
							}
							break;

						case NetProto_ResetCheckPoint::UNITID:
							{
								static NetProto_ResetCheckPoint proto;
								if (!proto.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								if (Fight_RestCheckPoint(component->fight)) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						case NetProto_RegistDeviceServer::UNITID:
							{
								static NetProto_RegistDeviceServer proto;
								if (!proto.ParseFromArray(data, size))
									break;

								/*
								   static DCProto_RegistDeviceServer msg;
								   msg.Clear();
								   msg.set_deviceNum(proto.deviceID());
								   msg.set_id(id);
								   msg.set_idfa(proto.idfa());
								   msg.set_noLine(true);
								   msg.set_time(0);
								   GCAgent_SendProtoToDCAgent(&msg);
								   */
							}
							break;

						case NetProto_StartLoad1::UNITID:
							{
								static NetProto_StartLoad1 proto;
								if (!proto.ParseFromArray(data, size))
									break;

								static PlayerInfo info;
								info.set_deviceID(proto.deviceID());
								info.set_idfa(proto.idfa());
								ScribeClient_SendMsg(ScribeClient_Format(&info, id, "StartLoad1"));
							}
							break;

						case NetProto_EndLoad1::UNITID:
							{
								static NetProto_EndLoad1 proto;
								if (!proto.ParseFromArray(data, size))
									break;

								static PlayerInfo info;
								info.set_deviceID(proto.deviceID());
								info.set_idfa(proto.idfa());
								ScribeClient_SendMsg(ScribeClient_Format(&info, id, "EndLoad1"));
							}
							break;

						case NetProto_StartLoad2::UNITID:
							{
								static NetProto_StartLoad2 proto;
								if (!proto.ParseFromArray(data, size))
									break;

								ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "StartLoad2"));
							}
							break;

						case NetProto_EndLoad2::UNITID:
							{
								static NetProto_EndLoad2 proto;
								if (!proto.ParseFromArray(data, size))
									break;

								ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "EndLoad2"));
							}
							break;

						case NetProto_FactionWarPrevWinner::UNITID:
							{
								static NetProto_FactionWarPrevWinner proto;
								if (!proto.ParseFromArray(data, size))
									break;

								int64_t hurt;
								string name;
								PlayerPool_GetWinFactionFight(MapPool_FactionWarNum(), &name, &hurt);

								proto.set_name(name);
								GCAgent_SendProtoToClients(&id, 1, &proto);
							}
							break;

						case NetProto_GetMonthCardAward::UNITID:
							{
								static NetProto_GetMonthCardAward proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								if (component->playerAtt->dayEvent(5) & (1 << 6))
									break;

								int event = component->playerAtt->fixedEvent(85);
								if (event > 0 && Time_TimeStamp() < event) {
									int v = Config_MonthCardAward();
									Item_ModifyRMB(component->item, v, false, -10);
									static NetProto_GetRes gr;
									gr.Clear();
									PB_ItemInfo *item = gr.add_items();
									item->set_type(PB_ItemInfo::RMB);
									item->set_count(v);
									GCAgent_SendProtoToClients(&id, 1, &gr);

									PlayerEntity_SetDayEvent(component->player, 5, component->playerAtt->dayEvent(5) | (1 << 6));
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						case NetProto_ExchangeGoods::UNITID:
							{
								static NetProto_ExchangeGoods proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								if (Item_ExchangeGoods(component->item, proto.index(), proto.all()) == -1)
									break;

								GCAgent_SendProtoToClients(&id, 1, &proto);
							}
							break;

						case NetProto_BuyMonthCard::UNITID:
							{
								static NetProto_BuyMonthCard proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								int event = component->playerAtt->fixedEvent(85);
								if (event > 0 && Time_TimeStamp() < event)
									break;

								int cost = Config_MonthCardCost();
								if (!Item_HasRMB(component->item, cost, 1))
									break;
								Item_ModifyRMB(component->item, -cost, true, 31);

								time_t cur = Time_TimeStamp();
								tm t;
								Time_LocalTime(cur, &t);
								t.tm_hour = 0;
								t.tm_min = 0;
								t.tm_sec = 0;
								time_t expire = mktime(&t) + (3600 * 24 * 30);
								PlayerEntity_SetFixedEvent(component->player, 85, expire);

								int v = Config_MonthCardFirstAward();
								Item_ModifyRMB(component->item, v, false, -10);
								static NetProto_GetRes gr;
								gr.Clear();
								PB_ItemInfo *item = gr.add_items();
								item->set_type(PB_ItemInfo::SUBRMB);
								item->set_count(v);
								GCAgent_SendProtoToClients(&id, 1, &gr);

								PlayerEntity_SetDayEvent(component->player, 5, component->playerAtt->dayEvent(5) | (1 << 6));

								proto.set_expire(expire);
								GCAgent_SendProtoToClients(&id, 1, &proto);
							}
							break;

						case NetProto_UseLuckyCard::UNITID:
							{
								static NetProto_UseLuckyCard proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;

								if (Event_AwardFromSky(entity)) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}

							}
							break;

						case NetProto_BIActive::UNITID:
							{
								static NetProto_BIActive proto;
								if (!proto.ParseFromArray(data, size))
									break;

								if (Config_ScribeHost() != NULL) {
									static char buf[CONFIG_FIXEDARRAY];
									memset(buf, 0, sizeof(buf) / sizeof(char));
									char *index = buf;

									SNPRINTF2(buf, index, "{\"deviceID\":\"%s", proto.deviceID().c_str());
									index += strlen(index);
									SNPRINTF2(buf, index, "\",\"idfa\":\"%s", proto.idfa().c_str());
									index += strlen(index);
									SNPRINTF2(buf, index, "}");
									ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "BIActive", buf));
								}

							}
							break;
						case NetProto_ClearQuickFightTime::UNITID:
							{
								static NetProto_ClearQuickFightTime proto;
								if(!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								int32_t cur = (int32_t)Time_TimeStamp();

								if(cur <= component->playerAtt->fixedEvent(56))
									proto.set_re(-1);

								int remain = component->playerAtt->fixedEvent(56) - cur;
								int a = remain / 60;
								int b = remain % 60;
								if(b > 0)
									a += 1;

								if(!Item_HasRMB(component->item, a))
									proto.set_re(-1);
								else
								{
									Item_ModifyRMB(component->item, -a, false, 41, 0, 0);

									PlayerEntity_SetFixedEvent(entity, 56, cur, false);
									proto.set_re(0);
								}

								GCAgent_SendProtoToClients(&id, 1, &proto);
							}
							break;

						default:
							break;
					}
				}
				break;

				case NetProto_Move::GROUPID:
				{
					switch(unitID) {
						case NetProto_Attack::UNITID:
							{
								DEBUG_LOGERROR("attack");
								if (InSingle(id))
									break;

								static NetProto_Attack attack;
								if (!attack.ParseFromArray(data, size))
									break;

								struct PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								const PlayerAtt *att = PlayerEntity_Att(entity);
								if (att == NULL) {
									break;
								}

								DEBUG_LOG("TTTTTTTTTTTTTTTTTTTTTTTTT");
								if (att->att().movementAtt().mapID() == PlayerPool_ReservationMap()) {
									int64_t roleID = PlayerEntity_RoleID(entity);
									if (!PlayerPool_ReservationEnterOrPower(roleID, 2)) {
										DEBUG_LOG("TTTTTTTTTTTTTTTTTTTTTTTTT~~~~~~~~~~~~~~~~~");
										break;
									}
								}

								struct Fight *aFight = NULL, *dFight = NULL;
								struct Movement *aMovement = NULL;

								if (attack.aType() == NetProto_Attack::PLAYER) {
									if (attack.aID() != id)
										break;

									struct PlayerEntity *aEntity = PlayerEntity_Player(attack.aID());
									if (aEntity == NULL)
										break;

									aFight = PlayerEntity_Component(aEntity)->fight;
									aMovement = PlayerEntity_Component(aEntity)->movement;
								} else {
									break;
								}

								if (attack.dID() == -1) {
									if (!attack.has_tPos())
										break;
								} else {
									if (attack.dType() == NetProto_Attack::PLAYER) {
										if (attack.aID() == attack.dID())
											break;

										struct PlayerEntity *dEntity = PlayerEntity_Player(attack.dID());
										if (dEntity == NULL)
											break;

										dFight = PlayerEntity_Component(dEntity)->fight;
									} else if (attack.dType() == NetProto_Attack::NPC) {
										struct NPCEntity *dEntity = NPCPool_NPC(Movement_Att(aMovement)->mapID(), attack.dID());
										if (dEntity == NULL)
											break;

										dFight = NPCEntity_Component(dEntity)->fight;
									} else {
										break;
									}
								}

								const SkillInfo *skill = SkillInfoManager_SkillInfo(attack.aSkill().id(), attack.aSkill().level());
								if (skill == NULL) {
									DEBUG_LOG("skillid: %d, level: %d", attack.aSkill().id(), attack.aSkill().level());
									break;
								}
								if (!Fight_HasSkill(aFight, skill)) {
									DEBUG_LOG("skillid: %d, level: %d", attack.aSkill().id(), attack.aSkill().level());
									break;
								}
								/*
								   skill = Fight_NextSkill(aFight, skill);
								   if (skill == NULL)
								   break;

								   attack.mutable_aSkill()->set_id(skill->id());
								   attack.mutable_aSkill()->set_level(skill->level());
								   */

								Vector3f tPos;
								tPos.FromPB(attack.mutable_tPos());
								int res = Fight_Attack(aFight, skill, dFight, dFight == NULL ? &tPos : NULL, attack.serverTime());
								if (res < 0) {
									// DEBUG_LOGERROR("Failed to attack, errno: %d", res);
									Fight_DestroyClientSkills(aFight, attack.skills().data(), attack.skills_size());
									break;
								} else if (res == 0) {
									// GCAgent_SendProtoToAllClientsExceptOneInSameScene(id, Movement_Att(aMovement)->mapID(), &attack);
								} else if (res == 1) {
									// GCAgent_SendProtoToAllClientsExceptOneInSameScene(id, Movement_Att(aMovement)->mapID(), &attack);
									res = Fight_DoAttack(aFight, attack.skills().data(), attack.skills_size(), attack.serverTime());
									if (res != 0) {
										//DEBUG_LOGERROR("Failed to do attack, errno: %d", res);
										Fight_DestroyClientSkills(aFight, attack.skills().data(), attack.skills_size());
									}
								}
							}
							break;

						case NetProto_Revive::UNITID:
							{
								static NetProto_Revive revive;
								if (!revive.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								int32_t mapID = component->roleAtt->movementAtt().mapID();
								const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(mapID));
								if (mapInfo == NULL || mapInfo->mapType() == MapInfo::PEACE)
									break;

								if (revive.type() == NetProto_Revive::PET) {
									if (Movement_InSingle(component->movement)) {
										Fight_RevivePetInSingle(component->fight);
										break;
									}

									struct NPCEntity *pet = NPCPool_Pet(mapID, component);
									if (pet == NULL)
										break;
									component = NPCEntity_Component(pet);
									if (component == NULL)
										break;
								}

								Fight_Revive(component->fight, &component->roleAtt->movementAtt().logicCoord(), 1.0f, 1.0f);
							}
							break;

						case NetProto_QueryPlayer::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_QueryPlayer queryPlayer;
								if (!queryPlayer.ParseFromArray(data, size))
									break;

								struct PlayerEntity *target = PlayerEntity_PlayerByRoleID(queryPlayer.roleID());
								if (target == NULL) {
									static DCProto_QueryRole queryRole;
									queryRole.set_id(id);
									queryRole.set_roleID(queryPlayer.roleID());
									queryRole.set_name(queryPlayer.name());
									GCAgent_SendProtoToDCAgent(&queryRole);
								} else {
									PlayerEntity_ToSceneData(PlayerEntity_Att(target), queryPlayer.mutable_att());
									queryPlayer.set_online(true);
									GCAgent_SendProtoToClients(&id, 1, &queryPlayer);
								}
							}
							break;

						case NetProto_PlayerStatus::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_PlayerStatus playerStatus;
								if (!playerStatus.ParseFromArray(data, size))
									break;

								playerStatus.clear_statuses();
								int offCount = 0;
								for (int i = 0; i < playerStatus.roles_size(); i++) {
									struct PlayerEntity *target = PlayerEntity_PlayerByRoleID(playerStatus.roles(i));
									if (target == NULL)
										playerStatus.add_statuses(NetProto_PlayerStatus::OFFLINE);
									else if (PlayerEntity_AccountStatus(target) == ACCOUNT_ONLINE)
										playerStatus.add_statuses(NetProto_PlayerStatus::ONLINE);
									else
										playerStatus.add_statuses(NetProto_PlayerStatus::OFFLINE);

									if (target == NULL) {
										playerStatus.add_level(-1);
										playerStatus.add_vip(0);
										offCount++;
									} else {
										struct Component *component = PlayerEntity_Component(target);
										playerStatus.add_level(component->roleAtt->fightAtt().level());
										playerStatus.add_vip(component->playerAtt->itemPackage().vip());
									}
								}

								if (offCount > 0) {
									static DCProto_PlayerStatus ps;
									ps.Clear();
									ps.set_id(id);
									*ps.mutable_ps() = playerStatus;
									GCAgent_SendProtoToDCAgent(&ps);
								} else {
									GCAgent_SendProtoToClients(&id, 1, &playerStatus);
								}
							}
							break;

						case NetProto_AddFriend::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_AddFriend addFriend;
								if (!addFriend.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								struct PlayerEntity *receiver = PlayerEntity_PlayerByRoleID(addFriend.roleID());
								if (player == receiver) {
									static NetProto_Error error;
									error.Clear();
									error.set_content(Config_Words(38));
									GCAgent_SendProtoToClients(&id, 1, &error);
									break;
								}

								if (PlayerEntity_AddFriend(player, addFriend.roleID(), addFriend.name().c_str(), (ProfessionInfo::Type)addFriend.professionType())) { 
									GCAgent_SendProtoToClients(&id, 1, &addFriend);

									PlayerEntity *fan = PlayerEntity_PlayerByRoleID(addFriend.roleID());
									if (NULL == fan) {
										break;
									}
									const PlayerAtt *att = PlayerEntity_Att(player);
									if (att == NULL) {
										break;
									}	
									static NetProto_ChangePartner addFan;
									addFan.Clear();
									addFan.set_index(1);
									addFan.mutable_fans()->set_roleID(att->att().baseAtt().roleID());
									addFan.mutable_fans()->set_name(att->att().baseAtt().name());
									addFan.mutable_fans()->set_professionType((PB_ProfessionInfo::Type)att->att().baseAtt().professionType());
									addFan.mutable_fans()->set_loveHeart(false);
									int32_t fanID =  PlayerEntity_ID(fan);
									if (-1 == fanID) {
										break;
									}
									GCAgent_SendProtoToClients(&fanID, 1, &addFan);
									DEBUG_LOG("Add Fan success !");
								}
							}
							break;

						case NetProto_DelFriend::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_DelFriend delFriend;
								if (!delFriend.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								if (!PlayerEntity_DelFriend(player, delFriend.roleID(), delFriend.flag())) {
									break;
								}

								GCAgent_SendProtoToClients(&id, 1, &delFriend);

								PlayerEntity *partner = PlayerEntity_PlayerByRoleID(delFriend.roleID());
								if (NULL == partner) {
									break;
								}
								const PlayerAtt *att = PlayerEntity_Att(player);
								if (att == NULL) {
									break;
								}
								static NetProto_ChangePartner changePartner;
								changePartner.Clear();
								if (delFriend.flag()) {
									changePartner.set_index(3);
								} else {
									changePartner.set_index(2);
								}
								changePartner.mutable_fans()->set_roleID(att->att().baseAtt().roleID());
								changePartner.mutable_fans()->set_name(att->att().baseAtt().name());
								changePartner.mutable_fans()->set_professionType((PB_ProfessionInfo::Type)att->att().baseAtt().professionType());
								changePartner.mutable_fans()->set_loveHeart(false);
								int32_t partnerID =  PlayerEntity_ID(partner);
								if (-1 == partnerID) {
									break;
								}
								GCAgent_SendProtoToClients(&partnerID, 1, &changePartner);
								DEBUG_LOG("del partner success !");
							}
							break;

						case NetProto_RequestPK::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_RequestPK requestPK;
								if (!requestPK.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								struct PlayerEntity *target = PlayerEntity_PlayerByRoleID(requestPK.target().roleID());
								if (target == NULL) {
									static NetProto_Info error;
									error.set_content(Config_Words(70));
									GCAgent_SendProtoToClients(&id, 1, &error);
									//requestPK.set_res(-1);
									//GCAgent_SendProtoToClients(&id, 1, &requestPK);
									break;
								}

								int res = Fight_SetPKTarget(component->fight, requestPK.target().roleID());
								if (res < 0) {
									requestPK.set_res(res);
									GCAgent_SendProtoToClients(&id, 1, &requestPK);
									break;
								}

								requestPK.set_res(-4);
								GCAgent_SendProtoToClients(&id, 1, &requestPK);
								requestPK.set_res(0);

								requestPK.mutable_target()->set_roleID(PlayerEntity_RoleID(player));
								requestPK.mutable_target()->set_name(component->roleAtt->baseAtt().name());
								requestPK.mutable_target()->set_professionType((PB_ProfessionInfo::Type)component->roleAtt->baseAtt().professionType());

								int32_t targetID = PlayerEntity_ID(target);
								GCAgent_SendProtoToClients(&targetID, 1, &requestPK);
							}
							break;

						case NetProto_ApplyPK::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_ApplyPK applyPK;
								if (!applyPK.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								struct PlayerEntity *origin = PlayerEntity_PlayerByRoleID(applyPK.origin());
								if (origin == NULL)
									break;
								struct Component *oComponent = PlayerEntity_Component(origin);

								if (Fight_PKTarget(oComponent->fight) != PlayerEntity_RoleID(player)) {
									// DEBUG_LOGERROR("PK origin is not match");
									break;
								}

								if (!Fight_DoPK(oComponent->fight)) {
									static NetProto_Error error;
									error.set_content(Config_Words(64));
									GCAgent_SendProtoToClients(&id, 1, &error);
								}
							}
							break;

						case NetProto_Strong::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_Strong strong;
								if (!strong.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								int res = PlayerEntity_Strong(player, strong.pos() == NetProto_Strong::BODY, strong.id(), strong.protect());
								if (res == -1) {
									return;
								} else if (res == 0) {
									strong.set_results(NetProto_Strong::SUCCESS);
								} else if (res == 1) {
									strong.set_results(NetProto_Strong::NOTHING);
								} else if (res == 2) {
									strong.set_results(NetProto_Strong::FAILURE);
								} else {
									break;
								}

								PlayerEntity_ActiveAddGoods(player, 1);
								GCAgent_SendProtoToClients(&id, 1, &strong);
							}
							break;

						case NetProto_Transform::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_Transform transform;
								if (!transform.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(component->roleAtt->movementAtt().mapID()));
								if (mapInfo == NULL || mapInfo->mapType() == MapInfo::PEACE)
									break;

								if (Fight_Transform(component->fight, true) == -1) {
									break;
								}

								transform.set_id(id);
								GCAgent_SendProtoToAllClientsInSameScene(component->roleAtt->movementAtt().mapID(), player, &transform);
							}
							break;

						case NetProto_Mount::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_Mount mount;
								if (!mount.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								int32_t res = PlayerEntity_Mount(player, mount.pos() == NetProto_Mount::BODY, mount.id(), mount.mountPos(), mount.gemPos());
								if (res == -1)
									break;

								GCAgent_SendProtoToClients(&id, 1, &mount);
							}
							break;

							/*
							   case NetProto_Recover::UNITID:
							   {
							   if (InSingle(id))
							   break;

							   static NetProto_Recover recover;
							   if (!recover.ParseFromArray(data, size))
							   break;

							   struct PlayerEntity *player = PlayerEntity_Player(id);
							   if (player == NULL)
							   break;
							   struct Component *component = PlayerEntity_Component(player);

							   if (Fight_Recover(component->fight) == -1)
							   break;

							   recover.set_id(id);
							   GCAgent_SendProtoToAllClientsInSameScene(Movement_Att(component->movement)->mapID(), player, &recover);
							   }
							   break;
							   */

						case NetProto_AddBloodNode::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_AddBloodNode addBloodNode;
								if (!addBloodNode.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								int res = Fight_AddBloodNode(component->fight, addBloodNode.type());
								if (res == 0)
									addBloodNode.set_res(true);
								else if (res == -2)
									addBloodNode.set_res(false);
								else
									break;

								GCAgent_SendProtoToClients(&id, 1, &addBloodNode);
							}
							break;

						case NetProto_AddBloodEffect::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_AddBloodEffect addBloodEffect;
								if (!addBloodEffect.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								int count;
								int res = Fight_AddBloodEffect(component->fight, addBloodEffect.soul(), addBloodEffect.type() == NetProto_AddBloodEffect::ALL, &count);
								if (res == -1)
									break;

								addBloodEffect.set_res(res);
								addBloodEffect.set_count(count);
								GCAgent_SendProtoToClients(&id, 1, &addBloodEffect);
							}
							break;

						case NetProto_Explore::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_Explore explore;
								if (!explore.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								int32_t res = Item_Explore(component->item, explore.type(), false);
								if (res == -1) {
									// DEBUG_LOGERROR("Failed to explore");
									break;
								}

								explore.set_event1(res);
								explore.set_event2(-1);

								/*
								   bool lucky = false;
								   if (explore.type() == ExploreInfo::NORMAL)
								   lucky = Time_CanPass(Config_NormalExploreRate());
								   else if (explore.type() == ExploreInfo::HIGH)
								   lucky = Time_CanPass(Config_HighExploreRate());

								   if (lucky) {
								   res = Item_Explore(component->item, explore.type(), true);
								   if (res == -1) {
								   DEBUG_LOGERROR("Failed to explore");
								   break;
								   }

								   explore.set_event2(res);
								   }
								   */

								GCAgent_SendProtoToClients(&id, 1, &explore);
							}
							break;

							/*
							   case NetProto_UnlockBlood::UNITID:
							   {
							   if (InSingle(id))
							   break;

							   static NetProto_UnlockBlood unlockBlood;
							   if (!unlockBlood.ParseFromArray(data, size))
							   break;

							   struct PlayerEntity *player = PlayerEntity_Player(id);
							   if (player == NULL)
							   break;
							   struct Component *component = PlayerEntity_Component(player);

							   if (Fight_UnlockBlood(component->fight) == -1) {
							   DEBUG_LOGERROR("Failed to unlock blood");
							   break;
							   }

							   GCAgent_SendProtoToClients(&id, 1, &unlockBlood);
							   }
							   break;
							   */

						case NetProto_UpdatePos::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_UpdatePos updatePos;
								if (!updatePos.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								if (Movement_CantMove(component->movement))
									break;

								static Vector3f pos;
								pos.FromPB(&updatePos.pos());
								Movement_SetPosition(component->movement, &pos);

								updatePos.set_id(id);
								GCAgent_SendProtoToAllClientsExceptOneInSameScene(player, &updatePos);
							}
							break;

						case NetProto_SayHello::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_SayHello sayHello;
								if (!sayHello.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								struct PlayerEntity *target = PlayerEntity_PlayerByRoleID(sayHello.destID());
								if (target == NULL)
									break;
								struct Component *tComponent = PlayerEntity_Component(target);

								/*
								   Item_ModifyDurability(component->item, Config_DurabilityDelta());
								   PlayerEntity_SetDayEvent(player, 2, component->playerAtt->dayEvent(2) + Config_DurabilityDelta());
								   */

								if (tComponent->playerAtt->dayEvent(2) < Config_MaxDurabilityDelta()) {
									Item_ModifyDurability(tComponent->item, Config_DurabilityDelta());
									PlayerEntity_SetDayEvent(target, 2, tComponent->playerAtt->dayEvent(2) + Config_DurabilityDelta());

									sayHello.set_srcID(PlayerEntity_RoleID(player));
									sayHello.set_srcName(component->baseAtt->name());
									int32_t tID = PlayerEntity_ID(target);
									GCAgent_SendProtoToClients(&tID, 1, &sayHello);
									GCAgent_SendProtoToClients(&id, 1, &sayHello);
								} else {
									sayHello.set_srcID(PlayerEntity_RoleID(player));
									sayHello.set_destID(-1);
									GCAgent_SendProtoToClients(&id, 1, &sayHello);
								}
							}
							break;

						case NetProto_CompleteGuide::UNITID:
							{
								static NetProto_CompleteGuide completeGuide;
								if (!completeGuide.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								PlayerEntity_CompleteGuide(player, completeGuide.id());

								char arg1[16];
								SNPRINTF1(arg1, "{\"id\":\"%d\"}", completeGuide.id());
								ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "NewUserGuid", arg1));
							}
							break;

						case NetProto_Arrange::UNITID:
							{
								static NetProto_Arrange arrange;
								if (!arrange.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								int res = Item_Arrange(component->item, (ItemPackage::Begin)arrange.begin());
								if (res == -1) {
									break;
								} else if (res == 0) {
									arrange.clear_items();
									const ItemPackage *package = Item_Package(component->item);
									if (arrange.begin() == PB_ItemPackage::EQUIPMENT) {
										for (int i = 0; i < package->validNumEquipment(); i++)
											package->items((int)PB_ItemPackage::EQUIPMENT + i).ToPB(arrange.add_items());
									} else if (arrange.begin() == PB_ItemPackage::GOODS) {
										for (int i = 0; i < package->validNumGoods(); i++)
											package->items((int)PB_ItemPackage::GOODS + i).ToPB(arrange.add_items());
									} else if (arrange.begin() == PB_ItemPackage::GEM) {
										for (int i = 0; i < package->validNumGem(); i++)
											package->items((int)PB_ItemPackage::GEM + i).ToPB(arrange.add_items());
									} else {
										break;
									}

									GCAgent_SendProtoToClients(&id, 1, &arrange);
								} else if (res == 1) {
									arrange.clear_items();
									GCAgent_SendProtoToClients(&id, 1, &arrange);
								}
							}
							break;

						case NetProto_IgnorePK::UNITID:
							{
								static NetProto_IgnorePK ignorePK;
								if (!ignorePK.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								Fight_SetIgnorePK(component->fight, true);
							}
							break;

						case NetProto_RandomGodTarget::UNITID:
							{
								static NetProto_RandomGodTarget rgt;
								if (!rgt.ParseFromArray(data, size))
									break;

								// TODO
								/*
								   struct PlayerEntity *player = PlayerEntity_Player(id);
								   if (player == NULL)
								   break;
								   struct Component *component = PlayerEntity_Component(player);

								   const ItemPackage *package = Item_Package(component->item);
								   if (package->money() < Config_RandomGodTargetCost())
								   break;

								   int range = Fight_PowerRank(component->fight);
								   if (range == -1)
								   break;

								   int64_t target = PlayerPool_RandomPower(range, PlayerEntity_RoleID(player), Fight_Att(component->fight)->godTarget());
								   if (target == -1)
								   break;

								   Item_ModifyMoney(component->item, -Config_RandomGodTargetCost());

								   static DCProto_GodTarget gt;
								   gt.set_id(id);
								   gt.set_roleID(target);
								   GCAgent_SendProtoToDCAgent(&gt);
								   */
							}
							break;

						case NetProto_Inspire::UNITID:
							{
								static NetProto_Inspire inspire;
								if (!inspire.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								const PlayerAtt *roleAtt = PlayerEntity_Att(player);
								if (roleAtt == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								int index = 0;
								if (inspire.dest() == NetProto_Inspire::GOD)
									index = 13;
								else if (inspire.dest() == NetProto_Inspire::ONE_TO_ONE)
									index = 17;
								else if (inspire.dest() == NetProto_Inspire::HELL)
									index = 18;
								else if (inspire.dest() == NetProto_Inspire::WORLD_BOSS)
									index = 19;
								else
									break;

								int event = component->playerAtt->fixedEvent(index);
								NetProto_Inspire::Type r[3];
								int n1 = event & 0xff;
								int n2 = (event & 0xff00) >> 8;
								int n3 = (event & 0xff0000) >> 16;
								int n = 0;

								const VIPInfo *vip = VIPInfoManager_VIPInfo(component->playerAtt->itemPackage().vip());
								if (n1 + n2 + n3 >= vip->inspireCount())
									break;

								if (n1 < Config_MaxInspireCount())
									r[n++] = NetProto_Inspire::ATK;
								if (n2 < Config_MaxInspireCount())
									r[n++] = NetProto_Inspire::DEF;
								if (n3 < Config_MaxInspireCount())
									r[n++] = NetProto_Inspire::SPECIAL;
								if (n <= 0)
									break;

								int32_t cost = 1 + n1 + n2 + n3;
								cost *= 2;
								//const ItemPackage *package = Item_Package(component->item);
								//if (package->subRMB() + package->rmb() < cost)
								if (!Item_HasRMB(component->item, cost))
									break;

								NetProto_Inspire::Type type = r[Time_Random(0, n)];
								if (type == NetProto_Inspire::ATK) {
									PlayerEntity_SetFixedEvent(player, index, (event & 0xffffff00) | (n1 + 1));
								} else if (type == NetProto_Inspire::DEF) {
									PlayerEntity_SetFixedEvent(player, index, (event & 0xffff00ff) | ((n2 + 1) << 8));
								} else if (type == NetProto_Inspire::SPECIAL) {
									PlayerEntity_SetFixedEvent(player, index, (event & 0xff00ffff) | ((n3 + 1) << 16));
								} else {
									assert(0);
								}

								if (inspire.dest() == NetProto_Inspire::ONE_TO_ONE) {
									PlayerEntity_SetFixedEvent(component->player, 40, (int)Time_TimeStamp() + 3600 * 24);
								}

								Item_ModifyRMB(component->item, -cost, false, 9, type, 0);

								inspire.set_type(type);
								GCAgent_SendProtoToClients(&id, 1, &inspire);

								if (Config_ScribeHost() != NULL) {
									int event = component->playerAtt->fixedEvent(18);
									int n1 = event & 0xff;
									int n2 = (event & 0xff00) >> 8;
									int n3 = (event & 0xff0000) >> 16;
									static char buf[CONFIG_FIXEDARRAY];
									memset(buf, 0, sizeof(buf) / sizeof(char));
									char *index = buf;

									SNPRINTF2(buf, index, "{\"roleID\":\"%lld", (long long int)roleAtt->att().baseAtt().roleID());
									index += strlen(index);
									SNPRINTF2(buf, index, "\",\"roleName\":\"%s", roleAtt->att().baseAtt().name());
									index += strlen(index);
									SNPRINTF2(buf, index, "\",\"inspireCount\":\"%d", n1 + n2 + n3);
									index += strlen(index);
									SNPRINTF2(buf, index, "}");
									ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "InspireCount", buf));
								}
							}
							break;

						case NetProto_RequestGodTarget::UNITID:
							{
								static NetProto_RequestGodTarget rgt;
								if (!rgt.ParseFromArray(data, size))
									break;

								// TODO
								/*
								   struct PlayerEntity *player = PlayerEntity_Player(id);
								   if (player == NULL)
								   break;
								   struct Component *component = PlayerEntity_Component(player);

								   int64_t target = Fight_Att(component->fight)->godTarget();
								   if (target == -1) {
								   int range = Fight_PowerRank(component->fight);
								   if (range == -1)
								   break;

								   target = PlayerPool_RandomPower(range, PlayerEntity_RoleID(player), Fight_Att(component->fight)->godTarget());
								   if (target == -1)
								   break;
								   }

								   static DCProto_GodTarget gt;
								   gt.set_id(id);
								   gt.set_roleID(target);
								   GCAgent_SendProtoToDCAgent(&gt);
								   */
							}
							break;

						case NetProto_GodRank::UNITID:
							{
								static NetProto_GodRank gr;
								if (!gr.ParseFromArray(data, size))
									break;

								// TODO
								/*
								   struct PlayerEntity *player = PlayerEntity_Player(id);
								   if (player == NULL)
								   break;
								   struct Component *component = PlayerEntity_Component(player);

								   int64_t target = Fight_Att(component->fight)->godTarget();
								   if (target == -1)
								   break;

								   gr.set_self(PlayerPool_SelfInRank(NetProto_Rank::GOD, player, NULL));
								   gr.set_target(PlayerPool_SelfInRank(NetProto_Rank::GOD, target, Fight_GodTargetScore(component->fight), NULL));
								   GCAgent_SendProtoToClients(&id, 1, &gr);
								   */
							}
							break;

						case NetProto_GenEquip::UNITID:
							{
								static NetProto_GenEquip ge;
								if (!ge.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								Item_GenEquip(component->item, ge.id());
							}
							break;

						case NetProto_GenGem::UNITID:
							{
								static NetProto_GenGem ge;
								if (!ge.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								Item_GenGem(component->item, ge.level(), ge.type(), ge.way());
							}
							break;

						case NetProto_UnmountGem::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_UnmountGem proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								if (PlayerEntity_Unmount(player, proto.pos() == NetProto_UnmountGem::BODY, proto.id(), proto.index()) == -1)
									break;

								GCAgent_SendProtoToClients(&id, 1, &proto);
							}
							break;

						case NetProto_EnhanceDelta::UNITID:
							{
								static NetProto_EnhanceDelta proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								int32_t delta[CONFIG_FIXEDARRAY];
								int count = PlayerEntity_EnhanceDelta(player, proto.pos() == NetProto_EnhanceDelta::BODY, proto.id(), proto.ten(), delta, CONFIG_FIXEDARRAY);
								if (count == -1)
									break;

								proto.clear_delta();
								for (int i = 0; i < count; i++)
									proto.add_delta(delta[i]);

								GCAgent_SendProtoToClients(&id, 1, &proto);
								DEBUG_LOG("send");
							}
							break;

						case NetProto_Enhance::UNITID:
							{
								static NetProto_Enhance proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								if (PlayerEntity_Enhance(player, proto.pos() == NetProto_Enhance::BODY, proto.id()) == -1)
									break;

								GCAgent_SendProtoToClients(&id, 1, &proto);
							}
							break;

						case NetProto_Inherit::UNITID:
							{
								static NetProto_Inherit proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								if (PlayerEntity_Inherit(player, proto.parentPos() == NetProto_Inherit::BODY, proto.parentID(), proto.childPos() == NetProto_Inherit::BODY,
											proto.childID(), proto.useRMB(), proto.useStone()) == -1)
									break;

								GCAgent_SendProtoToClients(&id, 1, &proto);
							}
							break;

						case NetProto_StrongBaseWing::UNITID:
							{
								static NetProto_StrongBaseWing proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								if (Fight_StrongBaseWing(component->fight, &proto) >= 0) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						case NetProto_BuyWing::UNITID:
							{
								static NetProto_BuyWing proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								Item_BuyWing(component->item, proto.id(), proto.forever());
							}
							break;

						case NetProto_WearWing::UNITID:
							{
								static NetProto_WearWing proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								if (Equipment_WearWing(component->equipment, proto.id(), proto.baseWing()) == -2)
									break;

								GCAgent_SendProtoToClients(&id, 1, &proto);
							}
							break;

						case NetProto_UnwearWing::UNITID:
							{
								static NetProto_UnwearWing proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								if (Equipment_UnwearWing(component->equipment) == -2)
									break;

								GCAgent_SendProtoToClients(&id, 1, &proto);
							}
							break;

						case NetProto_BuyFashion::UNITID:
							{
								static NetProto_BuyFashion proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								Item_BuyFashion(component->item, proto.id(), proto.forever());
							}
							break;
						case NetProto_MakeFashionHole::UNITID:
							{
								static NetProto_MakeFashionHole proto;
								if (!proto.ParseFromArray(data, size)) {
									break;
								}
								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL) {
									break;
								}
								struct Component *component = PlayerEntity_Component(player);
								if (Item_MakeFashionHole(component->item, proto.id(), proto.index())) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}

							break;
						case NetProto_FashionInlay::UNITID:
							{
								static NetProto_FashionInlay proto;
								if (!proto.ParseFromArray(data, size)) {
									break;
								}
								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL) {
									break;
								}
								struct Component *component = PlayerEntity_Component(player);
								if (Item_FashionInlay(component->item, proto.id(), proto.index(), proto.runeid(), proto.flag())) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;
						case NetProto_FashionUnInlay::UNITID:
							{
								static NetProto_FashionUnInlay proto;
								if (!proto.ParseFromArray(data, size)) {
									break;
								}
								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL) {
									break;
								}
								struct Component *component = PlayerEntity_Component(player);
								if (Item_FashionUnInlay(component->item, proto.id(), proto.index())) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;
						case NetProto_ServerLimitItem::UNITID:
							{
								static NetProto_ServerLimitItem proto;
								if (!proto.ParseFromArray(data, size))
									break;

								proto.set_count(PlayerPool_ServerDayNum((ItemInfo::Type)proto.type(), proto.id()));
								GCAgent_SendProtoToClients(&id, 1, &proto);
							}
							break;

						case NetProto_FriendsLove::UNITID:
							{
								static NetProto_FriendsLove love;
								love.Clear();
								if (!love.ParseFromArray(data, size)) {
									break;
								}
								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								if (PlayerEntity_FriendsLove(player, love.roleID(), love.flag())) {
									GCAgent_SendProtoToClients(&id, 1, &love);
								}
							}
							break;

						case NetProto_BuyDurability::UNITID:
							{
								static NetProto_BuyDurability gc;
								gc.Clear();
								if (!gc.ParseFromArray(data, size)) {
									break;
								}

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								if (Item_BuyDurability(component->item, gc.index())) {
									GCAgent_SendProtoToClients(&id, 1, &gc);
								}
							}
							break;

						case NetProto_LoginObtRMB::UNITID:
							{
								static NetProto_LoginObtRMB gc;
								gc.Clear();
								if (!gc.ParseFromArray(data, size)) {
									break;
								}

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								int count = PlayerEntity_LoginObtRMB(player);
								if (-1 != count) {
									gc.set_count(count);
									GCAgent_SendProtoToClients(&id, 1, &gc);
								}
							}
							break;	

						case NetProto_ResetCount::UNITID:
							{
								static NetProto_ResetCount proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								if (Fight_ResetCount(component->fight, proto.type(), proto.arg()))
									GCAgent_SendProtoToClients(&id, 1, &proto);
							}
							break;

						case NetProto_GodRandomPlayer::UNITID:
							{
								static NetProto_GodRandomPlayer proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								const PlayerAtt *roleAtt = PlayerEntity_Att(player);
								if (roleAtt == NULL)
									break;

								int res = PlayerPool_RandomGod(roleAtt->att().baseAtt().roleID(), &proto);
								DEBUG_LOG("LLLLLLL%d", res);
								if (res == 0)
									GCAgent_SendProtoToClients(&id, 1, &proto);
							}
							break;

						case NetProto_GodRecords::UNITID:
							{
								static NetProto_GodRecords proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								PlayerEntity_GodRecordsRequest(player, &proto);
								GCAgent_SendProtoToClients(&id, 1, &proto);
							}
							break;

						case NetProto_GodPanel::UNITID:
							{
								static NetProto_GodPanel proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								const PlayerAtt *roleAtt = PlayerEntity_Att(player);
								if (roleAtt == NULL)
									break;

								PlayerEntity_GodPanel(player, &proto);
								PlayerPool_QueryGodRank(roleAtt->att().baseAtt().roleID(), &proto);
								GCAgent_SendProtoToClients(&id, 1, &proto);
							}
							break;

						case NetProto_SelectGodRole::UNITID:
							{
								static NetProto_SelectGodRole proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								const PlayerAtt *roleAtt = PlayerEntity_Att(player);
								if (roleAtt == NULL)
									break;

								struct Component *component = PlayerEntity_Component(player);
								int event = component->playerAtt->dayEvent(29);
								time_t OpenTime = (time_t)Config_OpenTime(NULL);
								struct tm local;
								Time_LocalTime(OpenTime, &local);
								tm tmOpenTime = local;
								time_t t = Time_TimeStamp();
								Time_LocalTime(t, &local);

								if (t - roleAtt->godDueDate() < 300) {
									break;
								}	

								if (tmOpenTime.tm_year == local.tm_year && tmOpenTime.tm_mon == local.tm_mon && tmOpenTime.tm_mday == local.tm_mday) {
								} else {
									if ((event & 0xFFFF) >= Config_MaxGodCount())
										break;
								}

								PlayerPool_SelectGodRole(roleAtt->att().baseAtt().roleID(), proto.roleID());
								struct PlayerEntity *target = PlayerEntity_PlayerByRoleID(proto.roleID());
								if (target == NULL) {
									static DCProto_QueryGodRole queryRole;
									queryRole.set_id(id);
									queryRole.set_roleID(proto.roleID());
									GCAgent_SendProtoToDCAgent(&queryRole);
								} else {
									PlayerEntity_ToSceneData(PlayerEntity_Att(target), proto.mutable_att());
									for (int i = 0; i < proto.att().att().equipmentAtt().equipments_size(); ++i) {
										DEBUG_LOG("DDDDDDDDDDDDDDDD%lld", proto.att().att().equipmentAtt().equipments(i));
									}
									for (int i = 0; i < proto.att().itemPackage().equips_size(); ++i) {
										DEBUG_LOG("WWWWWWWWWWWWWWWW%lld", proto.att().itemPackage().equips(i).mode());
									}
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						case NetProto_ResolveEquips::UNITID:
							{
								static NetProto_ResolveEquips proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								struct Component *component = PlayerEntity_Component(player);
								if (component == NULL)
									break;


								static NetProto_GetRes getRes;
								getRes.Clear();
								int totalMoney = 0;
								if (proto.op() >= 0) {
									const ItemInfo* info = Item_PackageItem(component->item, proto.op());
									if (info == NULL) {
										break;
									}

									if (info->type() != ItemInfo::EQUIPMENT) {
										break;
									}

									const EquipmentInfo* equipInfo = Item_EquipInfo(component->item, info->id());
									if (equipInfo == NULL) {
										break;
									}

									int moneyCount = 0;
									for (int i = 0; i < equipInfo->decomposeItems_size(); ++i) {
										if (equipInfo->decomposeItems(i).type() == PB_ItemInfo::NONE)
											continue;

										if (equipInfo->decomposeItems(i).type() == PB_ItemInfo::MONEY) {
											moneyCount += equipInfo->decomposeItems(i).count();
										}else {
											Item_AddToPackage(component->item, (ItemInfo::Type)equipInfo->decomposeItems(i).type(), equipInfo->decomposeItems(i).id(), equipInfo->decomposeItems(i).count(), &getRes);

											int64_t roleID = PlayerEntity_RoleID(player);
											char ch[1024];
											SNPRINTF1(ch, "1-%d-%d", (int)equipInfo->decomposeItems(i).id(), (int)equipInfo->decomposeItems(i).count());
											DCProto_SaveSingleRecord saveRecord;
											saveRecord.set_mapID(-23);
											saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
											saveRecord.mutable_record()->mutable_role()->set_name(ch);
											saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
											GCAgent_SendProtoToDCAgent(&saveRecord);

										}
									}
									totalMoney += moneyCount;

									Item_ModifyMoney(component->item, moneyCount);
									Item_DelFromPackage(component->item, proto.op(), 1, true);
								}else {
									const ItemPackage *package = Item_Package(component->item);
									if (package == NULL)
										break;

									for (int i = ItemPackage::EQUIPMENT; i < ItemPackage::EQUIPMENT + package->validNumEquipment(); ++i) {
										const ItemInfo* info = Item_PackageItem(component->item, i);
										if (info == NULL)
											continue;

										if (info->type() != ItemInfo::EQUIPMENT)
											continue;

										const EquipmentInfo* equipInfo = Item_EquipInfo(component->item, info->id());
										if (equipInfo == NULL) {
											break;
										}

										int moneyCount = 0;
										int op = abs(proto.op()) - 1;
										if (equipInfo->colorType() <= (EquipmentInfo::ColorType)op) {
											for (int j = 0; j < equipInfo->decomposeItems_size(); ++j) {
												if (equipInfo->decomposeItems(j).type() == PB_ItemInfo::NONE)
													continue;

												if (equipInfo->decomposeItems(j).type() == PB_ItemInfo::MONEY) {
													moneyCount += equipInfo->decomposeItems(j).count();
												}else {
													Item_AddToPackage(component->item, (ItemInfo::Type)equipInfo->decomposeItems(j).type(), equipInfo->decomposeItems(j).id(), equipInfo->decomposeItems(j).count(), &getRes);
													int64_t roleID = PlayerEntity_RoleID(player);
													char ch[1024];
													SNPRINTF1(ch, "2-%d-%d", (int)equipInfo->decomposeItems(j).id(), (int)equipInfo->decomposeItems(j).count());
													DCProto_SaveSingleRecord saveRecord;
													saveRecord.set_mapID(-23);
													saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
													saveRecord.mutable_record()->mutable_role()->set_name(ch);
													saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
													GCAgent_SendProtoToDCAgent(&saveRecord);
												}
											}
											Item_ModifyMoney(component->item, moneyCount);
											Item_DelFromPackage(component->item, i, 1, true);
										}
										totalMoney += moneyCount;
									}
								}

								if (totalMoney > 0) {
									PB_ItemInfo *item = getRes.add_items();
									item->set_type(PB_ItemInfo::MONEY);
									item->set_count(totalMoney);
								}
								GCAgent_SendProtoToClients(&id, 1, &proto);
								if (getRes.items_size() > 0)
									GCAgent_SendProtoToClients(&id, 1, &getRes);
							}
							break;

						case NetProto_DropItem::UNITID:
							{
								static NetProto_DropItem proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								const MapInfo *mapInfo = MapInfoManager_MapInfo(MapPool_MapInfo(component->roleAtt->movementAtt().mapID()));
								if (mapInfo == NULL)
									break;
								const BlockInfo *blockInfo = MapInfoManager_BlockInfo(mapInfo);
								if (blockInfo == NULL)
									break;

								int x = blockInfo->logicLength();
								int y = blockInfo->logicWidth();

								const int npcKey = 278394234;
								const int indexKey = 848293;
								const int dropKey = 73847293;

								int npc = proto.npc() ^ npcKey;
								int index = proto.index() ^ indexKey;

								int64_t v1 = (npc ^ dropKey) + ((x | 0x308) & y) + (index ^ proto.v1());
								if (v1 != proto.v()) {
									DEBUG_LOG("Failed to check drop item, client: %lld, server: %lld", proto.v(), v1);
									break;
								}

								int ret = Fight_DropItem(component->fight, npc, index, proto.v2());
								if (ret >= 0) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								} else {
									DEBUG_LOG("Failed to drop item, err: %d", ret);
								}
							}
							break;

						case NetProto_SellGoods::UNITID:
							{
								static NetProto_SellGoods proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								struct Component *component = PlayerEntity_Component(player);
								if (component == NULL)
									break;

								const ItemPackage *package = Item_Package(component->item);
								if (package == NULL)
									break;

								const VIPInfo *vipInfo = VIPInfoManager_VIPInfo(package->vip());
								if (vipInfo != NULL) {
									if (!vipInfo->quickSell())
										break;
								}

								int count = 0;
								int price = 0;
								int moneyCount = 0;
								if (proto.type() == PB_ItemPackage::GOODS) {
									for (int i = ItemPackage::GOODS; i < ItemPackage::GOODS + package->validNumGoods(); ++i) {
										const ItemInfo* info = Item_PackageItem(component->item, i);
										if (info == NULL)
											continue;

										if (info->type() != ItemInfo::GOODS)
											continue;

										const GoodsInfo *goods = GoodsInfoManager_GoodsInfo(info->id());
										if (goods == NULL) 
											continue;

										if (goods->colorType() <= proto.op()) {
											DEBUG_LOG("goodsid: %d", info->id());
											count = Item_Count(component->item, ItemInfo::GOODS, info->id());
											price = GoodsInfoManager_GoodsInfo(info->id())->price() / 2;
											moneyCount += price * count;
											Item_DelFromPackage(component->item, ItemInfo::GOODS, info->id(), count);
										}
									}
								}else if (proto.type() == PB_ItemPackage::GEM) {
									for (int i = ItemPackage::GEM; i < ItemPackage::GEM + package->validNumGem(); ++i) {
										const ItemInfo* info = Item_PackageItem(component->item, i);
										if (info == NULL)
											continue;

										if (info->type() != ItemInfo::GOODS)
											continue;

										const GoodsInfo *goods = GoodsInfoManager_GoodsInfo(info->id());
										if (goods == NULL) 
											continue;

										if (goods->colorType() <= proto.op()) {
											DEBUG_LOG("goodsid: %d", info->id());
											count = Item_Count(component->item, ItemInfo::GOODS, info->id());
											price = GoodsInfoManager_GoodsInfo(info->id())->price() / 2;
											moneyCount += price * count;
											Item_DelFromPackage(component->item, ItemInfo::GOODS, info->id(), count);
										}
									}
								}

								Item_ModifyMoney(component->item, moneyCount);
								static NetProto_Info error;
								char buf[CONFIG_FIXEDARRAY];
								SNPRINTF1(buf, Config_Words(72), moneyCount);
								error.set_content(buf);
								GCAgent_SendProtoToClients(&id, 1, &error);
								GCAgent_SendProtoToClients(&id, 1, &proto);
							}
							break;

						case NetProto_Luck::UNITID:
							{
								static NetProto_Luck proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								int res = PlayerEntity_Luck(player, &proto);
								if (res == 0) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						case NetProto_GrabRedEnvelope::UNITID:
							{
								static NetProto_GrabRedEnvelope proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								int64_t roleID = PlayerEntity_RoleID(player);
								PlayerPool_GrabRedEnvelope(roleID);
							}
							break;

						case NetProto_CatGift::UNITID:
							{
								static NetProto_CatGift proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								int64_t roleID = PlayerEntity_RoleID(player);
								PlayerPool_CatGift(roleID, &proto);
							}
							break;

						case NetProto_GroupPurchase::UNITID:
							{
								static NetProto_GroupPurchase proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								int64_t roleID = PlayerEntity_RoleID(player);
								PlayerPool_GroupPurchase(roleID, &proto);
							}
							break;
						case NetProto_Rides::UNITID:
							{
								static NetProto_Rides proto;
								proto.Clear();
								if(!proto.ParseFromArray(data, size))
									break;
								struct PlayerEntity *player = PlayerEntity_Player(id);
								if(player == NULL)
									break;

								struct Component *component = PlayerEntity_Component(player);
								if(component == NULL)
									break;

								int re = Equipment_Ride(component->equipment, proto.rides(), proto.ride());
								if(re < 0)
								{
									DEBUG_LOGERROR("ride failed->%d", re);
									proto.set_rides(re);
								}

								GCAgent_SendProtoToClients(&id, 1, &proto);
							}
							break;
						case NetProto_RidesTrain::UNITID:
							{
								static NetProto_RidesTrain proto;
								proto.Clear();
								if(!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if(player == NULL)
									break;

								struct Component *component = PlayerEntity_Component(player);
								if(component == NULL)
									break;

								int re = Item_RidesTrain(component->item, proto.index(), proto.high());
								if(re < 0)
								{
									proto.set_re(re);
									GCAgent_SendProtoToClients(&id, 1, &proto);

									DEBUG_LOGERROR("ride train failed->%d", re);
								}
							}
							break;
						case NetProto_RidesConfirmTrain::UNITID:
							{
								static NetProto_RidesConfirmTrain proto;
								proto.Clear();
								if(!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if(player == NULL)
									break;

								struct Component *component = PlayerEntity_Component(player);
								if(component == NULL)
									break;

								int re = Item_RidesConfirmTrain(component->item, proto);
								if(re < 0)
								{
									proto.set_re(re);
									GCAgent_SendProtoToClients(&id, 1, &proto);

									DEBUG_LOGERROR("rides confirm train error->%d", re);
								}
							}
							break;
						case NetProto_RidesInherit::UNITID:
							{
								static NetProto_RidesInherit proto;
								proto.Clear();
								if(!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if(player == NULL)
									break;

								struct Component *component = PlayerEntity_Component(player);
								if(component == NULL)
									break;

								int re = Item_RidesInherit(component->item, proto);
								if(re < 0)
								{
									proto.set_re(re);
									GCAgent_SendProtoToClients(&id, 1, &proto);
									DEBUG_LOGERROR("rides inherit error->%d", re);
								}
							}
							break;
						case NetProto_RidesUP::UNITID:
							{
								static NetProto_RidesUP proto;
								proto.Clear();
								if(!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if(player == NULL)
									break;

								struct Component *component = PlayerEntity_Component(player);
								if(component == NULL)
									break;

								int re = Item_RidesUP(component->item, proto);
								if(re < 0)
								{
									proto.set_re(re);
									GCAgent_SendProtoToClients(&id, 1, &proto);
									DEBUG_LOGERROR("rides up error->%d", re);
								}
							}
							break;
						case NetProto_RidesLockAtt::UNITID:
							{
								static NetProto_RidesLockAtt proto;
								proto.Clear();
								if(!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if(player == NULL)
									break;

								struct Component *component = PlayerEntity_Component(player);
								if(component == NULL)
									break;

								int re = Item_RidesLockAtt(component->item, proto);
								if(re < 0)
								{
									proto.set_re(re);
									GCAgent_SendProtoToClients(&id, 1, &proto);
									DEBUG_LOGERROR("rides lockatt->%d", re);
								}
							}
							break;
						case NetProto_GodShip::UNITID:
							{    
								static NetProto_GodShip proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								struct Component *component = PlayerEntity_Component(player);
								if (component == NULL)
									break;
								const ItemPackage *package = Item_Package(component->item);
								if (package == NULL)
									break;

								if (proto.next() && proto.cut() == 0) {
									if (component->playerAtt->fixedEvent(112) == 0)
										PlayerEntity_SetFixedEvent(component->player, 112, component->playerAtt->fixedEvent(112) | (1 << 0));

									for (int index = 0; index < 5; index++) {
										if (component->playerAtt->fixedEvent(112) & (1 << index)) {

											if (package->money() < Config_GodShipCost(index))
												break;
											Item_ModifyMoney(component->item, -Config_GodShipCost(index));
											int32_t res[CONFIG_FIXEDARRAY];
											Item_Lottery(component->item, 695 + index, res, CONFIG_FIXEDARRAY, true, NULL);
											PlayerEntity_SetFixedEvent(component->player, 112, component->playerAtt->fixedEvent(112) & ~(1 << index), true);

											int randnum = Time_Random(1, 100);
											if (randnum <= Config_GodShipRandom(index)) {
												proto.set_index(index + 1);
												PlayerEntity_SetFixedEvent(component->player, 112, component->playerAtt->fixedEvent(112) | (1 << (index + 1)), true);
											} else {
												proto.set_index(0);
												PlayerEntity_SetFixedEvent(component->player, 112, component->playerAtt->fixedEvent(112) | (1 << 0), true);
											}

											GCAgent_SendProtoToClients(&id, 1, &proto);

											break;
										}
									}
								}

								if (!proto.next() && proto.cut() == 0) {
									for (int i = 0; i < 10; i++) {
										if (component->playerAtt->fixedEvent(112) == 0)
											PlayerEntity_SetFixedEvent(component->player, 112, component->playerAtt->fixedEvent(112) | (1 << 0));

										for (int index = 0; index < 5; index++) {
											if (component->playerAtt->fixedEvent(112) & (1 << index)) {

												if (package->money() < Config_GodShipCost(index))
													break;
												Item_ModifyMoney(component->item, -Config_GodShipCost(index));
												int32_t res[CONFIG_FIXEDARRAY];
												Item_Lottery(component->item, 695 + index, res, CONFIG_FIXEDARRAY, true, NULL);
												PlayerEntity_SetFixedEvent(component->player, 112, component->playerAtt->fixedEvent(112) & ~(1 << index), true);

												int randnum = Time_Random(1, 100);
												if (randnum <= Config_GodShipRandom(index)) {
													proto.set_index(index + 1);
													PlayerEntity_SetFixedEvent(component->player, 112, component->playerAtt->fixedEvent(112) | (1 << (index + 1)), true);
												} else {
													proto.set_index(0);
													PlayerEntity_SetFixedEvent(component->player, 112, component->playerAtt->fixedEvent(112) | (1 << 0), true);
												}

												GCAgent_SendProtoToClients(&id, 1, &proto);

												break;
											}
										}
									}
								}

								if (proto.cut() == 1) { 
									if (!Item_HasRMB(component->item, 200)) 
										break;
									Item_ModifyRMB(component->item, -200, false, 0);
									int32_t res[CONFIG_FIXEDARRAY];
									Item_Lottery(component->item, 699, res, CONFIG_FIXEDARRAY, true, NULL);
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;
						case NetProto_WearGodShip::UNITID:
							{
								static NetProto_WearGodShip proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								struct Component *component = PlayerEntity_Component(player);
								if (component == NULL)
									break;
								int32_t i = Item_UseGodShip(component->item, proto.index());
								proto.set_index(i);
								GCAgent_SendProtoToClients(&id, 1, &proto);

							}
							break;
						case NetProto_UnWearGodShip::UNITID:
							{
								static NetProto_UnWearGodShip proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								struct Component *component = PlayerEntity_Component(player);
								if (component == NULL)
									break;
								int i = Item_UnUseGodShip(component->item, proto.index());
								proto.set_index(i);
								GCAgent_SendProtoToClients(&id, 1, &proto);

							}
							break;
						case NetProto_Swallow::UNITID:
							{
								static NetProto_Swallow proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								GCAgent_SendProtoToClients(&id, 1, &proto);

								struct Component *component = PlayerEntity_Component(player);
								if (component == NULL)
									break;
								Item_Swallow(component->item, proto.index(), proto.select());


							}
							break;
						case NetProto_ArrangeGodShip::UNITID:
							{
								static NetProto_ArrangeGodShip arrangeGodShip;
								if (!arrangeGodShip.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);
								if (component == NULL)
									break;

								//Item_ArrangeGodShip(component->item);
								GCAgent_SendProtoToClients(&id, 1, &arrangeGodShip);
							}
							break;
						case NetProto_SingleGodShip::UNITID:
							{
								static NetProto_SingleGodShip proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								GCAgent_SendProtoToClients(&id, 1, &proto);
								struct Component *component = PlayerEntity_Component(player);
								//int * p = new int[5];
								//delete [] p;
								int size = proto.eat_size();
								int *eat = new int[size];
								for (int i = 0; i < size; i++) {
									if (proto.eat(i) >= 0 ) {
										eat[i] = proto.eat(i);
										DEBUG_LOG("eat:%d", eat[i]);
									}
								}
								GCAgent_SendProtoToClients(&id, 1, &proto);
								Item_SingleGodShip(component->item, proto.index(), eat, size);
								delete [] eat;
							}
							break;
						default:
							break;
					}
				}
				break;
				case NetProto_Hit::GROUPID:
				{
					switch(unitID) {
						case NetProto_SkillLevelUp::UNITID:
							{
								DEBUG_LOG("NetProto_SkillLevelUp start");
								static NetProto_SkillLevelUp skillLevel;
								if (!skillLevel.ParseFromArray(data, size)) {
									break;
								}

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL) {
									break;
								}
								//	int skillID = skillLevel.id();

								Component *component = PlayerEntity_Component(player);
								//const FightAtt* att = Fight_Att(component->fight);
								//if (NULL == att) {
								//	break;
								//}

								//DEBUG_LOG("NetProto_SkillLevelUp ...");

								//for (int i = 0; i < att->skills_size(); ++i) {
								//	if (skillID == att->skills(i).id()) {
								//DEBUG_LOG("NetProto_SkillLevelUp ...");
								//		if (0 == Fight_SkillLevelUp(component->fight, i, skillLevel.delta())) {
								//			GCAgent_SendProtoToClients(&id, 1, &skillLevel);
								//			DEBUG_LOG("NetProto_SkillLevelUp success");
								//		}	
								//	}
								//}

								if (0 == Fight_SkillLevelUp(component->fight, skillLevel.id(), skillLevel.delta())) { 
									GCAgent_SendProtoToClients(&id, 1, &skillLevel);
									DEBUG_LOG("NetProto_SkillLevelUp success");
								}

								DEBUG_LOG("NetProto_SkillLevelUp end");
							}
							break;
						default:
							break;
					}
				}
				break;

				case NetProto_AddGoods::GROUPID:
				{
					switch(unitID) {
						case NetProto_ShiftItem::UNITID:
							{
								static NetProto_ShiftItem shiftItem;
								if (!shiftItem.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								if (shiftItem.prevType() == NetProto_ShiftItem::PACKAGE) {
									if (shiftItem.newType() == NetProto_ShiftItem::BODY) {
										const ItemInfo *prev = Item_PackageItem(component->item, shiftItem.prevPos());
										if (prev == NULL || prev->type() == ItemInfo::NONE)
											break;
										if ((shiftItem.count() < 0 && shiftItem.count() != -1) || shiftItem.count() > prev->count())
											break;
										if (prev->type() != ItemInfo::EQUIPMENT)
											break;

										const EquipmentInfo *info = Item_EquipInfo(component->item, prev->id());
										struct Equipment *equipment = component->equipment;

										int64_t prevItem = Equipment_Wear(equipment, prev->id());
										if (prevItem == -2) {
											break;
										}
										else {
											if (prevItem != -1) {
												ItemInfo bodyItem;
												bodyItem.set_type(ItemInfo::EQUIPMENT);
												bodyItem.set_id(prevItem);
												bodyItem.set_count(1);
												Item_AddToPackage(component->item, &bodyItem, shiftItem.prevPos(), NULL);

												int64_t roleID = PlayerEntity_RoleID(player);
												char ch[1024];
												SNPRINTF1(ch, "3-%d-%d", (int)prevItem, 1);
												DCProto_SaveSingleRecord saveRecord;
												saveRecord.set_mapID(-23);
												saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
												saveRecord.mutable_record()->mutable_role()->set_name(ch);
												saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
												GCAgent_SendProtoToDCAgent(&saveRecord);
											} else {
												Item_DelFromPackage(component->item, shiftItem.prevPos(), 1, false);
											}
											shiftItem.set_newPos(info->type());
										}

										/*
										   static NetProto_Wear wear;
										   wear.Clear();
										   wear.set_id(id);
										   wear.set_equipment(prev->id());
										   GCAgent_SendProtoToAllClientsExceptOneInSameScene(player, &wear);
										   */
									} else if (shiftItem.newType() == NetProto_ShiftItem::FASHION) {
										const ItemPackage *package = Item_Package(component->item);
										if (shiftItem.prevPos() < 0 || shiftItem.prevPos() >= package->fashions_size())
											break;
										const FashionAsset *asset = &package->fashions(shiftItem.prevPos());
										int v = asset->v();
										if (v == -1 || v == 0)
											break;
										Equipment_WearFashion(component->equipment, shiftItem.prevPos());
									} else if (shiftItem.newType() == NetProto_ShiftItem::WING) {
									} else {
										break;
									}
								} else if (shiftItem.prevType() == NetProto_ShiftItem::FASHION) {
									if (shiftItem.newType() == NetProto_ShiftItem::PACKAGE) {
										Equipment_UnwearFashion(component->equipment);
									} else {
										break;
									}
								} else if (shiftItem.prevType() == NetProto_ShiftItem::BODY) {
									if (!EquipmentInfo::Type_IsValid(shiftItem.prevPos()))
										break;

									struct Equipment *equipment = component->equipment;
									int32_t prevEquipment = Equipment_Att(equipment)->equipments(shiftItem.prevPos());
									DEBUG_LOG("Unwear Equip start! shiftItem.prevPos = %d", shiftItem.prevPos());
									if (prevEquipment == -1)
										break;
									DEBUG_LOG("Unwear Equip start!");
									if (shiftItem.newType() == NetProto_ShiftItem::PACKAGE) {
										if (shiftItem.newPos() < ItemPackage::EQUIPMENT || shiftItem.newPos() >= (int)ItemPackage::EQUIPMENT + (int)ItemPackage::LENGTH)
											break;
										const ItemInfo *cur = Item_PackageItem(component->item, shiftItem.newPos());
										if (cur == NULL || cur->type() != ItemInfo::NONE) {
											break;
										}

										if (Equipment_Unwear(equipment, (EquipmentInfo::Type)shiftItem.prevPos()) == -2) {
											break;
										}

										ItemInfo temp;
										temp.set_type(ItemInfo::EQUIPMENT);
										temp.set_id(prevEquipment);
										temp.set_count(1);
										Item_AddToPackage(component->item, &temp, shiftItem.newPos(), NULL);

										int64_t roleID = PlayerEntity_RoleID(player);
										char ch[1024];
										SNPRINTF1(ch, "4-%d-%d", prevEquipment, 1);
										DCProto_SaveSingleRecord saveRecord;
										saveRecord.set_mapID(-23);
										saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
										saveRecord.mutable_record()->mutable_role()->set_name(ch);
										saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
										GCAgent_SendProtoToDCAgent(&saveRecord);

										/*
										   static NetProto_Unwear unwear;
										   unwear.Clear();
										   unwear.set_id(id);
										   unwear.set_pos((PB_EquipmentInfo::Type)shiftItem.prevPos());
										   GCAgent_SendProtoToAllClientsExceptOneInSameScene(player, &unwear);
										   */
									}
									else {
										DEBUG_LOG("shiftItem.newType() != NetPeoto_shiftItem::PACKAGE !");
										break;
									}
								}
								else if (shiftItem.prevType() == NetProto_ShiftItem::SKILL) {
									const FightAtt *att = Fight_Att(component->fight);
									if (shiftItem.prevPos() < 1 || shiftItem.prevPos() >= att->skills_size())
										break;
									if (att->skills(shiftItem.prevPos()).id() == -1 || att->skills(shiftItem.prevPos()).level() <= 0)
										break;

									if (shiftItem.newType() == NetProto_ShiftItem::ALT) {
										const ALT *alt = Item_ALT(component->item);
										if (shiftItem.newPos() < 1 || shiftItem.newPos() >= alt->alt_size())
											break;

										ItemInfo temp;
										temp.set_type(ItemInfo::SKILL);
										temp.set_id(shiftItem.prevPos());
										Item_AddToALT(component->item, &temp, shiftItem.newPos());
									}
									else {
										break;
									}
								}
								else {
									break;
								}

								GCAgent_SendProtoToClients(&id, 1, &shiftItem);
							}
							break;

						case NetProto_BuyItem::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_BuyItem buyItem;
								if (!buyItem.ParseFromArray(data, size))
									break;

								if (buyItem.count() < 1)
									break;

								if (buyItem.count() > 1000)
									break;

								const BusinessInfo *business = BusinessInfoManager_BusinessInfo(buyItem.business());
								if (business == NULL)
									break;

								DEBUG_LOG("FFFFFFFFFFFFF %d", buyItem.business());
								if (buyItem.business() >= 10 && buyItem.business() <= 16) {
									int day = (Time_TodayZeroTime(Time_TimeStamp()) - Time_TodayZeroTime(Config_OpenTime(NULL))) / 86400;
									DEBUG_LOG("FFFFFFFFFFFFF %d", day);
									if (day < 0 || day > 6) 
										break;

									DEBUG_LOG("FFFFFFFFFFFFF %d", day);
									if (buyItem.business() > (10 + day))
										break;								
								}

								if (buyItem.id() < 0 || buyItem.id() >= business->items_size())
									break;
								const BusinessUnit *cur = &business->items(buyItem.id());

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								const PlayerAtt* playerAtt = PlayerEntity_Att(player);
								if (playerAtt == NULL)
									break;

								struct Component *component = PlayerEntity_Component(player);

								DEBUG_LOG("FFFFFFFFFFFFF");
								int curTime = (int)Time_TimeStamp();
								if (cur->begin() != 0 && cur->begin() > curTime)
									break;
								if (cur->end() != 0 && cur->end() < curTime)
									break;

								DEBUG_LOG("FFFFFFFFFFFFF");
								int limitIndex = -1;
								if (cur->limitType() == BusinessUnit::ROLE_DAY) {
									for (int i = 7; i <= 26; i++) {
										int event = component->playerAtt->dayEvent(i);
										PB_ItemInfo::Type type = (PB_ItemInfo::Type)(event & 0xff);
										if (type == PB_ItemInfo::NONE) {
											limitIndex = i;
											break;
										}
										int id = (event & 0xfff00) >> 8;
										int count = (event & 0x7ff00000) >> 20;
										if (type == cur->type() && id == cur->id()) {
											if (count + buyItem.count() > cur->arg())
												return;
											limitIndex = i;
											break;
										}
									}
								} else if (cur->limitType() == BusinessUnit::ROLE_CAREER) {
									DEBUG_LOG("FFFFFFFFFFFFF");
									for (int i = 20; i <= 29; i++) {
										int event = component->playerAtt->fixedEvent(i);
										PB_ItemInfo::Type type = (PB_ItemInfo::Type)(event & 0xff);
										if (type == PB_ItemInfo::NONE) {
											limitIndex = i;
											break;
										}
										int id = (event & 0xfff00) >> 8;
										int count = (event & 0x7ff00000) >> 20;
										if (type == cur->type() && id == cur->id()) {
											if (count + buyItem.count() > cur->arg())
												return;
											limitIndex = i;
											break;
										}
									}
									for (int i = 115; i <= 121; i++) {
										int event = component->playerAtt->fixedEvent(i);
										PB_ItemInfo::Type type = (PB_ItemInfo::Type)(event & 0xff);
										if (type == PB_ItemInfo::NONE) {
											limitIndex = i;
											break;
										}
										int id = (event & 0xfff00) >> 8;
										int count = (event & 0x7ff00000) >> 20;
										DEBUG_LOG("FFFFFFFFFFFFFFFF %d, %d, %d", i, type, id);
										if (type == cur->type() && id == cur->id()) {
											DEBUG_LOG("FFFFFFFFFFFFFFFF %d, %d, %d", i, type, id);
											if (count + buyItem.count() > cur->arg())
												return;
											limitIndex = i;
											break;
										}
									}

								} else if (cur->limitType() == BusinessUnit::SERVER_DAY) {
									int curCount = PlayerPool_ServerDayNum((ItemInfo::Type)cur->type(), cur->id());
									if (curCount + buyItem.count() > cur->arg()) {
										static NetProto_Error error;
										error.set_content(Config_Words(37));
										GCAgent_SendProtoToClients(&id, 1, &error);
										return;
									}
								}

								DEBUG_LOG("FFFFFFFFFFFFF");
								int64_t price = 0;
								int64_t rmb = 0;
								int64_t subRMB = 0;
								int32_t lovePoint = 0;
								int32_t pvpScore = 0;
								int32_t godScore = 0;
								int32_t factionContribute = 0;
								int32_t openServerScore = 0;	

								int finalCount = cur->count();
								int gmDiscountPercent = GmSet_GetPercent(GmSet_Type_GOODS, cur->id(), cur->type());
								if(gmDiscountPercent != -1)
									finalCount = gmDiscountPercent;
								DEBUG_LOG("FFFFFFFFFFFFF");

								if (cur->type() == PB_ItemInfo::EQUIPMENT) {
									const EquipmentInfo *equip = EquipmentInfoManager_EquipmentInfo(cur->id());
									if (business->currencyType() == BusinessInfo::RMB)
										rmb = equip->rmb() * finalCount / Config_Divisor();
									else if (business->currencyType() == BusinessInfo::SUB_RMB)
										subRMB = equip->rmb() * finalCount / Config_Divisor();
									else if (business->currencyType() == BusinessInfo::MONEY)
										price = equip->price() * finalCount / Config_Divisor();
									else if (business->currencyType() == BusinessInfo::LOVE_POINT)
										lovePoint = equip->lovePoint() * finalCount / Config_Divisor();
									else if (business->currencyType() == BusinessInfo::PVP_SCORE)
										pvpScore = equip->pvpScore() * finalCount / Config_Divisor();
									else if (business->currencyType() == BusinessInfo::GOD_SCORE)
										godScore = equip->godScore() * finalCount / Config_Divisor();
									else if (business->currencyType() == BusinessInfo::FACTION_CONTRIBUTE)
										factionContribute = equip->factionContribute() * finalCount / Config_Divisor();
									else
										break;
								} else if (cur->type() == PB_ItemInfo::GOODS) {
									const GoodsInfo *info = GoodsInfoManager_GoodsInfo(cur->id());
									if (business->currencyType() == BusinessInfo::RMB)
										rmb = info->rmb() * finalCount / Config_Divisor() * buyItem.count();
									else if (business->currencyType() == BusinessInfo::SUB_RMB)
										subRMB = info->rmb() * finalCount / Config_Divisor() * buyItem.count();
									else if (business->currencyType() == BusinessInfo::MONEY)
										price = info->price() * finalCount / Config_Divisor() * buyItem.count();
									else if (business->currencyType() == BusinessInfo::LOVE_POINT)
										lovePoint = info->lovePoint() * finalCount / Config_Divisor() * buyItem.count();
									else if (business->currencyType() == BusinessInfo::PVP_SCORE)
										pvpScore = info->pvpScore() * finalCount / Config_Divisor() * buyItem.count();
									else if (business->currencyType() == BusinessInfo::GOD_SCORE)
										godScore = info->godScore() * finalCount / Config_Divisor() * buyItem.count();
									else if (business->currencyType() == BusinessInfo::FACTION_CONTRIBUTE)
										factionContribute = info->factionContribute() * finalCount / Config_Divisor() * buyItem.count();
									else
										break;
								} else if (cur->type() == PB_ItemInfo::FASHION) {
									if (Item_BuyFashion(component->item, cur->id(), buyItem.count() > 1, (float)finalCount / Config_Divisor(), business->currencyType()) != 0)
										break;
								} else if (cur->type() == PB_ItemInfo::WING) {
									if (Item_BuyWing(component->item, cur->id(), buyItem.count() > 1, (float)finalCount / Config_Divisor(), business->currencyType()) != 0)
										break;
								} else {
									assert(0);
								}
								DEBUG_LOG("FFFFFFFFFFFFF");
								const ItemPackage *package = Item_Package(component->item);
								int contribute = FactionPool_GetRoleContribute(playerAtt);
								if (price > package->money()
										//		|| rmb > package->rmb()
										|| (!Item_HasRMB(component->item, subRMB))
										|| (!Item_HasRMB(component->item, rmb))
										//			|| subRMB > (package->rmb() + package->subRMB())
										|| lovePoint > package->lovePoint()
										|| pvpScore > package->pkScore()
										|| godScore > package->godScore()
										|| factionContribute > contribute)
									break;

								static NetProto_GetRes gr;
								gr.Clear();

								const char *buyType = NULL;
								int32_t arg1 = 0;
								if (cur->type() == PB_ItemInfo::EQUIPMENT) {
									assert(EquipmentInfoManager_EquipmentInfo(cur->id()) != NULL);
									Item_AddToPackage(component->item, ItemInfo::EQUIPMENT, cur->id(), 1, &gr);

									int64_t roleID = PlayerEntity_RoleID(player);
									char ch[1024];
									SNPRINTF1(ch, "5-%d-%d", cur->id(), 1);
									DCProto_SaveSingleRecord saveRecord;
									saveRecord.set_mapID(-23);
									saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
									saveRecord.mutable_record()->mutable_role()->set_name(ch);
									saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
									GCAgent_SendProtoToDCAgent(&saveRecord);

									arg1 = 20;
									buyType = "equip";
								} else if (cur->type() == PB_ItemInfo::GOODS) {
									const GoodsInfo *goodsInfo = GoodsInfoManager_GoodsInfo(cur->id());
									assert(goodsInfo != NULL);

									int32_t goods[CONFIG_FIXEDARRAY];
									int n = Event_Goods(goods, CONFIG_FIXEDARRAY);
									if (n > 0) {
										for (int i = 0; i < n; i++) {
											if (goods[i] == cur->id()) {
												int32_t event = component->playerAtt->dayEvent(5);
												if (event & (1 << i)) {
													static NetProto_Error error;
													char buf[CONFIG_FIXEDARRAY];
													SNPRINTF1(buf, Config_Words(26), goodsInfo->name().c_str());
													error.set_content(buf);
													GCAgent_SendProtoToClients(&id, 1, &error);
													return;
												} else {
													PlayerEntity_SetDayEvent(player, 5, event | (1 << i));
													break;
												}
											}
										}
									}

									Item_AddToPackage(component->item, ItemInfo::GOODS, cur->id(), buyItem.count(), &gr);

									int64_t roleID = PlayerEntity_RoleID(player);
									char ch[1024];
									SNPRINTF1(ch, "6-%d-%d", cur->id(), buyItem.count());
									DCProto_SaveSingleRecord saveRecord;
									saveRecord.set_mapID(-23);
									saveRecord.mutable_record()->mutable_role()->set_roleID(roleID);
									saveRecord.mutable_record()->mutable_role()->set_name(ch);
									saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
									GCAgent_SendProtoToDCAgent(&saveRecord);

									{
										static DCProto_SaveSingleRecord saveRecord;
										static char ch[1024];
										SNPRINTF1(ch, "1-%d-%d", cur->id(), buyItem.count());
										saveRecord.set_mapID(-20);
										saveRecord.mutable_record()->mutable_role()->set_roleID(component->baseAtt->roleID());
										saveRecord.mutable_record()->mutable_role()->set_name(ch);
										saveRecord.mutable_record()->set_arg1(Time_TimeStamp());
										GCAgent_SendProtoToDCAgent(&saveRecord);
									}

									arg1 = 5;
									buyType = "goods";
								} else if (cur->type() == PB_ItemInfo::FASHION) {
									PB_ItemInfo *item = gr.add_items();
									item->set_type(PB_ItemInfo::FASHION);
									item->set_id(cur->id());
									item->set_count(1);
									buyType = "fashion";
								} else if (cur->type() == PB_ItemInfo::WING) {
									PB_ItemInfo *item = gr.add_items();
									item->set_type(PB_ItemInfo::WING);
									item->set_id(cur->id());
									item->set_count(1);
									buyType = "wing";
								} else {
									assert(0);
								}

								GCAgent_SendProtoToClients(&id, 1, &gr);

								if (price > 0)
									Item_ModifyMoney(component->item, -price);
								if (rmb > 0)
									Item_ModifyRMB(component->item, -rmb, false, arg1, cur->id(), finalCount);
								if (subRMB > 0) {
									Item_ModifyRMB(component->item, -subRMB, false, arg1, cur->id(), finalCount);
								}
								if (lovePoint > 0) {
									Item_ModifyLovePoint(component->item, -lovePoint);
									char index[128];
									SNPRINTF1(index, "{\"score\":\"%d\",\"lovePoint\":\"%d\"}", lovePoint, package->lovePoint());
									ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "LovePoint", index));
								}	
								if (pvpScore > 0) {
									Item_ModifyPKScore(component->item, -pvpScore);
									char index[128];
									SNPRINTF1(index, "{\"score\":\"%d\",\"pvpScore\":\"%lld\"}", pvpScore, (long long int)package->pkScore());
									ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "PvpScore", index));
								}
								if (godScore > 0) {
									Item_ModifyGodScore(component->item, -godScore);
									char index[128];
									SNPRINTF1(index, "{\"score\":\"%d\",\"godScore\":\"%d\"}", godScore, package->godScore());
									ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "GodScore", index));
								}
								if (factionContribute > 0)
									FactionPool_UpateRoleContribute(playerAtt, -factionContribute);

								if (buyItem.business() >= 10 && buyItem.business() <= 16) {
									if (cur->resType() == 1) {
										openServerScore = cur->resValue() * buyItem.count();
										if (openServerScore > 0) {
											Item_ModifyOpenServerScore(component->item, openServerScore);
										}
									}
								}

								char str[128];
								SNPRINTF1(str, "{\"type\":\"%s\",\"id\":\"%d\",\"vip\":\"%d\"}", buyType == NULL ? "" : buyType, (int)cur->id(), package->vip());
								ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "BuyItem", str));

								if (cur->limitType() == BusinessUnit::ROLE_DAY) {
									if (limitIndex != -1) {
										int event = component->playerAtt->dayEvent(limitIndex);
										int count = ((event & 0x7ff00000) >> 20) + buyItem.count();
										event = 0;
										event |= (int)cur->type();
										event |= (cur->id() << 8);
										event |= (count << 20);
										PlayerEntity_SetDayEvent(component->player, limitIndex, event);
									}
								} else if (cur->limitType() == BusinessUnit::ROLE_CAREER) {
									if (limitIndex != -1) {
										int event = component->playerAtt->fixedEvent(limitIndex);
										int count = ((event & 0x7ff00000) >> 20) + buyItem.count();
										event = 0;
										event |= (int)cur->type();
										event |= (cur->id() << 8);
										event |= (count << 20);
										PlayerEntity_SetFixedEvent(component->player, limitIndex, event);
									}
								} else if (cur->limitType() == BusinessUnit::SERVER_DAY) {
									PlayerPool_AddServerDayNum((ItemInfo::Type)cur->type(), cur->id(), buyItem.count());
								}

								if (cur->type() == PB_ItemInfo::FASHION)
									break;
								else if (cur->type() == PB_ItemInfo::WING)
									break;

								GCAgent_SendProtoToClients(&id, 1, &buyItem);
							}
							break;

						case NetProto_SellItem::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_SellItem sellItem;
								if (!sellItem.ParseFromArray(data, size))
									break;

								if (sellItem.count() == 0)
									sellItem.set_count(-1);

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								const ItemInfo *cur = Item_PackageItem(component->item, sellItem.id());
								if (cur == NULL || cur->type() == ItemInfo::NONE)
									break;

								int64_t price = 0;
								if (cur->type() == ItemInfo::EQUIPMENT) {
									sellItem.set_count(-1);
									const EquipmentInfo *info = Item_EquipInfo(component->item, cur->id());
									price = info->price() / 2;

									/*
									   static NetProto_GetRes gr;
									   gr.Clear();
									   for (int i = 0; i < info->gemModel_size(); i++) {
									   if (info->gemModel(i) >= 0) {
									   Item_AddToPackage(component->item, ItemInfo::GOODS, info->gemModel(i), 1);
									   bool has = false;
									   for (int j = 0; j < gr.items_size(); j++) {
									   if (gr.items(j).type() == PB_ItemInfo::GOODS && gr.items(j).id() == info->gemModel(i)) {
									   gr.mutable_items(j)->set_count(gr.items(j).count() + 1);
									   has = true;
									   break;
									   }
									   }
									   if (!has) {
									   PB_ItemInfo *item = gr.add_items();
									   item->set_type(PB_ItemInfo::GOODS);
									   item->set_id(info->gemModel(i));
									   item->set_count(1);
									   }
									   }
									   }
									   if (gr.items_size() > 0)
									   GCAgent_SendProtoToClients(&id, 1, &gr);
									   */
								}
								else if (cur->type() == ItemInfo::GOODS) {
									if (sellItem.count() == -1) {
										price = GoodsInfoManager_GoodsInfo(cur->id())->price() / 2 * cur->count();
									}
									else if (sellItem.count() > 0) {
										if (sellItem.count() > cur->count())
											sellItem.set_count(cur->count());
										price = GoodsInfoManager_GoodsInfo(cur->id())->price() / 2 * sellItem.count();
									}
									else {
										return;
									}
								}
								else {
									assert(0);
								}

								Item_DelFromPackage(component->item, sellItem.id(), sellItem.count());
								Item_ModifyMoney(component->item, price);

								GCAgent_SendProtoToClients(&id, 1, &sellItem);
							}
							break;

						case NetProto_UseGoods::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_UseGoods useGoods;
								if (!useGoods.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								int32_t results[CONFIG_FIXEDARRAY];
								int count = Item_UseGoods(component->item, useGoods.id(), useGoods.all(), results, CONFIG_FIXEDARRAY);
								if (count == -1)
									break;

								for (int i = 0; i < count; i++)
									useGoods.add_boxItems(results[i]);

								GCAgent_SendProtoToClients(&id, 1, &useGoods);
							}
							break;

						case NetProto_UnlockPackage::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_UnlockPackage unlockPackage;
								if (!unlockPackage.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								if (Item_UnlockPackage(component->item, (ItemPackage::Begin)unlockPackage.begin(), unlockPackage.count()) == -1)
									break;

								GCAgent_SendProtoToClients(&id, 1, &unlockPackage);
							}
							break;

						case NetProto_RecoverDurability::UNITID:
							{
								if (InSingle(id))
									break;

								static NetProto_RecoverDurability rd;
								if (!rd.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								int count = component->playerAtt->dayEvent(30) & 0xff;
								if (count >= Config_RecoverDurabilityCount(component->playerAtt->itemPackage().vip()))
									break;

								//const ItemPackage *package = Item_Package(component->item);
								int32_t cost = Config_RecoverDurabilityGem() * (count + 1);
								//if (package->subRMB() + package->rmb() < cost)
								if (!Item_HasRMB(component->item, cost))
									break;

								PlayerEntity_SetDayEvent(player, 30, (component->playerAtt->dayEvent(30) & 0x7fffff00) | (count + 1));

								Item_ModifyRMB(component->item, -cost, false, 6, 0, 0);
								Item_ModifyDurability(component->item, Config_RecoverDurability());

								GCAgent_SendProtoToClients(&id, 1, &rd);

								static NetProto_GetRes gr;
								gr.Clear();
								PB_ItemInfo *item = gr.add_items();
								item->set_type(PB_ItemInfo::DURABILITY);
								item->set_count(Config_RecoverDurability());
								GCAgent_SendProtoToClients(&id, 1, &gr);
							}
							break;

						case NetProto_ShowDesignation::UNITID:
							{
								static NetProto_ShowDesignation sd;
								if (!sd.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								PlayerEntity_ShowDesignation(player, sd.id());
							}
							break;

						case NetProto_UnshowDesignation::UNITID:
							{
								static NetProto_UnshowDesignation ud;
								if (!ud.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								PlayerEntity_UnshowDesignation(player, ud.id());
							}
							break;

						case NetProto_Recharge::UNITID:
							{
								static NetProto_Recharge recharge;
								if (!recharge.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								const PlayerInfo *info = AccountPool_IDToAccount(id);
								if (info == NULL)
									break;

								static DCProto_Recharge dRecharge;
								dRecharge.Clear();
								*dRecharge.mutable_recharge() = recharge;
								dRecharge.set_id(id);
								dRecharge.set_roleID(component->baseAtt->roleID());
								dRecharge.set_level(component->roleAtt->fightAtt().level());
								dRecharge.set_over(false);
								*dRecharge.mutable_info() = *info;
								GCAgent_SendProtoToDCAgent(&dRecharge);
							}
							break;

						case NetProto_BusinessInfo::UNITID:
							{
								static NetProto_BusinessInfo proto;
								if (!proto.ParseFromArray(data, size))
									break;

								const BusinessInfo *info = BusinessInfoManager_BusinessInfo(proto.id());
								if (info == NULL)
									break;

								*proto.mutable_info() = *info;
								GCAgent_SendProtoToClients(&id, 1, &proto);
							}
							break;

						case NetProto_TransformWar::UNITID:
							{
								static NetProto_TransformWar proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								struct Component *component = PlayerEntity_Component(player);
								if (component == NULL)
									break;
								const ItemPackage *package = Item_Package(component->item);
								if (package == NULL)
									break;

								int32_t map = Movement_Att(component->movement)->mapID();
								const MapInfo *info = MapInfoManager_MapInfo(MapPool_MapInfo(map));
								if (info == NULL)
									break;
								if (info->mapType() != MapInfo::PEACE)
									break;

								bool flag = (proto.id() == -1) ? true : false;	
								for (int i = 0; i < package->transforms_size(); ++i) {
									if (package->transforms(i).id() == proto.id()) {
										flag = true;
										break;
									}
								}

								if (flag) {
									Fight_TransformWar(component->fight, proto.id());
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						case NetProto_TransformLevelUp::UNITID:
							{
								static NetProto_TransformLevelUp proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								struct Component *component = PlayerEntity_Component(player);
								if (component == NULL)
									break;

								int res = Item_TransformLevelUp(component->item, proto.id());
								if (res == 0) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}

							}
							break;

						case NetProto_Reservation::UNITID:
							{
								static NetProto_Reservation proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								int res = PlayerPool_Reservation(player, &proto);
								DEBUG_LOG("MMMMMMMMMMMMMMMMMMMMM %d", res);
								if (res == 0) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						case NetProto_ReservationList::UNITID:
							{
								static NetProto_ReservationList proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size))
									break;

								int res = PlayerPool_ReservationList(&proto);
								DEBUG_LOG("MMMMMMMMMMMMMMMMMMMMM %d, %d", res, proto.list_size());
								if (res == 0) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						default:
							break;
					}
				}
				break;

				case NetProto_Talk::
					GROUPID:
					{
						switch(unitID) {
							case NetProto_Talk::UNITID:
								{
									if (InSingle(id))
										break;

									static NetProto_Talk talk;
									if (!talk.ParseFromArray(data, size))
										break;

									struct PlayerEntity *player = PlayerEntity_Player(id);
									if (player == NULL)
										break;
									struct Component *component = PlayerEntity_Component(player);

									int32_t map = PlayerEntity_Att(player)->att().movementAtt().mapID();

									struct NPCEntity *npc = NPCPool_NPC(map, talk.id());
									if (npc == NULL)
										break;

									struct Func *func = NPCEntity_Component(npc)->func;
									if (func == NULL)
										break;

									/*
									   static Vector3f pos;
									   pos.FromPB(&talk.pos());
									   if (Movement_SetPosition(component->movement, &pos) == -1)
									   DEBUG_LOGERROR("Failed to set pos when talking");
									   */

									Mission_CompleteTarget(component->mission, MissionTarget::TALK, talk.id(), -1);

									static NetProto_Answer answer;
									answer.Clear();
									answer.set_id(talk.id());

									const FuncAtt *att = Func_Att(func);
									for (int i = 0; i < att->funcs_size(); i++) {
										if (att->funcs(i).type() == FuncInfo::MISSION) {
											if (Mission_CanShow(component->mission, att->funcs(i).arg(0))) {
												att->funcs(i).ToPB(answer.add_func());
												// const MissionInfo *mission = MissionInfoManager_MissionInfo(att->funcs(i).arg(0));
												// answer.add_str(mission->name());
											}
										}
										else if (att->funcs(i).type() == FuncInfo::BUSINESS) {
											att->funcs(i).ToPB(answer.add_func());
										}
										else if (att->funcs(i).type() == FuncInfo::ROOM) {
											const MapInfo *next = MapInfoManager_MapInfo(att->funcs(i).arg(0));
											assert(next != NULL);
											if (next->requireMission() != -1 && next->requireMission() != 0) {
												if (Mission_MissionCompleteCount(component->mission, next->requireMission()) <= 0)
													continue;
											}
											att->funcs(i).ToPB(answer.add_func());
										}
										else {
											break;
										}
									}

									GCAgent_SendProtoToClients(&id, 1, &answer);
									// DEBUG_LOG("Send answer");
								}
								break;

							case NetProto_ApplyMission::UNITID:
								{
									static NetProto_ApplyMission applyMission;
									if (!applyMission.ParseFromArray(data, size))
										break;

									struct PlayerEntity *player = PlayerEntity_Player(id);
									if (player == NULL)
										break;

									const MissionInfo *info = MissionInfoManager_MissionInfo(applyMission.id());
									if (info == NULL || info->type() == MissionInfo::DAILY)
										break;

									if (Mission_CanApply(PlayerEntity_Component(player)->mission, applyMission.id())) {
										Mission_Add(PlayerEntity_Component(player)->mission, applyMission.id());
									}
									else {
										applyMission.set_id(-1);
									}

									GCAgent_SendProtoToClients(&id, 1, &applyMission);
									DEBUG_LOG("Send apply: %d", applyMission.id());
								}
								break;

							case NetProto_CompleteMission::UNITID:
								{
									static NetProto_CompleteMission completeMission;
									if (!completeMission.ParseFromArray(data, size))
										break;

									struct PlayerEntity *player = PlayerEntity_Player(id);
									if (player == NULL)
										break;

									if (Mission_IsMissionCompleted(PlayerEntity_Component(player)->mission, completeMission.id())) {
										GCAgent_SendProtoToClients(&id, 1, &completeMission);
										// DEBUG_LOG("Send complete");

										Mission_CompleteMission(PlayerEntity_Component(player)->mission, completeMission.id());
									}
									else {
										// DEBUG_LOGERROR("Mission is not completed, id: %d", completeMission.id());
										completeMission.set_id(-1);
										GCAgent_SendProtoToClients(&id, 1, &completeMission);
									}
								}
								break;

							case NetProto_GiveUpMission::UNITID:
								{
									if (InSingle(id))
										break;

									static NetProto_GiveUpMission giveUpMission;
									if (!giveUpMission.ParseFromArray(data, size))
										break;

									struct PlayerEntity *player = PlayerEntity_Player(id);
									if (player == NULL)
										break;
									struct Component *component = PlayerEntity_Component(player);

									if (Mission_GiveUp(component->mission, giveUpMission.id()) == -1)
										giveUpMission.set_id(-1);

									GCAgent_SendProtoToClients(&id, 1, &giveUpMission);
								}
								break;

							default:
								break;
						}
					}
				break;

				case NetProto_Chat::GROUPID:
				{
					switch(unitID) {
						case NetProto_Chat::UNITID:
							{
								static NetProto_Chat chat;
								if (!chat.ParseFromArray(data, size))
									break;

								if (chat.sType() != NetProto_Chat::PLAYER)
									break;

								struct PlayerEntity *sender = PlayerEntity_Player(id);
								if (sender == NULL)
									break;

								struct Component *component = PlayerEntity_Component(sender);
								if (component == NULL)
									break;

								if (Event_IsNoTalking(component->baseAtt->roleID())) {
									static NetProto_Error error;
									error.set_content(Config_Words(41));
									GCAgent_SendProtoToClients(&id, 1, &error);
									break;
								}

								switch(chat.channel()) {
									case NetProto_Chat::GUILD:
										{
											const PlayerAtt* playerAtt = PlayerEntity_Att(sender);

											static int ids[CONFIG_FIXEDARRAY];
											int countMem = FactionPool_GetFactionMem(playerAtt, ids);
											if (countMem > 0) {
												GCAgent_SendProtoToClients(ids, countMem, &chat);
											}

											int32_t gms[CONFIG_FIXEDARRAY];
											int count = GMPool_GMs(gms, CONFIG_FIXEDARRAY);
											if (count > 0) {
												static NetProto_GMChat gmChat;
												gmChat.Clear();
												gmChat.set_channel(chat.channel());
												gmChat.set_content(chat.content());
												//gmChat.set_sender(PlayerEntity_Att(sender)->att().baseAtt().name());
												*gmChat.mutable_sender() = chat.sender();
												gmChat.set_time(Time_TimeStamp());
												GCAgent_SendProtoToClients(gms, count, &gmChat);
											}

											static DCProto_SaveChat sc;
											sc.Clear();
											sc.set_type(2);
											sc.set_sender(PlayerEntity_Att(sender)->att().baseAtt().name());
											sc.set_senderID(PlayerEntity_Att(sender)->att().baseAtt().roleID());
											sc.set_receiverID(-1);
											sc.set_content(chat.content().c_str());
											GCAgent_SendProtoToDCAgent(&sc);
										}
										break;
									case NetProto_Chat::TEAM:
										// TODO
										break;
									case NetProto_Chat::WORLD:
										{
											if (component->roleAtt->fightAtt().level() < 10) {
												static NetProto_Error error;
												error.set_content(Config_Words(94));
												GCAgent_SendProtoToClients(&id, 1, &error);
												break;
											}

											if (!PlayerEntity_DoWorldChat(sender)) {
												chat.set_receiver(-1);
												GCAgent_SendProtoToClients(&id, 1, &chat);
												break;
											}

											chat.clear_receiver();
											GCAgent_SendProtoToAllClients(&chat);
											/*
											   struct PlayerEntity *all[CONFIG_FIXEDARRAY];
											   int count = PlayerPool_Players(all, CONFIG_FIXEDARRAY);
											   for (int i = 0; i < count; i++) {
											   int32_t target = PlayerEntity_ID(all[i]);
											   GCAgent_SendProtoToClients(&target, 1, &chat);
											   }


											//old
											int32_t gms[CONFIG_FIXEDARRAY];
											int count = GMPool_GMs(gms, CONFIG_FIXEDARRAY);
											if (count > 0) {
											static NetProto_GMChat gmChat;
											gmChat.Clear();
											gmChat.set_channel(chat.channel());
											gmChat.set_content(chat.content());
											 *gmChat.mutable_sender() = chat.sender();
											 GCAgent_SendProtoToClients(gms, count, &gmChat);
											 }
											 */
											//New
											int32_t gms[CONFIG_FIXEDARRAY];
											int count = GMPool_GMs(gms, CONFIG_FIXEDARRAY);
											if (count > 0) {
												static NetProto_GMChat gmChat;
												gmChat.Clear();
												gmChat.set_channel(chat.channel());
												gmChat.set_content(chat.content());
												//gmChat.set_sender(PlayerEntity_Att(sender)->att().baseAtt().name());
												*gmChat.mutable_sender() = chat.sender();
												gmChat.set_time(Time_TimeStamp());
												GCAgent_SendProtoToClients(gms, count, &gmChat);
											}

											static DCProto_SaveChat sc;
											sc.Clear();
											sc.set_type(0);
											sc.set_sender(PlayerEntity_Att(sender)->att().baseAtt().name());
											sc.set_senderID(PlayerEntity_Att(sender)->att().baseAtt().roleID());
											sc.set_receiverID(-1);
											sc.set_content(chat.content().c_str());
											GCAgent_SendProtoToDCAgent(&sc);
										}
										break;
									case NetProto_Chat::SECRET:
										{
											if (!PlayerEntity_DoSecretChat(sender)) {
												chat.set_receiver(-1);
												GCAgent_SendProtoToClients(&id, 1, &chat);
												break;
											}

											GCAgent_SendProtoToClients(&id, 1, &chat);

											struct PlayerEntity *receiver = PlayerEntity_PlayerByRoleID(chat.receiver());
											/*
											   if (receiver == NULL) {
											   chat.set_receiver(-2);
											   GCAgent_SendProtoToClients(&id, 1, &chat);
											   break;
											   }
											   else
											   */
											if (receiver == sender) {
												break;
											} else if (receiver != NULL) {
												int32_t receiverID = PlayerEntity_ID(receiver);
												GCAgent_SendProtoToClients(&receiverID, 1, &chat);
											}

											//New
											int32_t gms[CONFIG_FIXEDARRAY];
											int count = GMPool_GMs(gms, CONFIG_FIXEDARRAY);
											if (count > 0) {
												static NetProto_GMChat gmChat;
												gmChat.Clear();
												gmChat.set_channel(chat.channel());
												gmChat.set_content(chat.content());
												*gmChat.mutable_sender() = chat.sender();
												gmChat.set_time(Time_TimeStamp());
												PB_FriendInfo* info = gmChat.mutable_recver();
												if (receiver != NULL) {
													info->set_roleID(PlayerEntity_Att(receiver)->att().baseAtt().roleID());
													info->set_name(PlayerEntity_Att(receiver)->att().baseAtt().name());
													info->set_professionType((::PB_ProfessionInfo_Type)PlayerEntity_Att(receiver)->att().baseAtt().professionType());
												} else {
													info->set_roleID(-1);
													info->set_name("");
												}
												GCAgent_SendProtoToClients(gms, count, &gmChat);
											}

											static DCProto_SaveChat sc;
											sc.Clear();
											sc.set_type(1);
											sc.set_sender(PlayerEntity_Att(sender)->att().baseAtt().name());
											sc.set_senderID(PlayerEntity_Att(sender)->att().baseAtt().roleID());
											if (receiver != NULL) {
												sc.set_receiver(PlayerEntity_Att(receiver)->att().baseAtt().name());
												sc.set_receiverID(PlayerEntity_Att(receiver)->att().baseAtt().roleID());
											}
											sc.set_content(chat.content().c_str());
											GCAgent_SendProtoToDCAgent(&sc);
										}
										break;
									case NetProto_Chat::CURSENCE:
										{
											int32_t map = Movement_Att(component->movement)->mapID();
											std::vector<struct PlayerEntity*> players;

											int count = PlayerPool_Players(map, &players);
											if (count == -1)
												break;

											int32_t gms[CONFIG_FIXEDARRAY];
											count = GMPool_GMs(gms, CONFIG_FIXEDARRAY);
											if (count > 0) {
												static NetProto_GMChat gmChat;
												gmChat.Clear();
												gmChat.set_channel(chat.channel());
												gmChat.set_content(chat.content());
												//gmChat.set_sender(PlayerEntity_Att(sender)->att().baseAtt().name());
												*gmChat.mutable_sender() = chat.sender();
												gmChat.set_time(Time_TimeStamp());
												GCAgent_SendProtoToClients(gms, count, &gmChat);
											}

											count = 0;
											static int ids[CONFIG_FIXEDARRAY * 32];
											for (std::vector<struct PlayerEntity*>::iterator iter = players.begin(); iter != players.end(); ++iter) {
												int32_t id = PlayerEntity_ID(*iter);
												if (-1 != id) {
													ids[count++] = id;
												}
											}
											GCAgent_SendProtoToClients(ids, count, &chat);

											static DCProto_SaveChat sc;
											sc.Clear();
											sc.set_type(5);
											sc.set_sender(PlayerEntity_Att(sender)->att().baseAtt().name());
											sc.set_senderID(PlayerEntity_Att(sender)->att().baseAtt().roleID());
											sc.set_receiverID(-1);
											sc.set_content(chat.content().c_str());
											GCAgent_SendProtoToDCAgent(&sc);
										}
										break;
									default:
										break;
								}
							}
							break;

						case NetProto_SendMail::UNITID:
							{
								static NetProto_SendMail sm;
								if (!sm.ParseFromArray(data, size))
									break;

								struct PlayerEntity *sender = PlayerEntity_Player(id);
								if (sender == NULL) {
									// GM can send mail too.
									if (!GMPool_Has(id))
										break;
								}

								struct Component *component = NULL;
								const ItemPackage *package = NULL;
								if (sender != NULL) {
									component = PlayerEntity_Component(sender);
									package = Item_Package(component->item);
									if (package->money() < Config_MailCost())
										break;
								}

								if (sender != NULL) {
									if (sm.mail().item().type() != PB_ItemInfo::NONE)
										break;
									if (sm.mail().rmb() > 0)
										break;
								} else {
									if (sm.mail().item().type() != PB_ItemInfo::NONE) {
										if (sm.mail().item().count() <= 0)
											break;
										if (sm.mail().item().type() == PB_ItemInfo::EQUIPMENT) {
											const EquipmentInfo *equip = EquipmentInfoManager_EquipmentInfo(sm.mail().item().id());
											if (equip == NULL)
												break;
											sm.mutable_mail()->mutable_item()->set_count(1);
										} else if (sm.mail().item().type() == PB_ItemInfo::GOODS) {
											const GoodsInfo *goods = GoodsInfoManager_GoodsInfo(sm.mail().item().id());
											if (goods == NULL)
											{
												DEBUG_LOGERROR("send mail error->%d", sm.mail().item().id());
												break;
											}
										}
									}
								}

								struct PlayerEntity *receiver = PlayerEntity_PlayerByRoleID(sm.receiver());

								if(sender == NULL)
									DEBUG_LOGERROR("gm send mail->%lld, %d, %d, %lld, %lld", sm.receiver(), sm.mail().item().type(), sm.mail().item().id(), sm.mail().item().count(), sm.mail().rmb());

								if (sender != NULL && sender == receiver)
									break;

								if (receiver == NULL) {
									if (sm.receiver() == -1) {
										static DCProto_SendMailExceptRoles dsm;
										dsm.Clear();
										*dsm.mutable_sm() = sm;
										vector<struct PlayerEntity*> players;
										int count = PlayerPool_Players(&players);
										for (int i = 0; i < count; ++i) {
											int32_t target = PlayerEntity_ID(players[i]);
											int64_t roleID = PlayerEntity_RoleID(players[i]);
											int res = PlayerEntity_AddMail(players[i], sm.mutable_mail());
											if (-1 != res) {
												sm.set_receiver(roleID);
												GCAgent_SendProtoToClients(&target, 1, &sm);
											}

											dsm.add_roleIDs(roleID);
										}
										GCAgent_SendProtoToDCAgent(&dsm);
									}else {
										static DCProto_SendMail dsm;
										dsm.Clear();
										*dsm.mutable_sm() = sm;
										dsm.set_id(id);
										GCAgent_SendProtoToDCAgent(&dsm);
									}
								} else {
									int res = PlayerEntity_AddMail(receiver, sm.mutable_mail());
									if (res == -1) {
										sm.set_receiver(-1);
										GCAgent_SendProtoToClients(&id, 1, &sm);
									} else {
										if (sender != NULL) {
											Item_ModifyMoney(component->item, -Config_MailCost());
										}

										sm.set_pos(res);
										int32_t target = PlayerEntity_ID(receiver);
										GCAgent_SendProtoToClients(&target, 1, &sm);
										GCAgent_SendProtoToClients(&id, 1, &sm);
									}
								}
							}
							break;

						case NetProto_GetMailItem::UNITID:
							{
								static NetProto_GetMailItem gmi;
								if (!gmi.ParseFromArray(data, size))
									break;

								struct PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;

								int ret = PlayerEntity_GetMailItem(entity, gmi.id());
								if (ret != 0) {
									DEBUG_LOG("Failed to get mail item, id: %d, ret: %d", gmi.id(), ret);
									gmi.set_id(-1);
								}

								GCAgent_SendProtoToClients(&id, 1, &gmi);
							}
							break;

						case NetProto_ReadMail::UNITID:
							{
								static NetProto_ReadMail rm;
								if (!rm.ParseFromArray(data, size))
									break;

								struct PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;

								PlayerEntity_ReadMail(entity, rm.id());
							}
							break;

						case NetProto_DelMail::UNITID:
							{
								static NetProto_DelMail dm;
								if (!dm.ParseFromArray(data, size))
									break;

								struct PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;

								PlayerEntity_DelMail(entity, dm.id(), true);
								GCAgent_SendProtoToClients(&id, 1, &dm);
							}
							break;

						default:
							break;
					}
				}
				break;

				case NetProto_GMLogin::GROUPID:
				{
					switch(unitID) {
						case NetProto_GMLogin::UNITID:
							{
								static NetProto_GMLogin gmLogin;
								if (!gmLogin.ParseFromArray(data, size))
									break;

								int32_t oldID = GMPool_AccountIDByID(gmLogin.account());
								if (oldID != -1) {
									GMPool_Del(oldID);
									AccountPool_Logout(oldID);
								}

								if (!Event_IsGM(&gmLogin)) {
									DCProto_QueryGMAccount gmAccount;
									gmAccount.set_id(id);
									*gmAccount.mutable_gm() = gmLogin;
									GCAgent_SendProtoToDCAgent(&gmAccount);
									break;
								} else {
									GMPool_Add(id, gmLogin.account());
								}
								GCAgent_SendProtoToClients(&id, 1, &gmLogin);
							}
							break;

						case NetProto_GMOrder::UNITID:
							{
								static NetProto_GMOrder gmOrder;
								if (!gmOrder.ParseFromArray(data, size))
									break;

								if (!GMPool_Has(id))
									break;

								switch(gmOrder.type()) {
									case NetProto_GMOrder::ONLINE_NUM:
										{
											gmOrder.set_nValue(PlayerPool_TotalCount());
											GCAgent_SendProtoToClients(&id, 1, &gmOrder);
										}
										break;
									case NetProto_GMOrder::MESSAGE:
										{
											static NetProto_Message message;
											message.Clear();
											message.set_content(gmOrder.sValue());
											message.set_count((int32_t)gmOrder.nValue());
											GCAgent_SendProtoToAllClients(&message);
										}
										break;
									case NetProto_GMOrder::SYSTEM:
										{
											static NetProto_Chat chat;
											chat.Clear();
											chat.set_channel(NetProto_Chat::SYSTEM);
											chat.set_content(gmOrder.sValue());
											GCAgent_SendProtoToAllClients(&chat);
										}
										break;
									case NetProto_GMOrder::RELOAD:
										{
											if (gmOrder.sValue() == "BusinessInfo") {
												BusinessInfoManager_Reload();
											}
										}
										break;
									case NetProto_GMOrder::MUL_EXP:
										{
											Config_SetMulExp((int)gmOrder.nValue());
										}
										break;
									default:
										break;
								}
							}
							break;

						case NetProto_GMOnlinePlayers::UNITID:
							{
								static NetProto_GMOnlinePlayers proto;
								if (!proto.ParseFromArray(data, size))
									break;

								if (!GMPool_Has(id))
									break;

								static vector<struct PlayerEntity *> players;
								players.clear();
								int count = PlayerPool_Players(&players);
								for (int i = 0; i < count; i++) {
									struct Component *component = PlayerEntity_Component(players[i]);
									if (component == NULL)
										continue;

									proto.add_name(component->baseAtt->name());
									proto.add_roleID(component->baseAtt->roleID());
								}
								GCAgent_SendProtoToClients(&id, 1, &proto);
							}
							break;

						case NetProto_GMChat::UNITID:
							{
								/*
								   static NetProto_GMChat gmChat;
								   gmChat.Clear();

								   if (!gmChat.ParseFromArray(data, size)) {
								   break;
								   }

								   if (!GMPool_Has(id)) {
								   break;
								   }

								   GMPool_SetChatFlag(id, gmChat.flag());
								   GCAgent_SendProtoToClients(&id, 1, &gmChat);
								   */
							}
							break;

						case NetProto_GMServerMgr::UNITID:
							{
								static NetProto_GMServerMgr gmServerMgr;
								gmServerMgr.Clear();

								if (!gmServerMgr.ParseFromArray(data, size)) {
									break;
								}

								if (!GMPool_Has(id)) {
									break;
								}

								gmServerMgr.set_time(Config_ServerStartTime());
								gmServerMgr.set_num(PlayerPool_TotalCount());
								GCAgent_SendProtoToClients(&id, 1, &gmServerMgr);
							}
							break;

						case NetProto_GMPlayerQuery::UNITID:
							{
								static NetProto_GMPlayerQuery gmPlayerQuery;
								gmPlayerQuery.Clear();

								if (!gmPlayerQuery.ParseFromArray(data, size)) {
									break;
								}

								if (!GMPool_Has(id)) {
									break;
								}

								if (gmPlayerQuery.att().att().baseAtt().has_name()) {
									static DCProto_GMPlayerQuery gmQuery;
									gmQuery.Clear();
									gmQuery.set_account(GMPool_IDByAccountID(id));
									gmQuery.mutable_att()->mutable_att()->mutable_baseAtt()->set_name(gmPlayerQuery.att().att().baseAtt().name());
									GCAgent_SendProtoToDCAgent(&gmQuery);
									break;
								}else {
									int64_t roleID = gmPlayerQuery.att().att().baseAtt().roleID();
									struct PlayerEntity *player = PlayerEntity_PlayerByRoleID(roleID);
									if (player != NULL) {
										const PlayerAtt* playerAtt = PlayerEntity_Att(player);
										if (playerAtt != NULL) {
											playerAtt->ToPB(gmPlayerQuery.mutable_att());
											gmPlayerQuery.set_online(true);
											GCAgent_SendProtoToClients(&id, 1, &gmPlayerQuery);
										}
										break;
									} else {
										static DCProto_GMPlayerQuery gmQuery;
										gmQuery.Clear();
										gmQuery.set_account(GMPool_IDByAccountID(id));
										gmQuery.mutable_att()->mutable_att()->mutable_baseAtt()->set_roleID(roleID);
										gmPlayerQuery.set_online(false);
										GCAgent_SendProtoToDCAgent(&gmQuery);
										break;
									}
								}
							}
							break;

						case NetProto_GMForbid::UNITID:
							{
								static NetProto_GMForbid gmForbid;
								gmForbid.Clear();

								if (!gmForbid.ParseFromArray(data, size)) {
									break;
								}

								if (!GMPool_Has(id)) {
									break;
								}

								string str = GMPool_IDByAccountID(id);
								if (!Event_GMPermission(str.c_str())) {
									break;
								}

								int64_t roleID = gmForbid.roleID();
								struct PlayerEntity *player = PlayerEntity_PlayerByRoleID(roleID);
								stForbid forbid;

								if (player != NULL) {
									const PlayerAtt* playerAtt = PlayerEntity_Att(player);
									if (playerAtt == NULL) {
										break;
									}

									forbid.roleID = gmForbid.roleID();
									if (gmForbid.flag()) {
										forbid.id = Event_CreateGMID();
										forbid.name = playerAtt->att().baseAtt().name();
										forbid.level = playerAtt->att().fightAtt().level();
										forbid.profession = (int)playerAtt->att().baseAtt().professionType();
										forbid.startTime = Time_TimeStamp();
										if (gmForbid.endTime() != 0) {
											forbid.endTime = forbid.startTime + gmForbid.endTime();
										} else {
											forbid.endTime = 0;
										}
										forbid.GM = GMPool_IDByAccountID(id); 
										if (gmForbid.select() == NetProto_GMForbid::NOTALKING) {
											forbid.flag = true; 
										} else if (gmForbid.select() == NetProto_GMForbid::FREEZE) {
											forbid.flag = false;
											struct PlayerEntity* playerEntity = PlayerEntity_PlayerByRoleID(gmForbid.roleID());
											if (playerEntity != NULL) {
												int32_t playerID = PlayerEntity_ID(playerEntity);
												if (-1 != playerID) {
													AccountPool_Logout(playerID);
												}
											}
										}

										if (Event_AddGMInfo(forbid, true)) {
											gmForbid.set_id(forbid.id);
											gmForbid.set_name(forbid.name);
											gmForbid.set_level(forbid.level);
											gmForbid.set_professionType((::PB_ProfessionInfo_Type)playerAtt->att().baseAtt().professionType());
											GCAgent_SendProtoToClients(&id, 1, &gmForbid);
										}
									} else {
										if (Event_DelForbid(gmForbid.id())) {
											GCAgent_SendProtoToClients(&id, 1, &gmForbid);
										}
									}
								} else {
									forbid.roleID = gmForbid.roleID();
									if (gmForbid.flag()) {
										forbid.id = Event_CreateGMID();
										forbid.name = gmForbid.name();
										forbid.level = gmForbid.level();
										forbid.profession = (int)gmForbid.professionType();
										forbid.startTime = Time_TimeStamp();
										if (gmForbid.endTime() != 0) {
											forbid.endTime = forbid.startTime + gmForbid.endTime();
										} else {
											forbid.endTime = 0;
										}
										forbid.GM = GMPool_IDByAccountID(id); 
										if (gmForbid.select() == NetProto_GMForbid::NOTALKING) {
											forbid.flag = true; 
										} else if (gmForbid.select() == NetProto_GMForbid::FREEZE) {
											forbid.flag = false;
										}

										if (Event_AddGMInfo(forbid, true)) {
											gmForbid.set_id(forbid.id);
											//	gmForbid.set_name(forbid.name);
											//	gmForbid.set_level(forbid.level);
											//	gmForbid.set_professionType((::PB_ProfessionInfo_Type)playerAtt->att().baseAtt().professionType());
											GCAgent_SendProtoToClients(&id, 1, &gmForbid);
										}
									} else {
										if (Event_DelForbid(gmForbid.id())) {
											GCAgent_SendProtoToClients(&id, 1, &gmForbid);
										}
									}
								}

							}
							break;

						case NetProto_GMNoticeMgr::UNITID:
							{
								static NetProto_GMNoticeMgr gmNoticeMgr;
								gmNoticeMgr.Clear();

								if (!gmNoticeMgr.ParseFromArray(data, size)) {
									break;
								}

								if (!GMPool_Has(id)) {
									break;
								}

								if (Event_NoticeOperator(&gmNoticeMgr, id)) {
									GCAgent_SendProtoToClients(&id, 1, &gmNoticeMgr);
								}
							}
							break;

						case NetProto_GMRequest::UNITID:
							{
								static NetProto_GMRequest gmRequest;
								gmRequest.Clear();

								if (!gmRequest.ParseFromArray(data, size)) {
									break;
								}

								if (!GMPool_Has(id)) {
									break;
								}

								if (Event_GMRequest(gmRequest)) {
									GCAgent_SendProtoToClients(&id, 1, &gmRequest);
								}
							}
							break;

						case NetProto_GMChatRecords::UNITID: 
							{
								static NetProto_GMChatRecords chats;
								chats.Clear();

								if (!chats.ParseFromArray(data, size)) {
									break;
								}

								static DCProto_GMChatRecords dc;
								dc.Clear();
								dc.set_id(id);
								*dc.mutable_record() = chats;
								GCAgent_SendProtoToDCAgent(&dc);
							}
							break;

						case NetProto_GMRegistrCount::UNITID: 
							{
								static NetProto_GMRegistrCount proto;
								proto.Clear();

								if (!proto.ParseFromArray(data, size)) {
									break;
								}

								static DCProto_GMRegistrCount dc;
								dc.Clear();
								dc.set_id(id);
								*dc.mutable_record() = proto;
								GCAgent_SendProtoToDCAgent(&dc);
							}
							break;

						case NetProto_GMRoleCount::UNITID:
							{
								static NetProto_GMRoleCount proto;
								proto.Clear();

								if (!proto.ParseFromArray(data, size)) {
									break;
								}

								DEBUG_LOGRECORD("recv NetProto_GMRoleCount, send DCProto_GMRoleCount to DC");
								static DCProto_GMRoleCount dc;
								dc.Clear();
								dc.set_id(id);
								*dc.mutable_record() = proto;
								GCAgent_SendProtoToDCAgent(&dc);
							}
							break;

						case NetProto_GMLevelStatistics::UNITID:
							{
								static NetProto_GMLevelStatistics proto;
								proto.Clear();

								if (!proto.ParseFromArray(data, size)) {
									break;
								}

								static DCProto_GMLevelStatistics dc;
								dc.Clear();
								dc.set_id(id);
								*dc.mutable_record() = proto;
								GCAgent_SendProtoToDCAgent(&dc);
							}
							break;

						case NetProto_GMRankStatistics::UNITID:
							{
								static NetProto_GMRankStatistics proto;
								proto.Clear();

								if (!proto.ParseFromArray(data, size)) {
									break;
								}

								int res = PlayerPool_GMRankStatistics(&proto);
								if (res == 0)	
									GCAgent_SendProtoToClients(&id, 1, &proto);

							}
							break;

						case NetProto_GMOpenGuide::UNITID:
							{
								static NetProto_GMOpenGuide proto;
								proto.Clear();

								if (!proto.ParseFromArray(data, size)) {
									break;
								}

								PlayerEntity *target = PlayerEntity_PlayerByRoleID(proto.roleID());
								if (target != NULL) {
									PlayerEntity_CompleteGuide(target, proto.id());
								} else {
									static DCProto_GMOpenGuide dc;
									dc.Clear();
									*dc.mutable_info() = proto;
									GCAgent_SendProtoToDCAgent(&dc);
								}
							}
							break;

						case NetProto_GMRegister::UNITID:
							{
								static NetProto_GMRegister proto;
								proto.Clear();

								if (!proto.ParseFromArray(data, size)) {
									break;
								}

								string str = GMPool_IDByAccountID(id);
								if (str != "") {
									int res = Event_GMRegister(&proto, str.c_str());
									if (res == 0) {
										GCAgent_SendProtoToClients(&id, 1, &proto);
									}
								}
							}
							break;

						case NetProto_GMShutDownMessage::UNITID:
							{
								static NetProto_GMShutDownMessage proto;
								proto.Clear();

								if (!proto.ParseFromArray(data, size)) {
									break;
								}

								string str = GMPool_IDByAccountID(id);
								if (str != "") {
									int res = Event_GMShutDownMessage(&proto, str.c_str());
									if (res == 0) {
										GCAgent_SendProtoToClients(&id, 1, &proto);
									}
								}
							}
							break;

						case NetProto_GMOpenMessage::UNITID:
							{
								static NetProto_GMOpenMessage proto;
								proto.Clear();

								if (!proto.ParseFromArray(data, size)) {
									break;
								}

								string str = GMPool_IDByAccountID(id);
								if (str != "") {
									int res = Event_GMOpenMessage(&proto, str.c_str());
									if (res == 0) {
										GCAgent_SendProtoToClients(&id, 1, &proto);
									}
								}
							}
							break;

						case NetProto_GMModifyVIP::UNITID:
							{
								static NetProto_GMModifyVIP proto;
								proto.Clear();

								if (!proto.ParseFromArray(data, size)) {
									break;
								}

								string str = GMPool_IDByAccountID(id);
								if (str != "") {
									int res = Event_GMModifyVIP(&proto, str.c_str());
									if (res == 0) {
										GCAgent_SendProtoToClients(&id, 1, &proto);
									}
								}
							}
							break;

						case NetProto_GMAddExchange::UNITID:
							{
								static NetProto_GMAddExchange proto;
								proto.Clear();

								if (!proto.ParseFromArray(data, size)) {
									break;
								}

								string str = GMPool_IDByAccountID(id);
								if (str != "") {
									int res = Event_GMAddExchange(&proto, str.c_str());
									if (res == 0) {
										GCAgent_SendProtoToClients(&id, 1, &proto);
									}
								}
							}
							break;

						case NetProto_GMQueryFaction::UNITID:
							{
								static NetProto_GMQueryFaction proto;
								proto.Clear();

								if (!proto.ParseFromArray(data, size)) {
									break;
								}

								string str = GMPool_IDByAccountID(id);
								if (str != "") {
									int res = Event_GMQueryFaction(&proto, str.c_str());
									if (res == 0) {
										GCAgent_SendProtoToClients(&id, 1, &proto);
									}
								}
							}
							break;

						case NetProto_GMChangeFactionMem::UNITID:
							{
								static NetProto_GMChangeFactionMem proto;
								proto.Clear();

								if (!proto.ParseFromArray(data, size)) {
									break;
								}

								string str = GMPool_IDByAccountID(id);
								if (str != "") {
									int res = Event_GMChangeFactionMem(&proto, str.c_str());
									if (res == 0) {
										GCAgent_SendProtoToClients(&id, 1, &proto);
									}
								}
							}
							break;

						case NetProto_GMAddRekooRole::UNITID:
							{
								static NetProto_GMAddRekooRole proto;
								proto.Clear();

								if (!proto.ParseFromArray(data, size)) {
									break;
								}

								string str = GMPool_IDByAccountID(id);
								if (str != "") {
									int res = Event_GMAddRekooRole(&proto, str.c_str());
									if (res == 0) {
										GCAgent_SendProtoToClients(&id, 1, &proto);
									}
								}
							}
							break;

						case NetProto_GMLoginInfo::UNITID:
							{
								static NetProto_GMLoginInfo proto;
								proto.Clear();

								if (!proto.ParseFromArray(data, size)) {
									break;
								}

								PlayerPool_GMSetMaxRoleCountInfo(&proto);
							}
							break;

						case NetProto_GMReload::UNITID:
							{
								static NetProto_GMReload proto;
								if (!proto.ParseFromArray(data, size)) {
									break;
								}
								int id = proto.id();
								if(id == 1)
								{
									GmSet_Reload();
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
								else if( id == 2)
								{
									Config_ReloadChannelData();
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
								else
								{
									proto.set_id(-1);
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						case NetProto_GMDelProp::UNITID:
							{
								static NetProto_GMDelProp proto;
								proto.Clear();

								if (!proto.ParseFromArray(data, size)) {
									break;
								}

								DEBUG_LOG("roleID == %d", proto.roleID());

								struct PlayerEntity * entity = PlayerEntity_PlayerByRoleID(proto.roleID());
								if (entity == NULL)
									break;

								struct Component *component = PlayerEntity_Component(entity);
								if (component == NULL)
									break;

								DEBUG_LOG("propID == %d", proto.id());
								DEBUG_LOG("select == %d", proto.select());

								Item_DelFromPackage(component->item, proto.select(), proto.id());
								PlayerEntity_DelFromMail(entity, proto.select(), proto.id());
							}
							break;

						default:
							break;
					}
				}
				break;

				case NetProto_ObtainPet::GROUPID:
				{
					switch(unitID) {
						case NetProto_ObtainPet::UNITID:
							{
								static NetProto_ObtainPet proto;
								if (!proto.ParseFromArray(data, size)) {
									break;
								}
								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL) {
									break;
								}

								int index = PlayerEntity_ObtainPet(player, proto.id());
								if (index >= 0) {
									proto.set_index(index);
									GCAgent_SendProtoToClients(&id, 1, &proto);
									DEBUG_LOG("Success to obtainpet, ret: %d", index);
									/*
									   static NetProto_GetRes gr;
									   gr.Clear();
									   PB_ItemInfo *item = gr.add_items();
									   item->set_type(PB_ItemInfo::PET);
									   item->set_id(proto.id());
									   item->set_count(1);
									   GCAgent_SendProtoToClients(&id, 1, &gr);
									   */
								} else {
									DEBUG_LOG("Failed to obtainpet, ret: %d", index);
								}
							}
							break;

						case NetProto_PetFighting::UNITID:
							{
								static NetProto_PetFighting proto;
								if (!proto.ParseFromArray(data, size)) {
									break;
								}
								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL) {
									break;
								}

								if (PlayerEntity_SetPetFight(player, proto.index())) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						case NetProto_PetRest::UNITID:
							{
								static NetProto_PetRest proto;
								if (!proto.ParseFromArray(data, size)) {
									break;
								}
								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL) {
									break;
								}

								if (PlayerEntity_SetPetRest(player, proto.index(), proto.flag())) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						case NetProto_PetAttach::UNITID:
							{
							}
							break;

						case NetProto_PetLevelUp::UNITID:
							{
								static NetProto_PetLevelUp proto;
								if (!proto.ParseFromArray(data, size)) {
									break;
								}
								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL) {
									break;
								}

								int change = PlayerEntity_SetPetLevelUp(player, proto.index());
								if (change > 0) {
									proto.set_change(change);
									GCAgent_SendProtoToClients(&id, 1, &proto);

									//PlayerEntity_PetPower(player, proto.index());
								}
							}
							break;

						case NetProto_PetLearnSkill::UNITID:
							{
								/*
								   static NetProto_PetLearnSkill proto;
								   if (!proto.ParseFromArray(data, size)) {
								   break;
								   }
								   struct PlayerEntity *player = PlayerEntity_Player(id);
								   if (player == NULL) {
								   break;
								   }

								   int skillsIndex = PlayerEntity_PetLearnSkill(player, proto.index(), proto.skillID(), true);
								   if (-1 != skillsIndex) {
								   proto.set_skillsIndex(skillsIndex);
								   GCAgent_SendProtoToClients(&id, 1, &proto);
								   }
								   */
							}						
							break;

						case NetProto_PetInherit::UNITID:
							{
								/*
								   static NetProto_PetInherit proto;
								   if (!proto.ParseFromArray(data, size)) {
								   break;
								   }
								   struct PlayerEntity *player = PlayerEntity_Player(id);
								   if (player == NULL) {
								   break;
								   }

								   PlayerEntity_PetInherit(player, proto.indexPre(), proto.indexAfter(), proto.inheritSkill());
								   */
							}
							break;

						case NetProto_PetAdvance::UNITID:
							{
								static NetProto_PetAdvance proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size)) {
									break;
								}

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL) {
									break;
								}

								int res = PlayerEntity_PetAdvance(player, proto.index());
								if (res >= 0) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						case NetProto_PetPsychicsLevelUp::UNITID:
							{
								static NetProto_PetPsychicsLevelUp proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size)) {
									break;
								}

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL) {
									break;
								}

								struct Component *component = PlayerEntity_Component(player);
								Item_PetHaloAttributeIncrease(component->item, false);
								int res = PlayerEntity_PetPsychicsLevelUp(player, &proto);
								Item_PetHaloAttributeIncrease(component->item, true);

								if (res > 0) {
									proto.set_res(res);
									GCAgent_SendProtoToClients(&id, 1, &proto);

									//PlayerEntity_PetPower(player, -2);
								}
							}
							break;

					}
				}
				break;

				case NetProto_ActiveGemRequest::GROUPID:
				{
					switch(unitID) {
						case NetProto_ActiveGemRequest::UNITID:
							{
								static NetProto_ActiveGemRequest proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size)) {
									break;
								}
								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL) {
									break;
								}

								int value = PlayerEntity_ActiveGenRequest(player);
								if (-1 != value) {
									proto.set_value(value);
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						case NetProto_ActiveDoubleGem::UNITID:
							{
								/*
								   static NetProto_ActiveDoubleGem proto;
								   proto.Clear();
								   if (!proto.ParseFromArray(data, size)) {
								   break;
								   }
								   struct PlayerEntity *player = PlayerEntity_Player(id);
								   if (player == NULL) {
								   break;
								   }
								   if (PlayerEntity_ActiveDoubleGen(player)) {
								   GCAgent_SendProtoToClients(&id, 1, &proto);
								   }
								   */
							}
							break;

						case NetProto_ActiveUpGradeGem::UNITID:
							{
								static NetProto_ActiveUpGradeGem proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size)) {
									break;
								}
								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL) {
									break;
								}
								if (PlayerEntity_ActiveUpGradeGen(player, proto.index())) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						case NetProto_ActiveGetGem::UNITID:
							{
								static NetProto_ActiveGetGem proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size)) {
									break;
								}
								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL) {
									break;
								}
								if (PlayerEntity_ActiveGetGen(player, proto.flag())) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						case NetProto_ActiveFight::UNITID:
							{
								static NetProto_ActiveFight proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size)) {
									break;
								}
								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL) {
									break;
								}
								if (PlayerEntity_ActiveFight(player, true)) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						case NetProto_ActiveStrongeSolider::UNITID:
							{
								static NetProto_ActiveStrongeSolider proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size)) {
									break;
								}
								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL) {
									break;
								}
								int count = PlayerEntity_ActiveStrongeSolider(player, false);
								if (-1 != count) {
									proto.set_count(count);
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						case NetProto_PlayOffInfo::UNITID:
							{
								static NetProto_PlayOffInfo proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								int day, pass, turn, overTime;
								int ret = PlayOffManager_CurData(proto.id(), &day, &pass, &turn, &overTime);
								if (ret == -1) {
									proto.set_res(-1);
								} else {
									int right = PlayerEntity_CanEnterPlayOff(player, proto.id(), day, pass);
									if (right == -1) {
										proto.set_res(-2);
									} else if (right == -2) {
										proto.set_res(-3);
									} else if (right == 0 || right == 1 || right == 2) {
										if (right == 0)
											proto.set_res(0);
										else if (right == 1)
											proto.set_res(-4);
										else if (right == 2)
											proto.set_res(-5);

										int64_t roleID = component->roleAtt->baseAtt().roleID();
										struct PlayOffUnit *unit = MapPool_SelectPlayOff(roleID, proto.id(), day, pass, turn, overTime);
										if (unit == NULL)
											break;
										if (roleID == unit->lhs)
											proto.set_result((unit->lWin << 16) | unit->rWin);
										else if (roleID == unit->rhs)
											proto.set_result((unit->rWin << 16) | unit->lWin);

										const PB_PlayerAtt *another = NULL;
										if (unit->lhs == roleID)
											another = &unit->rAtt;
										else if (unit->rhs == roleID)
											another = &unit->lAtt;
										proto.mutable_att()->mutable_att()->mutable_baseAtt()->set_roleID(-1);
										const PlayOff *info = PlayOffManager_PlayOff(proto.id());
										if (PlayOffManager_Count(info->over(day)) > 1) {
											if (another != NULL)
												*proto.mutable_att() = *another;
										}
									}
								}
								proto.set_day(day);
								proto.set_pass(pass);
								proto.set_overTime(overTime);
								proto.set_turn(turn);
								GCAgent_SendProtoToClients(&id, 1, &proto);
							}
							break;

						case NetProto_EnterPlayOff::UNITID:
							{
								static NetProto_EnterPlayOff proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;
								struct Component *component = PlayerEntity_Component(player);

								int day, pass, turn, overTime;
								int ret = PlayOffManager_CurData(proto.id(), &day, &pass, &turn, &overTime);
								if (ret == -1 || ret == 1) {
									static NetProto_Error error;
									error.set_content(Config_Words(32));
									GCAgent_SendProtoToClients(&id, 1, &error);
									break;
								}

								int right = PlayerEntity_CanEnterPlayOff(player, proto.id(), day, pass);
								if (right == -1) {
									static NetProto_Error error;
									error.set_content(Config_Words(52));
									GCAgent_SendProtoToClients(&id, 1, &error);
									break;
								} else if (right == -2) {
									static NetProto_Error error;
									error.set_content(Config_Words(32));
									GCAgent_SendProtoToClients(&id, 1, &error);
									break;
								} else if (right == 1) {
									static NetProto_Error error;
									error.set_content(Config_Words(53));
									GCAgent_SendProtoToClients(&id, 1, &error);
									break;
								} else if (right == 2) {
									static NetProto_Error error;
									error.set_content(Config_Words(59));
									GCAgent_SendProtoToClients(&id, 1, &error);
									break;
								}

								struct PlayOffUnit *unit = MapPool_SelectPlayOff(component->baseAtt->roleID(), proto.id(), day, pass, turn, overTime);
								if (unit == NULL) {
									DEBUG_LOGERROR("Failed to select playoff, id: %d, day: %d, pass: %d", proto.id(), day, pass);
									break;
								}
								if (unit->room == -1) {
									DEBUG_LOGERROR("Failed to enter playoff room");
									break;
								}
								const MapInfo *info = MapInfoManager_MapInfo(MapPool_MapInfo(unit->room));
								if (info == NULL)
									break;

								int count1 = PlayerPool_Count(unit->room, -1);
								if (count1 < 0)
									count1 = 0;
								struct Movement *movements[MAX_ROOM_PLAYERS];
								int count2 = MapPool_Linkers(unit->room, movements, MAX_ROOM_PLAYERS);
								if (count2 < 0)
									count2 = 0;
								static Vector2i nextCoord;
								if (!MapInfoManager_EnterCoord(info, count1 + count2, &nextCoord)) {
									DEBUG_LOGERROR("Failed to get enter coord");
									break;
								}

								Movement_BeginChangeScene(component->movement, unit->room, &nextCoord);
							}
							break;

						case NetProto_Treasure::UNITID:
							{
								static NetProto_Treasure proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								struct Component *component = PlayerEntity_Component(player);
								if (component == NULL)
									break;

								int res = PlayerEntity_Treasure(player, &proto);
								DEBUG_LOG("HHHHHHHHHHHHHH%d", res);
								if (res >= 0) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						case NetProto_Hire::UNITID:
							{
								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;
								struct Component *component = PlayerEntity_Component(entity);

								Movement_BeginMultiRoom(component->movement);

								/*
								   static NetProto_Hire proto;
								   proto.Clear();
								   if (!proto.ParseFromArray(data, size))
								   break;

								   struct PlayerEntity *player = PlayerEntity_Player(id);
								   if (player == NULL)
								   break;

								   int res = PlayerEntity_Hire(player, &proto);
								   if (res == 0) {
								   GCAgent_SendProtoToClients(&id, 1, &proto);
								   }
								   */
							}
							break;

						case NetProto_MoneyTree::UNITID:
							{
								static NetProto_MoneyTree proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								int res = PlayerEntity_MoneyTree(player, &proto);
								if (res == 0) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						case NetProto_FlyPlan::UNITID:
							{
								static NetProto_FlyPlan proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								struct Component *component = PlayerEntity_Component(player);
								int res = Item_FlyPlan(component->item, &proto);
								if (res == 0) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						case NetProto_BlessCome::UNITID:
							{
								static NetProto_BlessCome proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								int res = PlayerPool_Blessing(player, &proto);
								if (res == 0) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						case NetProto_TopUpObtRMB::UNITID:
							{
								static NetProto_TopUpObtRMB proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								int res = PlayerEntity_TopUpObtRMB(player, &proto);
								if (res == 0) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						case NetProto_ServerOpenTime::UNITID:
							{
								static NetProto_ServerOpenTime proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size))
									break;

								PlayerPool_ServerOpenTime(&proto);
								GCAgent_SendProtoToClients(&id, 1, &proto);
							}
							break;

						case NetProto_InvateCode::UNITID:
							{
								static NetProto_InvateCode proto;
								proto.Clear();
								if (!proto.ParseFromArray(data, size))
									break;

								PlayerEntity *entity = PlayerEntity_Player(id);
								if (entity == NULL)
									break;

								int64_t roleID = PlayerEntity_RoleID(entity);
								int res = PlayerPool_AddInvateCode(roleID, &proto);
								if (res == 0) {
									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;
					}
				}
				break;

				case NetProto_CreateFaction::GROUPID:
				{
					switch(unitID) {
						case NetProto_CreateFaction::UNITID:
							{
								static NetProto_CreateFaction faction;
								if (!faction.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								const PlayerAtt *att = PlayerEntity_Att(player);
								if (att == NULL)
									break;

								struct Component *component = PlayerEntity_Component(player);
								if (!Item_HasRMB(component->item, Config_CreateFaction())) {
									faction.set_res(-3);
									GCAgent_SendProtoToClients(&id, 1, &faction);
									break;
								}

								int res = FactionPool_CreateFaction(att, faction.str().c_str());
								if (res == 0) {
									Item_ModifyRMB(component->item, -Config_CreateFaction(), false, 21, 0, 0);
									PlayerEntity_SysFaction(player, faction.str().c_str());
								}

								faction.set_res(res);
								GCAgent_SendProtoToClients(&id, 1, &faction);
							}
							break;

						case NetProto_DelFaction::UNITID:
							{
								static NetProto_DelFaction faction;
								if (!faction.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								const PlayerAtt *att = PlayerEntity_Att(player);
								if (att == NULL)
									break;

								int res = FactionPool_DelFaction(att);
								if (res == 0) {
									PlayerEntity_SysFaction(player, "");
								}

								faction.set_res(res);
								GCAgent_SendProtoToClients(&id, 1, &faction);
							}
							break;

						case NetProto_Donate::UNITID:
							{
								static NetProto_Donate proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								const PlayerAtt *att = PlayerEntity_Att(player);
								if (att == NULL)
									break;

								int expend = 0;
								int dayEvent = 0;
								int factionExp = 0;
								int exp = 0;
								int item = 0;
								Config_FactionDoante(proto.type(), &expend, &dayEvent, &factionExp, &exp, &item);
								struct Component *component = PlayerEntity_Component(player);
								int num = component->playerAtt->dayEvent(34) & 0x0FFF;
								if (proto.type() == 1)
									num = num & 0x0F;
								else if (proto.type() == 2) 
									num = (num & 0x0F0) >> 4;
								else if (proto.type() == 3)
									num = (num & 0x0F00) >> 8;
								else 
									break;

								if (num >= 10)
									break;

								if (proto.type() == 1) {
									if (Item_Package(component->item)->money() < expend) {
										break;
									}
								} else if (proto.type() == 2 || proto.type() == 3) {
									if (!Item_HasRMB(component->item, expend)) {
										break;
									}
								} else {
									break;
								}

								int res = FactionPool_Donate(att, proto.type());
								if (0 == res) {
									if (proto.type() == 1) {
										Item_ModifyMoney(component->item, -expend);
										PlayerEntity_SetDayEvent(player, 34, component->playerAtt->dayEvent(34) + 1);
									} else if (proto.type() == 2) {
										PlayerEntity_SetDayEvent(player, 34, component->playerAtt->dayEvent(34) + 0x010);
										Item_ModifyRMB(component->item, -expend, false, 21, 0, 0);
									} else if (proto.type() == 3) {
										PlayerEntity_SetDayEvent(player, 34, component->playerAtt->dayEvent(34) + 0x0100);
										Item_ModifyRMB(component->item, -expend, false, 21, 0, 0);
									}

									GCAgent_SendProtoToClients(&id, 1, &proto);
								}
							}
							break;

						case NetProto_FactionNotice::UNITID:
							{
								static NetProto_FactionNotice notice;
								if (!notice.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								const PlayerAtt *att = PlayerEntity_Att(player);
								if (att == NULL)
									break;

								int res = FactionPool_Notice(att, notice.str().c_str());
								if (0 == res)
									GCAgent_SendProtoToClients(&id, 1, &notice);
							}
							break;

						case NetProto_Designate::UNITID:
							{
								static NetProto_Designate designate;
								if (!designate.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								const PlayerAtt *att = PlayerEntity_Att(player);
								if (att == NULL)
									break;

								int res = FactionPool_Designate(att, designate.roleID(), designate.type());
								DEBUG_LOG("NNNNNNNNNNNN:%d", res);
								if (0 == res)
									GCAgent_SendProtoToClients(&id, 1, &designate);
							}
							break;

						case NetProto_AddMem::UNITID:
							{
								static NetProto_AddMem mem;
								if (!mem.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								const PlayerAtt *att = PlayerEntity_Att(player);
								if (att == NULL)
									break;

								int res = FactionPool_AddMem(att, mem.roleID(), mem.flag());
								DEBUG_LOG("KKKKKKKK:%d", res);
								if (0 <= res) {
									GCAgent_SendProtoToClients(&id, 1, &mem);
									if (res == 0) {
										PlayerEntity_SysFactionMem(mem.roleID(), att->faction());
										PlayerEntity *target = PlayerEntity_PlayerByRoleID(mem.roleID());
										if (target != NULL) {
											int32_t targetID = PlayerEntity_ID(target);
											static NetProto_AcceptToFaction accept;
											accept.set_factionName(att->faction());
											GCAgent_SendProtoToClients(&targetID, 1, &accept);
										}
									}
								} else if (res == -4) {
									mem.set_roleID(-4);
									GCAgent_SendProtoToClients(&id, 1, &mem);
								}
							}
							break;

						case NetProto_DelMem::UNITID:
							{
								static NetProto_DelMem mem;
								if (!mem.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								const PlayerAtt *att = PlayerEntity_Att(player);
								if (att == NULL)
									break;

								int res = FactionPool_DelMem(att, mem.roleID());
								if (0 == res) {
									GCAgent_SendProtoToClients(&id, 1, &mem);
									PlayerEntity_SysFactionMem(mem.roleID(), "");

									PlayerEntity *target = PlayerEntity_PlayerByRoleID(mem.roleID());
									if(target != NULL){
										int32_t targetID = PlayerEntity_ID(target);
										static NetProto_AcceptToFaction accept;
										accept.set_factionName("");
										GCAgent_SendProtoToClients(&targetID, 1, &accept);
									}
								}
							}
							break;

						case NetProto_Applicant::UNITID:
							{
								static NetProto_Applicant mem;
								if (!mem.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								const PlayerAtt *att = PlayerEntity_Att(player);
								if (att == NULL)
									break;

								struct Component *component = PlayerEntity_Component(player);
								if (component == NULL)
									break;

								int vip = component->playerAtt->itemPackage().vip();
								int power = Fight_Power(component->fight);
								int res = FactionPool_Applicant(att, mem.str().c_str(), power, vip);
								if (0 == res)
									GCAgent_SendProtoToClients(&id, 1, &mem);
							}
							break;

						case NetProto_FactionInfo::UNITID:
							{
								static NetProto_FactionInfo info;
								if (!info.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								const PlayerAtt *att = PlayerEntity_Att(player);
								if (att == NULL)
									break;

								int res = FactionPool_FactionInfo(att, &info);
								if (0 == res)
									GCAgent_SendProtoToClients(&id, 1, &info);
							}
							break;

						case NetProto_Guardian::UNITID:
							{
								static NetProto_Guardian info;
								if (!info.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								const PlayerAtt *att = PlayerEntity_Att(player);
								if (att == NULL)
									break;

								int res = FactionPool_Guardian(att, info.type());
								DEBUG_LOG("JJJJJJJJJJJJ%d", res);
								if (0 == res) {
									GCAgent_SendProtoToClients(&id, 1, &info);
								} else if (res == -4) {
									info.set_type(-4);
									GCAgent_SendProtoToClients(&id, 1, &info);
								}
							}
							break;

						case NetProto_FactionChangeMem::UNITID:
							{
								static NetProto_FactionChangeMem mem;
								if (!mem.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								const PlayerAtt *att = PlayerEntity_Att(player);
								if (att == NULL)
									break;

								int res = FactionPool_ChangeMem(att, &mem);
								if (0 == res)
									GCAgent_SendProtoToClients(&id, 1, &mem);
							}
							break;

						case NetProto_FactionGuardian::UNITID:
							{
								static NetProto_FactionGuardian proto;
								if (!proto.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								const PlayerAtt *att = PlayerEntity_Att(player);
								if (att == NULL)
									break;

								int res = FactionPool_FactionGuardian(att, &proto);
								if (0 == res)
									GCAgent_SendProtoToClients(&id, 1, &proto);
							}
							break;

						case NetProto_FactionAllApplicant::UNITID:
							{
								static NetProto_FactionAllApplicant mem;
								if (!mem.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								const PlayerAtt *att = PlayerEntity_Att(player);
								if (att == NULL)
									break;

								int res = FactionPool_ApplicantListRequest(att, &mem);
								if (0 == res)
									GCAgent_SendProtoToClients(&id, 1, &mem);
							}
							break;

						case NetProto_FactionList::UNITID:
							{
								static NetProto_FactionList list;
								if (!list.ParseFromArray(data, size))
									break;

								struct PlayerEntity *player = PlayerEntity_Player(id);
								if (player == NULL)
									break;

								const PlayerAtt *att = PlayerEntity_Att(player);
								if (att == NULL)
									break;

								int res = FactionPool_FactionListRequest(att, &list);
								if (0 == res)
									GCAgent_SendProtoToClients(&id, 1, &list);
							}
							break;

						default:
							break;
					}
				}
				break;

				case NetProto_AsyncStatistics::GROUPID:
				{
					switch(unitID) {
						case NetProto_AsyncStatistics::UNITID:
							{
								static NetProto_AsyncStatistics proto;
								proto.Clear();

								if (!proto.ParseFromArray(data, size))
									break;
								DEBUG_LOG("recv");

								bool flag = proto.isFirstIN();
								static char buf[CONFIG_FIXEDARRAY];
								if (proto.beginAutoInit()) {
									SNPRINTF1(buf, "{\"isFirstIN\":\"%d\"}", (int)flag);
									ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "StartUpdate", buf));
								}else {
									break;
								}

								if (proto.beginDecompress()) {
									SNPRINTF1(buf, "{\"isFirstIN\":\"%d\"}", (int)flag);
									ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "StartDecompress", buf));
									DEBUG_LOG("send");
								}else {
									break;
								}

								if (proto.decompressCur() != -1 && proto.decompressTotal() != 0) {
									float fValue = (float)proto.decompressCur()/proto.decompressTotal();
									SNPRINTF1(buf, "{\"isFirstIN\":\"%d\",\"decompressTotal\":\"%d\",\"precent\":\"%f\"}", (int)flag, proto.decompressTotal(), fValue);
									ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "DecompressProgess", buf));
									DEBUG_LOG("send");
								}

								if (proto.beginInit()) {
									SNPRINTF1(buf, "{\"isFirstIN\":\"%d\"}", (int)flag);
									ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "InitDownLoad", buf));
								}else {
									break;
								}

								if (proto.checkVersion()) {
									SNPRINTF1(buf, "{\"isFirstIN\":\"%d\"}", (int)flag);
									ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "CheckVersion", buf));
								}else {
									break;
								}

								if (proto.beginDownload()) {
									SNPRINTF1(buf, "{\"isFirstIN\":\"%d\"}", (int)flag);
									ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "BeginDownLoad", buf));

									if (proto.md5() != "") {
										SNPRINTF1(buf, "{\"isFirstIN\":\"%d\",\"md5\":\"%s\"}", (int)flag, proto.md5().c_str());
										ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "VersionInfo", buf));
									}

									if (true) {
										char *index = buf;
										SNPRINTF2(buf, index, "{\"isFirstIN\":\"%d", (int)flag);
										for (int i = 0; i < proto.needDownload_size(); ++i) {
											if (i == 0) {
												index += strlen(index);
												SNPRINTF2(buf, index, "\",\"downloadFile\":\"");
											}
											index += strlen(index);
											SNPRINTF2(buf, index, "%d,",proto.needDownload(i));
										}
										index += strlen(index);
										SNPRINTF2(buf, index, "\"");
										index += strlen(index);
										SNPRINTF2(buf, index, "}");
										ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "DownloadFile", buf));

										index = buf;
										SNPRINTF1(buf, index, "{\"isFirstIN\":\"%d", (int)flag);
										for (int i = 0; i < proto.needDownload_size(); ++i) {
											if (i == 0) {
												index += strlen(index);
												SNPRINTF2(buf, index, "\",\"overDownloadFile\":\"");
											}
											index += strlen(index);
											SNPRINTF2(buf, index, "%d,", proto.needDownload(i));
										}
										index += strlen(index);
										SNPRINTF2(buf, index, "\"");
										index += strlen(index);
										SNPRINTF2(buf, index, "}");
										ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "OverDownloadFile", buf));
									}
								}

								if (proto.endDownload()) {
									SNPRINTF1(buf, "{\"isFirstIN\":\"%d\"}", (int)flag);
									ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "EndDownloadFile", buf));
								}else {
									break;
								}

								if (proto.beginLoad()) {
									SNPRINTF1(buf, "{\"isFirstIN\":\"%d\"}", (int)flag);
									ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "BeginLoad", buf));
								}else {
									break;
								}

								if (proto.endLoad()) {
									SNPRINTF1(buf, "{\"isFirstIN\":\"%d\"}", (int)flag);
									ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "EndLoad", buf));
								}else {
									break;
								}
							}
							break;

						case NetProto_StatisticsAGSLoadScene::UNITID:
							{
								if (groupID != NetProto_AsyncStatistics::GROUPID || unitID != NetProto_StatisticsAGSLoadScene::UNITID)
									DEBUG_LOGERROR("groupid: %d, unitid: %d", (int)groupID, (int)unitID);

								static NetProto_StatisticsAGSLoadScene proto;
								proto.Clear();

								if (!proto.ParseFromArray(data, size)) {
									break;
								}
								static char buf[CONFIG_FIXEDARRAY];
								SNPRINTF1(buf, "{\"after\":\"%d\"}", (int)proto.after());
								ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "StatisticsAGSLoadScene", buf));
							}
							break;

						case NetProto_StatisticsAGSLoadNPC::UNITID:
							{
								if (groupID != NetProto_AsyncStatistics::GROUPID || unitID != NetProto_StatisticsAGSLoadNPC::UNITID)
									DEBUG_LOGERROR("groupid: %d, unitid: %d", (int)groupID, (int)unitID);

								static NetProto_StatisticsAGSLoadNPC proto;
								proto.Clear();

								if (!proto.ParseFromArray(data, size)) {
									break;
								}
								static char buf[CONFIG_FIXEDARRAY];
								SNPRINTF1(buf, "{\"index\":\"%d\",\"after\":\"%d\"}", proto.index(), (int)proto.after());
								ScribeClient_SendMsg(ScribeClient_Format(AccountPool_IDToAccount(id), id, "StatisticsAGSLoadNPC", buf));
							}
							break;
					}
					break;
				}
				break;

				default:
				break;
			}
	}

