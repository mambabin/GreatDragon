
#include <RidesInfo.pb.h>
#include <SkillInfo.pb.h>
#include <StatusInfo.pb.h>
#include <EquipmentInfo.pb.h>
#include <EquipmentRandom.pb.h>
#include <ItemInfo.pb.h>
#include <RoleAtt.pb.h>
#include <MissionInfo.pb.h>
#include <BusinessInfo.pb.h>
#include <NPCAtt.pb.h>
#include <BloodInfo.pb.h>
#include <BoxInfo.pb.h>
#include <GuideInfo.pb.h>
#include <Award.pb.h>
#include <DesignationInfo.pb.h>
#include <VIPInfo.pb.h>
#include <Fashion.pb.h>
#include <MapInfo.pb.h>
#include <ActiveInfo.pb.h>
#include <NoticeInfo.pb.h>
#include <FactionInfo.pb.h>
#include <TransformInfo.pb.h>
#include <Helper.pb.h>
#include <PetHalo.pb.h>
#include <PlayOff.pb.h>
#include <GodShip.pb.h>
#include <Reservation.pb.h>
#include <GmSet.pb.h>
#include <google/protobuf/descriptor.h>
#include <tinyxml2.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <string>
#include <cstring>
#include <cstdlib>
#include <list>

using namespace std;
using namespace tinyxml2;
using namespace google::protobuf;

#define SKILL_XML "Skills.xml"
#define SKILL_PROTOBUF "Skills.bytes"

#define STATUS_XML "Statuses.xml"
#define STATUS_PROTOBUF "Statuses.bytes"

#define EQUIPMENT_XML "Equipments.xml"
#define EQUIPMENT_PROTOBUF "Equipments.bytes"

#define GOODS_XML "Goods.xml"
#define GOODS_PROTOBUF "Goods.bytes"

#define NPC_XML "NPCs.xml"
#define NPC_PROTOBUF "NPCs.bytes"

#define PET_XML "Pets.xml"
#define PET_PROTOBUF "Pets.bytes"

#define MISSION_XML "Missions.xml"
#define MISSION_PROTOBUF "Missions.bytes"

#define BUSINESS_XML "Business.xml"
#define BUSINESS_PROTOBUF "Business.bytes"

#define BLOOD_XML "Blood.xml"
#define BLOOD_PROTOBUF "Blood.bytes"

#define BLOODNODES_XML "BloodNodes.xml"
#define BLOODNODES_PROTOBUF "BloodNodes.bytes"

#define BOXES_XML "Boxes.xml"
#define BOXES_PROTOBUF "Boxes.bytes"

#define EXPLORE_XML "Explore.xml"
#define EXPLORE_PROTOBUF "Explore.bytes"

#define GUIDENODE_XML "GuideNode.xml"
#define GUIDENODE_PROTOBUF "GuideNode.bytes"

#define GUIDEPASS_XML "GuidePass.xml"
#define GUIDEPASS_PROTOBUF "GuidePass.bytes"

#define AWARD_XML "Award.xml"
#define AWARD_PROTOBUF "Award.bytes"

#define DESIGNATION_XML "Designation.xml"
#define DESIGNATION_PROTOBUF "Designation.bytes"

#define VIP_XML "VIP.xml"
#define VIP_PROTOBUF "VIP.bytes"

#define FASHION_XML "Fashion.xml"
#define FASHION_PROTOBUF "Fashion.bytes"

#define MAPINFO_XML "MapInfo.xml"
#define MAPINFO_PROTOBUF "MapInfo.bytes"

#define EQUIPRECIPE_XML "EquipmentRecipe.xml"
#define EQUIPRECIPE_PROTOBUF "EquipmentRecipe.bytes"

#define BASEWING_XML "BaseWing.xml"
#define BASEWING_PROTOBUF "BaseWing.bytes"

#define WING_XML "Wing.xml"
#define WING_PROTOBUF "Wing.bytes"

#define ACTIVE_XML "Active.xml"
#define ACTIVE_PROTOBUF "Active.bytes"

#define NOTICE_XML "Notice.xml"
#define NOTICE_PROTOBUF "Notice.bytes"

#define MAILGIFT_XML "MailGift.xml"
#define MAILGIFT_PROTOBUF "MailGift.bytes"

#define SOCIATY_XML "Sociaty.xml"
#define SOCIATY_PROTOBUF "Sociaty.bytes"

#define SOCIATYSKILL_XML "SociatySkill.xml"
#define SOCIATYSKILL_PROTOBUF "SociatySkill.bytes"

#define PLAYOFF_XML "PlayOff.xml"
#define PLAYOFF_PROTOBUF "PlayOff.bytes"

#define TRANSFORMS_XML "Transforms.xml"
#define TRANSFORMS_PROTOBUF "Transforms.bytes"

#define DROPTABLES_XML "DropTables.xml"
#define DROPTABLES_PROTOBUF "DropTables.bytes"

#define HELPERNODE_XML "HelperNode.xml"
#define HELPERNODE_PROTOBUF "HelperNode.bytes"

