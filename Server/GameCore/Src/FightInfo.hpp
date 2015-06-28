#ifndef _FIGHTINFO_HPP_
#define _FIGHTINFO_HPP_

#include "FightInfo.pb.h"
#include <sys/types.h>
#include <cassert>
#include <string>
#include <cstring>

class Skill {

	public:
		Skill() {
			id_ = -1;
			level_ = 0;
		}

	public:
		void ToPB(PB_Skill *pb_) const {
			pb_->Clear();
			pb_->set_id(id_);
			pb_->set_level(level_);
		}
		void FromPB(const PB_Skill *pb_) {
			id_ = pb_->id();
			level_ = pb_->level();
		}

	public:
		inline int32_t id() const {
			return id_;
		}
		inline void set_id(int32_t value) {
			id_ = value;
		}

		inline int32_t level() const {
			return level_;
		}
		inline void set_level(int32_t value) {
			level_ = value;
		}

	private:
		int32_t id_;

		int32_t level_;

};

class FightPropertyDelta {

	public:
		FightPropertyDelta() {
			delta_ = 0;
			percent_ = 0.0f;
		}

	public:
		void ToPB(PB_FightPropertyDelta *pb_) const {
			pb_->Clear();
			pb_->set_delta(delta_);
			pb_->set_percent(percent_);
		}
		void FromPB(const PB_FightPropertyDelta *pb_) {
			delta_ = pb_->delta();
			percent_ = pb_->percent();
		}

	public:
		inline int32_t delta() const {
			return delta_;
		}
		inline void set_delta(int32_t value) {
			delta_ = value;
		}

		inline float percent() const {
			return percent_;
		}
		inline void set_percent(float value) {
			percent_ = value;
		}

	private:
		int32_t delta_;

		float percent_;

};

class RoomHistory {

	public:
		RoomHistory() {
			count_ = 0;
		}

	public:
		void ToPB(PB_RoomHistory *pb_) const {
			pb_->Clear();
			pb_->set_count(count_);
		}
		void FromPB(const PB_RoomHistory *pb_) {
			count_ = pb_->count();
		}

	public:
		inline int32_t count() const {
			return count_;
		}
		inline void set_count(int32_t value) {
			count_ = value;
		}

	private:
		int32_t count_;

};

class BloodDelta {

	public:
		BloodDelta() {
			toAtk_ = 0;
			toDef_ = 0;
			toDodge_ = 0;
			toAccuracy_ = 0;
		}

	public:
		void ToPB(PB_BloodDelta *pb_) const {
			pb_->Clear();
			pb_->set_toAtk(toAtk_);
			pb_->set_toDef(toDef_);
			pb_->set_toDodge(toDodge_);
			pb_->set_toAccuracy(toAccuracy_);
		}
		void FromPB(const PB_BloodDelta *pb_) {
			toAtk_ = pb_->toAtk();
			toDef_ = pb_->toDef();
			toDodge_ = pb_->toDodge();
			toAccuracy_ = pb_->toAccuracy();
		}

	public:
		inline int32_t toAtk() const {
			return toAtk_;
		}
		inline void set_toAtk(int32_t value) {
			toAtk_ = value;
		}

		inline int32_t toDef() const {
			return toDef_;
		}
		inline void set_toDef(int32_t value) {
			toDef_ = value;
		}

		inline int32_t toDodge() const {
			return toDodge_;
		}
		inline void set_toDodge(int32_t value) {
			toDodge_ = value;
		}

		inline int32_t toAccuracy() const {
			return toAccuracy_;
		}
		inline void set_toAccuracy(int32_t value) {
			toAccuracy_ = value;
		}

	private:
		int32_t toAtk_;

		int32_t toDef_;

		int32_t toDodge_;

		int32_t toAccuracy_;

};

class FightAtt {

	public:
		enum Status {
			IDLE = 0,
			MOVE = 1,
			ATTACK = 2,
			DEAD = 3,
		};
		static bool Status_IsValid(int value) {
			switch(value) {
				case IDLE:
				case MOVE:
				case ATTACK:
				case DEAD:
					return true;
				default:
					return false;
			}
		}

