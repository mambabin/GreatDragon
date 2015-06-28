#ifndef _FUNCINFO_HPP_
#define _FUNCINFO_HPP_

#include "FuncInfo.pb.h"
#include <sys/types.h>
#include <cassert>
#include <string>
#include <cstring>

class FuncInfo {

	public:
		enum Type {
			NONE = 0,
			MISSION = 1,
			BUSINESS = 2,
			ROOM = 3,
		};
		static bool Type_IsValid(int value) {
			switch(value) {
				case NONE:
				case MISSION:
				case BUSINESS:
				case ROOM:
					return true;
				default:
					return false;
			}
		}

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
		FuncInfo() {
			type_ = (Type)0;
			for (int i = 0; i < arg_size(); i++)
				arg_[i] = 0;
		}

	public:
		void ToPB(PB_FuncInfo *pb_) const {
			pb_->Clear();
			pb_->set_type((PB_FuncInfo::Type)type_);
			for (int i = 0; i < arg_size(); i++)
				pb_->add_arg(arg_[i]);
		}
		void FromPB(const PB_FuncInfo *pb_) {
			type_ = (Type)pb_->type();
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
		inline Type type() const {
			return type_;
		}
		inline void set_type(Type value) {
			type_ = value;
		}

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
		Type type_;

		int32_t arg_[2];

};

class FuncAtt {

	public:
		enum FuncsSizeType {
			FUNCS_SIZE = 50,
		};
		static bool FuncsSizeType_IsValid(int value) {
			switch(value) {
				case FUNCS_SIZE:
					return true;
				default:
					return false;
			}
		}

	public:
		FuncAtt() {
		}

	public:
		void ToPB(PB_FuncAtt *pb_) const {
			pb_->Clear();
			for (int i = 0; i < funcs_size(); i++)
				funcs_[i].ToPB(pb_->add_funcs());
		}
		void FromPB(const PB_FuncAtt *pb_) {
			if (funcs_size() <= pb_->funcs_size()) {
				for (int i = 0; i < funcs_size(); i++)
					funcs_[i].FromPB(&pb_->funcs(i));
			} else {
				for (int i = 0; i < pb_->funcs_size(); i++)
					funcs_[i].FromPB(&pb_->funcs(i));
				for (int i = pb_->funcs_size(); i < funcs_size(); i++)
					funcs_[i] = FuncInfo();
			}
		}

	public:
		inline const FuncInfo & funcs(int index) const {
			if (index < 0 || index >= funcs_size()) {
				assert(0);
			}
			return funcs_[index];
		}
		inline FuncInfo * mutable_funcs(int index) {
			if (index < 0 || index >= funcs_size()) {
				assert(0);
			}
			return &funcs_[index];
		}
		inline int funcs_size() const {
			return (int)(sizeof(funcs_) / sizeof(funcs_[0]));
		}

	private:
		FuncInfo funcs_[50];

};

#endif