#define HELPERGROUP_XML "HelperGroup.xml"
#define HELPERGROUP_PROTOBUF "HelperGroup.bytes"

#define PETHALO_XML "PetHalo.xml"
#define PETHALO_PROTOBUF "PetHalo.bytes"

#define EXCHANGETABLE_XML "ExchangeTable.xml"
#define EXCHANGETABLE_PROTOBUF "ExchangeTable.bytes"

#define GODSHIP_XML "GodShip.xml"
#define GODSHIP_PROTOBUF "GodShip.bytes"

#define RESERVATION_XML "Fight.xml"
#define RESERVATION_PROTOBUF "Fight.bytes"

#define RIDESINFO_XML "Rides.xml"
#define RIDESINFO_PROTOBUF "Rides.bytes"

#define EQUIPMENTRANDOMCOLOR_XML "EquipmentRandomColor.xml"
#define EQUIPMENTRANDOMCOLOR_PROTOBUF "EquipmentRandomColor.bytes"

#define EQUIPMENTRANDOMPOS_XML "EquipmentRandomPos.xml"
#define EQUIPMENTRANDOMPOS_PROTOBUF "EquipmentRandomPos.bytes"

#define EQUIPMENTRANDOMLEVEL_XML "EquipmentRandomLevel.xml"
#define EQUIPMENTRANDOMLEVEL_PROTOBUF "EquipmentRandomLevel.bytes"

#define EQUIPMENTRANDOMEFFECT_XML "EquipmentRandomEffect.xml"
#define EQUIPMENTRANDOMEFFECT_PROTOBUF "EquipmentRandomEffect.bytes"

#define GMSET_XML "GmSet.xml"
#define GMSET_PROTOBUF "GmSet.bytes"

static inline void ShowHelp(const char *me){
	printf("Usage: %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n", me, SKILL_XML, STATUS_XML, EQUIPMENT_XML, GOODS_XML, NPC_XML, PET_XML, MISSION_XML, BUSINESS_XML, BLOOD_XML, BLOODNODES_XML, BOXES_XML, EXPLORE_XML, GUIDENODE_XML, GUIDEPASS_XML, AWARD_XML, DESIGNATION_XML, VIP_XML, FASHION_XML, MAPINFO_XML, EQUIPRECIPE_XML, BASEWING_XML, WING_XML, ACTIVE_XML, NOTICE_XML, MAILGIFT_XML, SOCIATY_XML, SOCIATYSKILL_XML, PLAYOFF_XML, TRANSFORMS_XML, DROPTABLES_XML, HELPERNODE_XML, HELPERGROUP_XML, PETHALO_XML, EXCHANGETABLE_XML, GODSHIP_XML, RESERVATION_XML, RIDESINFO_XML, EQUIPMENTRANDOMCOLOR_XML, EQUIPMENTRANDOMPOS_XML, EQUIPMENTRANDOMLEVEL_XML, EQUIPMENTRANDOMEFFECT_XML, GMSET_XML);
}

static const XMLNode * SearchNode(const XMLNode *parent, const char *name){
	if(strcmp(parent->Value(), name) == 0)
		return parent;

	for(const XMLNode *child = parent->FirstChild(); child != NULL; child = child->NextSibling()){
		return SearchNode(child, name);
	}

	return NULL;
}

static const char * ReadValue(const XMLNode *parent, const char *name){
	for(const XMLNode *first = parent->FirstChild(); first != NULL; first = first->NextSibling()){
		if(strcmp(first->Value(), name) == 0){
			return first->FirstChild()->Value();
		}
	}

	return NULL;
}

static bool FindFinalField(Message *message, char *key, const char *value, const FieldDescriptor **finalFD, Message **finalMessage){
	const FieldDescriptor *fd = NULL;

	for(char *token = strtok(key, "."); token != NULL; token = strtok(NULL, ".")){
		char *c = strchr(token, '-');
		int index = -1;
		if(c != NULL){
			*c = '\0';
			index = atoi(c + 1);
		}

		const Descriptor *desc = message->GetDescriptor();
		const Reflection *refl = message->GetReflection();
		fd = desc->FindFieldByName(token);
		if(fd == NULL)
			return false;

		if(fd->type() == FieldDescriptor::TYPE_MESSAGE){
			if(fd->label() == FieldDescriptor::LABEL_OPTIONAL){
				assert(index == -1);
				message = refl->MutableMessage(message, fd);
			}
			else if(fd->label() == FieldDescriptor::LABEL_REPEATED){
				assert(index != -1);
				for(int i = refl->FieldSize(*message, fd); i <= index; i++)
					refl->AddMessage(message, fd);
				message = refl->MutableRepeatedMessage(message, fd, index);
			}
			else{
				assert(0);
			}
		}
		else{
			if(index != -1){
				for(int i = refl->FieldSize(*message, fd); i < index; i++){
					switch(fd->type()){
						case FieldDescriptor::TYPE_INT32:
							refl->AddInt32(message, fd, atoi(value));
							break;
						case FieldDescriptor::TYPE_INT64:
							refl->AddInt64(message, fd, atoll(value));
							break;
						case FieldDescriptor::TYPE_FLOAT:
							refl->AddFloat(message, fd, atof(value));
							break;
						case FieldDescriptor::TYPE_BOOL:
							refl->AddBool(message, fd, atoi(value));
							break;
						case FieldDescriptor::TYPE_ENUM:
							{
								const EnumValueDescriptor *enumValue = fd->enum_type()->FindValueByNumber(atoi(value));
								assert(enumValue != NULL);
								refl->AddEnum(message, fd, enumValue);
							}
							break;
						default:
							assert(0);
					}
				}
				assert(refl->FieldSize(*message, fd) == index);
			}

			assert(strtok(NULL, ".") == NULL);
			*finalFD = fd;
			*finalMessage = message;
			return true;
		}
	}

	return false;
}


