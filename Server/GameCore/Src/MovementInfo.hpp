#ifndef _MOVEMENTINFO_HPP_
#define _MOVEMENTINFO_HPP_

#include "MovementInfo.pb.h"
#include "Math.hpp"
#include "Math.pb.h"
#include <sys/types.h>
#include <cassert>
#include <string>
#include <cstring>

class MovementAtt {

	public:
		enum Status {
			IDLE = 0,
			MOVE = 1,
			FOLLOW = 2,
			TALK_TO = 3,
		};
		static bool Status_IsValid(int value) {
			switch(value) {
				case IDLE:
				case MOVE:
				case FOLLOW:
				case TALK_TO:
					return true;
				default:
					return false;
			}
		}

	public:
		MovementAtt() {
			status_ = (Status)0;
			mapID_ = 0;
			prevNormalMap_ = 0;
			moveSpeed_ = 0;
			radius_ = 0;
		}

	public:
		void ToPB(PB_MovementAtt *pb_) const {
			pb_->Clear();
			pb_->set_status((PB_MovementAtt::Status)status_);
			pb_->set_mapID(mapID_);
			pb_->set_prevNormalMap(prevNormalMap_);
			prevCoord_.ToPB(pb_->mutable_prevCoord());
			logicCoord_.ToPB(pb_->mutable_logicCoord());
			position_.ToPB(pb_->mutable_position());
			pb_->set_moveSpeed(moveSpeed_);
			pb_->set_radius(radius_);
		}
		void FromPB(const PB_MovementAtt *pb_) {
			status_ = (Status)pb_->status();
			mapID_ = pb_->mapID();
			prevNormalMap_ = pb_->prevNormalMap();
			prevCoord_.FromPB(&pb_->prevCoord());
			logicCoord_.FromPB(&pb_->logicCoord());
			position_.FromPB(&pb_->position());
			moveSpeed_ = pb_->moveSpeed();
			radius_ = pb_->radius();
		}

	public:
		inline Status status() const {
			return status_;
		}
		inline void set_status(Status value) {
			status_ = value;
		}

		inline int32_t mapID() const {
			return mapID_;
		}
		inline void set_mapID(int32_t value) {
			mapID_ = value;
		}

		inline int32_t prevNormalMap() const {
			return prevNormalMap_;
		}
		inline void set_prevNormalMap(int32_t value) {
			prevNormalMap_ = value;
		}

		inline const Vector2i & prevCoord() const {
			return prevCoord_;
		}
		inline Vector2i * mutable_prevCoord() {
			return &prevCoord_;
		}

		inline const Vector2i & logicCoord() const {
			return logicCoord_;
		}
		inline Vector2i * mutable_logicCoord() {
			return &logicCoord_;
		}

		inline const Vector3f & position() const {
			return position_;
		}
		inline Vector3f * mutable_position() {
			return &position_;
		}

		inline int32_t moveSpeed() const {
			return moveSpeed_;
		}
		inline void set_moveSpeed(int32_t value) {
			moveSpeed_ = value;
		}

		inline int32_t radius() const {
			return radius_;
		}
		inline void set_radius(int32_t value) {
			radius_ = value;
		}

	private:
		Status status_;

		int32_t mapID_;

		int32_t prevNormalMap_;

		Vector2i prevCoord_;

		Vector2i logicCoord_;

		Vector3f position_;

		int32_t moveSpeed_;

		int32_t radius_;

};

#endif
