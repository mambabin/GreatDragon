#ifndef _ITEMBASEINFO_HPP_
#define _ITEMBASEINFO_HPP_

#include "ItemBaseInfo.pb.h"
#include <sys/types.h>
#include <cassert>
#include <string>
#include <cstring>

class ItemInfo {

	public:
		enum Type {
			NONE = 0,
			GOODS = 1,
			EQUIPMENT = 2,
			MONEY = 3,
			SKILL = 4,
			FASHION = 5,
			DESIGNATION = 6,
			RMB = 7,
			SOULJADE = 8,
			SOUL = 9,
			SOULSTONE = 10,
			EXP = 11,
			HONOR = 12,
			DURABILITY = 13,
			SUBRMB = 14,
			WING = 15,
			RUNE = 16,
			GODSCORE = 17,
			TRANSFORM = 18,
			HP = 19,
			PET = 20,
			VIP = 21,
			RIDES = 22,
			GODSHIP = 23,
			RIDESFOOD = 24,
		};
		static bool Type_IsValid(int value) {
			switch(value) {
				case NONE:
				case GOODS:
				case EQUIPMENT:
				case MONEY:
				case SKILL:
				case FASHION:
				case DESIGNATION:
				case RMB:
				case SOULJADE:
				case SOUL:
				case SOULSTONE:
				case EXP:
				case HONOR:
				case DURABILITY:
				case SUBRMB:
				case WING:
				case RUNE:
				case GODSCORE:
				case TRANSFORM:
				case HP:
				case PET:
				case VIP:
				case RIDES:
				case GODSHIP:
				case RIDESFOOD:
					return true;
				default:
					return false;
			}
		}

	public:
		ItemInfo() {
			type_ = (Type)0;
			id_ = 0;
			count_ = 0;
		}

	public:
		void ToPB(PB_ItemInfo *pb_) const {
			pb_->Clear();
			pb_->set_type((PB_ItemInfo::Type)type_);
			pb_->set_id(id_);
			pb_->set_count(count_);
		}
		void FromPB(const PB_ItemInfo *pb_) {
			type_ = (Type)pb_->type();
			id_ = pb_->id();
			count_ = pb_->count();
		}

	public:
		inline Type type() const {
			return type_;
		}
		inline void set_type(Type value) {
			type_ = value;
		}

		inline int64_t id() const {
			return id_;
		}
		inline void set_id(int64_t value) {
			id_ = value;
		}

		inline int32_t count() const {
			return count_;
		}
		inline void set_count(int32_t value) {
			count_ = value;
		}

	private:
		Type type_;

		int64_t id_;

		int32_t count_;

};

#endif
