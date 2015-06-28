#ifndef _GODSHIP_HPP_
#define _GODSHIP_HPP_

#include "GodShip.pb.h"
#include <sys/types.h>
#include <cassert>
#include <string>
#include <cstring>

class GodShipAsset {

	public:
		GodShipAsset() {
			id_ = -1;
			exp_ = 0;
			quality_ = 0;
			level_ = 0;
		}

	public:
		void ToPB(PB_GodShipAsset *pb_) const {
			pb_->Clear();
			pb_->set_id(id_);
			pb_->set_exp(exp_);
			pb_->set_quality(quality_);
			pb_->set_level(level_);
		}
		void FromPB(const PB_GodShipAsset *pb_) {
			id_ = pb_->id();
			exp_ = pb_->exp();
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

		inline int32_t exp() const {
			return exp_;
		}
		inline void set_exp(int32_t value) {
			exp_ = value;
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

		int32_t exp_;

		int32_t quality_;

		int32_t level_;

};

#endif
