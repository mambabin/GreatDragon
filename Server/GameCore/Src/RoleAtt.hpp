#ifndef _ROLEATT_HPP_
#define _ROLEATT_HPP_

#include "RoleAtt.pb.h"
#include "BaseInfo.hpp"
#include "BaseInfo.pb.h"
#include "MovementInfo.hpp"
#include "MovementInfo.pb.h"
#include "FightInfo.hpp"
#include "FightInfo.pb.h"
#include "AIInfo.hpp"
#include "AIInfo.pb.h"
#include "EquipmentInfo.hpp"
#include "EquipmentInfo.pb.h"
#include <sys/types.h>
#include <cassert>
#include <string>
#include <cstring>

class RoleAtt {

	public:
		RoleAtt() {
		}

	public:
		void ToPB(PB_RoleAtt *pb_) const {
			pb_->Clear();
			baseAtt_.ToPB(pb_->mutable_baseAtt());
			movementAtt_.ToPB(pb_->mutable_movementAtt());
			equipmentAtt_.ToPB(pb_->mutable_equipmentAtt());
			fightAtt_.ToPB(pb_->mutable_fightAtt());
			aiAtt_.ToPB(pb_->mutable_aiAtt());
		}
		void FromPB(const PB_RoleAtt *pb_) {
			baseAtt_.FromPB(&pb_->baseAtt());
			movementAtt_.FromPB(&pb_->movementAtt());
			equipmentAtt_.FromPB(&pb_->equipmentAtt());
			fightAtt_.FromPB(&pb_->fightAtt());
			aiAtt_.FromPB(&pb_->aiAtt());
		}

	public:
		inline const BaseAtt & baseAtt() const {
			return baseAtt_;
		}
		inline BaseAtt * mutable_baseAtt() {
			return &baseAtt_;
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

	private:
		BaseAtt baseAtt_;

		MovementAtt movementAtt_;

		EquipmentAtt equipmentAtt_;

		FightAtt fightAtt_;

		AIAtt aiAtt_;

};

#endif
