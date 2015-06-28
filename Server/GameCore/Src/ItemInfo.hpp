#ifndef _ITEMINFO_HPP_
#define _ITEMINFO_HPP_

#include "ItemInfo.pb.h"
#include "Math.hpp"
#include "Math.pb.h"
#include "Fashion.hpp"
#include "Fashion.pb.h"
#include "EquipmentInfo.hpp"
#include "EquipmentInfo.pb.h"
#include "ItemBaseInfo.hpp"
#include "ItemBaseInfo.pb.h"
#include "GodShip.hpp"
#include "GodShip.pb.h"
#include "RidesInfo.hpp"
#include "RidesInfo.pb.h"
#include <sys/types.h>
#include <cassert>
#include <string>
#include <cstring>

class TransformAsset {

	public:
		TransformAsset() {
			id_ = -1;
			quality_ = 0;
			level_ = 0;
		}

	public:
		void ToPB(PB_TransformAsset *pb_) const {
			pb_->Clear();
			pb_->set_id(id_);
			pb_->set_quality(quality_);
			pb_->set_level(level_);
		}
		void FromPB(const PB_TransformAsset *pb_) {
			id_ = pb_->id();
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

		int32_t quality_;

		int32_t level_;

};

class ItemPackage {

	public:
		enum Begin {
			EQUIPMENT = 0,
			GOODS = 105,
			GEM = 210,
		};
		static bool Begin_IsValid(int value) {
			switch(value) {
				case EQUIPMENT:
				case GOODS:
				case GEM:
					return true;
				default:
					return false;
			}
		}

		enum Length {
			LENGTH = 105,
		};
		static bool Length_IsValid(int value) {
			switch(value) {
				case LENGTH:
					return true;
				default:
					return false;
			}
		}

