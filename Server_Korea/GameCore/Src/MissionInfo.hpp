#ifndef _MISSIONINFO_HPP_
#define _MISSIONINFO_HPP_

#include "MissionInfo.pb.h"
#include <sys/types.h>
#include <cassert>
#include <string>
#include <cstring>

class MissionTargetRecord {

	public:
		enum ArgSizeType {
			ARG_SIZE = 2,
		};
		static bool ArgSizeType_IsValid(int value) {
			switch(value) {
				case ARG_SIZE:
					return true;
				default:
					return false;
			}
		}

	public:
		MissionTargetRecord() {
			for (int i = 0; i < arg_size(); i++)
				arg_[i] = 0;
		}

	public:
		void ToPB(PB_MissionTargetRecord *pb_) const {
			pb_->Clear();
			for (int i = 0; i < arg_size(); i++)
				pb_->add_arg(arg_[i]);
		}
		void FromPB(const PB_MissionTargetRecord *pb_) {
			if (arg_size() <= pb_->arg_size()) {
				for (int i = 0; i < arg_size(); i++)
					arg_[i] = pb_->arg(i);
			} else {
				for (int i = 0; i < pb_->arg_size(); i++)
					arg_[i] = pb_->arg(i);
				for (int i = pb_->arg_size(); i < arg_size(); i++)
					arg_[i] = 0;
			}
		}

	public:
		inline int32_t arg(int index) const {
			if (index < 0 || index >= arg_size()) {
				assert(0);
			}
			return arg_[index];
		}
		inline void set_arg(int index, int32_t value) {
			if (index < 0 || index >= arg_size()) {
				assert(0);
			}
			arg_[index] = value;
		}
		inline int arg_size() const {
			return (int)(sizeof(arg_) / sizeof(arg_[0]));
		}

	private:
		int32_t arg_[2];

};

class MissionRecord {

	public:
		enum TargetSizeType {
			TARGET_SIZE = 2,
		};
		static bool TargetSizeType_IsValid(int value) {
			switch(value) {
				case TARGET_SIZE:
					return true;
				default:
					return false;
			}
		}

	public:
		MissionRecord() {
			count_ = 0;
		}

	public:
		void ToPB(PB_MissionRecord *pb_) const {
			pb_->Clear();
			pb_->set_count(count_);
			for (int i = 0; i < target_size(); i++)
				target_[i].ToPB(pb_->add_target());
		}
		void FromPB(const PB_MissionRecord *pb_) {
			count_ = pb_->count();
			if (target_size() <= pb_->target_size()) {
				for (int i = 0; i < target_size(); i++)
					target_[i].FromPB(&pb_->target(i));
			} else {
				for (int i = 0; i < pb_->target_size(); i++)
					target_[i].FromPB(&pb_->target(i));
				for (int i = pb_->target_size(); i < target_size(); i++)
					target_[i] = MissionTargetRecord();
			}
		}

	public:
		inline int32_t count() const {
			return count_;
		}
		inline void set_count(int32_t value) {
			count_ = value;
		}

		inline const MissionTargetRecord & target(int index) const {
			if (index < 0 || index >= target_size()) {
				assert(0);
			}
			return target_[index];
		}
		inline MissionTargetRecord * mutable_target(int index) {
			if (index < 0 || index >= target_size()) {
				assert(0);
			}
			return &target_[index];
		}
		inline int target_size() const {
			return (int)(sizeof(target_) / sizeof(target_[0]));
		}

	private:
		int32_t count_;

		MissionTargetRecord target_[2];

};

class MissionAllRecord {

	public:
		enum RecordsSizeType {
			RECORDS_SIZE = 512,
		};
		static bool RecordsSizeType_IsValid(int value) {
			switch(value) {
				case RECORDS_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum CurSizeType {
			CUR_SIZE = 32,
		};
		static bool CurSizeType_IsValid(int value) {
			switch(value) {
				case CUR_SIZE:
					return true;
				default:
					return false;
			}
		}

	public:
		MissionAllRecord() {
			for (int i = 0; i < cur_size(); i++)
				cur_[i] = -1;
		}

	public:
		void ToPB(PB_MissionAllRecord *pb_) const {
			pb_->Clear();
			for (int i = 0; i < records_size(); i++)
				records_[i].ToPB(pb_->add_records());
			for (int i = 0; i < cur_size(); i++)
				pb_->add_cur(cur_[i]);
		}
		void FromPB(const PB_MissionAllRecord *pb_) {
			if (records_size() <= pb_->records_size()) {
				for (int i = 0; i < records_size(); i++)
					records_[i].FromPB(&pb_->records(i));
			} else {
				for (int i = 0; i < pb_->records_size(); i++)
					records_[i].FromPB(&pb_->records(i));
				for (int i = pb_->records_size(); i < records_size(); i++)
					records_[i] = MissionRecord();
			}
			if (cur_size() <= pb_->cur_size()) {
				for (int i = 0; i < cur_size(); i++)
					cur_[i] = pb_->cur(i);
			} else {
				for (int i = 0; i < pb_->cur_size(); i++)
					cur_[i] = pb_->cur(i);
				for (int i = pb_->cur_size(); i < cur_size(); i++)
					cur_[i] = -1;
			}
		}

	public:
		inline const MissionRecord & records(int index) const {
			if (index < 0 || index >= records_size()) {
				assert(0);
			}
			return records_[index];
		}
		inline MissionRecord * mutable_records(int index) {
			if (index < 0 || index >= records_size()) {
				assert(0);
			}
			return &records_[index];
		}
		inline int records_size() const {
			return (int)(sizeof(records_) / sizeof(records_[0]));
		}

		inline int32_t cur(int index) const {
			if (index < 0 || index >= cur_size()) {
				assert(0);
			}
			return cur_[index];
		}
		inline void set_cur(int index, int32_t value) {
			if (index < 0 || index >= cur_size()) {
				assert(0);
			}
			cur_[index] = value;
		}
		inline int cur_size() const {
			return (int)(sizeof(cur_) / sizeof(cur_[0]));
		}

	private:
		MissionRecord records_[512];

		int32_t cur_[32];

};

#endif
