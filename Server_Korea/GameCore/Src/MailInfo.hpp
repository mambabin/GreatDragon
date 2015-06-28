#ifndef _MAILINFO_HPP_
#define _MAILINFO_HPP_

#include "MailInfo.pb.h"
#include "ItemBaseInfo.hpp"
#include "ItemBaseInfo.pb.h"
#include <sys/types.h>
#include <cassert>
#include <string>
#include <cstring>

class MailInfo {

	public:
		enum SenderSizeType {
			SENDER_SIZE = 32,
		};
		static bool SenderSizeType_IsValid(int value) {
			switch(value) {
				case SENDER_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum TitleSizeType {
			TITLE_SIZE = 64,
		};
		static bool TitleSizeType_IsValid(int value) {
			switch(value) {
				case TITLE_SIZE:
					return true;
				default:
					return false;
			}
		}

		enum ContentSizeType {
			CONTENT_SIZE = 256,
		};
		static bool ContentSizeType_IsValid(int value) {
			switch(value) {
				case CONTENT_SIZE:
					return true;
				default:
					return false;
			}
		}

	public:
		MailInfo() {
			sender_[0] = '\0';
			time_ = 0;
			title_[0] = '\0';
			content_[0] = '\0';
			read_ = false;
			rmb_ = 0;
			isRmb_ = false;
		}

	public:
		void ToPB(PB_MailInfo *pb_) const {
			pb_->Clear();
			pb_->set_sender(sender_);
			pb_->set_time(time_);
			pb_->set_title(title_);
			pb_->set_content(content_);
			item_.ToPB(pb_->mutable_item());
			pb_->set_read(read_);
			pb_->set_rmb(rmb_);
			pb_->set_isRmb(isRmb_);
		}
		void FromPB(const PB_MailInfo *pb_) {
			strncpy(sender_, pb_->sender().c_str(), sender_size() - 1);
			sender_[sender_size() - 1] = '\0';
			time_ = pb_->time();
			strncpy(title_, pb_->title().c_str(), title_size() - 1);
			title_[title_size() - 1] = '\0';
			strncpy(content_, pb_->content().c_str(), content_size() - 1);
			content_[content_size() - 1] = '\0';
			item_.FromPB(&pb_->item());
			read_ = pb_->read();
			rmb_ = pb_->rmb();
			isRmb_ = pb_->isRmb();
		}

	public:
		inline const char * sender() const {
			return sender_;
		}
		inline void set_sender(const char * value) {
			strncpy(sender_, value, sender_size() - 1);
			sender_[sender_size() - 1] = '\0';
		}
		inline int sender_size() const {
			return (int)(sizeof(sender_) / sizeof(sender_[0]));
		}

		inline int64_t time() const {
			return time_;
		}
		inline void set_time(int64_t value) {
			time_ = value;
		}

		inline const char * title() const {
			return title_;
		}
		inline void set_title(const char * value) {
			strncpy(title_, value, title_size() - 1);
			title_[title_size() - 1] = '\0';
		}
		inline int title_size() const {
			return (int)(sizeof(title_) / sizeof(title_[0]));
		}

		inline const char * content() const {
			return content_;
		}
		inline void set_content(const char * value) {
			strncpy(content_, value, content_size() - 1);
			content_[content_size() - 1] = '\0';
		}
		inline int content_size() const {
			return (int)(sizeof(content_) / sizeof(content_[0]));
		}

		inline const ItemInfo & item() const {
			return item_;
		}
		inline ItemInfo * mutable_item() {
			return &item_;
		}

		inline bool read() const {
			return read_;
		}
		inline void set_read(bool value) {
			read_ = value;
		}

		inline int64_t rmb() const {
			return rmb_;
		}
		inline void set_rmb(int64_t value) {
			rmb_ = value;
		}

		inline bool isRmb() const {
			return isRmb_;
		}
		inline void set_isRmb(bool value) {
			isRmb_ = value;
		}

	private:
		char sender_[32];

		int64_t time_;

		char title_[64];

		char content_[256];

		ItemInfo item_;

		bool read_;

		int64_t rmb_;

		bool isRmb_;

};

#endif
