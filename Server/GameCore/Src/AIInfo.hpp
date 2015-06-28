#ifndef _AIINFO_HPP_
#define _AIINFO_HPP_

#include "AIInfo.pb.h"
#include "Math.hpp"
#include "Math.pb.h"
#include <sys/types.h>
#include <cassert>
#include <string>
#include <cstring>

class AIAtt {

	public:
		enum Status {
			BORN = 0,
			IDLE = 1,
			BUSY = 2,
			FLEE = 3,
			RESET = 4,
		};
		static bool Status_IsValid(int value) {
			switch(value) {
				case BORN:
				case IDLE:
				case BUSY:
				case FLEE:
				case RESET:
					return true;
				default:
					return false;
			}
		}

		enum MoveType {
			DONTMOVE = 0,
			FREE = 1,
			PATH = 2,
		};
		static bool MoveType_IsValid(int value) {
			switch(value) {
				case DONTMOVE:
				case FREE:
				case PATH:
					return true;
				default:
					return false;
			}
		}

		enum SearchType {
			DONTSEARCH = 0,
			MINDIST = 1,
		};
		static bool SearchType_IsValid(int value) {
			switch(value) {
				case DONTSEARCH:
				case MINDIST:
					return true;
				default:
					return false;
			}
		}

		enum FleeType {
			DONTFLEE = 0,
			HP = 1,
			SEARCH = 2,
		};
		static bool FleeType_IsValid(int value) {
			switch(value) {
				case DONTFLEE:
				case HP:
				case SEARCH:
					return true;
				default:
					return false;
			}
		}

		enum AIType {
			NORMAL = 0,
			BOX = 1,
			TIME = 2,
			TRIGGER = 3,
			FOLLOW = 4,
		};
		static bool AIType_IsValid(int value) {
			switch(value) {
				case NORMAL:
				case BOX:
				case TIME:
				case TRIGGER:
				case FOLLOW:
					return true;
				default:
					return false;
			}
		}

		enum ArgSizeType {
			ARG_SIZE = 2,
		};
		static bool ArgSizeType_IsValid(int value) {
			switch(value) {
				case ARG_SIZE:
					return true;
				default:
					return false;
			}
		}

	public:
		AIAtt() {
			status_ = (Status)0;
			moveRadius_ = 0;
			moveType_ = (MoveType)0;
			followRadius_ = 0;
			searchRadius_ = 0;
			searchType_ = (SearchType)0;
			fleeType_ = (FleeType)0;
			fleeHP_ = 0.0f;
			callPercent_ = 0.0f;
			followDelta_ = 0;
			canAttackBack_ = false;
			moveInterval_ = 0;
			searchInterval_ = 0;
			aiType_ = (AIType)0;
			for (int i = 0; i < arg_size(); i++)
				arg_[i] = 0;
		}

	public:
		void ToPB(PB_AIAtt *pb_) const {
			pb_->Clear();
			pb_->set_status((PB_AIAtt::Status)status_);
			birthCoord_.ToPB(pb_->mutable_birthCoord());
			pb_->set_moveRadius(moveRadius_);
			pb_->set_moveType((PB_AIAtt::MoveType)moveType_);
			pb_->set_followRadius(followRadius_);
			pb_->set_searchRadius(searchRadius_);
			pb_->set_searchType((PB_AIAtt::SearchType)searchType_);
			pb_->set_fleeType((PB_AIAtt::FleeType)fleeType_);
			pb_->set_fleeHP(fleeHP_);
			pb_->set_callPercent(callPercent_);
			pb_->set_followDelta(followDelta_);
			pb_->set_canAttackBack(canAttackBack_);
			pb_->set_moveInterval(moveInterval_);
			pb_->set_searchInterval(searchInterval_);
			pb_->set_aiType((PB_AIAtt::AIType)aiType_);
			for (int i = 0; i < arg_size(); i++)
				pb_->add_arg(arg_[i]);
		}
		void FromPB(const PB_AIAtt *pb_) {
			status_ = (Status)pb_->status();
			birthCoord_.FromPB(&pb_->birthCoord());
			moveRadius_ = pb_->moveRadius();
			moveType_ = (MoveType)pb_->moveType();
			followRadius_ = pb_->followRadius();
			searchRadius_ = pb_->searchRadius();
			searchType_ = (SearchType)pb_->searchType();
			fleeType_ = (FleeType)pb_->fleeType();
			fleeHP_ = pb_->fleeHP();
			callPercent_ = pb_->callPercent();
			followDelta_ = pb_->followDelta();
			canAttackBack_ = pb_->canAttackBack();
			moveInterval_ = pb_->moveInterval();
			searchInterval_ = pb_->searchInterval();
			aiType_ = (AIType)pb_->aiType();
			if (arg_size() <= pb_->arg_size()) {
				for (int i = 0; i < arg_size(); i++)
					arg_[i] = pb_->arg(i);
			} else {
				for (int i = 0; i < pb_->arg_size(); i++)
					arg_[i] = pb_->arg(i);
				for (int i = pb_->arg_size(); i < arg_size(); i++)
					arg_[i] = 0;
			}
		}

