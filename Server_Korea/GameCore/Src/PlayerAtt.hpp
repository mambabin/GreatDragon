#ifndef _PLAYERATT_HPP_
#define _PLAYERATT_HPP_

#include "PlayerAtt.pb.h"
#include "ProfessionInfo.hpp"
#include "ProfessionInfo.pb.h"
#include "RoleAtt.hpp"
#include "RoleAtt.pb.h"
#include "MissionInfo.hpp"
#include "MissionInfo.pb.h"
#include "ItemInfo.hpp"
#include "ItemInfo.pb.h"
#include "MailInfo.hpp"
#include "MailInfo.pb.h"
#include "FightInfo.hpp"
#include "FightInfo.pb.h"
#include <sys/types.h>
#include <cassert>
#include <string>
#include <cstring>

class FriendInfo {

	public:
		enum NameSizeType {
			NAME_SIZE = 32,
		};
		static bool NameSizeType_IsValid(int value) {
			switch(value) {
				case NAME_SIZE:
					return true;
				default:
					return false;
			}
		}

	public:
		FriendInfo() {
			roleID_ = -1;
			name_[0] = '\0';
			professionType_ = (ProfessionInfo::Type)0;
			loveHeart_ = false;
			flag_ = false;
		}

	public:
		void ToPB(PB_FriendInfo *pb_) const {
			pb_->Clear();
			pb_->set_roleID(roleID_);
			pb_->set_name(name_);
			pb_->set_professionType((PB_ProfessionInfo::Type)professionType_);
			pb_->set_loveHeart(loveHeart_);
			pb_->set_flag(flag_);
		}
		void FromPB(const PB_FriendInfo *pb_) {
			roleID_ = pb_->roleID();
			strncpy(name_, pb_->name().c_str(), name_size() - 1);
			name_[name_size() - 1] = '\0';
			professionType_ = (ProfessionInfo::Type)pb_->professionType();
			loveHeart_ = pb_->loveHeart();
			flag_ = pb_->flag();
		}

	public:
		inline int64_t roleID() const {
			return roleID_;
		}
		inline void set_roleID(int64_t value) {
			roleID_ = value;
		}

		inline const char * name() const {
			return name_;
		}
		inline void set_name(const char * value) {
			strncpy(name_, value, name_size() - 1);
			name_[name_size() - 1] = '\0';
		}
		inline int name_size() const {
			return (int)(sizeof(name_) / sizeof(name_[0]));
		}

		inline ProfessionInfo::Type professionType() const {
			return professionType_;
		}
		inline void set_professionType(ProfessionInfo::Type value) {
			professionType_ = value;
		}

		inline bool loveHeart() const {
			return loveHeart_;
		}
		inline void set_loveHeart(bool value) {
			loveHeart_ = value;
		}

		inline bool flag() const {
			return flag_;
		}
		inline void set_flag(bool value) {
			flag_ = value;
		}

	private:
		int64_t roleID_;

		char name_[32];

		ProfessionInfo::Type professionType_;

		bool loveHeart_;

		bool flag_;

};

class DesignationRecord {

	public:
		DesignationRecord() {
			has_ = false;
			start_ = 0;
		}

	public:
		void ToPB(PB_DesignationRecord *pb_) const {
			pb_->Clear();
			pb_->set_has(has_);
			pb_->set_start(start_);
		}
		void FromPB(const PB_DesignationRecord *pb_) {
			has_ = pb_->has();
			start_ = pb_->start();
		}

	public:
		inline bool has() const {
			return has_;
		}
		inline void set_has(bool value) {
			has_ = value;
		}

		inline int64_t start() const {
			return start_;
		}
		inline void set_start(int64_t value) {
			start_ = value;
		}

	private:
		bool has_;

		int64_t start_;

};

class PetAsset {

	public:
		enum NameSizeType {
			NAME_SIZE = 32,
		};
		static bool NameSizeType_IsValid(int value) {
			switch(value) {
				case NAME_SIZE:
					return true;
				default:
					return false;
			}
		}

	public:
		PetAsset() {
			id_ = -1;
			name_[0] = '\0';
			pet_level_ = 0;
			quality_ = 0;
			level_ = 0;
		}

