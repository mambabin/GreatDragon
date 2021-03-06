// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: MailInfo.proto

#ifndef PROTOBUF_MailInfo_2eproto__INCLUDED
#define PROTOBUF_MailInfo_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 2004000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 2004001 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/generated_message_reflection.h>
#include "ItemBaseInfo.pb.h"
// @@protoc_insertion_point(includes)

// Internal implementation detail -- do not call these.
void  protobuf_AddDesc_MailInfo_2eproto();
void protobuf_AssignDesc_MailInfo_2eproto();
void protobuf_ShutdownFile_MailInfo_2eproto();

class PB_MailInfo;

// ===================================================================

class PB_MailInfo : public ::google::protobuf::Message {
 public:
  PB_MailInfo();
  virtual ~PB_MailInfo();
  
  PB_MailInfo(const PB_MailInfo& from);
  
  inline PB_MailInfo& operator=(const PB_MailInfo& from) {
    CopyFrom(from);
    return *this;
  }
  
  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }
  
  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }
  
  static const ::google::protobuf::Descriptor* descriptor();
  static const PB_MailInfo& default_instance();
  
  void Swap(PB_MailInfo* other);
  
  // implements Message ----------------------------------------------
  
  PB_MailInfo* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const PB_MailInfo& from);
  void MergeFrom(const PB_MailInfo& from);
  void Clear();
  bool IsInitialized() const;
  
  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  
  ::google::protobuf::Metadata GetMetadata() const;
  
  // nested types ----------------------------------------------------
  
  // accessors -------------------------------------------------------
  
  // optional string sender = 1;
  inline bool has_sender() const;
  inline void clear_sender();
  static const int kSenderFieldNumber = 1;
  inline const ::std::string& sender() const;
  inline void set_sender(const ::std::string& value);
  inline void set_sender(const char* value);
  inline void set_sender(const char* value, size_t size);
  inline ::std::string* mutable_sender();
  inline ::std::string* release_sender();
  
  // optional int64 time = 2;
  inline bool has_time() const;
  inline void clear_time();
  static const int kTimeFieldNumber = 2;
  inline ::google::protobuf::int64 time() const;
  inline void set_time(::google::protobuf::int64 value);
  
  // optional string title = 3;
  inline bool has_title() const;
  inline void clear_title();
  static const int kTitleFieldNumber = 3;
  inline const ::std::string& title() const;
  inline void set_title(const ::std::string& value);
  inline void set_title(const char* value);
  inline void set_title(const char* value, size_t size);
  inline ::std::string* mutable_title();
  inline ::std::string* release_title();
  
  // optional string content = 4;
  inline bool has_content() const;
  inline void clear_content();
  static const int kContentFieldNumber = 4;
  inline const ::std::string& content() const;
  inline void set_content(const ::std::string& value);
  inline void set_content(const char* value);
  inline void set_content(const char* value, size_t size);
  inline ::std::string* mutable_content();
  inline ::std::string* release_content();
  
  // optional .PB_ItemInfo item = 5;
  inline bool has_item() const;
  inline void clear_item();
  static const int kItemFieldNumber = 5;
  inline const ::PB_ItemInfo& item() const;
  inline ::PB_ItemInfo* mutable_item();
  inline ::PB_ItemInfo* release_item();
  
  // optional bool read = 6;
  inline bool has_read() const;
  inline void clear_read();
  static const int kReadFieldNumber = 6;
  inline bool read() const;
  inline void set_read(bool value);
  
  // optional int64 rmb = 7;
  inline bool has_rmb() const;
  inline void clear_rmb();
  static const int kRmbFieldNumber = 7;
  inline ::google::protobuf::int64 rmb() const;
  inline void set_rmb(::google::protobuf::int64 value);
  
  // optional bool isRmb = 8;
  inline bool has_isRmb() const;
  inline void clear_isRmb();
  static const int kIsRmbFieldNumber = 8;
  inline bool isRmb() const;
  inline void set_isRmb(bool value);
  
  // @@protoc_insertion_point(class_scope:PB_MailInfo)
 private:
  inline void set_has_sender();
  inline void clear_has_sender();
  inline void set_has_time();
  inline void clear_has_time();
  inline void set_has_title();
  inline void clear_has_title();
  inline void set_has_content();
  inline void clear_has_content();
  inline void set_has_item();
  inline void clear_has_item();
  inline void set_has_read();
  inline void clear_has_read();
  inline void set_has_rmb();
  inline void clear_has_rmb();
  inline void set_has_isRmb();
  inline void clear_has_isRmb();
  
  ::google::protobuf::UnknownFieldSet _unknown_fields_;
  
  ::std::string* sender_;
  ::google::protobuf::int64 time_;
  ::std::string* title_;
  ::std::string* content_;
  ::PB_ItemInfo* item_;
  ::google::protobuf::int64 rmb_;
  bool read_;
  bool isRmb_;
  
  mutable int _cached_size_;
  ::google::protobuf::uint32 _has_bits_[(8 + 31) / 32];
  
  friend void  protobuf_AddDesc_MailInfo_2eproto();
  friend void protobuf_AssignDesc_MailInfo_2eproto();
  friend void protobuf_ShutdownFile_MailInfo_2eproto();
  
  void InitAsDefaultInstance();
  static PB_MailInfo* default_instance_;
};
// ===================================================================


