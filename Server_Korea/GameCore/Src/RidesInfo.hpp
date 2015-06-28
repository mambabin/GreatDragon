#ifndef _RIDESINFO_HPP_
#define _RIDESINFO_HPP_

#include "RidesInfo.pb.h"
#include <sys/types.h>
#include <cassert>
#include <string>
#include <cstring>

class RidesAsset {

	public:
		enum AttSizeType {
			ATT_SIZE = 6,
		};
		static bool AttSizeType_IsValid(int value) {
			switch(value) {
				case ATT_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum LockAttSizeType {
			LOCKATT_SIZE = 6,
		};
		static bool LockAttSizeType_IsValid(int value) {
			switch(value) {
				case LOCKATT_SIZE:
					return true;
				default:
					return false;
			}
		}

	public:
		RidesAsset() {
			model_ = -1;
			star_ = 0;
			level_ = 0;
			exp_ = 0;
			potential_ = 0;
			for (int i = 0; i < att_size(); i++)
				att_[i] = 0;
			for (int i = 0; i < lockAtt_size(); i++)
				lockAtt_[i] = false;
		}

	public:
		void ToPB(PB_RidesAsset *pb_) const {
			pb_->Clear();
			pb_->set_model(model_);
			pb_->set_star(star_);
			pb_->set_level(level_);
			pb_->set_exp(exp_);
			pb_->set_potential(potential_);
			for (int i = 0; i < att_size(); i++)
				pb_->add_att(att_[i]);
			for (int i = 0; i < lockAtt_size(); i++)
				pb_->add_lockAtt(lockAtt_[i]);
		}
		void FromPB(const PB_RidesAsset *pb_) {
			model_ = pb_->model();
			star_ = pb_->star();
			level_ = pb_->level();
			exp_ = pb_->exp();
			potential_ = pb_->potential();
			if (att_size() <= pb_->att_size()) {
				for (int i = 0; i < att_size(); i++)
					att_[i] = pb_->att(i);
			} else {
				for (int i = 0; i < pb_->att_size(); i++)
					att_[i] = pb_->att(i);
				for (int i = pb_->att_size(); i < att_size(); i++)
					att_[i] = 0;
			}
			if (lockAtt_size() <= pb_->lockAtt_size()) {
				for (int i = 0; i < lockAtt_size(); i++)
					lockAtt_[i] = pb_->lockAtt(i);
			} else {
				for (int i = 0; i < pb_->lockAtt_size(); i++)
					lockAtt_[i] = pb_->lockAtt(i);
				for (int i = pb_->lockAtt_size(); i < lockAtt_size(); i++)
					lockAtt_[i] = false;
			}
		}

	public:
		inline int32_t model() const {
			return model_;
		}
		inline void set_model(int32_t value) {
			model_ = value;
		}

		inline int32_t star() const {
			return star_;
		}
		inline void set_star(int32_t value) {
			star_ = value;
		}

		inline int32_t level() const {
			return level_;
		}
		inline void set_level(int32_t value) {
			level_ = value;
		}

		inline int64_t exp() const {
			return exp_;
		}
		inline void set_exp(int64_t value) {
			exp_ = value;
		}

		inline int32_t potential() const {
			return potential_;
		}
		inline void set_potential(int32_t value) {
			potential_ = value;
		}

		inline int32_t att(int index) const {
			if (index < 0 || index >= att_size()) {
				assert(0);
			}
			return att_[index];
		}
		inline void set_att(int index, int32_t value) {
			if (index < 0 || index >= att_size()) {
				assert(0);
			}
			att_[index] = value;
		}
		inline int att_size() const {
			return (int)(sizeof(att_) / sizeof(att_[0]));
		}

		inline bool lockAtt(int index) const {
			if (index < 0 || index >= lockAtt_size()) {
				assert(0);
			}
			return lockAtt_[index];
		}
		inline void set_lockAtt(int index, bool value) {
			if (index < 0 || index >= lockAtt_size()) {
				assert(0);
			}
			lockAtt_[index] = value;
		}
		inline int lockAtt_size() const {
			return (int)(sizeof(lockAtt_) / sizeof(lockAtt_[0]));
		}

	private:
		int32_t model_;

		int32_t star_;

		int32_t level_;

		int64_t exp_;

		int32_t potential_;

		int32_t att_[6];

		bool lockAtt_[6];

};

#endif
