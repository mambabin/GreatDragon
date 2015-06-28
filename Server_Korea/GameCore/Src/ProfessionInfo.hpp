#ifndef _PROFESSIONINFO_HPP_
#define _PROFESSIONINFO_HPP_

#include "ProfessionInfo.pb.h"
#include "EquipmentInfo.hpp"
#include "EquipmentInfo.pb.h"
#include "MovementInfo.hpp"
#include "MovementInfo.pb.h"
#include "FightInfo.hpp"
#include "FightInfo.pb.h"
#include "AIInfo.hpp"
#include "AIInfo.pb.h"
#include "ItemInfo.hpp"
#include "ItemInfo.pb.h"
#include "MissionInfo.hpp"
#include "MissionInfo.pb.h"
#include <sys/types.h>
#include <cassert>
#include <string>
#include <cstring>

class ProfessionInfo {

	public:
		enum Type {
			NPC = 0,
			KNIGHT = 1,
			RANGER = 2,
			MAGICIAN = 3,
		};
		static bool Type_IsValid(int value) {
			switch(value) {
				case NPC:
				case KNIGHT:
				case RANGER:
				case MAGICIAN:
					return true;
				default:
					return false;
			}
		}

	public:
		ProfessionInfo() {
			type_ = (Type)0;
			male_ = false;
			height_ = 0.0f;
		}

	public:
		void ToPB(PB_ProfessionInfo *pb_) const {
			pb_->Clear();
			pb_->set_type((PB_ProfessionInfo::Type)type_);
			pb_->set_male(male_);
			pb_->set_height(height_);
			movementAtt_.ToPB(pb_->mutable_movementAtt());
			equipmentAtt_.ToPB(pb_->mutable_equipmentAtt());
			fightAtt_.ToPB(pb_->mutable_fightAtt());
			aiAtt_.ToPB(pb_->mutable_aiAtt());
			itemPackage_.ToPB(pb_->mutable_itemPackage());
			alt_.ToPB(pb_->mutable_alt());
			missions_.ToPB(pb_->mutable_missions());
		}
		void FromPB(const PB_ProfessionInfo *pb_) {
			type_ = (Type)pb_->type();
			male_ = pb_->male();
			height_ = pb_->height();
			movementAtt_.FromPB(&pb_->movementAtt());
			equipmentAtt_.FromPB(&pb_->equipmentAtt());
			fightAtt_.FromPB(&pb_->fightAtt());
			aiAtt_.FromPB(&pb_->aiAtt());
			itemPackage_.FromPB(&pb_->itemPackage());
			alt_.FromPB(&pb_->alt());
			missions_.FromPB(&pb_->missions());
		}

	public:
		inline Type type() const {
			return type_;
		}
		inline void set_type(Type value) {
			type_ = value;
		}

		inline bool male() const {
			return male_;
		}
		inline void set_male(bool value) {
			male_ = value;
		}

		inline float height() const {
			return height_;
		}
		inline void set_height(float value) {
			height_ = value;
		}

		inline const MovementAtt & movementAtt() const {
			return movementAtt_;
		}
		inline MovementAtt * mutable_movementAtt() {
			return &movementAtt_;
		}

		inline const EquipmentAtt & equipmentAtt() const {
			return equipmentAtt_;
		}
		inline EquipmentAtt * mutable_equipmentAtt() {
			return &equipmentAtt_;
		}

		inline const FightAtt & fightAtt() const {
			return fightAtt_;
		}
		inline FightAtt * mutable_fightAtt() {
			return &fightAtt_;
		}

		inline const AIAtt & aiAtt() const {
			return aiAtt_;
		}
		inline AIAtt * mutable_aiAtt() {
			return &aiAtt_;
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

		inline const MissionAllRecord & missions() const {
			return missions_;
		}
		inline MissionAllRecord * mutable_missions() {
			return &missions_;
		}

	private:
		Type type_;

		bool male_;

		float height_;

		MovementAtt movementAtt_;

		EquipmentAtt equipmentAtt_;

		FightAtt fightAtt_;

		AIAtt aiAtt_;

		ItemPackage itemPackage_;

		ALT alt_;

		MissionAllRecord missions_;

};

#endif