// ===================================================================

// PB_MailInfo

// optional string sender = 1;
inline bool PB_MailInfo::has_sender() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void PB_MailInfo::set_has_sender() {
  _has_bits_[0] |= 0x00000001u;
}
inline void PB_MailInfo::clear_has_sender() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void PB_MailInfo::clear_sender() {
  if (sender_ != &::google::protobuf::internal::kEmptyString) {
    sender_->clear();
  }
  clear_has_sender();
}
inline const ::std::string& PB_MailInfo::sender() const {
  return *sender_;
}
inline void PB_MailInfo::set_sender(const ::std::string& value) {
  set_has_sender();
  if (sender_ == &::google::protobuf::internal::kEmptyString) {
    sender_ = new ::std::string;
  }
  sender_->assign(value);
}
inline void PB_MailInfo::set_sender(const char* value) {
  set_has_sender();
  if (sender_ == &::google::protobuf::internal::kEmptyString) {
    sender_ = new ::std::string;
  }
  sender_->assign(value);
}
inline void PB_MailInfo::set_sender(const char* value, size_t size) {
  set_has_sender();
  if (sender_ == &::google::protobuf::internal::kEmptyString) {
    sender_ = new ::std::string;
  }
  sender_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* PB_MailInfo::mutable_sender() {
  set_has_sender();
  if (sender_ == &::google::protobuf::internal::kEmptyString) {
    sender_ = new ::std::string;
  }
  return sender_;
}
inline ::std::string* PB_MailInfo::release_sender() {
  clear_has_sender();
  if (sender_ == &::google::protobuf::internal::kEmptyString) {
    return NULL;
  } else {
    ::std::string* temp = sender_;
    sender_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
    return temp;
  }
}

// optional int64 time = 2;
inline bool PB_MailInfo::has_time() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void PB_MailInfo::set_has_time() {
  _has_bits_[0] |= 0x00000002u;
}
inline void PB_MailInfo::clear_has_time() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void PB_MailInfo::clear_time() {
  time_ = GOOGLE_LONGLONG(0);
  clear_has_time();
}
inline ::google::protobuf::int64 PB_MailInfo::time() const {
  return time_;
}
inline void PB_MailInfo::set_time(::google::protobuf::int64 value) {
  set_has_time();
  time_ = value;
}

// optional string title = 3;
inline bool PB_MailInfo::has_title() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void PB_MailInfo::set_has_title() {
  _has_bits_[0] |= 0x00000004u;
}
inline void PB_MailInfo::clear_has_title() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void PB_MailInfo::clear_title() {
  if (title_ != &::google::protobuf::internal::kEmptyString) {
    title_->clear();
  }
  clear_has_title();
}
inline const ::std::string& PB_MailInfo::title() const {
  return *title_;
}
inline void PB_MailInfo::set_title(const ::std::string& value) {
  set_has_title();
  if (title_ == &::google::protobuf::internal::kEmptyString) {
    title_ = new ::std::string;
  }
  title_->assign(value);
}
inline void PB_MailInfo::set_title(const char* value) {
  set_has_title();
  if (title_ == &::google::protobuf::internal::kEmptyString) {
    title_ = new ::std::string;
  }
  title_->assign(value);
}
inline void PB_MailInfo::set_title(const char* value, size_t size) {
  set_has_title();
  if (title_ == &::google::protobuf::internal::kEmptyString) {
    title_ = new ::std::string;
  }
  title_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* PB_MailInfo::mutable_title() {
  set_has_title();
  if (title_ == &::google::protobuf::internal::kEmptyString) {
    title_ = new ::std::string;
  }
  return title_;
}
inline ::std::string* PB_MailInfo::release_title() {
  clear_has_title();
  if (title_ == &::google::protobuf::internal::kEmptyString) {
    return NULL;
  } else {
    ::std::string* temp = title_;
    title_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
    return temp;
  }
}

// optional string content = 4;
inline bool PB_MailInfo::has_content() const {
  return (_has_bits_[0] & 0x00000008u) != 0;
}
inline void PB_MailInfo::set_has_content() {
  _has_bits_[0] |= 0x00000008u;
}
inline void PB_MailInfo::clear_has_content() {
  _has_bits_[0] &= ~0x00000008u;
}
inline void PB_MailInfo::clear_content() {
  if (content_ != &::google::protobuf::internal::kEmptyString) {
    content_->clear();
  }
  clear_has_content();
}
inline const ::std::string& PB_MailInfo::content() const {
  return *content_;
}
inline void PB_MailInfo::set_content(const ::std::string& value) {
  set_has_content();
  if (content_ == &::google::protobuf::internal::kEmptyString) {
    content_ = new ::std::string;
  }
  content_->assign(value);
}
inline void PB_MailInfo::set_content(const char* value) {
  set_has_content();
  if (content_ == &::google::protobuf::internal::kEmptyString) {
    content_ = new ::std::string;
  }
  content_->assign(value);
}
inline void PB_MailInfo::set_content(const char* value, size_t size) {
  set_has_content();
  if (content_ == &::google::protobuf::internal::kEmptyString) {
    content_ = new ::std::string;
  }
  content_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* PB_MailInfo::mutable_content() {
  set_has_content();
  if (content_ == &::google::protobuf::internal::kEmptyString) {
    content_ = new ::std::string;
  }
  return content_;
}
inline ::std::string* PB_MailInfo::release_content() {
  clear_has_content();
  if (content_ == &::google::protobuf::internal::kEmptyString) {
    return NULL;
  } else {
    ::std::string* temp = content_;
    content_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
    return temp;
  }
}

// optional .PB_ItemInfo item = 5;
inline bool PB_MailInfo::has_item() const {
  return (_has_bits_[0] & 0x00000010u) != 0;
}
inline void PB_MailInfo::set_has_item() {
  _has_bits_[0] |= 0x00000010u;
}
inline void PB_MailInfo::clear_has_item() {
  _has_bits_[0] &= ~0x00000010u;
}
inline void PB_MailInfo::clear_item() {
  if (item_ != NULL) item_->::PB_ItemInfo::Clear();
  clear_has_item();
}
inline const ::PB_ItemInfo& PB_MailInfo::item() const {
  return item_ != NULL ? *item_ : *default_instance_->item_;
}
inline ::PB_ItemInfo* PB_MailInfo::mutable_item() {
  set_has_item();
  if (item_ == NULL) item_ = new ::PB_ItemInfo;
  return item_;
}
inline ::PB_ItemInfo* PB_MailInfo::release_item() {
  clear_has_item();
  ::PB_ItemInfo* temp = item_;
  item_ = NULL;
  return temp;
}

// optional bool read = 6;
inline bool PB_MailInfo::has_read() const {
  return (_has_bits_[0] & 0x00000020u) != 0;
}
inline void PB_MailInfo::set_has_read() {
  _has_bits_[0] |= 0x00000020u;
}
inline void PB_MailInfo::clear_has_read() {
  _has_bits_[0] &= ~0x00000020u;
}
inline void PB_MailInfo::clear_read() {
  read_ = false;
  clear_has_read();
}
inline bool PB_MailInfo::read() const {
  return read_;
}
inline void PB_MailInfo::set_read(bool value) {
  set_has_read();
  read_ = value;
}

// optional int64 rmb = 7;
inline bool PB_MailInfo::has_rmb() const {
  return (_has_bits_[0] & 0x00000040u) != 0;
}
inline void PB_MailInfo::set_has_rmb() {
  _has_bits_[0] |= 0x00000040u;
}
inline void PB_MailInfo::clear_has_rmb() {
  _has_bits_[0] &= ~0x00000040u;
}
inline void PB_MailInfo::clear_rmb() {
  rmb_ = GOOGLE_LONGLONG(0);
  clear_has_rmb();
}
inline ::google::protobuf::int64 PB_MailInfo::rmb() const {
  return rmb_;
}
inline void PB_MailInfo::set_rmb(::google::protobuf::int64 value) {
  set_has_rmb();
  rmb_ = value;
}

// optional bool isRmb = 8;
inline bool PB_MailInfo::has_isRmb() const {
  return (_has_bits_[0] & 0x00000080u) != 0;
}
inline void PB_MailInfo::set_has_isRmb() {
  _has_bits_[0] |= 0x00000080u;
}
inline void PB_MailInfo::clear_has_isRmb() {
  _has_bits_[0] &= ~0x00000080u;
}
inline void PB_MailInfo::clear_isRmb() {
  isRmb_ = false;
  clear_has_isRmb();
}
inline bool PB_MailInfo::isRmb() const {
  return isRmb_;
}
inline void PB_MailInfo::set_isRmb(bool value) {
  set_has_isRmb();
  isRmb_ = value;
}


// @@protoc_insertion_point(namespace_scope)

#ifndef SWIG
namespace google {
namespace protobuf {


}  // namespace google
}  // namespace protobuf
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_MailInfo_2eproto__INCLUDED
