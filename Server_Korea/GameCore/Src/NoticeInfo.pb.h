// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: NoticeInfo.proto

#ifndef PROTOBUF_NoticeInfo_2eproto__INCLUDED
#define PROTOBUF_NoticeInfo_2eproto__INCLUDED

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
// @@protoc_insertion_point(includes)

// Internal implementation detail -- do not call these.
void  protobuf_AddDesc_NoticeInfo_2eproto();
void protobuf_AssignDesc_NoticeInfo_2eproto();
void protobuf_ShutdownFile_NoticeInfo_2eproto();

class NoticeInfo;
class AllNotice;

enum NoticeInfo_BtnFunc {
  NoticeInfo_BtnFunc_NONE = 0,
  NoticeInfo_BtnFunc_UI_ONLINE = 1,
  NoticeInfo_BtnFunc_UI_SIGNIN_CONTINUE = 2,
  NoticeInfo_BtnFunc_UI_SIGNIN_TOTAL = 3,
  NoticeInfo_BtnFunc_UI_LUCKY = 4,
  NoticeInfo_BtnFunc_UI_FOOD = 5,
  NoticeInfo_BtnFunc_UI_VCOIN = 6
};
bool NoticeInfo_BtnFunc_IsValid(int value);
const NoticeInfo_BtnFunc NoticeInfo_BtnFunc_BtnFunc_MIN = NoticeInfo_BtnFunc_NONE;
const NoticeInfo_BtnFunc NoticeInfo_BtnFunc_BtnFunc_MAX = NoticeInfo_BtnFunc_UI_VCOIN;
const int NoticeInfo_BtnFunc_BtnFunc_ARRAYSIZE = NoticeInfo_BtnFunc_BtnFunc_MAX + 1;

const ::google::protobuf::EnumDescriptor* NoticeInfo_BtnFunc_descriptor();
inline const ::std::string& NoticeInfo_BtnFunc_Name(NoticeInfo_BtnFunc value) {
  return ::google::protobuf::internal::NameOfEnum(
    NoticeInfo_BtnFunc_descriptor(), value);
}
inline bool NoticeInfo_BtnFunc_Parse(
    const ::std::string& name, NoticeInfo_BtnFunc* value) {
  return ::google::protobuf::internal::ParseNamedEnum<NoticeInfo_BtnFunc>(
    NoticeInfo_BtnFunc_descriptor(), name, value);
}
// ===================================================================

class NoticeInfo : public ::google::protobuf::Message {
 public:
  NoticeInfo();
  virtual ~NoticeInfo();
  
  NoticeInfo(const NoticeInfo& from);
  
  inline NoticeInfo& operator=(const NoticeInfo& from) {
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
  static const NoticeInfo& default_instance();
  
  void Swap(NoticeInfo* other);
  
  // implements Message ----------------------------------------------
  
  NoticeInfo* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const NoticeInfo& from);
  void MergeFrom(const NoticeInfo& from);
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
  
  typedef NoticeInfo_BtnFunc BtnFunc;
  static const BtnFunc NONE = NoticeInfo_BtnFunc_NONE;
  static const BtnFunc UI_ONLINE = NoticeInfo_BtnFunc_UI_ONLINE;
  static const BtnFunc UI_SIGNIN_CONTINUE = NoticeInfo_BtnFunc_UI_SIGNIN_CONTINUE;
  static const BtnFunc UI_SIGNIN_TOTAL = NoticeInfo_BtnFunc_UI_SIGNIN_TOTAL;
  static const BtnFunc UI_LUCKY = NoticeInfo_BtnFunc_UI_LUCKY;
  static const BtnFunc UI_FOOD = NoticeInfo_BtnFunc_UI_FOOD;
  static const BtnFunc UI_VCOIN = NoticeInfo_BtnFunc_UI_VCOIN;
  static inline bool BtnFunc_IsValid(int value) {
    return NoticeInfo_BtnFunc_IsValid(value);
  }
  static const BtnFunc BtnFunc_MIN =
    NoticeInfo_BtnFunc_BtnFunc_MIN;
  static const BtnFunc BtnFunc_MAX =
    NoticeInfo_BtnFunc_BtnFunc_MAX;
  static const int BtnFunc_ARRAYSIZE =
    NoticeInfo_BtnFunc_BtnFunc_ARRAYSIZE;
  static inline const ::google::protobuf::EnumDescriptor*
  BtnFunc_descriptor() {
    return NoticeInfo_BtnFunc_descriptor();
  }
  static inline const ::std::string& BtnFunc_Name(BtnFunc value) {
    return NoticeInfo_BtnFunc_Name(value);
  }
  static inline bool BtnFunc_Parse(const ::std::string& name,
      BtnFunc* value) {
    return NoticeInfo_BtnFunc_Parse(name, value);
  }
  
