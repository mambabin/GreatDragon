#ifndef _NPCATT_HPP_
#define _NPCATT_HPP_

#include "NPCAtt.pb.h"
#include "RoleAtt.hpp"
#include "RoleAtt.pb.h"
#include "FuncInfo.hpp"
#include "FuncInfo.pb.h"
#include <sys/types.h>
#include <cassert>
#include <string>
#include <cstring>

class NPCAtt {

	public:
		enum ColorType {
			WHITE = 0,
			GREEN = 1,
			BLUE = 2,
			YELLOW = 3,
			RED = 4,
		};
		static bool ColorType_IsValid(int value) {
			switch(value) {
				case WHITE:
				case GREEN:
				case BLUE:
				case YELLOW:
				case RED:
					return true;
				default:
					return false;
			}
		}

		enum DescSizeType {
			DESC_SIZE = 256,
		};
		static bool DescSizeType_IsValid(int value) {
			switch(value) {
				case DESC_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum PeddleAudioSizeType {
			PEDDLEAUDIO_SIZE = 3,
		};
		static bool PeddleAudioSizeType_IsValid(int value) {
			switch(value) {
				case PEDDLEAUDIO_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum TalkAudioSizeType {
			TALKAUDIO_SIZE = 3,
		};
		static bool TalkAudioSizeType_IsValid(int value) {
			switch(value) {
				case TALKAUDIO_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum EquipsSizeType {
			EQUIPS_SIZE = 12,
		};
		static bool EquipsSizeType_IsValid(int value) {
			switch(value) {
				case EQUIPS_SIZE:
					return true;
				default:
					return false;
			}
		}

	public:
		NPCAtt() {
			id_ = 0;
			pic_ = 0;
			desc_[0] = '\0';
			dropID_ = 0;
			for (int i = 0; i < peddleAudio_size(); i++)
				peddleAudio_[i] = 0;
			peddleMaxDistance_ = 0.0f;
			peddleMinInterval_ = 0;
			peddleMaxInterval_ = 0;
			for (int i = 0; i < talkAudio_size(); i++)
				talkAudio_[i] = 0;
			bornTime_ = 0;
			shockDelay_ = 0;
			shockTime_ = 0;
			bornEffect_ = 0;
			bornEffectTime_ = 0;
			specialPercent_ = 0.0f;
			bearAudio_ = 0;
			dieAudio_ = 0;
			color_ = (ColorType)0;
			hue_ = 0.0f;
			newSpecial_ = false;
			type_ = 0;
			quality_ = 0;
			level_ = 0;
			goodsID_ = 0;
			goodsCount_ = 0;
			dieEffect_ = 0;
		}

	public:
		void ToPB(PB_NPCAtt *pb_) const {
			pb_->Clear();
			pb_->set_id(id_);
			att_.ToPB(pb_->mutable_att());
			pb_->set_pic(pic_);
			pb_->set_desc(desc_);
			funcAtt_.ToPB(pb_->mutable_funcAtt());
			pb_->set_dropID(dropID_);
			for (int i = 0; i < peddleAudio_size(); i++)
				pb_->add_peddleAudio(peddleAudio_[i]);
			pb_->set_peddleMaxDistance(peddleMaxDistance_);
			pb_->set_peddleMinInterval(peddleMinInterval_);
			pb_->set_peddleMaxInterval(peddleMaxInterval_);
			for (int i = 0; i < talkAudio_size(); i++)
				pb_->add_talkAudio(talkAudio_[i]);
			pb_->set_bornTime(bornTime_);
			pb_->set_shockDelay(shockDelay_);
			pb_->set_shockTime(shockTime_);
			pb_->set_bornEffect(bornEffect_);
			pb_->set_bornEffectTime(bornEffectTime_);
			pb_->set_specialPercent(specialPercent_);
			pb_->set_bearAudio(bearAudio_);
			pb_->set_dieAudio(dieAudio_);
			pb_->set_color((PB_NPCAtt::ColorType)color_);
			pb_->set_hue(hue_);
			for (int i = 0; i < equips_size(); i++)
				equips_[i].ToPB(pb_->add_equips());
			pb_->set_newSpecial(newSpecial_);
			pb_->set_type(type_);
			pb_->set_quality(quality_);
			pb_->set_level(level_);
			pb_->set_goodsID(goodsID_);
			pb_->set_goodsCount(goodsCount_);
			pb_->set_dieEffect(dieEffect_);
		}
		void FromPB(const PB_NPCAtt *pb_) {
			id_ = pb_->id();
			att_.FromPB(&pb_->att());
			pic_ = pb_->pic();
			strncpy(desc_, pb_->desc().c_str(), desc_size() - 1);
			desc_[desc_size() - 1] = '\0';
			funcAtt_.FromPB(&pb_->funcAtt());
			dropID_ = pb_->dropID();
			if (peddleAudio_size() <= pb_->peddleAudio_size()) {
				for (int i = 0; i < peddleAudio_size(); i++)
					peddleAudio_[i] = pb_->peddleAudio(i);
			} else {
				for (int i = 0; i < pb_->peddleAudio_size(); i++)
					peddleAudio_[i] = pb_->peddleAudio(i);
				for (int i = pb_->peddleAudio_size(); i < peddleAudio_size(); i++)
					peddleAudio_[i] = 0;
			}
			peddleMaxDistance_ = pb_->peddleMaxDistance();
			peddleMinInterval_ = pb_->peddleMinInterval();
			peddleMaxInterval_ = pb_->peddleMaxInterval();
			if (talkAudio_size() <= pb_->talkAudio_size()) {
				for (int i = 0; i < talkAudio_size(); i++)
					talkAudio_[i] = pb_->talkAudio(i);
			} else {
				for (int i = 0; i < pb_->talkAudio_size(); i++)
					talkAudio_[i] = pb_->talkAudio(i);
				for (int i = pb_->talkAudio_size(); i < talkAudio_size(); i++)
					talkAudio_[i] = 0;
			}
			bornTime_ = pb_->bornTime();
			shockDelay_ = pb_->shockDelay();
			shockTime_ = pb_->shockTime();
			bornEffect_ = pb_->bornEffect();
			bornEffectTime_ = pb_->bornEffectTime();
			specialPercent_ = pb_->specialPercent();
			bearAudio_ = pb_->bearAudio();
			dieAudio_ = pb_->dieAudio();
			color_ = (ColorType)pb_->color();
			hue_ = pb_->hue();
			if (equips_size() <= pb_->equips_size()) {
				for (int i = 0; i < equips_size(); i++)
					equips_[i].FromPB(&pb_->equips(i));
			} else {
				for (int i = 0; i < pb_->equips_size(); i++)
					equips_[i].FromPB(&pb_->equips(i));
				for (int i = pb_->equips_size(); i < equips_size(); i++)
					equips_[i] = EquipAsset();
			}
			newSpecial_ = pb_->newSpecial();
			type_ = pb_->type();
			quality_ = pb_->quality();
			level_ = pb_->level();
			goodsID_ = pb_->goodsID();
			goodsCount_ = pb_->goodsCount();
			dieEffect_ = pb_->dieEffect();
		}

	public:
		inline int32_t id() const {
			return id_;
		}
		inline void set_id(int32_t value) {
			id_ = value;
		}

		inline const RoleAtt & att() const {
			return att_;
		}
		inline RoleAtt * mutable_att() {
			return &att_;
		}

		inline int32_t pic() const {
			return pic_;
		}
		inline void set_pic(int32_t value) {
			pic_ = value;
		}

		inline const char * desc() const {
			return desc_;
		}
		inline void set_desc(const char * value) {
			strncpy(desc_, value, desc_size() - 1);
			desc_[desc_size() - 1] = '\0';
		}
		inline int desc_size() const {
			return (int)(sizeof(desc_) / sizeof(desc_[0]));
		}

		inline const FuncAtt & funcAtt() const {
			return funcAtt_;
		}
		inline FuncAtt * mutable_funcAtt() {
			return &funcAtt_;
		}

		inline int32_t dropID() const {
			return dropID_;
		}
		inline void set_dropID(int32_t value) {
			dropID_ = value;
		}

		inline int32_t peddleAudio(int index) const {
			if (index < 0 || index >= peddleAudio_size()) {
				assert(0);
			}
			return peddleAudio_[index];
		}
		inline void set_peddleAudio(int index, int32_t value) {
			if (index < 0 || index >= peddleAudio_size()) {
				assert(0);
			}
			peddleAudio_[index] = value;
		}
		inline int peddleAudio_size() const {
			return (int)(sizeof(peddleAudio_) / sizeof(peddleAudio_[0]));
		}

		inline float peddleMaxDistance() const {
			return peddleMaxDistance_;
		}
		inline void set_peddleMaxDistance(float value) {
			peddleMaxDistance_ = value;
		}

		inline int32_t peddleMinInterval() const {
			return peddleMinInterval_;
		}
		inline void set_peddleMinInterval(int32_t value) {
			peddleMinInterval_ = value;
		}

		inline int32_t peddleMaxInterval() const {
			return peddleMaxInterval_;
		}
		inline void set_peddleMaxInterval(int32_t value) {
			peddleMaxInterval_ = value;
		}

		inline int32_t talkAudio(int index) const {
			if (index < 0 || index >= talkAudio_size()) {
				assert(0);
			}
			return talkAudio_[index];
		}
		inline void set_talkAudio(int index, int32_t value) {
			if (index < 0 || index >= talkAudio_size()) {
				assert(0);
			}
			talkAudio_[index] = value;
		}
		inline int talkAudio_size() const {
			return (int)(sizeof(talkAudio_) / sizeof(talkAudio_[0]));
		}

		inline int32_t bornTime() const {
			return bornTime_;
		}
		inline void set_bornTime(int32_t value) {
			bornTime_ = value;
		}

		inline int32_t shockDelay() const {
			return shockDelay_;
		}
		inline void set_shockDelay(int32_t value) {
			shockDelay_ = value;
		}

		inline int32_t shockTime() const {
			return shockTime_;
		}
		inline void set_shockTime(int32_t value) {
			shockTime_ = value;
		}

		inline int32_t bornEffect() const {
			return bornEffect_;
		}
		inline void set_bornEffect(int32_t value) {
			bornEffect_ = value;
		}

		inline int32_t bornEffectTime() const {
			return bornEffectTime_;
		}
		inline void set_bornEffectTime(int32_t value) {
			bornEffectTime_ = value;
		}

		inline float specialPercent() const {
			return specialPercent_;
		}
		inline void set_specialPercent(float value) {
			specialPercent_ = value;
		}

		inline int32_t bearAudio() const {
			return bearAudio_;
		}
		inline void set_bearAudio(int32_t value) {
			bearAudio_ = value;
		}

		inline int32_t dieAudio() const {
			return dieAudio_;
		}
		inline void set_dieAudio(int32_t value) {
			dieAudio_ = value;
		}

		inline ColorType color() const {
			return color_;
		}
		inline void set_color(ColorType value) {
			color_ = value;
		}

		inline float hue() const {
			return hue_;
		}
		inline void set_hue(float value) {
			hue_ = value;
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

		inline bool newSpecial() const {
			return newSpecial_;
		}
		inline void set_newSpecial(bool value) {
			newSpecial_ = value;
		}

		inline int32_t type() const {
			return type_;
		}
		inline void set_type(int32_t value) {
			type_ = value;
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

		inline int32_t goodsID() const {
			return goodsID_;
		}
		inline void set_goodsID(int32_t value) {
			goodsID_ = value;
		}

		inline int32_t goodsCount() const {
			return goodsCount_;
		}
		inline void set_goodsCount(int32_t value) {
			goodsCount_ = value;
		}

		inline int32_t dieEffect() const {
			return dieEffect_;
		}
		inline void set_dieEffect(int32_t value) {
			dieEffect_ = value;
		}

	private:
		int32_t id_;

		RoleAtt att_;

		int32_t pic_;

		char desc_[256];

		FuncAtt funcAtt_;

		int32_t dropID_;

		int32_t peddleAudio_[3];

		float peddleMaxDistance_;

		int32_t peddleMinInterval_;

		int32_t peddleMaxInterval_;

		int32_t talkAudio_[3];

		int32_t bornTime_;

		int32_t shockDelay_;

		int32_t shockTime_;

		int32_t bornEffect_;

		int32_t bornEffectTime_;

		float specialPercent_;

		int32_t bearAudio_;

		int32_t dieAudio_;

		ColorType color_;

		float hue_;

		EquipAsset equips_[12];

		bool newSpecial_;

		int32_t type_;

		int32_t quality_;

		int32_t level_;

		int32_t goodsID_;

		int32_t goodsCount_;

		int32_t dieEffect_;

};

class AllNPCs {

	public:
		enum NpcsSizeType {
			NPCS_SIZE = 1024,
		};
		static bool NpcsSizeType_IsValid(int value) {
			switch(value) {
				case NPCS_SIZE:
					return true;
				default:
					return false;
			}
		}

	public:
		AllNPCs() {
		}

	public:
		void ToPB(PB_AllNPCs *pb_) const {
			pb_->Clear();
			for (int i = 0; i < npcs_size(); i++)
				npcs_[i].ToPB(pb_->add_npcs());
		}
		void FromPB(const PB_AllNPCs *pb_) {
			if (npcs_size() <= pb_->npcs_size()) {
				for (int i = 0; i < npcs_size(); i++)
					npcs_[i].FromPB(&pb_->npcs(i));
			} else {
				for (int i = 0; i < pb_->npcs_size(); i++)
					npcs_[i].FromPB(&pb_->npcs(i));
				for (int i = pb_->npcs_size(); i < npcs_size(); i++)
					npcs_[i] = NPCAtt();
			}
		}

	public:
		inline const NPCAtt & npcs(int index) const {
			if (index < 0 || index >= npcs_size()) {
				assert(0);
			}
			return npcs_[index];
		}
		inline NPCAtt * mutable_npcs(int index) {
			if (index < 0 || index >= npcs_size()) {
				assert(0);
			}
			return &npcs_[index];
		}
		inline int npcs_size() const {
			return (int)(sizeof(npcs_) / sizeof(npcs_[0]));
		}

	private:
		NPCAtt npcs_[1024];

};

class AllPets {

	public:
		enum PetsSizeType {
			PETS_SIZE = 64,
		};
		static bool PetsSizeType_IsValid(int value) {
			switch(value) {
				case PETS_SIZE:
					return true;
				default:
					return false;
			}
		}

	public:
		AllPets() {
		}

	public:
		void ToPB(PB_AllPets *pb_) const {
			pb_->Clear();
			for (int i = 0; i < pets_size(); i++)
				pets_[i].ToPB(pb_->add_pets());
		}
		void FromPB(const PB_AllPets *pb_) {
			if (pets_size() <= pb_->pets_size()) {
				for (int i = 0; i < pets_size(); i++)
					pets_[i].FromPB(&pb_->pets(i));
			} else {
				for (int i = 0; i < pb_->pets_size(); i++)
					pets_[i].FromPB(&pb_->pets(i));
				for (int i = pb_->pets_size(); i < pets_size(); i++)
					pets_[i] = NPCAtt();
			}
		}

	public:
		inline const NPCAtt & pets(int index) const {
			if (index < 0 || index >= pets_size()) {
				assert(0);
			}
			return pets_[index];
		}
		inline NPCAtt * mutable_pets(int index) {
			if (index < 0 || index >= pets_size()) {
				assert(0);
			}
			return &pets_[index];
		}
		inline int pets_size() const {
			return (int)(sizeof(pets_) / sizeof(pets_[0]));
		}

	private:
		NPCAtt pets_[64];

};

#endif