template<typename ProtoType>
static bool XMLToProto(const char *xmlFile, const char *group, list<ProtoType> *all){
	all->clear();

	XMLDocument xml;
	if(xml.LoadFile(xmlFile) != XML_NO_ERROR){
		printf("Failed to load %s\n", xmlFile);
		return false;
	}

	ProtoType proto;
	for(const XMLNode *root = xml.FirstChild(); root != NULL; root = root->NextSibling()){
		const XMLNode *first = SearchNode(root, group);
		for(const XMLNode *node = first; node != NULL; node = node->NextSibling()){
			proto.Clear();
			for(const XMLNode *unit = node->FirstChild(); unit != NULL; unit = unit->NextSibling()){
				static char key[1024 * 10], value[1024 * 1024];
				assert(strlen(unit->Value()) < sizeof(key));
				assert(strlen(unit->FirstChild()->Value()) < sizeof(value));

				strcpy(key, unit->Value());
				strcpy(value, unit->FirstChild()->Value());

				const FieldDescriptor *field = NULL;
				Message *message = NULL;
				if(!FindFinalField(&proto, key, value, &field, &message)){
					printf("Failed to find field \"%s(%s)\" in proto file\n", key, unit->Value());
					continue;
				}

				const Descriptor *desc = message->GetDescriptor();
				const Reflection *refl = message->GetReflection();

				switch(field->type()){
					case FieldDescriptor::TYPE_INT32:
						{
							if(field->label() == FieldDescriptor::LABEL_OPTIONAL){
								refl->SetInt32(message, field, atoi(value));
							}
							else if(field->label() == FieldDescriptor::LABEL_REPEATED){
								refl->AddInt32(message, field, atoi(value));
							}
							else{
								printf("Label of \"%s\" is not supported\n", key);
								continue;
							}
						}
						break;

					case FieldDescriptor::TYPE_INT64:
						{
							if(field->label() == FieldDescriptor::LABEL_OPTIONAL){
								refl->SetInt64(message, field, atoi(value));
							}
							else if(field->label() == FieldDescriptor::LABEL_REPEATED){
								refl->AddInt64(message, field, atoi(value));
							}
							else{
								printf("Label of \"%s\" is not supported\n", key);
								continue;
							}
						}
						break;

					case FieldDescriptor::TYPE_FLOAT:
						{
							if(field->label() == FieldDescriptor::LABEL_OPTIONAL){
								refl->SetFloat(message, field, (float)atof(value));
							}
							else if(field->label() == FieldDescriptor::LABEL_REPEATED){
								refl->AddFloat(message, field, (float)atof(value));
							}
							else{
								printf("Label of \"%s\" is not supported\n", key);
								continue;
							}
						}
						break;

					case FieldDescriptor::TYPE_BOOL:
						{
							if(field->label() == FieldDescriptor::LABEL_OPTIONAL){
								refl->SetBool(message, field, atoi(value));
							}
							else if(field->label() == FieldDescriptor::LABEL_REPEATED){
								refl->AddBool(message, field, atoi(value));
							}
							else{
								printf("Label of \"%s\" is not supported\n", key);
								continue;
							}
						}
						break;

					case FieldDescriptor::TYPE_ENUM:
						{
							if(field->label() == FieldDescriptor::LABEL_OPTIONAL){
								const EnumValueDescriptor *enumValue = field->enum_type()->FindValueByNumber(atoi(value));
								if(enumValue == NULL){
									printf("Enum value of \"%s\" is not valid\n", key);
									continue;
								}
								refl->SetEnum(message, field, enumValue);
							}
							else if(field->label() == FieldDescriptor::LABEL_REPEATED){
								const EnumValueDescriptor *enumValue = field->enum_type()->FindValueByNumber(atoi(value));
								if(enumValue == NULL){
									printf("Enum value of \"%s\" is not valid\n", key);
									continue;
								}
								refl->AddEnum(message, field, enumValue);
							}
							else{
								printf("Label of \"%s\" is not supported\n", key);
								continue;
							}
						}
						break;

					case FieldDescriptor::TYPE_STRING:
						{
							if(field->label() == FieldDescriptor::LABEL_OPTIONAL){
								refl->SetString(message, field, value);
							}
							else if(field->label() == FieldDescriptor::LABEL_REPEATED){
								printf("Repeated string type is not supported, label: %s\n", key);
								continue;
								// refl->AddString(message, field, value);
							}
							else{
								printf("Label of \"%s\" is not supported\n", key);
								continue;
							}
						}
						break;

					default:
						printf("Type of \"%s\" in xml file is not supported\n", key);
						continue;
				}

			}

			all->push_back(proto);
		}
	}

	return true;
}

