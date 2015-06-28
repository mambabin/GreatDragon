#include "DCMsgProcessor.hpp"
#include "RoleAtt.hpp"
#include "DCAgent.hpp"
#include "DCProto.pb.h"
#include "DataManager.hpp"
#include "PlayerEntity.hpp"
#include "VIPInfoManager.hpp"
#include "Web.hpp"
#include "AwardInfoManager.hpp"
#include "Debug.hpp"
#include <zmq.hpp>
#include <sys/types.h>
#include <string>
#include <map>
#include <string>

using namespace std;

static bool ExtraDCMsgHeader(zmq::message_t *msg, u_int8_t *groupID, u_int8_t *unitID, void **data, size_t *size) {
	if (msg->size() < 2)
		return false;

	*groupID = *((u_int8_t *)msg->data());
	*unitID = *((u_int8_t *)msg->data() + 1);
	*data = (u_int8_t *)msg->data() + 2;
	*size = msg->size() - 2;

	return true;
}

void DCMsgProcessor_ProcessDCMsg(zmq::message_t *msg) {
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
					case DCProto_SaveRoleData::UNITID:
						{
							static DCProto_SaveRoleData saveRoleData;
							if (!saveRoleData.ParseFromArray(data, size))
								return;

							for (int i = 0; i < saveRoleData.data_size(); i++) {
								DataManager_SaveRoleData(&saveRoleData.data(i), &saveRoleData.info(i));
							}
						}
						break;

					case DCProto_DeleteRoleData::UNITID:
						{
							static DCProto_DeleteRoleData deleteRoleData;
							if (!deleteRoleData.ParseFromArray(data, size))
								return;

							deleteRoleData.set_id(DataManager_DelRoleData(deleteRoleData.platform().c_str(), deleteRoleData.account().c_str(), deleteRoleData.id(), &deleteRoleData.equipments()));
							DCAgent_SendProtoToGCAgent(&deleteRoleData);
						}
						break;

					case DCProto_LoadRoleData::UNITID:
						{
							static DCProto_LoadRoleData loadRoleData;
							if (!loadRoleData.ParseFromArray(data, size))
								return;

							loadRoleData.clear_data();

							int64_t ids[128];
							int count = DataManager_RoleIDs(loadRoleData.platform().c_str(), loadRoleData.account().c_str(), ids, sizeof(ids) / sizeof(ids[0]));
							for (int i = 0; i < count; i++) {
								if (!DataManager_LoadRoleData(ids[i], loadRoleData.add_data())) {
									DEBUG_LOGERROR("Failed to load role data: %lld", ids[i]);
									loadRoleData.mutable_data()->RemoveLast();
									continue;
								}
							}

							DCAgent_SendProtoToGCAgent(&loadRoleData);
							DEBUG_LOG("send DCProto_LoadRoleData to GC");
						}
						break;

					case DCProto_CollectRole::UNITID:
						{
							DCProto_CollectRole collect;
							if (!collect.ParseFromArray(data, size))
								return;

							int64_t cur = -1;
							if (!DataManager_CollectRole(&cur, collect.mutable_singleRecord(), collect.mutable_Restriction(), collect.mutable_GodInfoTime(), collect.mutable_winFactionInfo(), collect.mutable_factionInfo()))
								collect.set_cur(-1);
							else
								collect.set_cur(cur);

							DCAgent_SendProtoToGCAgent(&collect, false);
						}
						break;

					case DCProto_AddAccount::UNITID:
						{
							static DCProto_AddAccount addAccount;
							if (!addAccount.ParseFromArray(data, size))
								return;

							// stress test
							/*
							static int n = 0;
							static int n1 = 0;
							static time_t t1 = Time_TimeStamp();
							if (Time_TimeStamp() - t1 > 0) {
								t1 = Time_TimeStamp();
								
								DEBUG_LOG("%d",  n - n1);
								n1 = n;
							}
							n++;
*/

							addAccount.set_res(DataManager_AddAccount(&addAccount.info(), addAccount.ip().c_str()) == 0);
							DCAgent_SendProtoToGCAgent(&addAccount);
						}
						break;

					case DCProto_AddRole::UNITID:
						{
							static DCProto_AddRole addRole;
							if (!addRole.ParseFromArray(data, size))
								return;

							DataManager_AddRole(&addRole.data(), &addRole.info());
						}
						break;

					case DCProto_SaveSingleRecord::UNITID:
						{
							static DCProto_SaveSingleRecord ssr;
							if (!ssr.ParseFromArray(data, size))
								return;

							if (ssr.mapID() == -1 || ssr.mapID() == -2) {
								string str = ssr.record().role().name();
								string str1 = str.substr(0, (int)(str.find(":")));
								string str2 = str.substr(str.find(":") + 1, (int)(str.length() - str1.length() - 1));
								DataManager_SaveRestrictionRecord(str2.c_str(), ssr.record().arg1(), atoi(str1.c_str()));	
							} else {
								DataManager_SaveSingleRecord(ssr.mapID(), &ssr.record());
							}
						}
						break;

					case DCProto_HasName::UNITID:
						{
							static DCProto_HasName hasName;
							if (!hasName.ParseFromArray(data, size))
								return;

							hasName.set_has(DataManager_HasName(hasName.cr().name().c_str()));
							DCAgent_SendProtoToGCAgent(&hasName);
						}
						break;

					case DCProto_Login::UNITID:
						{
							static DCProto_Login login;
							if (!login.ParseFromArray(data, size))
							{
								DEBUG_LOGERROR("dc recv login error");
								return;
							}

							DEBUG_LOGERROR("dc recv gc login->%s", login.login().platform().c_str());
							if (login.login().platform() == LOCAL_PLATFORM) {
								char addTime[32] = {'\0'};
								char deviceAddTime[32] = {'\0'};
								login.set_res(DataManager_Login(login.login().platform().c_str(), login.login().account().c_str(), login.login().password().c_str(), addTime, deviceAddTime));
								login.mutable_login()->set_addTime(addTime);
								login.mutable_login()->set_deviceAddTime(addTime);
								DCAgent_SendProtoToGCAgent(&login);
								if (login.res() == 0) {
									static PlayerInfo info;
									PlayerEntity_LoginToPlayerInfo(&login.login(), &info);
									DataManager_UpdateAccount(&info, login.ip().c_str());
								}
							} else {
								/*
								if (Config_ActivateKeyTable() != NULL) {
									int status = 0;
									if (login.login().activateKey().size() <= 0) {
										if (!DataManager_HasAccount(login.login().platform().c_str(), login.login().account().c_str()))
											status = -10;
									} else {
										if (DataManager_HasAccount(login.login().platform().c_str(), login.login().account().c_str())) {
											status = -10;
										} else {
											if (!DataManager_ActiveKey(login.login().activateKey().c_str()))
												status = -11;
										}
									}
									if (status != 0) {
										login.set_res(status);
										DCAgent_SendProtoToGCAgent(&login);
										break;
									}
								}
								*/
								Web_CheckUser(&login);
							}
						}
						break;

					case DCProto_QueryRole::UNITID:
						{
							static DCProto_QueryRole queryRole;
							if (!queryRole.ParseFromArray(data, size))
								return;

							if (queryRole.roleID() == -2) {
								if (!DataManager_LoadRoleData(queryRole.name(), queryRole.mutable_att())) {
									queryRole.mutable_att()->mutable_att()->mutable_baseAtt()->set_roleID(queryRole.att().att().baseAtt().roleID());
									queryRole.set_roleID(-1);
								}
							}else {
								if (!DataManager_LoadRoleData(queryRole.roleID(), queryRole.mutable_att())) {
									queryRole.mutable_att()->mutable_att()->mutable_baseAtt()->set_roleID(queryRole.att().att().baseAtt().roleID());
									queryRole.set_roleID(-1);
								}
							}
							DCAgent_SendProtoToGCAgent(&queryRole);
						}
						break;

					case DCProto_SendMail::UNITID:
						{
							static DCProto_SendMail sm;
							if (!sm.ParseFromArray(data, size))
								break;

							int res = DataManager_AddMail(sm.sm().receiver(), sm.mutable_sm()->mutable_mail());
							if (res == -1)
								sm.mutable_sm()->set_receiver(-1);
							else
								sm.mutable_sm()->set_pos(res);

							if (sm.id() != -1) {
								DCAgent_SendProtoToGCAgent(&sm);
							}

							if (sm.id() == -4) {
								static PB_PlayerAtt att;
								att.Clear();
								if (DataManager_LoadRoleData(sm.sm().receiver(), &att)) {
									att.mutable_itemPackage()->set_blessActive(0);
									DataManager_SaveRoleData(&att);
								}
							}else if (sm.id() == -5) {
								static PB_PlayerAtt att;
								att.Clear();
								if (DataManager_LoadRoleData(sm.sm().receiver(), &att)) {
									DataManager_SaveRoleData(&att);
								}
							}
						}
						break;

					case DCProto_GetKeyGift::UNITID:
						{
							static DCProto_GetKeyGift dgkg;
							if (!dgkg.ParseFromArray(data, size))
								break;

							int32_t event[CONFIG_FIXEDARRAY];
							int size = 0;
							for (; size < dgkg.event_size(); size++)
								event[size] = dgkg.event(size);
							int32_t group = -1;
							if (dgkg.done()) {
								DataManager_GetKeyGift(dgkg.key().c_str(), event, size, true, dgkg.roleID(), group);
							} else {
								dgkg.set_res(DataManager_GetKeyGift(dgkg.key().c_str(), event, size, false, 0, group));
								for (int i = 0; i < size; i++)
									dgkg.set_event(i, event[i]);

								if(group < 0)
									break;
								dgkg.set_group(group);
								DCAgent_SendProtoToGCAgent(&dgkg);
							}
						}
						break;

					case DCProto_Recharge::UNITID:
						{
							static DCProto_Recharge recharge;
							if (!recharge.ParseFromArray(data, size))
								break;

							if (recharge.over()) {
								DataManager_RechargeOver(recharge.recharge().order().c_str(), &recharge.info(), recharge.roleID(), recharge.level());
							} else {
								if (Web_CheckRecharge(&recharge) == 1) {
									int ret = DataManager_RechargeValue(&recharge);
									if (ret >= 0) {
										recharge.set_rmb(ret);
										DCAgent_SendProtoToGCAgent(&recharge);
									}
								}
							}
						}
						break;

					case DCProto_CostRecord::UNITID:
						{
							static DCProto_CostRecord cr;
							if (!cr.ParseFromArray(data, size))
								break;

							DataManager_CostRecord(&cr);
						}
						break;

					case DCProto_PlayerStatus::UNITID:
						{
							static DCProto_PlayerStatus ps;
							if (!ps.ParseFromArray(data, size))
								break;

							static PB_PlayerAtt att;
							for (int i = 0; i < ps.ps().statuses_size(); i++) {
								if (ps.ps().statuses(i) != NetProto_PlayerStatus::ONLINE) {
									if (DataManager_LoadRoleData(ps.ps().roles(i), &att)) {
										ps.mutable_ps()->set_level(i, att.att().fightAtt().level());
										ps.mutable_ps()->set_vip(i, att.itemPackage().vip());
									}
								}
							}
							DCAgent_SendProtoToGCAgent(&ps);
						}
						break;

					case DCProto_SaveChat::UNITID:
						{
							static DCProto_SaveChat sc;
							if (!sc.ParseFromArray(data, size))
								break;

							DataManager_SaveChat(&sc);
						}
						break;

					case DCProto_GodTarget::UNITID:
						{
							static DCProto_GodTarget gt;
							if (!gt.ParseFromArray(data, size))
								break;

							static PB_PlayerAtt att;
							att.Clear();
							if (DataManager_LoadRoleData(gt.roleID(), &att)) {
								PlayerEntity_ToSceneData(&att, gt.mutable_att());
								gt.set_event(att.fixedEvent(13));
								gt.set_res(true);
							} else {
								gt.set_res(false);
							}
							DCAgent_SendProtoToGCAgent(&gt);
						}
						break;

					case DCProto_LoadPlayerAtt::UNITID:
						{
							static DCProto_LoadPlayerAtt loadPlayerAtt;
							if (!loadPlayerAtt.ParseFromArray(data,size))
								break;

							static PB_PlayerAtt att;
							att.Clear();
							if (DataManager_LoadRoleData(loadPlayerAtt.roleID(), &att)) {
								if (loadPlayerAtt.flag()) {
									for (int i = 0; i < att.fans_size(); ++i) {
										if (att.fans(i).roleID() == loadPlayerAtt.roleID2()) {
											att.mutable_fans(i)->set_roleID(-1);
											att.mutable_fans(i)->set_name("");
											att.mutable_fans(i)->set_loveHeart(false);
											break;
										}
									}
								} else {
									for (int i = 0; i < att.friends_size(); ++i) {
										if (att.friends(i).roleID() == loadPlayerAtt.roleID2()) {
											att.mutable_friends(i)->set_roleID(-1);
											att.mutable_friends(i)->set_name("");
											att.mutable_friends(i)->set_loveHeart(false);
											break;
										}
									}
								}
								DataManager_SaveRoleData(&att);
							}
						}
						break;
					case DCProto_LoadFriendsFans::UNITID:
						{
							static DCProto_LoadFriendsFans proto;
							proto.Clear();
							if (!proto.ParseFromArray(data, size))
								break;

							static PB_PlayerAtt att;
							att.Clear();
							if (DataManager_LoadRoleData(proto.roleID(), &att)) {
								for (int i = 0; i < att.fans_size(); ++i) {
									if (att.fans(i).roleID() == proto.roleID2()) {
										att.mutable_fans(i)->set_loveHeart(true);
										break;
									}
								}
								DataManager_SaveRoleData(&att);
							}
						}
						break;
					case DCProto_ModifyFixedEventBit::UNITID:
						{
							static DCProto_ModifyFixedEventBit proto;
							if (!proto.ParseFromArray(data, size))
								break;

							static PB_PlayerAtt att;
							att.Clear();
							if (DataManager_LoadRoleData(proto.roleID(), &att)) {
								if (proto.id() >= 0 && proto.id() < att.fixedEvent_size()) {
									if (proto.one())
										att.set_fixedEvent(proto.id(), att.fixedEvent(proto.id()) | (1 << proto.bit()));
									else
										att.set_fixedEvent(proto.id(), att.fixedEvent(proto.id()) & (~(1 << proto.bit())));
								}
								DataManager_SaveRoleData(&att);
							}
						}
						break;

					case DCProto_ModifyGodRank::UNITID:
						{
							static DCProto_ModifyGodRank proto;
							if (!proto.ParseFromArray(data, size))
								break;

							DataManager_ModifyGodRank(&proto);

						}
						break;

					case DCProto_SaveGodRankInfoRecord::UNITID:
						{
							static DCProto_SaveGodRankInfoRecord proto;
							if (!proto.ParseFromArray(data, size))
								break;

							int res = DataManager_SaveGodRankInfoRecord(&proto);
							if (res == 0) {
								DCAgent_SendProtoToGCAgent(&proto);
							}

						}
						break;

					case DCProto_QueryGodRole::UNITID:
						{
							static DCProto_QueryGodRole proto;
							if (!proto.ParseFromArray(data, size))
								break;

							if (!DataManager_LoadRoleData(proto.roleID(), proto.mutable_att())) {
								proto.mutable_att()->mutable_att()->mutable_baseAtt()->set_roleID(proto.roleID());
								proto.set_roleID(-1);
							}
							DCAgent_SendProtoToGCAgent(&proto);
						}
						break;

					case DCProto_RandomRoles::UNITID:
						{
							static DCProto_RandomRoles proto;
							if (!proto.ParseFromArray(data, size))
								break;

							vector<int64_t> ids;
							DataManager_RandomRoleID(proto.count(), &ids);
							for (size_t i = 0; i < ids.size(); i++) {
								int64_t id = ids[i];
								if (!DataManager_LoadRoleData(id, proto.add_atts()))
									proto.mutable_atts()->RemoveLast();
							}
							DCAgent_SendProtoToGCAgent(&proto);
						}
						break;

					case DCProto_SendMailExceptRoles::UNITID:
						{
							static DCProto_SendMailExceptRoles proto;
							if (!proto.ParseFromArray(data, size))
								break;

							std::set<int64_t> roleIDs;
							for (int64_t i = 0; i < proto.roleIDs_size(); ++i) {
								roleIDs.insert(proto.roleIDs(i));
							}
							DataManager_AddMail(roleIDs, proto.mutable_sm()->mutable_mail());
						}
						break;

					case DCProto_AddOutLineFriends::UNITID:
						{
							static DCProto_AddOutLineFriends proto;
							proto.Clear();
							if (!proto.ParseFromArray(data, size))
								break;

							static PB_PlayerAtt att;
							att.Clear();
							if (DataManager_LoadRoleData(proto.roleID2(), &att)) {
								int flag = 0;
								int i = 0;
								for (i = 0; i < att.fans_size(); ++i) {
									if (att.fans(i).roleID() == proto.roleID1()) {
										flag = 1;
										break;
									}else {
										if (att.fans(i).roleID() == -1) {
											flag = 2;
											break;
										}
									}
								}
								if (flag == 1) 
									break;
								if (flag == 2) {
									att.mutable_fans(i)->set_roleID(proto.roleID1());
									att.mutable_fans(i)->set_name(proto.name().c_str());
									att.mutable_fans(i)->set_professionType(proto.professionType());
									att.mutable_fans(i)->set_loveHeart(false);

									proto.set_name(att.att().baseAtt().name());
									proto.set_professionType(att.att().baseAtt().professionType());
									DCAgent_SendProtoToGCAgent(&proto);

									DataManager_SaveRoleData(&att);
									break;
								}
							}
						}
						break;

					case DCProto_LoadHireRoleDate::UNITID:
						{
							static DCProto_LoadHireRoleDate proto;
							proto.Clear();
							if (!proto.ParseFromArray(data, size))
								break;

							for (int i = 0; i < proto.roleID_size(); ++i) {
								DataManager_LoadRoleData(proto.roleID(i), proto.add_atts());
							}
							DCAgent_SendProtoToGCAgent(&proto);
						}
						break;

					case DCProto_QueryRoleFaction::UNITID:
						{
							static DCProto_QueryRoleFaction proto;
							proto.Clear();
							if (!proto.ParseFromArray(data, size))
								break;

							int res = DataManager_QueryRoleFaction(&proto);
							if (res == 0) {
								DCAgent_SendProtoToGCAgent(&proto);
							}
						}
						break;

					case DCProto_Cost::UNITID:
						{
							static DCProto_Cost proto;
							if (!proto.ParseFromArray(data, size))
								break;

							Web_CheckCost(&proto);
						}
						break;

					case DCProto_FilterRecharge::UNITID:
						{
							static DCProto_FilterRecharge proto;
							if (!proto.ParseFromArray(data, size))
								break;

							DataManager_FilterRecharge(&proto);
							if (proto.recharge_size() > 0) {
								DCAgent_SendProtoToGCAgent(&proto);
							}
						}
						break;

					case DCProto_SetVIP::UNITID:
						{
							static DCProto_SetVIP proto;
							proto.Clear();
							if (!proto.ParseFromArray(data, size))
								break;

							static PB_PlayerAtt att;
							att.Clear();
							if (DataManager_LoadRoleData(proto.info().roleID(), &att)) {
								att.mutable_itemPackage()->set_vip(proto.info().delta());
								DataManager_SaveRoleData(&att);
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

		case DCProto_GMOrder::GROUPID:
			{
				switch(unitID) {
					case DCProto_GMOrder::UNITID:
						{
							static DCProto_GMOrder gmOrder;
							if (!gmOrder.ParseFromArray(data, size))
								break;

							static PB_PlayerAtt data;
							if (!DataManager_LoadRoleData(gmOrder.order().target(), &data))
								break;

							switch(gmOrder.order().type()) {
								default:
									return;
							}

							DataManager_SaveRoleData(&data);
						}
						break;

					case DCProto_GMSaveData::UNITID:
						{
							static DCProto_GMSaveData gmSaveData;
							gmSaveData.Clear();
							if (!gmSaveData.ParseFromArray(data, size)) {
								break;
							}
							DateManager_GMSaveData(&gmSaveData);
						}
						break;

					case DCProto_GMLoadData::UNITID:
						{
							static DCProto_GMLoadData gmLoadData;
							gmLoadData.Clear();
							if (!gmLoadData.ParseFromArray(data, size)) {
								break;
							}
							DataMansger_GMLoadData(&gmLoadData);
							DCAgent_SendProtoToGCAgent(&gmLoadData);

						}
						break;
					case DCProto_GMPlayerQuery::UNITID:
						{
							static DCProto_GMPlayerQuery gmQuery;
							gmQuery.Clear();
							if (!gmQuery.ParseFromArray(data, size)) {
								break;
							}

							if (DataManager_GMLoadAtt(&gmQuery)) {
								DCAgent_SendProtoToGCAgent(&gmQuery);
							}
						}
						break;

					case DCProto_GMChatRecords::UNITID:
						{
							static DCProto_GMChatRecords proto;
							proto.Clear();
							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							int res = DataManager_GMChatRecords(&proto);
							if (res == 0) {
								DCAgent_SendProtoToGCAgent(&proto);
							}
						}
						break;

					case DCProto_GMRegistrCount::UNITID:
						{
							static DCProto_GMRegistrCount proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							int res = DataManager_GMRegistrCount(&proto);
							if (res == 0) {
								DCAgent_SendProtoToGCAgent(&proto);
							}
						}
						break;

					case DCProto_GMRoleCount::UNITID:
						{
							static DCProto_GMRoleCount proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							DEBUG_LOGRECORD("recv DCProto_GMRoleCount, begin query");
							int res = DataManager_GMRoleCount(&proto);
							if (res == 0) {
								DEBUG_LOGRECORD("end query, send DCProto_GMRoleCount to GC");
								DCAgent_SendProtoToGCAgent(&proto);
							} else {
								DEBUG_LOGRECORD("end query, error");
							}
						}
						break;

					case DCProto_GMLevelStatistics::UNITID:
						{
							static DCProto_GMLevelStatistics proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							int res = DataManager_GMLevelStatistics(&proto);
							if (res == 0) {
								DCAgent_SendProtoToGCAgent(&proto);
							}
						}
						break;

					case DCProto_GMOpenGuide::UNITID:
						{
							static DCProto_GMOpenGuide proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							PB_PlayerAtt att;
							if (DataManager_LoadRoleData(proto.info().roleID(), &att)) {
								PlayerEntity_CompleteGuide(&att, proto.info().id());
								DataManager_SaveRoleData(&att, NULL);
							}
						}
						break;

					case DCProto_LoadAllDataFromGMDataTable::UNITID:
						{
							static DCProto_LoadAllDataFromGMDataTable proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							int res = DataManager_LoadAllDataFromGMDataTable(&proto);
							if (res == 0) {
								DCAgent_SendProtoToGCAgent(&proto);
							}
						}
						break;

					case DCProto_SaveGMDataTable::UNITID:
						{
							static DCProto_SaveGMDataTable proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							DataManager_SaveGMDataTable(&proto);
						}
						break;

					case DCProto_GMAddExchange::UNITID:
						{
							static DCProto_GMAddExchange proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							DataManager_GMAddExchange(&proto);
						}
						break;

					case DCProto_GMRekooRole::UNITID:
						{
							static DCProto_GMRekooRole proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							DataManager_GMAddRekooRole(&proto);
						}
						break;

					case DCProto_GMAddRekooRMB::UNITID:
						{
							static DCProto_GMAddRekooRMB proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							static PB_PlayerAtt att;
							att.Clear();
							if (DataManager_LoadRoleData(proto.roleID(), &att)) {
								att.mutable_itemPackage()->set_rmb(att.itemPackage().rmb() + proto.rmb());
								att.mutable_itemPackage()->set_totalRMB(att.itemPackage().totalRMB() + proto.rmb());
								DataManager_SaveRoleData(&att);
							}
						}
						break;

					case DCProto_LoadRekooRole::UNITID:
						{
							static DCProto_LoadRekooRole proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							int res = DataManager_LoadRekooRole(&proto);
							if (res == 0) {
								DCAgent_SendProtoToGCAgent(&proto);
							}
						}
						break;

					case DCProto_AddCodeCount::UNITID:
						{
							static DCProto_AddCodeCount proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							static PB_PlayerAtt att;
							if (DataManager_LoadRoleData(proto.roleID(), &att)) {
								int n = (att.fixedEvent(95) & 0xFFFF);
								if (n != 0xFFFF) {
									att.set_fixedEvent(95, (att.fixedEvent(95) & 0xFFFF0000) | (n + 1));
								}
								DataManager_SaveRoleData(&att);
							}
						}
						break;

					case DCProto_LoadInviteCode::UNITID:
						{
							static DCProto_LoadInviteCode proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							int res = DataManager_LoadInviteCode(&proto);
							if (res == 0) {
								DCAgent_SendProtoToGCAgent(&proto);
							}
						}
						break;

					case DCProto_QueryGMAccount::UNITID:
						{
							static DCProto_QueryGMAccount proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							DataManager_QueryGMAccountFromSql(&proto);
							DCAgent_SendProtoToGCAgent(&proto);
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

							int value = DataManager_RegistDeviceServer(regist.deviceNum().c_str(), regist.noLine(), regist.time());
							if (-1 != value) {
								DCAgent_SendProtoToGCAgent(&regist);
							}
						}
						break;

					default:
						break;
				}
			}
			break;

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

							int rst = DataManager_FactionLoadData(&proto);
							if (rst == 0) {
								DCAgent_SendProtoToGCAgent(&proto);
							}
						}
						break;

					case DCProto_FactionSaveData::UNITID:
						{
							static DCProto_FactionSaveData proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							DataManager_FactionSaveData(&proto);
						}
						break;

					case DCProto_FactionAddRecord::UNITID:
						{
							static DCProto_FactionAddRecord proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							DataManager_FactionAddRecord(&proto);
						}
						break;

					case DCProto_FactionDelRecord::UNITID:
						{
							static DCProto_FactionDelRecord proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							DataManager_FactionDelRecord(&proto);
						}
						break;

					case DCProto_FactionUpdateRecord::UNITID:
						{
							static DCProto_FactionUpdateRecord proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							DataManager_FactionUpdateRecord(&proto);
						}
						break;

					case DCProto_SysFactionMemInfo::UNITID:
						{
							static DCProto_SysFactionMemInfo proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							DataManager_SysFactionMemInfo(&proto);
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

							int res = DataManager_LoadRankRecord(&proto);
							if (0 == res) {
								DEBUG_LOG("AAAAAAAAAAAAAAAAAAAAAAA, %d", proto.type());
								DCAgent_SendProtoToGCAgent(&proto);
							}
						}
						break;

					case DCProto_PingPongAward::UNITID:
						{
							static DCProto_PingPongAward proto;
							if (!proto.ParseFromArray(data, size))
								break;

							if (proto.type() == NetProto_Rank::AWARD_FROM_SKY) {
								const AwardInfo *award = AwardInfoManager_AwardInfo(AwardInfo::AWARD_FROM_SKY_FINAL, 0);
								if (award != NULL) {
									static PB_PlayerAtt att;
									if (DataManager_LoadRoleData(proto.roleID(), &att)) {
										int n = (att.fixedEvent(35) & 0xffff0000) >> 16;
										if (n >= award->arg()) {
											static PB_MailInfo mail;
											mail.set_title(award->name());
											mail.set_sender(Config_Words(1));
											mail.set_content(award->content());
											mail.mutable_item()->set_type(PB_ItemInfo::GOODS);
											mail.mutable_item()->set_id(award->award());
											mail.mutable_item()->set_count(1);
											DataManager_AddMail(&att, &mail);
											att.set_fixedEvent(35, att.fixedEvent(35) & 0xffff);
										}
										DataManager_SaveRoleData(&att);
									}
								}
							}
							DCAgent_SendProtoToGCAgent(&proto);
						}
						break;

					case DCProto_FactionPower::UNITID:
						{
							static DCProto_FactionPower proto;
							proto.Clear();

							if (!proto.ParseFromArray(data, size)) {
								break;
							}

							int res = DataManager_FactionPower(&proto);
							if (0 == res) {
								DCAgent_SendProtoToGCAgent(&proto);
							}
						}
						break;

					default:
						break;
				}
				break;
			}
			break;

		default:
			break;
	}
}