		enum ItemsSizeType {
			ITEMS_SIZE = 315,
		};
		static bool ItemsSizeType_IsValid(int value) {
			switch(value) {
				case ITEMS_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum WingsSizeType {
			WINGS_SIZE = 32,
		};
		static bool WingsSizeType_IsValid(int value) {
			switch(value) {
				case WINGS_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum FashionsSizeType {
			FASHIONS_SIZE = 32,
		};
		static bool FashionsSizeType_IsValid(int value) {
			switch(value) {
				case FASHIONS_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum EquipsSizeType {
			EQUIPS_SIZE = 150,
		};
		static bool EquipsSizeType_IsValid(int value) {
			switch(value) {
				case EQUIPS_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum TransformsSizeType {
			TRANSFORMS_SIZE = 12,
		};
		static bool TransformsSizeType_IsValid(int value) {
			switch(value) {
				case TRANSFORMS_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum GodShipsSizeType {
			GODSHIPS_SIZE = 510,
		};
		static bool GodShipsSizeType_IsValid(int value) {
			switch(value) {
				case GODSHIPS_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum GodShipsPackageSizeType {
			GODSHIPSPACKAGE_SIZE = 500,
		};
		static bool GodShipsPackageSizeType_IsValid(int value) {
			switch(value) {
				case GODSHIPSPACKAGE_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum RidesSizeType {
			RIDES_SIZE = 100,
		};
		static bool RidesSizeType_IsValid(int value) {
			switch(value) {
				case RIDES_SIZE:
					return true;
				default:
					return false;
			}
		}

	public:
		ItemPackage() {
			money_ = 0;
			rmb_ = 0;
			validNumEquipment_ = 0;
			validNumGoods_ = 0;
			validNumGem_ = 0;
			soul_ = 0;
			smallSoul_ = 0;
			mediumSoul_ = 0;
			bigSoul_ = 0;
			perfectSoul_ = 0;
			honor_ = 0;
			durability_ = 0;
			totalRMB_ = 0;
			soulStone_ = 0;
			pkScore_ = 0;
			vip_ = 0;
			godScore_ = 0;
			subRMB_ = 0;
			for (int i = 0; i < wings_size(); i++)
				wings_[i] = -1;
			lovePoint_ = 0;
			pkScoreActive_ = 0;
			rmbActive_ = 0;
			totalCost_ = 0;
			blessActive_ = 0;
			activeCost_ = 0;
			activeCostEndTime_ = 0;
			for (int i = 0; i < godShipsPackage_size(); i++)
				godShipsPackage_[i] = -1;
			ridesFood_ = 0;
		}

	public:
		void ToPB(PB_ItemPackage *pb_) const {
			pb_->Clear();
			for (int i = 0; i < items_size(); i++)
				items_[i].ToPB(pb_->add_items());
			pb_->set_money(money_);
			pb_->set_rmb(rmb_);
			pb_->set_validNumEquipment(validNumEquipment_);
			pb_->set_validNumGoods(validNumGoods_);
			pb_->set_validNumGem(validNumGem_);
			pb_->set_soul(soul_);
			pb_->set_smallSoul(smallSoul_);
			pb_->set_mediumSoul(mediumSoul_);
			pb_->set_bigSoul(bigSoul_);
			pb_->set_perfectSoul(perfectSoul_);
			pb_->set_honor(honor_);
			pb_->set_durability(durability_);
			pb_->set_totalRMB(totalRMB_);
			pb_->set_soulStone(soulStone_);
			pb_->set_pkScore(pkScore_);
			pb_->set_vip(vip_);
			pb_->set_godScore(godScore_);
			pb_->set_subRMB(subRMB_);
			for (int i = 0; i < wings_size(); i++)
				pb_->add_wings(wings_[i]);
			for (int i = 0; i < fashions_size(); i++)
				fashions_[i].ToPB(pb_->add_fashions());
			pb_->set_lovePoint(lovePoint_);
			for (int i = 0; i < equips_size(); i++)
				equips_[i].ToPB(pb_->add_equips());
			pb_->set_pkScoreActive(pkScoreActive_);
			pb_->set_rmbActive(rmbActive_);
			pb_->set_totalCost(totalCost_);
			pb_->set_blessActive(blessActive_);
			for (int i = 0; i < transforms_size(); i++)
				transforms_[i].ToPB(pb_->add_transforms());
			pb_->set_activeCost(activeCost_);
			pb_->set_activeCostEndTime(activeCostEndTime_);
			for (int i = 0; i < rides_size(); i++)
				rides_[i].ToPB(pb_->add_rides());
			for (int i = 0; i < godShips_size(); i++)
				godShips_[i].ToPB(pb_->add_godShips());
			for (int i = 0; i < godShipsPackage_size(); i++)
				pb_->add_godShipsPackage(godShipsPackage_[i]);
			pb_->set_ridesFood(ridesFood_);
		}
		void FromPB(const PB_ItemPackage *pb_) {
			if (items_size() <= pb_->items_size()) {
				for (int i = 0; i < items_size(); i++)
					items_[i].FromPB(&pb_->items(i));
			} else {
				for (int i = 0; i < pb_->items_size(); i++)
					items_[i].FromPB(&pb_->items(i));
				for (int i = pb_->items_size(); i < items_size(); i++)
					items_[i] = ItemInfo();
			}
			money_ = pb_->money();
			rmb_ = pb_->rmb();
			validNumEquipment_ = pb_->validNumEquipment();
			validNumGoods_ = pb_->validNumGoods();
			validNumGem_ = pb_->validNumGem();
			soul_ = pb_->soul();
			smallSoul_ = pb_->smallSoul();
			mediumSoul_ = pb_->mediumSoul();
			bigSoul_ = pb_->bigSoul();
			perfectSoul_ = pb_->perfectSoul();
			honor_ = pb_->honor();
			durability_ = pb_->durability();
			totalRMB_ = pb_->totalRMB();
			soulStone_ = pb_->soulStone();
			pkScore_ = pb_->pkScore();
			vip_ = pb_->vip();
			godScore_ = pb_->godScore();
			subRMB_ = pb_->subRMB();
			if (wings_size() <= pb_->wings_size()) {
				for (int i = 0; i < wings_size(); i++)
					wings_[i] = pb_->wings(i);
			} else {
				for (int i = 0; i < pb_->wings_size(); i++)
					wings_[i] = pb_->wings(i);
				for (int i = pb_->wings_size(); i < wings_size(); i++)
					wings_[i] = -1;
			}
			if (fashions_size() <= pb_->fashions_size()) {
				for (int i = 0; i < fashions_size(); i++)
					fashions_[i].FromPB(&pb_->fashions(i));
			} else {
				for (int i = 0; i < pb_->fashions_size(); i++)
					fashions_[i].FromPB(&pb_->fashions(i));
				for (int i = pb_->fashions_size(); i < fashions_size(); i++)
					fashions_[i] = FashionAsset();
			}
			lovePoint_ = pb_->lovePoint();
			if (equips_size() <= pb_->equips_size()) {
				for (int i = 0; i < equips_size(); i++)
					equips_[i].FromPB(&pb_->equips(i));
			} else {
				for (int i = 0; i < pb_->equips_size(); i++)
					equips_[i].FromPB(&pb_->equips(i));
				for (int i = pb_->equips_size(); i < equips_size(); i++)
					equips_[i] = EquipAsset();
			}
			pkScoreActive_ = pb_->pkScoreActive();
			rmbActive_ = pb_->rmbActive();
			totalCost_ = pb_->totalCost();
			blessActive_ = pb_->blessActive();
			if (transforms_size() <= pb_->transforms_size()) {
				for (int i = 0; i < transforms_size(); i++)
					transforms_[i].FromPB(&pb_->transforms(i));
			} else {
				for (int i = 0; i < pb_->transforms_size(); i++)
					transforms_[i].FromPB(&pb_->transforms(i));
				for (int i = pb_->transforms_size(); i < transforms_size(); i++)
					transforms_[i] = TransformAsset();
			}
			activeCost_ = pb_->activeCost();
			activeCostEndTime_ = pb_->activeCostEndTime();
			if (rides_size() <= pb_->rides_size()) {
				for (int i = 0; i < rides_size(); i++)
					rides_[i].FromPB(&pb_->rides(i));
			} else {
				for (int i = 0; i < pb_->rides_size(); i++)
					rides_[i].FromPB(&pb_->rides(i));
				for (int i = pb_->rides_size(); i < rides_size(); i++)
					rides_[i] = RidesAsset();
			}
			if (godShips_size() <= pb_->godShips_size()) {
				for (int i = 0; i < godShips_size(); i++)
					godShips_[i].FromPB(&pb_->godShips(i));
			} else {
				for (int i = 0; i < pb_->godShips_size(); i++)
					godShips_[i].FromPB(&pb_->godShips(i));
				for (int i = pb_->godShips_size(); i < godShips_size(); i++)
					godShips_[i] = GodShipAsset();
			}
			if (godShipsPackage_size() <= pb_->godShipsPackage_size()) {
				for (int i = 0; i < godShipsPackage_size(); i++)
					godShipsPackage_[i] = pb_->godShipsPackage(i);
			} else {
				for (int i = 0; i < pb_->godShipsPackage_size(); i++)
					godShipsPackage_[i] = pb_->godShipsPackage(i);
				for (int i = pb_->godShipsPackage_size(); i < godShipsPackage_size(); i++)
					godShipsPackage_[i] = -1;
			}
			ridesFood_ = pb_->ridesFood();
		}

	public:
		inline const ItemInfo & items(int index) const {
			if (index < 0 || index >= items_size()) {
				assert(0);
			}
			return items_[index];
		}
		inline ItemInfo * mutable_items(int index) {
			if (index < 0 || index >= items_size()) {
				assert(0);
			}
			return &items_[index];
		}
		inline int items_size() const {
			return (int)(sizeof(items_) / sizeof(items_[0]));
		}

		inline int64_t money() const {
			return money_;
		}
		inline void set_money(int64_t value) {
			money_ = value;
		}

		inline int64_t rmb() const {
			return rmb_;
		}
		inline void set_rmb(int64_t value) {
			rmb_ = value;
		}

		inline int32_t validNumEquipment() const {
			return validNumEquipment_;
		}
		inline void set_validNumEquipment(int32_t value) {
			validNumEquipment_ = value;
		}

		inline int32_t validNumGoods() const {
			return validNumGoods_;
		}
		inline void set_validNumGoods(int32_t value) {
			validNumGoods_ = value;
		}

		inline int32_t validNumGem() const {
			return validNumGem_;
		}
		inline void set_validNumGem(int32_t value) {
			validNumGem_ = value;
		}

		inline int64_t soul() const {
			return soul_;
		}
		inline void set_soul(int64_t value) {
			soul_ = value;
		}

		inline int64_t smallSoul() const {
			return smallSoul_;
		}
		inline void set_smallSoul(int64_t value) {
			smallSoul_ = value;
		}

		inline int64_t mediumSoul() const {
			return mediumSoul_;
		}
		inline void set_mediumSoul(int64_t value) {
			mediumSoul_ = value;
		}

		inline int64_t bigSoul() const {
			return bigSoul_;
		}
		inline void set_bigSoul(int64_t value) {
			bigSoul_ = value;
		}

		inline int64_t perfectSoul() const {
			return perfectSoul_;
		}
		inline void set_perfectSoul(int64_t value) {
			perfectSoul_ = value;
		}

		inline int32_t honor() const {
			return honor_;
		}
		inline void set_honor(int32_t value) {
			honor_ = value;
		}

		inline int32_t durability() const {
			return durability_;
		}
		inline void set_durability(int32_t value) {
			durability_ = value;
		}

		inline int64_t totalRMB() const {
			return totalRMB_;
		}
		inline void set_totalRMB(int64_t value) {
			totalRMB_ = value;
		}

		inline int64_t soulStone() const {
			return soulStone_;
		}
		inline void set_soulStone(int64_t value) {
			soulStone_ = value;
		}

		inline int64_t pkScore() const {
			return pkScore_;
		}
		inline void set_pkScore(int64_t value) {
			pkScore_ = value;
		}

		inline int32_t vip() const {
			return vip_;
		}
		inline void set_vip(int32_t value) {
			vip_ = value;
		}

		inline int32_t godScore() const {
			return godScore_;
		}
		inline void set_godScore(int32_t value) {
			godScore_ = value;
		}

		inline int64_t subRMB() const {
			return subRMB_;
		}
		inline void set_subRMB(int64_t value) {
			subRMB_ = value;
		}

		inline int32_t wings(int index) const {
			if (index < 0 || index >= wings_size()) {
				assert(0);
			}
			return wings_[index];
		}
		inline void set_wings(int index, int32_t value) {
			if (index < 0 || index >= wings_size()) {
				assert(0);
			}
			wings_[index] = value;
		}
		inline int wings_size() const {
			return (int)(sizeof(wings_) / sizeof(wings_[0]));
		}

		inline const FashionAsset & fashions(int index) const {
			if (index < 0 || index >= fashions_size()) {
				assert(0);
			}
			return fashions_[index];
		}
		inline FashionAsset * mutable_fashions(int index) {
			if (index < 0 || index >= fashions_size()) {
				assert(0);
			}
			return &fashions_[index];
		}
		inline int fashions_size() const {
			return (int)(sizeof(fashions_) / sizeof(fashions_[0]));
		}

		inline int32_t lovePoint() const {
			return lovePoint_;
		}
		inline void set_lovePoint(int32_t value) {
			lovePoint_ = value;
		}

		inline const EquipAsset & equips(int index) const {
			if (index < 0 || index >= equips_size()) {
				assert(0);
			}
			return equips_[index];
		}
		inline EquipAsset * mutable_equips(int index) {
			if (index < 0 || index >= equips_size()) {
				assert(0);
			}
			return &equips_[index];
		}
		inline int equips_size() const {
			return (int)(sizeof(equips_) / sizeof(equips_[0]));
		}

		inline int64_t pkScoreActive() const {
			return pkScoreActive_;
		}
		inline void set_pkScoreActive(int64_t value) {
			pkScoreActive_ = value;
		}

		inline int32_t rmbActive() const {
			return rmbActive_;
		}
		inline void set_rmbActive(int32_t value) {
			rmbActive_ = value;
		}

		inline int64_t totalCost() const {
			return totalCost_;
		}
		inline void set_totalCost(int64_t value) {
			totalCost_ = value;
		}

		inline int32_t blessActive() const {
			return blessActive_;
		}
		inline void set_blessActive(int32_t value) {
			blessActive_ = value;
		}

		inline const TransformAsset & transforms(int index) const {
			if (index < 0 || index >= transforms_size()) {
				assert(0);
			}
			return transforms_[index];
		}
		inline TransformAsset * mutable_transforms(int index) {
			if (index < 0 || index >= transforms_size()) {
				assert(0);
			}
			return &transforms_[index];
		}
		inline int transforms_size() const {
			return (int)(sizeof(transforms_) / sizeof(transforms_[0]));
		}

		inline int32_t activeCost() const {
			return activeCost_;
		}
		inline void set_activeCost(int32_t value) {
			activeCost_ = value;
		}

		inline int32_t activeCostEndTime() const {
			return activeCostEndTime_;
		}
		inline void set_activeCostEndTime(int32_t value) {
			activeCostEndTime_ = value;
		}

		inline const RidesAsset & rides(int index) const {
			if (index < 0 || index >= rides_size()) {
				assert(0);
			}
			return rides_[index];
		}
		inline RidesAsset * mutable_rides(int index) {
			if (index < 0 || index >= rides_size()) {
				assert(0);
			}
			return &rides_[index];
		}
		inline int rides_size() const {
			return (int)(sizeof(rides_) / sizeof(rides_[0]));
		}

		inline const GodShipAsset & godShips(int index) const {
			if (index < 0 || index >= godShips_size()) {
				assert(0);
			}
			return godShips_[index];
		}
		inline GodShipAsset * mutable_godShips(int index) {
			if (index < 0 || index >= godShips_size()) {
				assert(0);
			}
			return &godShips_[index];
		}
		inline int godShips_size() const {
			return (int)(sizeof(godShips_) / sizeof(godShips_[0]));
		}

		inline int32_t godShipsPackage(int index) const {
			if (index < 0 || index >= godShipsPackage_size()) {
				assert(0);
			}
			return godShipsPackage_[index];
		}
		inline void set_godShipsPackage(int index, int32_t value) {
			if (index < 0 || index >= godShipsPackage_size()) {
				assert(0);
			}
			godShipsPackage_[index] = value;
		}
		inline int godShipsPackage_size() const {
			return (int)(sizeof(godShipsPackage_) / sizeof(godShipsPackage_[0]));
		}

		inline int64_t ridesFood() const {
			return ridesFood_;
		}
		inline void set_ridesFood(int64_t value) {
			ridesFood_ = value;
		}

	private:
		ItemInfo items_[315];

		int64_t money_;

		int64_t rmb_;

		int32_t validNumEquipment_;

		int32_t validNumGoods_;

		int32_t validNumGem_;

		int64_t soul_;

		int64_t smallSoul_;

		int64_t mediumSoul_;

		int64_t bigSoul_;

		int64_t perfectSoul_;

		int32_t honor_;

		int32_t durability_;

		int64_t totalRMB_;

		int64_t soulStone_;

		int64_t pkScore_;

		int32_t vip_;

		int32_t godScore_;

		int64_t subRMB_;

		int32_t wings_[32];

		FashionAsset fashions_[32];

		int32_t lovePoint_;

		EquipAsset equips_[150];

		int64_t pkScoreActive_;

		int32_t rmbActive_;

		int64_t totalCost_;

		int32_t blessActive_;

		TransformAsset transforms_[12];

		int32_t activeCost_;

		int32_t activeCostEndTime_;

		RidesAsset rides_[100];

		GodShipAsset godShips_[510];

		int32_t godShipsPackage_[500];

		int64_t ridesFood_;

};

class ALT {

	public:
		enum Pos {
			BEGIN = 0,
			END = 6,
		};
		static bool Pos_IsValid(int value) {
			switch(value) {
				case BEGIN:
				case END:
					return true;
				default:
					return false;
			}
		}

		enum AltSizeType {
			ALT_SIZE = 6,
		};
		static bool AltSizeType_IsValid(int value) {
			switch(value) {
				case ALT_SIZE:
					return true;
				default:
					return false;
			}
		}

	public:
		ALT() {
		}

	public:
		void ToPB(PB_ALT *pb_) const {
			pb_->Clear();
			for (int i = 0; i < alt_size(); i++)
				alt_[i].ToPB(pb_->add_alt());
		}
		void FromPB(const PB_ALT *pb_) {
			if (alt_size() <= pb_->alt_size()) {
				for (int i = 0; i < alt_size(); i++)
					alt_[i].FromPB(&pb_->alt(i));
			} else {
				for (int i = 0; i < pb_->alt_size(); i++)
					alt_[i].FromPB(&pb_->alt(i));
				for (int i = pb_->alt_size(); i < alt_size(); i++)
					alt_[i] = ItemInfo();
			}
		}

	public:
		inline const ItemInfo & alt(int index) const {
			if (index < 0 || index >= alt_size()) {
				assert(0);
			}
			return alt_[index];
		}
		inline ItemInfo * mutable_alt(int index) {
			if (index < 0 || index >= alt_size()) {
				assert(0);
			}
			return &alt_[index];
		}
		inline int alt_size() const {
			return (int)(sizeof(alt_) / sizeof(alt_[0]));
		}

	private:
		ItemInfo alt_[6];

};

#endif