template<typename ProtoType>
static void Serialize(const char *file, ProtoType *proto){
	fstream out(file, ios_base::out | ios_base::binary | ios::trunc);
	if(out.fail()){
		printf("Failed to create %s\n", file);
		return;
	}

	printf("file->%s", file);
	proto->SerializeToOstream(&out);
	out.close();
}

int main(int argc, char *argv[]){
	if(argc <= 1){
		ShowHelp(argv[0]);
		return 0;
	}

	for(int i = 1; i < argc; i++){
		if(strcmp(argv[i], NPC_XML) == 0){
			list<PB_NPCAtt> all;
			if(!XMLToProto(NPC_XML, "NPC", &all))
				continue;

			PB_AllNPCs npcs;
			for(list<PB_NPCAtt>::iterator it = all.begin(); it != all.end(); it++){
				while(it->id() >= npcs.npcs_size())
					npcs.add_npcs()->set_id(-1);
				*npcs.mutable_npcs(it->id()) = *it;
			}

			Serialize(NPC_PROTOBUF, &npcs);
		}else if(strcmp(argv[i], PET_XML) == 0){
			list<PB_NPCAtt> all;
			if(!XMLToProto(PET_XML, "NPC", &all))
				continue;

			PB_AllPets pets;
			for(list<PB_NPCAtt>::iterator it = all.begin(); it != all.end(); it++){
				*pets.add_pets() = *it;
			}

			Serialize(PET_PROTOBUF, &pets);
		}else if(strcmp(argv[i], SKILL_XML) == 0){
			list<SkillInfo> all;
			if(!XMLToProto(SKILL_XML, "Skill", &all))
				continue;

			AllSkills skills;
			for(list<SkillInfo>::iterator it = all.begin(); it != all.end(); it++){
				while(it->id() >= skills.skills_size())
					skills.add_skills();
				SkillFamily *family = skills.mutable_skills(it->id());
				while(it->level() >= family->skills_size())
					family->add_skills()->set_id(-1);
				*family->mutable_skills(it->level()) = *it;
			}

			Serialize(SKILL_PROTOBUF, &skills);
		}else if(strcmp(argv[i], STATUS_XML) == 0){
			list<StatusInfo> all;
			if(!XMLToProto(STATUS_XML, "Status", &all))
				continue;

			AllStatuses statuses;
			for(list<StatusInfo>::iterator it = all.begin(); it != all.end(); it++){
				while(it->id() >= statuses.statuses_size())
					statuses.add_statuses()->set_id(-1);
				*statuses.mutable_statuses(it->id()) = *it;
			}

			Serialize(STATUS_PROTOBUF, &statuses);
		}else if(strcmp(argv[i], MISSION_XML) == 0){
			list<MissionInfo> all;
			if(!XMLToProto(MISSION_XML, "Mission", &all))
				continue;

			AllMissions missions;
			for(list<MissionInfo>::iterator it = all.begin(); it != all.end(); it++){
				while(it->id() >= missions.missions_size())
					missions.add_missions()->set_id(-1);
				*missions.mutable_missions(it->id()) = *it;
			}

			Serialize(MISSION_PROTOBUF, &missions);
		}else if(strcmp(argv[i], GOODS_XML) == 0){
			list<GoodsInfo> all;
			if(!XMLToProto(GOODS_XML, "Goods", &all))
				continue;

			AllGoods goods;
			for(list<GoodsInfo>::iterator it = all.begin(); it != all.end(); it++){
				while(it->id() >= goods.goods_size())
					goods.add_goods()->set_id(-1);
				*goods.mutable_goods(it->id()) = *it;
			}

			Serialize(GOODS_PROTOBUF, &goods);
		}else if(strcmp(argv[i], EQUIPMENT_XML) == 0){
			list<EquipmentInfo> all;
			if(!XMLToProto(EQUIPMENT_XML, "Equipment", &all))
				continue;

			AllEquipments equipments;
			for(list<EquipmentInfo>::iterator it = all.begin(); it != all.end(); it++){
				while(it->id() >= equipments.equipments_size())
					equipments.add_equipments()->set_id(-1);
				*equipments.mutable_equipments(it->id()) = *it;
			}

			Serialize(EQUIPMENT_PROTOBUF, &equipments);
		}else if(strcmp(argv[i], BUSINESS_XML) == 0){
			list<BusinessInfo> all;
			if(!XMLToProto(BUSINESS_XML, "Business", &all))
				continue;

			AllBusiness business;
			for(list<BusinessInfo>::iterator it = all.begin(); it != all.end(); it++){
				while(it->id() >= business.business_size())
					business.add_business()->set_id(-1);
				*business.mutable_business(it->id()) = *it;
			}
			Serialize(BUSINESS_PROTOBUF, &business);
		}else if(strcmp(argv[i], BLOOD_XML) == 0){
			list<BloodInfo> all;
			if(!XMLToProto(BLOOD_XML, "BloodInfo", &all))
				continue;

			AllBloodInfo allBloodInfo;
			for(list<BloodInfo>::iterator it = all.begin(); it != all.end(); it++){
				while(it->level() >= allBloodInfo.bloodInfo_size())
					allBloodInfo.add_bloodInfo()->set_level(-1);
				*allBloodInfo.mutable_bloodInfo(it->level()) = *it;
			}
			Serialize(BLOOD_PROTOBUF, &allBloodInfo);
		}else if(strcmp(argv[i], BLOODNODES_XML) == 0){
			list<BloodNodeInfo> all;
			if(!XMLToProto(BLOODNODES_XML, "BloodNodeInfo", &all))
				continue;

			AllBloodNodeInfo allBloodNodeInfo;
			for(list<BloodNodeInfo>::iterator it = all.begin(); it != all.end(); it++){
				while(it->id() >= allBloodNodeInfo.bloodNodeInfo_size())
					allBloodNodeInfo.add_bloodNodeInfo()->set_id(-1);
				*allBloodNodeInfo.mutable_bloodNodeInfo(it->id()) = *it;
			}
			Serialize(BLOODNODES_PROTOBUF, &allBloodNodeInfo);
		}else if(strcmp(argv[i], BOXES_XML) == 0){
			list<BoxInfo> all;
			if(!XMLToProto(BOXES_XML, "BoxInfo", &all))
				continue;

			AllBoxes allBoxes;
			for(list<BoxInfo>::iterator it = all.begin(); it != all.end(); it++){
				while(it->id() >= allBoxes.boxes_size())
					allBoxes.add_boxes()->set_id(-1);
				*allBoxes.mutable_boxes(it->id()) = *it;
			}
			Serialize(BOXES_PROTOBUF, &allBoxes);
		}else if(strcmp(argv[i], EXPLORE_XML) == 0){
			list<ExploreInfo> all;
			if(!XMLToProto(EXPLORE_XML, "ExploreInfo", &all))
				continue;

			AllExploreInfo allExploreInfo;
			for(list<ExploreInfo>::iterator it = all.begin(); it != all.end(); it++){
				while(it->id() >= allExploreInfo.exploreInfo_size())
					allExploreInfo.add_exploreInfo()->set_id(-1);
				*allExploreInfo.mutable_exploreInfo(it->id()) = *it;
			}
			Serialize(EXPLORE_PROTOBUF, &allExploreInfo);
		}else if(strcmp(argv[i], GUIDENODE_XML) == 0){
			list<GuideNode> all;
			if(!XMLToProto(GUIDENODE_XML, "GuideNode", &all))
				continue;

			AllGuideNode allGuideNode;
			for(list<GuideNode>::iterator it = all.begin(); it != all.end(); it++){
				while(it->id() >= allGuideNode.guideNodes_size())
					allGuideNode.add_guideNodes()->set_id(-1);
				*allGuideNode.mutable_guideNodes(it->id()) = *it;
			}
			Serialize(GUIDENODE_PROTOBUF, &allGuideNode);
		}else if(strcmp(argv[i], GUIDEPASS_XML) == 0){
			list<GuidePass> all;
			if(!XMLToProto(GUIDEPASS_XML, "GuidePass", &all))
				continue;

			AllGuidePass allGuidePass;
			for(list<GuidePass>::iterator it = all.begin(); it != all.end(); it++){
				while(it->id() >= allGuidePass.guidePasses_size())
					allGuidePass.add_guidePasses()->set_id(-1);
				*allGuidePass.mutable_guidePasses(it->id()) = *it;
			}
			Serialize(GUIDEPASS_PROTOBUF, &allGuidePass);
		}else if(strcmp(argv[i], AWARD_XML) == 0){
			list<AwardInfo> all;
			if(!XMLToProto(AWARD_XML, "AwardInfo", &all))
				continue;

			AllAwardInfo allAwardInfo;
			for(list<AwardInfo>::iterator it = all.begin(); it != all.end(); it++){
				while(it->id() >= allAwardInfo.awardInfo_size())
					allAwardInfo.add_awardInfo()->set_id(-1);
				*allAwardInfo.mutable_awardInfo(it->id()) = *it;
			}
			Serialize(AWARD_PROTOBUF, &allAwardInfo);
		}else if(strcmp(argv[i], DESIGNATION_XML) == 0){
			list<DesignationInfo> all;
			if(!XMLToProto(DESIGNATION_XML, "Designation", &all))
				continue;

			AllDesignationInfo allDesignationInfo;
			for(list<DesignationInfo>::iterator it = all.begin(); it != all.end(); it++){
				while(it->id() >= allDesignationInfo.designationInfo_size())
					allDesignationInfo.add_designationInfo()->set_id(-1);
				*allDesignationInfo.mutable_designationInfo(it->id()) = *it;
			}
			Serialize(DESIGNATION_PROTOBUF, &allDesignationInfo);
		}else if(strcmp(argv[i], VIP_XML) == 0){
			list<VIPInfo> all;
			if(!XMLToProto(VIP_XML, "VIPInfo", &all))
				continue;

			AllVIPInfo allVIPInfo;
			for(list<VIPInfo>::iterator it = all.begin(); it != all.end(); it++){
				while(it->level() >= allVIPInfo.vipInfo_size())
					allVIPInfo.add_vipInfo()->set_level(-1);
				*allVIPInfo.mutable_vipInfo(it->level()) = *it;
			}
			Serialize(VIP_PROTOBUF, &allVIPInfo);
		}else if(strcmp(argv[i], FASHION_XML) == 0){
			list<FashionInfo> all;
			if(!XMLToProto(FASHION_XML, "Fashion", &all))
				continue;

			AllFashions allFashions;
			for(list<FashionInfo>::iterator it = all.begin(); it != all.end(); it++){
				while(it->id() >= allFashions.fashions_size())
					allFashions.add_fashions()->set_id(-1);
				*allFashions.mutable_fashions(it->id()) = *it;
			}
			Serialize(FASHION_PROTOBUF, &allFashions);
		}else if(strcmp(argv[i], MAPINFO_XML) == 0){
			list<MapInfo> all;
			if(!XMLToProto(MAPINFO_XML, "MapInfo", &all))
				continue;

			AllMapInfo allMapInfo;
			for(list<MapInfo>::iterator it = all.begin(); it != all.end(); it++){
				while(it->id() >= allMapInfo.mapInfo_size())
					allMapInfo.add_mapInfo()->set_id(-1);
				*allMapInfo.mutable_mapInfo(it->id()) = *it;
			}
			Serialize(MAPINFO_PROTOBUF, &allMapInfo);
		}else if(strcmp(argv[i], EQUIPRECIPE_XML) == 0){
			list<EquipRecipe> all;
			if(!XMLToProto(EQUIPRECIPE_XML, "EquipmentRecipe", &all))
				continue;

			AllEquipRecipes allEquipRecipes;
			for(list<EquipRecipe>::iterator it = all.begin(); it != all.end(); it++){
				while(it->id() >= allEquipRecipes.recipes_size())
					allEquipRecipes.add_recipes()->set_id(-1);
				*allEquipRecipes.mutable_recipes(it->id()) = *it;
			}
			Serialize(EQUIPRECIPE_PROTOBUF, &allEquipRecipes);
		}else if(strcmp(argv[i], BASEWING_XML) == 0){
			list<BaseWing> all;
			if(!XMLToProto(BASEWING_XML, "BaseWing", &all))
				continue;

			AllBaseWings allBaseWings;
			for(list<BaseWing>::iterator it = all.begin(); it != all.end(); it++){
				*allBaseWings.add_wings() = *it;
			}
			Serialize(BASEWING_PROTOBUF, &allBaseWings);
		}else if(strcmp(argv[i], WING_XML) == 0){
			list<Wing> all;
			if(!XMLToProto(WING_XML, "Wing", &all))
				continue;

			AllWings allWings;
			for(list<Wing>::iterator it = all.begin(); it != all.end(); it++){
				while(it->id() >= allWings.wings_size())
					allWings.add_wings()->set_id(-1);
				*allWings.mutable_wings(it->id()) = *it;
			}
			Serialize(WING_PROTOBUF, &allWings);
		}else if(strcmp(argv[i], ACTIVE_XML) == 0){
			list<ActiveInfo> all;
			if(!XMLToProto(ACTIVE_XML, "Active", &all))
				continue;

			AllActives allActives;
			for(list<ActiveInfo>::iterator it = all.begin(); it != all.end(); it++){
				while(it->id() >= allActives.allActives_size())
					allActives.add_allActives()->set_id(-1);
				*allActives.mutable_allActives(it->id()) = *it;
			}
			Serialize(ACTIVE_PROTOBUF, &allActives);
		}else if(strcmp(argv[i], NOTICE_XML) == 0){
			list<NoticeInfo> all;
			if(!XMLToProto(NOTICE_XML, "Notice", &all))
				continue;

			AllNotice allNotice;
			for(list<NoticeInfo>::iterator it = all.begin(); it != all.end(); it++){
				*allNotice.add_allNotice() = *it;
			}
			Serialize(NOTICE_PROTOBUF, &allNotice);
		}else if(strcmp(argv[i], MAILGIFT_XML) == 0){
			list<MailGift> all;
			if(!XMLToProto(MAILGIFT_XML, "MailGift", &all))
				continue;

			AllMailGift allMailGift;
			for(list<MailGift>::iterator it = all.begin(); it != all.end(); it++){
				*allMailGift.add_mailGift() = *it;
			}
			Serialize(MAILGIFT_PROTOBUF, &allMailGift);
		}else if(strcmp(argv[i], SOCIATY_XML) == 0) {
			list<FactionBase> all;
			if(!XMLToProto(SOCIATY_XML, "Sociaty", &all))
				continue;
			FactionBaseInfo faction;
			for(list<FactionBase>::iterator it = all.begin(); it != all.end(); it++){
				*faction.add_faction() = *it;
			}
			Serialize(SOCIATY_PROTOBUF, &faction);
		}else if(strcmp(argv[i], SOCIATYSKILL_XML) == 0){
			list<SociatySkillBase> all;
			if(!XMLToProto(SOCIATYSKILL_XML, "SociatySkill", &all))
				continue;
			SociatySkillBaseInfo info;
			for(list<SociatySkillBase>::iterator it = all.begin(); it != all.end(); it++){
				*info.add_info() = *it;
			}
			Serialize(SOCIATYSKILL_PROTOBUF, &info);
		}else if(strcmp(argv[i], PLAYOFF_XML) == 0){
			list<PlayOff> all;
			if(!XMLToProto(PLAYOFF_XML, "PlayOff", &all))
				continue;
			AllPlayOff allPlayOff;
			for(list<PlayOff>::iterator it = all.begin(); it != all.end(); it++){
				while(it->id() >= allPlayOff.playoff_size())
					allPlayOff.add_playoff()->set_id(-1);
				*allPlayOff.mutable_playoff(it->id()) = *it;
			}
			Serialize(PLAYOFF_PROTOBUF, &allPlayOff);
		} else if (strcmp(argv[i], TRANSFORMS_XML) == 0) {
			list<TransformInfo> all;
			if(!XMLToProto(TRANSFORMS_XML, "Transform", &all))
				continue;
			AllTransforms allTransforms;
			for (list<TransformInfo>::iterator it = all.begin(); it != all.end(); it++) {
				*allTransforms.add_transforms() = *it;
			}
			Serialize(TRANSFORMS_PROTOBUF, &allTransforms);
		} else if (strcmp(argv[i], DROPTABLES_XML) == 0) {
			list<DropTable> all;
			if(!XMLToProto(DROPTABLES_XML, "DropTable", &all))
				continue;
			AllDropTables allDropTables;
			for(list<DropTable>::iterator it = all.begin(); it != all.end(); it++){
				while(it->id() >= allDropTables.dropTables_size())
					allDropTables.add_dropTables()->set_id(-1);
				*allDropTables.mutable_dropTables(it->id()) = *it;
			}
			Serialize(DROPTABLES_PROTOBUF, &allDropTables);
		} else if (strcmp(argv[i], HELPERNODE_XML) == 0) {
			list<HelperNode> allNode;
			if(!XMLToProto(HELPERNODE_XML, "HelperNode", &allNode))
				continue;
			AllHelperNodes allHelperNodes;
			for(list<HelperNode>::iterator it = allNode.begin(); it != allNode.end(); it++){
				while(it->id() >= allHelperNodes.nodes_size())
					allHelperNodes.add_nodes()->set_id(-1);
				*allHelperNodes.mutable_nodes(it->id()) = *it;
			}
			Serialize(HELPERNODE_PROTOBUF, &allHelperNodes);
		} else if (strcmp(argv[i], HELPERGROUP_XML) == 0) {
			list<HelperGroup> allGroup;
			if(!XMLToProto(HELPERGROUP_XML, "HelperGroup", &allGroup))
				continue;
			AllHelper allHelper;
			for(list<HelperGroup>::iterator it = allGroup.begin(); it != allGroup.end(); it++){
				*allHelper.add_helpers() = *it;
			}
			Serialize(HELPERGROUP_PROTOBUF, &allHelper);
		} else if (strcmp(argv[i], PETHALO_XML) == 0) {
			list<PB_PetHaloInfo> pet;
			if(!XMLToProto(PETHALO_XML, "PetHalo", &pet))
				continue;
			PB_AllPetHaloInfo allPet;
			for(list<PB_PetHaloInfo>::iterator it = pet.begin(); it != pet.end(); it++){
				*allPet.add_info() = *it;
			}
			Serialize(PETHALO_PROTOBUF, &allPet);
		} else if (strcmp(argv[i], EXCHANGETABLE_XML) == 0) {
			list<ExchangeTable> all;
			if(!XMLToProto(EXCHANGETABLE_XML, "ExchangeTable", &all))
				continue;
			AllExchangeTable exchangeTable;
			for(list<ExchangeTable>::iterator it = all.begin(); it != all.end(); it++){
				*exchangeTable.add_exchangeTable() = *it;
			}
			Serialize(EXCHANGETABLE_PROTOBUF, &exchangeTable);
		} else if (strcmp(argv[i], GODSHIP_XML) == 0) {
			list<GodShip> all;
			if(!XMLToProto(GODSHIP_XML, "GodShip", &all))
				continue;
			AllGodShips godShips;
			for(list<GodShip>::iterator it = all.begin(); it != all.end(); it++){
				*godShips.add_godShips() = *it;
			}
			Serialize(GODSHIP_PROTOBUF, &godShips);
		} else if (strcmp(argv[i], RESERVATION_XML) == 0) {
			list<PB_ReservationTime> all;
			if(!XMLToProto(RESERVATION_XML, "Fight", &all))
				continue;
			PB_AllReservationTimes allTime;
			for(list<PB_ReservationTime>::iterator it = all.begin(); it != all.end(); it++){
				*allTime.add_times() = *it;
			}
			Serialize(RESERVATION_PROTOBUF, &allTime);
		}
		else if(strcmp(argv[i], RIDESINFO_XML) == 0)
		{
			list<RidesInfo> all;
			if(!XMLToProto(RIDESINFO_XML, "Ride", &all))
				continue;

			AllRideses rides;
			for(list<RidesInfo>::iterator it = all.begin(); it != all.end(); it++)
			{
				while(it->id() >= rides.rides_size())
					rides.add_rides();

				RidesGroup *rg = rides.mutable_rides(it->id());
				while(it->tpStar() >= rg->rides_size())
					rg->add_rides()->set_id(-1);

				*rg->mutable_rides(it->tpStar()) = *it;
			}

			Serialize(RIDESINFO_PROTOBUF, &rides);
		}
		else if(strcmp(argv[i], EQUIPMENTRANDOMCOLOR_XML) == 0)
		{
			list<EquipmentRandomColor> all;
			if(!XMLToProto(EQUIPMENTRANDOMCOLOR_XML, "EquipmentRandomColor", &all))
				continue;

			AllEquipmentRandomColor equipmentRandomColor;
			int i = 0;
			for(list<EquipmentRandomColor>::iterator it = all.begin(); it != all.end(); it++)
			{
				equipmentRandomColor.add_equipmentRandomColor();
				*equipmentRandomColor.mutable_equipmentRandomColor(i) = *it;
				i++;
			}

			Serialize(EQUIPMENTRANDOMCOLOR_PROTOBUF, &equipmentRandomColor);
		}
		else if(strcmp(argv[i], EQUIPMENTRANDOMPOS_XML) == 0)
		{
			list<EquipmentRandomPos> all;
			if(!XMLToProto(EQUIPMENTRANDOMPOS_XML, "EquipmentRandomPos", &all))
				continue;

			AllEquipmentRandomPos equipmentRandomPos;
			int i = 0;
			for(list<EquipmentRandomPos>::iterator it = all.begin(); it != all.end(); it++)
			{
				equipmentRandomPos.add_equipmentRandomPos();
				*equipmentRandomPos.mutable_equipmentRandomPos(i) = *it;
				i++;
			}

			Serialize(EQUIPMENTRANDOMPOS_PROTOBUF, &equipmentRandomPos);
		}
		else if(strcmp(argv[i], EQUIPMENTRANDOMLEVEL_XML) == 0)
		{
			list<EquipmentRandomLevel> all;
			if(!XMLToProto(EQUIPMENTRANDOMLEVEL_XML, "EquipmentRandomLevel", &all))
				continue;

			AllEquipmentRandomLevel equipmentRandomLevel;
			int i = 0;
			for(list<EquipmentRandomLevel>::iterator it = all.begin(); it != all.end(); it++)
			{
				equipmentRandomLevel.add_equipmentRandomLevel();
				*equipmentRandomLevel.mutable_equipmentRandomLevel(i) = *it;
				i++;
			}

			Serialize(EQUIPMENTRANDOMLEVEL_PROTOBUF, &equipmentRandomLevel);
		}
		else if(strcmp(argv[i], EQUIPMENTRANDOMEFFECT_XML) == 0)
		{
			list<EquipmentRandomEffect> all;
			if(!XMLToProto(EQUIPMENTRANDOMEFFECT_XML, "EquipmentRandomEffect", &all))
				continue;

			AllEquipmentRandomEffect equipmentRandomEffect;
			int i = 0;
			for(list<EquipmentRandomEffect>::iterator it = all.begin(); it != all.end(); it++)
			{
				equipmentRandomEffect.add_equipmentRandomEffect();
				*equipmentRandomEffect.mutable_equipmentRandomEffect(i) = *it;
				i++;
			}

			Serialize(EQUIPMENTRANDOMEFFECT_PROTOBUF, &equipmentRandomEffect);
		}
		else if(strcmp(argv[i], GMSET_XML) == 0)
		{
			list<GmSet> all;
			if(!XMLToProto(GMSET_XML, "GmSet", &all))
				continue;

			AllGmSet allGmSet;
			int i = 0;
			for(list<GmSet>::iterator it = all.begin(); it != all.end(); it++)
			{
				allGmSet.add_gmSet();
				*allGmSet.mutable_gmSet(i) = *it;
				i++;
			}

			Serialize(GMSET_PROTOBUF, &allGmSet);
		}
		else
		{
			printf("Invalid argument: %s\n", argv[i]);
		}
	}

	return 0;
}