	public:
		void ToPB(PB_PetAsset *pb_) const {
			pb_->Clear();
			pb_->set_id(id_);
			pb_->set_name(name_);
			pb_->set_pet_level(pet_level_);
			pb_->set_quality(quality_);
			pb_->set_level(level_);
		}
		void FromPB(const PB_PetAsset *pb_) {
			id_ = pb_->id();
			strncpy(name_, pb_->name().c_str(), name_size() - 1);
			name_[name_size() - 1] = '\0';
			pet_level_ = pb_->pet_level();
			quality_ = pb_->quality();
			level_ = pb_->level();
		}

	public:
		inline int32_t id() const {
			return id_;
		}
		inline void set_id(int32_t value) {
			id_ = value;
		}

		inline const char * name() const {
			return name_;
		}
		inline void set_name(const char * value) {
			strncpy(name_, value, name_size() - 1);
			name_[name_size() - 1] = '\0';
		}
		inline int name_size() const {
			return (int)(sizeof(name_) / sizeof(name_[0]));
		}

		inline int32_t pet_level() const {
			return pet_level_;
		}
		inline void set_pet_level(int32_t value) {
			pet_level_ = value;
		}

		inline int32_t quality() const {
			return quality_;
		}
		inline void set_quality(int32_t value) {
			quality_ = value;
		}

		inline int32_t level() const {
			return level_;
		}
		inline void set_level(int32_t value) {
			level_ = value;
		}

	private:
		int32_t id_;

		char name_[32];

		int32_t pet_level_;

		int32_t quality_;

		int32_t level_;

};

class PlayerAtt {