		enum PropertyType {
			ATK = 0,
			DEF = 1,
			MAXHP = 2,
			CRIT = 3,
			ACCURACY = 4,
			DODGE = 5,
		};
		static bool PropertyType_IsValid(int value) {
			switch(value) {
				case ATK:
				case DEF:
				case MAXHP:
				case CRIT:
				case ACCURACY:
				case DODGE:
					return true;
				default:
					return false;
			}
		}

		enum SkillsSizeType {
			SKILLS_SIZE = 18,
		};
		static bool SkillsSizeType_IsValid(int value) {
			switch(value) {
				case SKILLS_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum PropertiesSizeType {
			PROPERTIES_SIZE = 6,
		};
		static bool PropertiesSizeType_IsValid(int value) {
			switch(value) {
				case PROPERTIES_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum PropertiesDeltaSizeType {
			PROPERTIESDELTA_SIZE = 6,
		};
		static bool PropertiesDeltaSizeType_IsValid(int value) {
			switch(value) {
				case PROPERTIESDELTA_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum BloodDeltaSizeType {
			BLOODDELTA_SIZE = 24,
		};
		static bool BloodDeltaSizeType_IsValid(int value) {
			switch(value) {
				case BLOODDELTA_SIZE:
					return true;
				default:
					return false;
			}
		}

	public:
		FightAtt() {
			status_ = (Status)0;
			selfFaction_ = 0;
			friendlyFaction_ = 0;
			reviveTime_ = 0;
			hp_ = 0;
			mana_ = 0;
			for (int i = 0; i < properties_size(); i++)
				properties_[i] = 0;
			level_ = 0;
			exp_ = 0;
			energy_ = 0;
			bloodLevel_ = 0;
			bloodNode_ = 0;
			curTower_ = 1;
			maxTower_ = 0;
			maxSurvive_ = 0;
			winPVP_ = 0;
			losePVP_ = 0;
			baseWingLevel_ = 0;
			baseWingDegree_ = 0;
			worldBossHurt_ = 0;
			worldBossNum_ = 0;
			fightingPet_ = -1;
			transformID_ = 0;
		}

	public:
		void ToPB(PB_FightAtt *pb_) const {
			pb_->Clear();
			pb_->set_status((PB_FightAtt::Status)status_);
			for (int i = 0; i < skills_size(); i++)
				skills_[i].ToPB(pb_->add_skills());
			pb_->set_selfFaction(selfFaction_);
			pb_->set_friendlyFaction(friendlyFaction_);
			pb_->set_reviveTime(reviveTime_);
			pb_->set_hp(hp_);
			pb_->set_mana(mana_);
			for (int i = 0; i < properties_size(); i++)
				pb_->add_properties(properties_[i]);
			for (int i = 0; i < propertiesDelta_size(); i++)
				propertiesDelta_[i].ToPB(pb_->add_propertiesDelta());
			pb_->set_level(level_);
			pb_->set_exp(exp_);
			pb_->set_energy(energy_);
			pb_->set_bloodLevel(bloodLevel_);
			pb_->set_bloodNode(bloodNode_);
			for (int i = 0; i < bloodDelta_size(); i++)
				bloodDelta_[i].ToPB(pb_->add_bloodDelta());
			pb_->set_curTower(curTower_);
			pb_->set_maxTower(maxTower_);
			pb_->set_maxSurvive(maxSurvive_);
			pb_->set_winPVP(winPVP_);
			pb_->set_losePVP(losePVP_);
			pb_->set_baseWingLevel(baseWingLevel_);
			pb_->set_baseWingDegree(baseWingDegree_);
			pb_->set_worldBossHurt(worldBossHurt_);
			pb_->set_worldBossNum(worldBossNum_);
			pb_->set_fightingPet(fightingPet_);
			pb_->set_transformID(transformID_);
		}
		void FromPB(const PB_FightAtt *pb_) {
			status_ = (Status)pb_->status();
			if (skills_size() <= pb_->skills_size()) {
				for (int i = 0; i < skills_size(); i++)
					skills_[i].FromPB(&pb_->skills(i));
			} else {
				for (int i = 0; i < pb_->skills_size(); i++)
					skills_[i].FromPB(&pb_->skills(i));
				for (int i = pb_->skills_size(); i < skills_size(); i++)
					skills_[i] = Skill();
			}
			selfFaction_ = pb_->selfFaction();
			friendlyFaction_ = pb_->friendlyFaction();
			reviveTime_ = pb_->reviveTime();
			hp_ = pb_->hp();
			mana_ = pb_->mana();
			if (properties_size() <= pb_->properties_size()) {
				for (int i = 0; i < properties_size(); i++)
					properties_[i] = pb_->properties(i);
			} else {
				for (int i = 0; i < pb_->properties_size(); i++)
					properties_[i] = pb_->properties(i);
				for (int i = pb_->properties_size(); i < properties_size(); i++)
					properties_[i] = 0;
			}
			if (propertiesDelta_size() <= pb_->propertiesDelta_size()) {
				for (int i = 0; i < propertiesDelta_size(); i++)
					propertiesDelta_[i].FromPB(&pb_->propertiesDelta(i));
			} else {
				for (int i = 0; i < pb_->propertiesDelta_size(); i++)
					propertiesDelta_[i].FromPB(&pb_->propertiesDelta(i));
				for (int i = pb_->propertiesDelta_size(); i < propertiesDelta_size(); i++)
					propertiesDelta_[i] = FightPropertyDelta();
			}
			level_ = pb_->level();
			exp_ = pb_->exp();
			energy_ = pb_->energy();
			bloodLevel_ = pb_->bloodLevel();
			bloodNode_ = pb_->bloodNode();
			if (bloodDelta_size() <= pb_->bloodDelta_size()) {
				for (int i = 0; i < bloodDelta_size(); i++)
					bloodDelta_[i].FromPB(&pb_->bloodDelta(i));
			} else {
				for (int i = 0; i < pb_->bloodDelta_size(); i++)
					bloodDelta_[i].FromPB(&pb_->bloodDelta(i));
				for (int i = pb_->bloodDelta_size(); i < bloodDelta_size(); i++)
					bloodDelta_[i] = BloodDelta();
			}
			curTower_ = pb_->curTower();
			maxTower_ = pb_->maxTower();
			maxSurvive_ = pb_->maxSurvive();
			winPVP_ = pb_->winPVP();
			losePVP_ = pb_->losePVP();
			baseWingLevel_ = pb_->baseWingLevel();
			baseWingDegree_ = pb_->baseWingDegree();
			worldBossHurt_ = pb_->worldBossHurt();
			worldBossNum_ = pb_->worldBossNum();
			fightingPet_ = pb_->fightingPet();
			transformID_ = pb_->transformID();
		}

	public:
		inline Status status() const {
			return status_;
		}
		inline void set_status(Status value) {
			status_ = value;
		}

		inline const Skill & skills(int index) const {
			if (index < 0 || index >= skills_size()) {
				assert(0);
			}
			return skills_[index];
		}
		inline Skill * mutable_skills(int index) {
			if (index < 0 || index >= skills_size()) {
				assert(0);
			}
			return &skills_[index];
		}
		inline int skills_size() const {
			return (int)(sizeof(skills_) / sizeof(skills_[0]));
		}

		inline int32_t selfFaction() const {
			return selfFaction_;
		}
		inline void set_selfFaction(int32_t value) {
			selfFaction_ = value;
		}

		inline int32_t friendlyFaction() const {
			return friendlyFaction_;
		}
		inline void set_friendlyFaction(int32_t value) {
			friendlyFaction_ = value;
		}

		inline int32_t reviveTime() const {
			return reviveTime_;
		}
		inline void set_reviveTime(int32_t value) {
			reviveTime_ = value;
		}

		inline int32_t hp() const {
			return hp_;
		}
		inline void set_hp(int32_t value) {
			hp_ = value;
		}

		inline int32_t mana() const {
			return mana_;
		}
		inline void set_mana(int32_t value) {
			mana_ = value;
		}

		inline int32_t properties(int index) const {
			if (index < 0 || index >= properties_size()) {
				assert(0);
			}
			return properties_[index];
		}
		inline void set_properties(int index, int32_t value) {
			if (index < 0 || index >= properties_size()) {
				assert(0);
			}
			properties_[index] = value;
		}
		inline int properties_size() const {
			return (int)(sizeof(properties_) / sizeof(properties_[0]));
		}

		inline const FightPropertyDelta & propertiesDelta(int index) const {
			if (index < 0 || index >= propertiesDelta_size()) {
				assert(0);
			}
			return propertiesDelta_[index];
		}
		inline FightPropertyDelta * mutable_propertiesDelta(int index) {
			if (index < 0 || index >= propertiesDelta_size()) {
				assert(0);
			}
			return &propertiesDelta_[index];
		}
		inline int propertiesDelta_size() const {
			return (int)(sizeof(propertiesDelta_) / sizeof(propertiesDelta_[0]));
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

		inline int32_t energy() const {
			return energy_;
		}
		inline void set_energy(int32_t value) {
			energy_ = value;
		}

		inline int32_t bloodLevel() const {
			return bloodLevel_;
		}
		inline void set_bloodLevel(int32_t value) {
			bloodLevel_ = value;
		}

		inline int32_t bloodNode() const {
			return bloodNode_;
		}
		inline void set_bloodNode(int32_t value) {
			bloodNode_ = value;
		}

		inline const BloodDelta & bloodDelta(int index) const {
			if (index < 0 || index >= bloodDelta_size()) {
				assert(0);
			}
			return bloodDelta_[index];
		}
		inline BloodDelta * mutable_bloodDelta(int index) {
			if (index < 0 || index >= bloodDelta_size()) {
				assert(0);
			}
			return &bloodDelta_[index];
		}
		inline int bloodDelta_size() const {
			return (int)(sizeof(bloodDelta_) / sizeof(bloodDelta_[0]));
		}

		inline int32_t curTower() const {
			return curTower_;
		}
		inline void set_curTower(int32_t value) {
			curTower_ = value;
		}

		inline int32_t maxTower() const {
			return maxTower_;
		}
		inline void set_maxTower(int32_t value) {
			maxTower_ = value;
		}

		inline int32_t maxSurvive() const {
			return maxSurvive_;
		}
		inline void set_maxSurvive(int32_t value) {
			maxSurvive_ = value;
		}

		inline int32_t winPVP() const {
			return winPVP_;
		}
		inline void set_winPVP(int32_t value) {
			winPVP_ = value;
		}

		inline int32_t losePVP() const {
			return losePVP_;
		}
		inline void set_losePVP(int32_t value) {
			losePVP_ = value;
		}

		inline int32_t baseWingLevel() const {
			return baseWingLevel_;
		}
		inline void set_baseWingLevel(int32_t value) {
			baseWingLevel_ = value;
		}

		inline int32_t baseWingDegree() const {
			return baseWingDegree_;
		}
		inline void set_baseWingDegree(int32_t value) {
			baseWingDegree_ = value;
		}

		inline int32_t worldBossHurt() const {
			return worldBossHurt_;
		}
		inline void set_worldBossHurt(int32_t value) {
			worldBossHurt_ = value;
		}

		inline int32_t worldBossNum() const {
			return worldBossNum_;
		}
		inline void set_worldBossNum(int32_t value) {
			worldBossNum_ = value;
		}

		inline int32_t fightingPet() const {
			return fightingPet_;
		}
		inline void set_fightingPet(int32_t value) {
			fightingPet_ = value;
		}

		inline int32_t transformID() const {
			return transformID_;
		}
		inline void set_transformID(int32_t value) {
			transformID_ = value;
		}

	private:
		Status status_;

		Skill skills_[18];

		int32_t selfFaction_;

		int32_t friendlyFaction_;

		int32_t reviveTime_;

		int32_t hp_;

		int32_t mana_;

		int32_t properties_[6];

		FightPropertyDelta propertiesDelta_[6];

		int32_t level_;

		int64_t exp_;

		int32_t energy_;

		int32_t bloodLevel_;

		int32_t bloodNode_;

		BloodDelta bloodDelta_[24];

		int32_t curTower_;

		int32_t maxTower_;

		int32_t maxSurvive_;

		int32_t winPVP_;

		int32_t losePVP_;

		int32_t baseWingLevel_;

		int32_t baseWingDegree_;

		int32_t worldBossHurt_;

		int32_t worldBossNum_;

		int32_t fightingPet_;

		int32_t transformID_;

};

#endif