  // accessors -------------------------------------------------------
  
  // optional string name = 1;
  inline bool has_name() const;
  inline void clear_name();
  static const int kNameFieldNumber = 1;
  inline const ::std::string& name() const;
  inline void set_name(const ::std::string& value);
  inline void set_name(const char* value);
  inline void set_name(const char* value, size_t size);
  inline ::std::string* mutable_name();
  inline ::std::string* release_name();
  
  // optional string context = 2;
  inline bool has_context() const;
  inline void clear_context();
  static const int kContextFieldNumber = 2;
  inline const ::std::string& context() const;
  inline void set_context(const ::std::string& value);
  inline void set_context(const char* value);
  inline void set_context(const char* value, size_t size);
  inline ::std::string* mutable_context();
  inline ::std::string* release_context();
  
  // optional .NoticeInfo.BtnFunc btnFunc = 3;
  inline bool has_btnFunc() const;
  inline void clear_btnFunc();
  static const int kBtnFuncFieldNumber = 3;
  inline ::NoticeInfo_BtnFunc btnFunc() const;
  inline void set_btnFunc(::NoticeInfo_BtnFunc value);
  
  // @@protoc_insertion_point(class_scope:NoticeInfo)
 private:
  inline void set_has_name();
  inline void clear_has_name();
  inline void set_has_context();
  inline void clear_has_context();
  inline void set_has_btnFunc();
  inline void clear_has_btnFunc();
  
  ::google::protobuf::UnknownFieldSet _unknown_fields_;
  
  ::std::string* name_;
  ::std::string* context_;
  int btnFunc_;
  
  mutable int _cached_size_;
  ::google::protobuf::uint32 _has_bits_[(3 + 31) / 32];
  
  friend void  protobuf_AddDesc_NoticeInfo_2eproto();
  friend void protobuf_AssignDesc_NoticeInfo_2eproto();
  friend void protobuf_ShutdownFile_NoticeInfo_2eproto();
  
  void InitAsDefaultInstance();
  static NoticeInfo* default_instance_;
};
// -------------------------------------------------------------------

class AllNotice : public ::google::protobuf::Message {
 public:
  AllNotice();
  virtual ~AllNotice();
  
  AllNotice(const AllNotice& from);
  
  inline AllNotice& operator=(const AllNotice& from) {
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
  static const AllNotice& default_instance();
  
  void Swap(AllNotice* other);
  
  // implements Message ----------------------------------------------
  
  AllNotice* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const AllNotice& from);
  void MergeFrom(const AllNotice& from);
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
  
  // repeated .NoticeInfo allNotice = 1;
  inline int allNotice_size() const;
  inline void clear_allNotice();
  static const int kAllNoticeFieldNumber = 1;
  inline const ::NoticeInfo& allNotice(int index) const;
  inline ::NoticeInfo* mutable_allNotice(int index);
  inline ::NoticeInfo* add_allNotice();
  inline const ::google::protobuf::RepeatedPtrField< ::NoticeInfo >&
      allNotice() const;
  inline ::google::protobuf::RepeatedPtrField< ::NoticeInfo >*
      mutable_allNotice();
  
  // @@protoc_insertion_point(class_scope:AllNotice)
 private:
  
  ::google::protobuf::UnknownFieldSet _unknown_fields_;
  
  ::google::protobuf::RepeatedPtrField< ::NoticeInfo > allNotice_;
  
  mutable int _cached_size_;
  ::google::protobuf::uint32 _has_bits_[(1 + 31) / 32];
  
  friend void  protobuf_AddDesc_NoticeInfo_2eproto();
  friend void protobuf_AssignDesc_NoticeInfo_2eproto();
  friend void protobuf_ShutdownFile_NoticeInfo_2eproto();
  
  void InitAsDefaultInstance();
  static AllNotice* default_instance_;
};
// ===================================================================


// ===================================================================

// NoticeInfo

