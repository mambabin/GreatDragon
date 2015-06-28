#ifndef _EQUIPMENTINFO_HPP_
#define _EQUIPMENTINFO_HPP_

#include "EquipmentInfo.pb.h"
#include "FightInfo.hpp"
#include "FightInfo.pb.h"
#include "ItemBaseInfo.hpp"
#include "ItemBaseInfo.pb.h"
#include "GodShip.hpp"
#include "GodShip.pb.h"
#include <sys/types.h>
#include <cassert>
#include <string>
#include <cstring>

class EquipAsset {

	public:
		enum GemModelSizeType {
			GEMMODEL_SIZE = 5,
		};
		static bool GemModelSizeType_IsValid(int value) {
			switch(value) {
				case GEMMODEL_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum GemTypeSizeType {
			GEMTYPE_SIZE = 5,
		};
		static bool GemTypeSizeType_IsValid(int value) {
			switch(value) {
				case GEMTYPE_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum EnhanceDeltaSizeType {
			ENHANCEDELTA_SIZE = 4,
		};
		static bool EnhanceDeltaSizeType_IsValid(int value) {
			switch(value) {
				case ENHANCEDELTA_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum RandomTypeSizeType {
			RANDOMTYPE_SIZE = 3,
		};
		static bool RandomTypeSizeType_IsValid(int value) {
			switch(value) {
				case RANDOMTYPE_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum RandomDeltaSizeType {
			RANDOMDELTA_SIZE = 3,
		};
		static bool RandomDeltaSizeType_IsValid(int value) {
			switch(value) {
				case RANDOMDELTA_SIZE:
					return true;
				default:
					return false;
			}
		}

	public:
		EquipAsset() {
			mode_ = -1;
			strongLevel_ = 0;
			for (int i = 0; i < gemModel_size(); i++)
				gemModel_[i] = 0;
			for (int i = 0; i < gemType_size(); i++)
				gemType_[i] = 0;
			for (int i = 0; i < enhanceDelta_size(); i++)
				enhanceDelta_[i] = 0;
			for (int i = 0; i < randomType_size(); i++)
				randomType_[i] = 0;
			for (int i = 0; i < randomDelta_size(); i++)
				randomDelta_[i] = 0;
			effectId_ = 0;
		}

	public:
		void ToPB(PB_EquipAsset *pb_) const {
			pb_->Clear();
			pb_->set_mode(mode_);
			pb_->set_strongLevel(strongLevel_);
			for (int i = 0; i < gemModel_size(); i++)
				pb_->add_gemModel(gemModel_[i]);
			for (int i = 0; i < gemType_size(); i++)
				pb_->add_gemType(gemType_[i]);
			for (int i = 0; i < enhanceDelta_size(); i++)
				pb_->add_enhanceDelta(enhanceDelta_[i]);
			for (int i = 0; i < randomType_size(); i++)
				pb_->add_randomType(randomType_[i]);
			for (int i = 0; i < randomDelta_size(); i++)
				pb_->add_randomDelta(randomDelta_[i]);
			pb_->set_effectId(effectId_);
		}
		void FromPB(const PB_EquipAsset *pb_) {
			mode_ = pb_->mode();
			strongLevel_ = pb_->strongLevel();
			if (gemModel_size() <= pb_->gemModel_size()) {
				for (int i = 0; i < gemModel_size(); i++)
					gemModel_[i] = pb_->gemModel(i);
			} else {
				for (int i = 0; i < pb_->gemModel_size(); i++)
					gemModel_[i] = pb_->gemModel(i);
				for (int i = pb_->gemModel_size(); i < gemModel_size(); i++)
					gemModel_[i] = 0;
			}
			if (gemType_size() <= pb_->gemType_size()) {
				for (int i = 0; i < gemType_size(); i++)
					gemType_[i] = pb_->gemType(i);
			} else {
				for (int i = 0; i < pb_->gemType_size(); i++)
					gemType_[i] = pb_->gemType(i);
				for (int i = pb_->gemType_size(); i < gemType_size(); i++)
					gemType_[i] = 0;
			}
			if (enhanceDelta_size() <= pb_->enhanceDelta_size()) {
				for (int i = 0; i < enhanceDelta_size(); i++)
					enhanceDelta_[i] = pb_->enhanceDelta(i);
			} else {
				for (int i = 0; i < pb_->enhanceDelta_size(); i++)
					enhanceDelta_[i] = pb_->enhanceDelta(i);
				for (int i = pb_->enhanceDelta_size(); i < enhanceDelta_size(); i++)
					enhanceDelta_[i] = 0;
			}
			if (randomType_size() <= pb_->randomType_size()) {
				for (int i = 0; i < randomType_size(); i++)
					randomType_[i] = pb_->randomType(i);
			} else {
				for (int i = 0; i < pb_->randomType_size(); i++)
					randomType_[i] = pb_->randomType(i);
				for (int i = pb_->randomType_size(); i < randomType_size(); i++)
					randomType_[i] = 0;
			}
			if (randomDelta_size() <= pb_->randomDelta_size()) {
				for (int i = 0; i < randomDelta_size(); i++)
					randomDelta_[i] = pb_->randomDelta(i);
			} else {
				for (int i = 0; i < pb_->randomDelta_size(); i++)
					randomDelta_[i] = pb_->randomDelta(i);
				for (int i = pb_->randomDelta_size(); i < randomDelta_size(); i++)
					randomDelta_[i] = 0;
			}
			effectId_ = pb_->effectId();
		}

	public:
		inline int32_t mode() const {
			return mode_;
		}
		inline void set_mode(int32_t value) {
			mode_ = value;
		}

		inline int32_t strongLevel() const {
			return strongLevel_;
		}
		inline void set_strongLevel(int32_t value) {
			strongLevel_ = value;
		}

		inline int32_t gemModel(int index) const {
			if (index < 0 || index >= gemModel_size()) {
				assert(0);
			}
			return gemModel_[index];
		}
		inline void set_gemModel(int index, int32_t value) {
			if (index < 0 || index >= gemModel_size()) {
				assert(0);
			}
			gemModel_[index] = value;
		}
		inline int gemModel_size() const {
			return (int)(sizeof(gemModel_) / sizeof(gemModel_[0]));
		}

		inline int32_t gemType(int index) const {
			if (index < 0 || index >= gemType_size()) {
				assert(0);
			}
			return gemType_[index];
		}
		inline void set_gemType(int index, int32_t value) {
			if (index < 0 || index >= gemType_size()) {
				assert(0);
			}
			gemType_[index] = value;
		}
		inline int gemType_size() const {
			return (int)(sizeof(gemType_) / sizeof(gemType_[0]));
		}

		inline int32_t enhanceDelta(int index) const {
			if (index < 0 || index >= enhanceDelta_size()) {
				assert(0);
			}
			return enhanceDelta_[index];
		}
		inline void set_enhanceDelta(int index, int32_t value) {
			if (index < 0 || index >= enhanceDelta_size()) {
				assert(0);
			}
			enhanceDelta_[index] = value;
		}
		inline int enhanceDelta_size() const {
			return (int)(sizeof(enhanceDelta_) / sizeof(enhanceDelta_[0]));
		}

		inline int32_t randomType(int index) const {
			if (index < 0 || index >= randomType_size()) {
				assert(0);
			}
			return randomType_[index];
		}
		inline void set_randomType(int index, int32_t value) {
			if (index < 0 || index >= randomType_size()) {
				assert(0);
			}
			randomType_[index] = value;
		}
		inline int randomType_size() const {
			return (int)(sizeof(randomType_) / sizeof(randomType_[0]));
		}

		inline int32_t randomDelta(int index) const {
			if (index < 0 || index >= randomDelta_size()) {
				assert(0);
			}
			return randomDelta_[index];
		}
		inline void set_randomDelta(int index, int32_t value) {
			if (index < 0 || index >= randomDelta_size()) {
				assert(0);
			}
			randomDelta_[index] = value;
		}
		inline int randomDelta_size() const {
			return (int)(sizeof(randomDelta_) / sizeof(randomDelta_[0]));
		}

		inline int32_t effectId() const {
			return effectId_;
		}
		inline void set_effectId(int32_t value) {
			effectId_ = value;
		}

	private:
		int32_t mode_;

		int32_t strongLevel_;

		int32_t gemModel_[5];

		int32_t gemType_[5];

		int32_t enhanceDelta_[4];

		int32_t randomType_[3];

		int32_t randomDelta_[3];

		int32_t effectId_;

};

class EquipmentAtt {

	public:
		enum EquipmentsSizeType {
			EQUIPMENTS_SIZE = 12,
		};
		static bool EquipmentsSizeType_IsValid(int value) {
			switch(value) {
				case EQUIPMENTS_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum GodShipsSizeType {
			GODSHIPS_SIZE = 6,
		};
		static bool GodShipsSizeType_IsValid(int value) {
			switch(value) {
				case GODSHIPS_SIZE:
					return true;
				default:
					return false;
			}
		}

	public:
		EquipmentAtt() {
			for (int i = 0; i < equipments_size(); i++)
				equipments_[i] = -1;
			wing_ = -1;
			fashion_ = -1;
			baseWing_ = false;
			rides_ = -1;
			for (int i = 0; i < godShips_size(); i++)
				godShips_[i] = -1;
		}

	public:
		void ToPB(PB_EquipmentAtt *pb_) const {
			pb_->Clear();
			for (int i = 0; i < equipments_size(); i++)
				pb_->add_equipments(equipments_[i]);
			pb_->set_wing(wing_);
			pb_->set_fashion(fashion_);
			pb_->set_baseWing(baseWing_);
			pb_->set_rides(rides_);
			for (int i = 0; i < godShips_size(); i++)
				pb_->add_godShips(godShips_[i]);
		}
		void FromPB(const PB_EquipmentAtt *pb_) {
			if (equipments_size() <= pb_->equipments_size()) {
				for (int i = 0; i < equipments_size(); i++)
					equipments_[i] = pb_->equipments(i);
			} else {
				for (int i = 0; i < pb_->equipments_size(); i++)
					equipments_[i] = pb_->equipments(i);
				for (int i = pb_->equipments_size(); i < equipments_size(); i++)
					equipments_[i] = -1;
			}
			wing_ = pb_->wing();
			fashion_ = pb_->fashion();
			baseWing_ = pb_->baseWing();
			rides_ = pb_->rides();
			if (godShips_size() <= pb_->godShips_size()) {
				for (int i = 0; i < godShips_size(); i++)
					godShips_[i] = pb_->godShips(i);
			} else {
				for (int i = 0; i < pb_->godShips_size(); i++)
					godShips_[i] = pb_->godShips(i);
				for (int i = pb_->godShips_size(); i < godShips_size(); i++)
					godShips_[i] = -1;
			}
		}

	public:
		inline int64_t equipments(int index) const {
			if (index < 0 || index >= equipments_size()) {
				assert(0);
			}
			return equipments_[index];
		}
		inline void set_equipments(int index, int64_t value) {
			if (index < 0 || index >= equipments_size()) {
				assert(0);
			}
			equipments_[index] = value;
		}
		inline int equipments_size() const {
			return (int)(sizeof(equipments_) / sizeof(equipments_[0]));
		}

		inline int32_t wing() const {
			return wing_;
		}
		inline void set_wing(int32_t value) {
			wing_ = value;
		}

		inline int32_t fashion() const {
			return fashion_;
		}
		inline void set_fashion(int32_t value) {
			fashion_ = value;
		}

		inline bool baseWing() const {
			return baseWing_;
		}
		inline void set_baseWing(bool value) {
			baseWing_ = value;
		}

		inline int32_t rides() const {
			return rides_;
		}
		inline void set_rides(int32_t value) {
			rides_ = value;
		}

		inline int32_t godShips(int index) const {
			if (index < 0 || index >= godShips_size()) {
				assert(0);
			}
			return godShips_[index];
		}
		inline void set_godShips(int index, int32_t value) {
			if (index < 0 || index >= godShips_size()) {
				assert(0);
			}
			godShips_[index] = value;
		}
		inline int godShips_size() const {
			return (int)(sizeof(godShips_) / sizeof(godShips_[0]));
		}

	private:
		int64_t equipments_[12];

		int32_t wing_;

		int32_t fashion_;

		bool baseWing_;

		int32_t rides_;

		int32_t godShips_[6];

};

#endif