	public:
		enum DayEventSizeType {
			DAYEVENT_SIZE = 128,
		};
		static bool DayEventSizeType_IsValid(int value) {
			switch(value) {
				case DAYEVENT_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum FixedEventSizeType {
			FIXEDEVENT_SIZE = 128,
		};
		static bool FixedEventSizeType_IsValid(int value) {
			switch(value) {
				case FIXEDEVENT_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum FriendsSizeType {
			FRIENDS_SIZE = 32,
		};
		static bool FriendsSizeType_IsValid(int value) {
			switch(value) {
				case FRIENDS_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum PassGuideSizeType {
			PASSGUIDE_SIZE = 10,
		};
		static bool PassGuideSizeType_IsValid(int value) {
			switch(value) {
				case PASSGUIDE_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum DesignationRecordSizeType {
			DESIGNATIONRECORD_SIZE = 128,
		};
		static bool DesignationRecordSizeType_IsValid(int value) {
			switch(value) {
				case DESIGNATIONRECORD_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum ShowDesignationSizeType {
			SHOWDESIGNATION_SIZE = 10,
		};
		static bool ShowDesignationSizeType_IsValid(int value) {
			switch(value) {
				case SHOWDESIGNATION_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum MailsSizeType {
			MAILS_SIZE = 30,
		};
		static bool MailsSizeType_IsValid(int value) {
			switch(value) {
				case MAILS_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum PetsSizeType {
			PETS_SIZE = 20,
		};
		static bool PetsSizeType_IsValid(int value) {
			switch(value) {
				case PETS_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum FansSizeType {
			FANS_SIZE = 64,
		};
		static bool FansSizeType_IsValid(int value) {
			switch(value) {
				case FANS_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum FirstLoginIPSizeType {
			FIRSTLOGINIP_SIZE = 16,
		};
		static bool FirstLoginIPSizeType_IsValid(int value) {
			switch(value) {
				case FIRSTLOGINIP_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum LastLoginIPSizeType {
			LASTLOGINIP_SIZE = 16,
		};
		static bool LastLoginIPSizeType_IsValid(int value) {
			switch(value) {
				case LASTLOGINIP_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum FactionSizeType {
			FACTION_SIZE = 22,
		};
		static bool FactionSizeType_IsValid(int value) {
			switch(value) {
				case FACTION_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum GodRecordInfoSizeType {
			GODRECORDINFO_SIZE = 10,
		};
		static bool GodRecordInfoSizeType_IsValid(int value) {
			switch(value) {
				case GODRECORDINFO_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum GodRecordArg1SizeType {
			GODRECORDARG1_SIZE = 10,
		};
		static bool GodRecordArg1SizeType_IsValid(int value) {
			switch(value) {
				case GODRECORDARG1_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum GodRankRecordsSizeType {
			GODRANKRECORDS_SIZE = 10,
		};
		static bool GodRankRecordsSizeType_IsValid(int value) {
			switch(value) {
				case GODRANKRECORDS_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum PropertieTypeSizeType {
			PROPERTIETYPE_SIZE = 6,
		};
		static bool PropertieTypeSizeType_IsValid(int value) {
			switch(value) {
				case PROPERTIETYPE_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum HaloLevelSizeType {
			HALOLEVEL_SIZE = 6,
		};
		static bool HaloLevelSizeType_IsValid(int value) {
			switch(value) {
				case HALOLEVEL_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum HaloValueSizeType {
			HALOVALUE_SIZE = 6,
		};
		static bool HaloValueSizeType_IsValid(int value) {
			switch(value) {
				case HALOVALUE_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum SelfcodeSizeType {
			SELFCODE_SIZE = 32,
		};
		static bool SelfcodeSizeType_IsValid(int value) {
			switch(value) {
				case SELFCODE_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum OthercodeSizeType {
			OTHERCODE_SIZE = 32,
		};
		static bool OthercodeSizeType_IsValid(int value) {
			switch(value) {
				case OTHERCODE_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum HaloCountSizeType {
			HALOCOUNT_SIZE = 6,
		};
		static bool HaloCountSizeType_IsValid(int value) {
			switch(value) {
				case HALOCOUNT_SIZE:
					return true;
				default:
					return false;
			}
		}

	public:
		PlayerAtt() {
			prevLogin_ = 0;
			for (int i = 0; i < passGuide_size(); i++)
				passGuide_[i] = 0;
			prevLogout_ = 0;
			for (int i = 0; i < dayEvent_size(); i++)
				dayEvent_[i] = 0;
			for (int i = 0; i < fixedEvent_size(); i++)
				fixedEvent_[i] = 0;
			for (int i = 0; i < showDesignation_size(); i++)
				showDesignation_[i] = -1;
			firstLoginIP_[0] = '\0';
			lastLoginIP_[0] = '\0';
			totalNum_ = 0;
			loginNum_ = 0;
			createTime_ = 0;
			faction_[0] = '\0';
			godDueDate_ = 0;
			godRankIndex_ = 0;
			for (int i = 0; i < godRecordArg1_size(); i++)
				godRecordArg1_[i] = 0;
			for (int i = 0; i < godRankRecords_size(); i++)
				godRankRecords_[i] = false;
			minGodRank_ = -1;
			for (int i = 0; i < propertieType_size(); i++)
				propertieType_[i] = 0;
			for (int i = 0; i < haloLevel_size(); i++)
				haloLevel_[i] = 0;
			for (int i = 0; i < haloValue_size(); i++)
				haloValue_[i] = 0;
			selfcode_[0] = '\0';
			othercode_[0] = '\0';
			for (int i = 0; i < haloCount_size(); i++)
				haloCount_[i] = 0;
		}

	public:
		void ToPB(PB_PlayerAtt *pb_) const {
			pb_->Clear();
			att_.ToPB(pb_->mutable_att());
			missions_.ToPB(pb_->mutable_missions());
			itemPackage_.ToPB(pb_->mutable_itemPackage());
			alt_.ToPB(pb_->mutable_alt());
			pb_->set_prevLogin(prevLogin_);
			for (int i = 0; i < friends_size(); i++)
				friends_[i].ToPB(pb_->add_friends());
			for (int i = 0; i < passGuide_size(); i++)
				pb_->add_passGuide(passGuide_[i]);
			pb_->set_prevLogout(prevLogout_);
			for (int i = 0; i < dayEvent_size(); i++)
				pb_->add_dayEvent(dayEvent_[i]);
			for (int i = 0; i < fixedEvent_size(); i++)
				pb_->add_fixedEvent(fixedEvent_[i]);
			for (int i = 0; i < designationRecord_size(); i++)
				designationRecord_[i].ToPB(pb_->add_designationRecord());
			for (int i = 0; i < showDesignation_size(); i++)
				pb_->add_showDesignation(showDesignation_[i]);
			for (int i = 0; i < mails_size(); i++)
				mails_[i].ToPB(pb_->add_mails());
			for (int i = 0; i < pets_size(); i++)
				pets_[i].ToPB(pb_->add_pets());
			for (int i = 0; i < fans_size(); i++)
				fans_[i].ToPB(pb_->add_fans());
			pb_->set_firstLoginIP(firstLoginIP_);
			pb_->set_lastLoginIP(lastLoginIP_);
			pb_->set_totalNum(totalNum_);
			pb_->set_loginNum(loginNum_);
			pb_->set_createTime(createTime_);
			pb_->set_faction(faction_);
			pb_->set_godDueDate(godDueDate_);
			pb_->set_godRankIndex(godRankIndex_);
			for (int i = 0; i < godRecordInfo_size(); i++)
				godRecordInfo_[i].ToPB(pb_->add_godRecordInfo());
			for (int i = 0; i < godRecordArg1_size(); i++)
				pb_->add_godRecordArg1(godRecordArg1_[i]);
			for (int i = 0; i < godRankRecords_size(); i++)
				pb_->add_godRankRecords(godRankRecords_[i]);
			pb_->set_minGodRank(minGodRank_);
			for (int i = 0; i < propertieType_size(); i++)
				pb_->add_propertieType(propertieType_[i]);
			for (int i = 0; i < haloLevel_size(); i++)
				pb_->add_haloLevel(haloLevel_[i]);
			for (int i = 0; i < haloValue_size(); i++)
				pb_->add_haloValue(haloValue_[i]);
			pb_->set_selfcode(selfcode_);
			pb_->set_othercode(othercode_);
			for (int i = 0; i < haloCount_size(); i++)
				pb_->add_haloCount(haloCount_[i]);
		}
		void FromPB(const PB_PlayerAtt *pb_) {
			att_.FromPB(&pb_->att());
			missions_.FromPB(&pb_->missions());
			itemPackage_.FromPB(&pb_->itemPackage());
			alt_.FromPB(&pb_->alt());
			prevLogin_ = pb_->prevLogin();
			if (friends_size() <= pb_->friends_size()) {
				for (int i = 0; i < friends_size(); i++)
					friends_[i].FromPB(&pb_->friends(i));
			} else {
				for (int i = 0; i < pb_->friends_size(); i++)
					friends_[i].FromPB(&pb_->friends(i));
				for (int i = pb_->friends_size(); i < friends_size(); i++)
					friends_[i] = FriendInfo();
			}
			if (passGuide_size() <= pb_->passGuide_size()) {
				for (int i = 0; i < passGuide_size(); i++)
					passGuide_[i] = pb_->passGuide(i);
			} else {
				for (int i = 0; i < pb_->passGuide_size(); i++)
					passGuide_[i] = pb_->passGuide(i);
				for (int i = pb_->passGuide_size(); i < passGuide_size(); i++)
					passGuide_[i] = 0;
			}
			prevLogout_ = pb_->prevLogout();
			if (dayEvent_size() <= pb_->dayEvent_size()) {
				for (int i = 0; i < dayEvent_size(); i++)
					dayEvent_[i] = pb_->dayEvent(i);
			} else {
				for (int i = 0; i < pb_->dayEvent_size(); i++)
					dayEvent_[i] = pb_->dayEvent(i);
				for (int i = pb_->dayEvent_size(); i < dayEvent_size(); i++)
					dayEvent_[i] = 0;
			}
			if (fixedEvent_size() <= pb_->fixedEvent_size()) {
				for (int i = 0; i < fixedEvent_size(); i++)
					fixedEvent_[i] = pb_->fixedEvent(i);
			} else {
				for (int i = 0; i < pb_->fixedEvent_size(); i++)
					fixedEvent_[i] = pb_->fixedEvent(i);
				for (int i = pb_->fixedEvent_size(); i < fixedEvent_size(); i++)
					fixedEvent_[i] = 0;
			}
			if (designationRecord_size() <= pb_->designationRecord_size()) {
				for (int i = 0; i < designationRecord_size(); i++)
					designationRecord_[i].FromPB(&pb_->designationRecord(i));
			} else {
				for (int i = 0; i < pb_->designationRecord_size(); i++)
					designationRecord_[i].FromPB(&pb_->designationRecord(i));
				for (int i = pb_->designationRecord_size(); i < designationRecord_size(); i++)
					designationRecord_[i] = DesignationRecord();
			}
			if (showDesignation_size() <= pb_->showDesignation_size()) {
				for (int i = 0; i < showDesignation_size(); i++)
					showDesignation_[i] = pb_->showDesignation(i);
			} else {
				for (int i = 0; i < pb_->showDesignation_size(); i++)
					showDesignation_[i] = pb_->showDesignation(i);
				for (int i = pb_->showDesignation_size(); i < showDesignation_size(); i++)
					showDesignation_[i] = -1;
			}
			if (mails_size() <= pb_->mails_size()) {
				for (int i = 0; i < mails_size(); i++)
					mails_[i].FromPB(&pb_->mails(i));
			} else {
				for (int i = 0; i < pb_->mails_size(); i++)
					mails_[i].FromPB(&pb_->mails(i));
				for (int i = pb_->mails_size(); i < mails_size(); i++)
					mails_[i] = MailInfo();
			}
			if (pets_size() <= pb_->pets_size()) {
				for (int i = 0; i < pets_size(); i++)
					pets_[i].FromPB(&pb_->pets(i));
			} else {
				for (int i = 0; i < pb_->pets_size(); i++)
					pets_[i].FromPB(&pb_->pets(i));
				for (int i = pb_->pets_size(); i < pets_size(); i++)
					pets_[i] = PetAsset();
			}
			if (fans_size() <= pb_->fans_size()) {
				for (int i = 0; i < fans_size(); i++)
					fans_[i].FromPB(&pb_->fans(i));
			} else {
				for (int i = 0; i < pb_->fans_size(); i++)
					fans_[i].FromPB(&pb_->fans(i));
				for (int i = pb_->fans_size(); i < fans_size(); i++)
					fans_[i] = FriendInfo();
			}
			strncpy(firstLoginIP_, pb_->firstLoginIP().c_str(), firstLoginIP_size() - 1);
			firstLoginIP_[firstLoginIP_size() - 1] = '\0';
			strncpy(lastLoginIP_, pb_->lastLoginIP().c_str(), lastLoginIP_size() - 1);
			lastLoginIP_[lastLoginIP_size() - 1] = '\0';
			totalNum_ = pb_->totalNum();
			loginNum_ = pb_->loginNum();
			createTime_ = pb_->createTime();
			strncpy(faction_, pb_->faction().c_str(), faction_size() - 1);
			faction_[faction_size() - 1] = '\0';
			godDueDate_ = pb_->godDueDate();
			godRankIndex_ = pb_->godRankIndex();
			if (godRecordInfo_size() <= pb_->godRecordInfo_size()) {
				for (int i = 0; i < godRecordInfo_size(); i++)
					godRecordInfo_[i].FromPB(&pb_->godRecordInfo(i));
			} else {
				for (int i = 0; i < pb_->godRecordInfo_size(); i++)
					godRecordInfo_[i].FromPB(&pb_->godRecordInfo(i));
				for (int i = pb_->godRecordInfo_size(); i < godRecordInfo_size(); i++)
					godRecordInfo_[i] = FriendInfo();
			}
			if (godRecordArg1_size() <= pb_->godRecordArg1_size()) {
				for (int i = 0; i < godRecordArg1_size(); i++)
					godRecordArg1_[i] = pb_->godRecordArg1(i);
			} else {
				for (int i = 0; i < pb_->godRecordArg1_size(); i++)
					godRecordArg1_[i] = pb_->godRecordArg1(i);
				for (int i = pb_->godRecordArg1_size(); i < godRecordArg1_size(); i++)
					godRecordArg1_[i] = 0;
			}
			if (godRankRecords_size() <= pb_->godRankRecords_size()) {
				for (int i = 0; i < godRankRecords_size(); i++)
					godRankRecords_[i] = pb_->godRankRecords(i);
			} else {
				for (int i = 0; i < pb_->godRankRecords_size(); i++)
					godRankRecords_[i] = pb_->godRankRecords(i);
				for (int i = pb_->godRankRecords_size(); i < godRankRecords_size(); i++)
					godRankRecords_[i] = false;
			}
			minGodRank_ = pb_->minGodRank();
			if (propertieType_size() <= pb_->propertieType_size()) {
				for (int i = 0; i < propertieType_size(); i++)
					propertieType_[i] = pb_->propertieType(i);
			} else {
				for (int i = 0; i < pb_->propertieType_size(); i++)
					propertieType_[i] = pb_->propertieType(i);
				for (int i = pb_->propertieType_size(); i < propertieType_size(); i++)
					propertieType_[i] = 0;
			}
			if (haloLevel_size() <= pb_->haloLevel_size()) {
				for (int i = 0; i < haloLevel_size(); i++)
					haloLevel_[i] = pb_->haloLevel(i);
			} else {
				for (int i = 0; i < pb_->haloLevel_size(); i++)
					haloLevel_[i] = pb_->haloLevel(i);
				for (int i = pb_->haloLevel_size(); i < haloLevel_size(); i++)
					haloLevel_[i] = 0;
			}
			if (haloValue_size() <= pb_->haloValue_size()) {
				for (int i = 0; i < haloValue_size(); i++)
					haloValue_[i] = pb_->haloValue(i);
			} else {
				for (int i = 0; i < pb_->haloValue_size(); i++)
					haloValue_[i] = pb_->haloValue(i);
				for (int i = pb_->haloValue_size(); i < haloValue_size(); i++)
					haloValue_[i] = 0;
			}
			strncpy(selfcode_, pb_->selfcode().c_str(), selfcode_size() - 1);
			selfcode_[selfcode_size() - 1] = '\0';
			strncpy(othercode_, pb_->othercode().c_str(), othercode_size() - 1);
			othercode_[othercode_size() - 1] = '\0';
			if (haloCount_size() <= pb_->haloCount_size()) {
				for (int i = 0; i < haloCount_size(); i++)
					haloCount_[i] = pb_->haloCount(i);
			} else {
				for (int i = 0; i < pb_->haloCount_size(); i++)
					haloCount_[i] = pb_->haloCount(i);
				for (int i = pb_->haloCount_size(); i < haloCount_size(); i++)
					haloCount_[i] = 0;
			}
		}

	public:
		inline const RoleAtt & att() const {
			return att_;
		}
		inline RoleAtt * mutable_att() {
			return &att_;
		}

		inline const MissionAllRecord & missions() const {
			return missions_;
		}
		inline MissionAllRecord * mutable_missions() {
			return &missions_;
		}

		inline const ItemPackage & itemPackage() const {
			return itemPackage_;
		}
		inline ItemPackage * mutable_itemPackage() {
			return &itemPackage_;
		}

		inline const ALT & alt() const {
			return alt_;
		}
		inline ALT * mutable_alt() {
			return &alt_;
		}

		inline int64_t prevLogin() const {
			return prevLogin_;
		}
		inline void set_prevLogin(int64_t value) {
			prevLogin_ = value;
		}

		inline const FriendInfo & friends(int index) const {
			if (index < 0 || index >= friends_size()) {
				assert(0);
			}
			return friends_[index];
		}
		inline FriendInfo * mutable_friends(int index) {
			if (index < 0 || index >= friends_size()) {
				assert(0);
			}
			return &friends_[index];
		}
		inline int friends_size() const {
			return (int)(sizeof(friends_) / sizeof(friends_[0]));
		}

		inline int32_t passGuide(int index) const {
			if (index < 0 || index >= passGuide_size()) {
				assert(0);
			}
			return passGuide_[index];
		}
		inline void set_passGuide(int index, int32_t value) {
			if (index < 0 || index >= passGuide_size()) {
				assert(0);
			}
			passGuide_[index] = value;
		}
		inline int passGuide_size() const {
			return (int)(sizeof(passGuide_) / sizeof(passGuide_[0]));
		}

		inline int64_t prevLogout() const {
			return prevLogout_;
		}
		inline void set_prevLogout(int64_t value) {
			prevLogout_ = value;
		}

		inline int32_t dayEvent(int index) const {
			if (index < 0 || index >= dayEvent_size()) {
				assert(0);
			}
			return dayEvent_[index];
		}
		inline void set_dayEvent(int index, int32_t value) {
			if (index < 0 || index >= dayEvent_size()) {
				assert(0);
			}
			dayEvent_[index] = value;
		}
		inline int dayEvent_size() const {
			return (int)(sizeof(dayEvent_) / sizeof(dayEvent_[0]));
		}

		inline int32_t fixedEvent(int index) const {
			if (index < 0 || index >= fixedEvent_size()) {
				assert(0);
			}
			return fixedEvent_[index];
		}
		inline void set_fixedEvent(int index, int32_t value) {
			if (index < 0 || index >= fixedEvent_size()) {
				assert(0);
			}
			fixedEvent_[index] = value;
		}
		inline int fixedEvent_size() const {
			return (int)(sizeof(fixedEvent_) / sizeof(fixedEvent_[0]));
		}

		inline const DesignationRecord & designationRecord(int index) const {
			if (index < 0 || index >= designationRecord_size()) {
				assert(0);
			}
			return designationRecord_[index];
		}
		inline DesignationRecord * mutable_designationRecord(int index) {
			if (index < 0 || index >= designationRecord_size()) {
				assert(0);
			}
			return &designationRecord_[index];
		}
		inline int designationRecord_size() const {
			return (int)(sizeof(designationRecord_) / sizeof(designationRecord_[0]));
		}

		inline int32_t showDesignation(int index) const {
			if (index < 0 || index >= showDesignation_size()) {
				assert(0);
			}
			return showDesignation_[index];
		}
		inline void set_showDesignation(int index, int32_t value) {
			if (index < 0 || index >= showDesignation_size()) {
				assert(0);
			}
			showDesignation_[index] = value;
		}
		inline int showDesignation_size() const {
			return (int)(sizeof(showDesignation_) / sizeof(showDesignation_[0]));
		}

		inline const MailInfo & mails(int index) const {
			if (index < 0 || index >= mails_size()) {
				assert(0);
			}
			return mails_[index];
		}
		inline MailInfo * mutable_mails(int index) {
			if (index < 0 || index >= mails_size()) {
				assert(0);
			}
			return &mails_[index];
		}
		inline int mails_size() const {
			return (int)(sizeof(mails_) / sizeof(mails_[0]));
		}

		inline const PetAsset & pets(int index) const {
			if (index < 0 || index >= pets_size()) {
				assert(0);
			}
			return pets_[index];
		}
		inline PetAsset * mutable_pets(int index) {
			if (index < 0 || index >= pets_size()) {
				assert(0);
			}
			return &pets_[index];
		}
		inline int pets_size() const {
			return (int)(sizeof(pets_) / sizeof(pets_[0]));
		}

		inline const FriendInfo & fans(int index) const {
			if (index < 0 || index >= fans_size()) {
				assert(0);
			}
			return fans_[index];
		}
		inline FriendInfo * mutable_fans(int index) {
			if (index < 0 || index >= fans_size()) {
				assert(0);
			}
			return &fans_[index];
		}
		inline int fans_size() const {
			return (int)(sizeof(fans_) / sizeof(fans_[0]));
		}

		inline const char * firstLoginIP() const {
			return firstLoginIP_;
		}
		inline void set_firstLoginIP(const char * value) {
			strncpy(firstLoginIP_, value, firstLoginIP_size() - 1);
			firstLoginIP_[firstLoginIP_size() - 1] = '\0';
		}
		inline int firstLoginIP_size() const {
			return (int)(sizeof(firstLoginIP_) / sizeof(firstLoginIP_[0]));
		}

		inline const char * lastLoginIP() const {
			return lastLoginIP_;
		}
		inline void set_lastLoginIP(const char * value) {
			strncpy(lastLoginIP_, value, lastLoginIP_size() - 1);
			lastLoginIP_[lastLoginIP_size() - 1] = '\0';
		}
		inline int lastLoginIP_size() const {
			return (int)(sizeof(lastLoginIP_) / sizeof(lastLoginIP_[0]));
		}

		inline int32_t totalNum() const {
			return totalNum_;
		}
		inline void set_totalNum(int32_t value) {
			totalNum_ = value;
		}

		inline int32_t loginNum() const {
			return loginNum_;
		}
		inline void set_loginNum(int32_t value) {
			loginNum_ = value;
		}

		inline int32_t createTime() const {
			return createTime_;
		}
		inline void set_createTime(int32_t value) {
			createTime_ = value;
		}

		inline const char * faction() const {
			return faction_;
		}
		inline void set_faction(const char * value) {
			strncpy(faction_, value, faction_size() - 1);
			faction_[faction_size() - 1] = '\0';
		}
		inline int faction_size() const {
			return (int)(sizeof(faction_) / sizeof(faction_[0]));
		}

		inline int32_t godDueDate() const {
			return godDueDate_;
		}
		inline void set_godDueDate(int32_t value) {
			godDueDate_ = value;
		}

		inline int32_t godRankIndex() const {
			return godRankIndex_;
		}
		inline void set_godRankIndex(int32_t value) {
			godRankIndex_ = value;
		}

		inline const FriendInfo & godRecordInfo(int index) const {
			if (index < 0 || index >= godRecordInfo_size()) {
				assert(0);
			}
			return godRecordInfo_[index];
		}
		inline FriendInfo * mutable_godRecordInfo(int index) {
			if (index < 0 || index >= godRecordInfo_size()) {
				assert(0);
			}
			return &godRecordInfo_[index];
		}
		inline int godRecordInfo_size() const {
			return (int)(sizeof(godRecordInfo_) / sizeof(godRecordInfo_[0]));
		}

		inline int64_t godRecordArg1(int index) const {
			if (index < 0 || index >= godRecordArg1_size()) {
				assert(0);
			}
			return godRecordArg1_[index];
		}
		inline void set_godRecordArg1(int index, int64_t value) {
			if (index < 0 || index >= godRecordArg1_size()) {
				assert(0);
			}
			godRecordArg1_[index] = value;
		}
		inline int godRecordArg1_size() const {
			return (int)(sizeof(godRecordArg1_) / sizeof(godRecordArg1_[0]));
		}

		inline bool godRankRecords(int index) const {
			if (index < 0 || index >= godRankRecords_size()) {
				assert(0);
			}
			return godRankRecords_[index];
		}
		inline void set_godRankRecords(int index, bool value) {
			if (index < 0 || index >= godRankRecords_size()) {
				assert(0);
			}
			godRankRecords_[index] = value;
		}
		inline int godRankRecords_size() const {
			return (int)(sizeof(godRankRecords_) / sizeof(godRankRecords_[0]));
		}

		inline int32_t minGodRank() const {
			return minGodRank_;
		}
		inline void set_minGodRank(int32_t value) {
			minGodRank_ = value;
		}

		inline int32_t propertieType(int index) const {
			if (index < 0 || index >= propertieType_size()) {
				assert(0);
			}
			return propertieType_[index];
		}
		inline void set_propertieType(int index, int32_t value) {
			if (index < 0 || index >= propertieType_size()) {
				assert(0);
			}
			propertieType_[index] = value;
		}
		inline int propertieType_size() const {
			return (int)(sizeof(propertieType_) / sizeof(propertieType_[0]));
		}

		inline int32_t haloLevel(int index) const {
			if (index < 0 || index >= haloLevel_size()) {
				assert(0);
			}
			return haloLevel_[index];
		}
		inline void set_haloLevel(int index, int32_t value) {
			if (index < 0 || index >= haloLevel_size()) {
				assert(0);
			}
			haloLevel_[index] = value;
		}
		inline int haloLevel_size() const {
			return (int)(sizeof(haloLevel_) / sizeof(haloLevel_[0]));
		}

		inline int32_t haloValue(int index) const {
			if (index < 0 || index >= haloValue_size()) {
				assert(0);
			}
			return haloValue_[index];
		}
		inline void set_haloValue(int index, int32_t value) {
			if (index < 0 || index >= haloValue_size()) {
				assert(0);
			}
			haloValue_[index] = value;
		}
		inline int haloValue_size() const {
			return (int)(sizeof(haloValue_) / sizeof(haloValue_[0]));
		}

		inline const char * selfcode() const {
			return selfcode_;
		}
		inline void set_selfcode(const char * value) {
			strncpy(selfcode_, value, selfcode_size() - 1);
			selfcode_[selfcode_size() - 1] = '\0';
		}
		inline int selfcode_size() const {
			return (int)(sizeof(selfcode_) / sizeof(selfcode_[0]));
		}

		inline const char * othercode() const {
			return othercode_;
		}
		inline void set_othercode(const char * value) {
			strncpy(othercode_, value, othercode_size() - 1);
			othercode_[othercode_size() - 1] = '\0';
		}
		inline int othercode_size() const {
			return (int)(sizeof(othercode_) / sizeof(othercode_[0]));
		}

		inline int32_t haloCount(int index) const {
			if (index < 0 || index >= haloCount_size()) {
				assert(0);
			}
			return haloCount_[index];
		}
		inline void set_haloCount(int index, int32_t value) {
			if (index < 0 || index >= haloCount_size()) {
				assert(0);
			}
			haloCount_[index] = value;
		}
		inline int haloCount_size() const {
			return (int)(sizeof(haloCount_) / sizeof(haloCount_[0]));
		}

	private:
		RoleAtt att_;

		MissionAllRecord missions_;

		ItemPackage itemPackage_;

		ALT alt_;

		int64_t prevLogin_;

		FriendInfo friends_[32];

		int32_t passGuide_[10];

		int64_t prevLogout_;

		int32_t dayEvent_[128];

		int32_t fixedEvent_[128];

		DesignationRecord designationRecord_[128];

		int32_t showDesignation_[10];

		MailInfo mails_[30];

		PetAsset pets_[20];

		FriendInfo fans_[64];

		char firstLoginIP_[16];

		char lastLoginIP_[16];

		int32_t totalNum_;

		int32_t loginNum_;

		int32_t createTime_;

		char faction_[22];

		int32_t godDueDate_;

		int32_t godRankIndex_;

		FriendInfo godRecordInfo_[10];

		int64_t godRecordArg1_[10];

		bool godRankRecords_[10];

		int32_t minGodRank_;

		int32_t propertieType_[6];

		int32_t haloLevel_[6];

		int32_t haloValue_[6];

		char selfcode_[32];

		char othercode_[32];

		int32_t haloCount_[6];

};

#endif