// optional string name = 1;
inline bool NoticeInfo::has_name() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void NoticeInfo::set_has_name() {
  _has_bits_[0] |= 0x00000001u;
}
inline void NoticeInfo::clear_has_name() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void NoticeInfo::clear_name() {
  if (name_ != &::google::protobuf::internal::kEmptyString) {
    name_->clear();
  }
  clear_has_name();
}
inline const ::std::string& NoticeInfo::name() const {
  return *name_;
}
inline void NoticeInfo::set_name(const ::std::string& value) {
  set_has_name();
  if (name_ == &::google::protobuf::internal::kEmptyString) {
    name_ = new ::std::string;
  }
  name_->assign(value);
}
inline void NoticeInfo::set_name(const char* value) {
  set_has_name();
  if (name_ == &::google::protobuf::internal::kEmptyString) {
    name_ = new ::std::string;
  }
  name_->assign(value);
}
inline void NoticeInfo::set_name(const char* value, size_t size) {
  set_has_name();
  if (name_ == &::google::protobuf::internal::kEmptyString) {
    name_ = new ::std::string;
  }
  name_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* NoticeInfo::mutable_name() {
  set_has_name();
  if (name_ == &::google::protobuf::internal::kEmptyString) {
    name_ = new ::std::string;
  }
  return name_;
}
inline ::std::string* NoticeInfo::release_name() {
  clear_has_name();
  if (name_ == &::google::protobuf::internal::kEmptyString) {
    return NULL;
  } else {
    ::std::string* temp = name_;
    name_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
    return temp;
  }
}

// optional string context = 2;
inline bool NoticeInfo::has_context() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void NoticeInfo::set_has_context() {
  _has_bits_[0] |= 0x00000002u;
}
inline void NoticeInfo::clear_has_context() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void NoticeInfo::clear_context() {
  if (context_ != &::google::protobuf::internal::kEmptyString) {
    context_->clear();
  }
  clear_has_context();
}
inline const ::std::string& NoticeInfo::context() const {
  return *context_;
}
inline void NoticeInfo::set_context(const ::std::string& value) {
  set_has_context();
  if (context_ == &::google::protobuf::internal::kEmptyString) {
    context_ = new ::std::string;
  }
  context_->assign(value);
}
inline void NoticeInfo::set_context(const char* value) {
  set_has_context();
  if (context_ == &::google::protobuf::internal::kEmptyString) {
    context_ = new ::std::string;
  }
  context_->assign(value);
}
inline void NoticeInfo::set_context(const char* value, size_t size) {
  set_has_context();
  if (context_ == &::google::protobuf::internal::kEmptyString) {
    context_ = new ::std::string;
  }
  context_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* NoticeInfo::mutable_context() {
  set_has_context();
  if (context_ == &::google::protobuf::internal::kEmptyString) {
    context_ = new ::std::string;
  }
  return context_;
}
inline ::std::string* NoticeInfo::release_context() {
  clear_has_context();
  if (context_ == &::google::protobuf::internal::kEmptyString) {
    return NULL;
  } else {
    ::std::string* temp = context_;
    context_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
    return temp;
  }
}

// optional .NoticeInfo.BtnFunc btnFunc = 3;
inline bool NoticeInfo::has_btnFunc() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void NoticeInfo::set_has_btnFunc() {
  _has_bits_[0] |= 0x00000004u;
}
inline void NoticeInfo::clear_has_btnFunc() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void NoticeInfo::clear_btnFunc() {
  btnFunc_ = 0;
  clear_has_btnFunc();
}
inline ::NoticeInfo_BtnFunc NoticeInfo::btnFunc() const {
  return static_cast< ::NoticeInfo_BtnFunc >(btnFunc_);
}
inline void NoticeInfo::set_btnFunc(::NoticeInfo_BtnFunc value) {
  GOOGLE_DCHECK(::NoticeInfo_BtnFunc_IsValid(value));
  set_has_btnFunc();
  btnFunc_ = value;
}

// -------------------------------------------------------------------

// AllNotice

// repeated .NoticeInfo allNotice = 1;
inline int AllNotice::allNotice_size() const {
  return allNotice_.size();
}
inline void AllNotice::clear_allNotice() {
  allNotice_.Clear();
}
inline const ::NoticeInfo& AllNotice::allNotice(int index) const {
  return allNotice_.Get(index);
}
inline ::NoticeInfo* AllNotice::mutable_allNotice(int index) {
  return allNotice_.Mutable(index);
}
inline ::NoticeInfo* AllNotice::add_allNotice() {
  return allNotice_.Add();
}
inline const ::google::protobuf::RepeatedPtrField< ::NoticeInfo >&
AllNotice::allNotice() const {
  return allNotice_;
}
inline ::google::protobuf::RepeatedPtrField< ::NoticeInfo >*
AllNotice::mutable_allNotice() {
  return &allNotice_;
}


// @@protoc_insertion_point(namespace_scope)

#ifndef SWIG
namespace google {
namespace protobuf {

template <>
inline const EnumDescriptor* GetEnumDescriptor< ::NoticeInfo_BtnFunc>() {
  return ::NoticeInfo_BtnFunc_descriptor();
}

}  // namespace google
}  // namespace protobuf
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_NoticeInfo_2eproto__INCLUDED
