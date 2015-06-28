#ifndef _BASEINFO_HPP_
#define _BASEINFO_HPP_

#include "BaseInfo.pb.h"
#include "ProfessionInfo.hpp"
#include "ProfessionInfo.pb.h"
#include <sys/types.h>
#include <cassert>
#include <string>
#include <cstring>

class BaseAtt {

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
		BaseAtt() {
			name_[0] = '\0';
			professionType_ = (ProfessionInfo::Type)0;
			male_ = false;
			roleID_ = 0;
			height_ = 0.0f;
			passStory_ = false;
		}

	public:
		void ToPB(PB_BaseAtt *pb_) const {
			pb_->Clear();
			pb_->set_name(name_);
			pb_->set_professionType((PB_ProfessionInfo::Type)professionType_);
			pb_->set_male(male_);
			pb_->set_roleID(roleID_);
			pb_->set_height(height_);
			pb_->set_passStory(passStory_);
		}
		void FromPB(const PB_BaseAtt *pb_) {
			strncpy(name_, pb_->name().c_str(), name_size() - 1);
			name_[name_size() - 1] = '\0';
			professionType_ = (ProfessionInfo::Type)pb_->professionType();
			male_ = pb_->male();
			roleID_ = pb_->roleID();
			height_ = pb_->height();
			passStory_ = pb_->passStory();
		}

	public:
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

		inline bool male() const {
			return male_;
		}
		inline void set_male(bool value) {
			male_ = value;
		}

		inline int64_t roleID() const {
			return roleID_;
		}
		inline void set_roleID(int64_t value) {
			roleID_ = value;
		}

		inline float height() const {
			return height_;
		}
		inline void set_height(float value) {
			height_ = value;
		}

		inline bool passStory() const {
			return passStory_;
		}
		inline void set_passStory(bool value) {
			passStory_ = value;
		}

	private:
		char name_[32];

		ProfessionInfo::Type professionType_;

		bool male_;

		int64_t roleID_;

		float height_;

		bool passStory_;

};

#endif
