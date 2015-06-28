#ifndef _FASHION_HPP_
#define _FASHION_HPP_

#include "Fashion.pb.h"
#include <sys/types.h>
#include <cassert>
#include <string>
#include <cstring>

class FashionAsset {

	public:
		enum RunesSizeType {
			RUNES_SIZE = 5,
		};
		static bool RunesSizeType_IsValid(int value) {
			switch(value) {
				case RUNES_SIZE:
					return true;
				default:
					return false;
			}
		}

	public:
		FashionAsset() {
			v_ = -1;
			for (int i = 0; i < runes_size(); i++)
				runes_[i] = -2;
		}

	public:
		void ToPB(PB_FashionAsset *pb_) const {
			pb_->Clear();
			pb_->set_v(v_);
			for (int i = 0; i < runes_size(); i++)
				pb_->add_runes(runes_[i]);
		}
		void FromPB(const PB_FashionAsset *pb_) {
			v_ = pb_->v();
			if (runes_size() <= pb_->runes_size()) {
				for (int i = 0; i < runes_size(); i++)
					runes_[i] = pb_->runes(i);
			} else {
				for (int i = 0; i < pb_->runes_size(); i++)
					runes_[i] = pb_->runes(i);
				for (int i = pb_->runes_size(); i < runes_size(); i++)
					runes_[i] = -2;
			}
		}

	public:
		inline int32_t v() const {
			return v_;
		}
		inline void set_v(int32_t value) {
			v_ = value;
		}

		inline int32_t runes(int index) const {
			if (index < 0 || index >= runes_size()) {
				assert(0);
			}
			return runes_[index];
		}
		inline void set_runes(int index, int32_t value) {
			if (index < 0 || index >= runes_size()) {
				assert(0);
			}
			runes_[index] = value;
		}
		inline int runes_size() const {
			return (int)(sizeof(runes_) / sizeof(runes_[0]));
		}

	private:
		int32_t v_;

		int32_t runes_[5];

};

#endif