	public:
		inline Status status() const {
			return status_;
		}
		inline void set_status(Status value) {
			status_ = value;
		}

		inline const Vector2i & birthCoord() const {
			return birthCoord_;
		}
		inline Vector2i * mutable_birthCoord() {
			return &birthCoord_;
		}

		inline int32_t moveRadius() const {
			return moveRadius_;
		}
		inline void set_moveRadius(int32_t value) {
			moveRadius_ = value;
		}

		inline MoveType moveType() const {
			return moveType_;
		}
		inline void set_moveType(MoveType value) {
			moveType_ = value;
		}

		inline int32_t followRadius() const {
			return followRadius_;
		}
		inline void set_followRadius(int32_t value) {
			followRadius_ = value;
		}

		inline int32_t searchRadius() const {
			return searchRadius_;
		}
		inline void set_searchRadius(int32_t value) {
			searchRadius_ = value;
		}

		inline SearchType searchType() const {
			return searchType_;
		}
		inline void set_searchType(SearchType value) {
			searchType_ = value;
		}

		inline FleeType fleeType() const {
			return fleeType_;
		}
		inline void set_fleeType(FleeType value) {
			fleeType_ = value;
		}

		inline float fleeHP() const {
			return fleeHP_;
		}
		inline void set_fleeHP(float value) {
			fleeHP_ = value;
		}

		inline float callPercent() const {
			return callPercent_;
		}
		inline void set_callPercent(float value) {
			callPercent_ = value;
		}

		inline int32_t followDelta() const {
			return followDelta_;
		}
		inline void set_followDelta(int32_t value) {
			followDelta_ = value;
		}

		inline bool canAttackBack() const {
			return canAttackBack_;
		}
		inline void set_canAttackBack(bool value) {
			canAttackBack_ = value;
		}

		inline int32_t moveInterval() const {
			return moveInterval_;
		}
		inline void set_moveInterval(int32_t value) {
			moveInterval_ = value;
		}

		inline int32_t searchInterval() const {
			return searchInterval_;
		}
		inline void set_searchInterval(int32_t value) {
			searchInterval_ = value;
		}

		inline AIType aiType() const {
			return aiType_;
		}
		inline void set_aiType(AIType value) {
			aiType_ = value;
		}

		inline int32_t arg(int index) const {
			if (index < 0 || index >= arg_size()) {
				assert(0);
			}
			return arg_[index];
		}
		inline void set_arg(int index, int32_t value) {
			if (index < 0 || index >= arg_size()) {
				assert(0);
			}
			arg_[index] = value;
		}
		inline int arg_size() const {
			return (int)(sizeof(arg_) / sizeof(arg_[0]));
		}

	private:
		Status status_;

		Vector2i birthCoord_;

		int32_t moveRadius_;

		MoveType moveType_;

		int32_t followRadius_;

		int32_t searchRadius_;

		SearchType searchType_;

		FleeType fleeType_;

		float fleeHP_;

		float callPercent_;

		int32_t followDelta_;

		bool canAttackBack_;

		int32_t moveInterval_;

		int32_t searchInterval_;

		AIType aiType_;

		int32_t arg_[2];

};

#endif
