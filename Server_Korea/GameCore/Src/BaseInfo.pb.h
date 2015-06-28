// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: BaseInfo.proto

#ifndef PROTOBUF_BaseInfo_2eproto__INCLUDED
#define PROTOBUF_BaseInfo_2eproto__INCLUDED

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
#include "ProfessionInfo.pb.h"
// @@protoc_insertion_point(includes)

// Internal implementation detail -- do not call these.
void  protobuf_AddDesc_BaseInfo_2eproto();
void protobuf_AssignDesc_BaseInfo_2eproto();
void protobuf_ShutdownFile_BaseInfo_2eproto();

class PB_BaseAtt;

// ===================================================================

class PB_BaseAtt : public ::google::protobuf::Message {
 public:
  PB_BaseAtt();
  virtual ~PB_BaseAtt();
  
  PB_BaseAtt(const PB_BaseAtt& from);
  
  inline PB_BaseAtt& operator=(const PB_BaseAtt& from) {
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
  static const PB_BaseAtt& default_instance();
  
  void Swap(PB_BaseAtt* other);
  
  // implements Message ----------------------------------------------
  
  PB_BaseAtt* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const PB_BaseAtt& from);
  void MergeFrom(const PB_BaseAtt& from);
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
  
  // optional .PB_ProfessionInfo.Type professionType = 2;
  inline bool has_professionType() const;
  inline void clear_professionType();
  static const int kProfessionTypeFieldNumber = 2;
  inline ::PB_ProfessionInfo_Type professionType() const;
  inline void set_professionType(::PB_ProfessionInfo_Type value);
  
  // optional bool male = 3;
  inline bool has_male() const;
  inline void clear_male();
  static const int kMaleFieldNumber = 3;
  inline bool male() const;
  inline void set_male(bool value);
  
  // optional int64 roleID = 4;
  inline bool has_roleID() const;
  inline void clear_roleID();
  static const int kRoleIDFieldNumber = 4;
  inline ::google::protobuf::int64 roleID() const;
  inline void set_roleID(::google::protobuf::int64 value);
  
  // optional float height = 5;
  inline bool has_height() const;
  inline void clear_height();
  static const int kHeightFieldNumber = 5;
  inline float height() const;
  inline void set_height(float value);
  
  // optional bool passStory = 6;
  inline bool has_passStory() const;
  inline void clear_passStory();
  static const int kPassStoryFieldNumber = 6;
  inline bool passStory() const;
  inline void set_passStory(bool value);
  
  // @@protoc_insertion_point(class_scope:PB_BaseAtt)
 private:
  inline void set_has_name();
  inline void clear_has_name();
  inline void set_has_professionType();
  inline void clear_has_professionType();
  inline void set_has_male();
  inline void clear_has_male();
  inline void set_has_roleID();
  inline void clear_has_roleID();
  inline void set_has_height();
  inline void clear_has_height();
  inline void set_has_passStory();
  inline void clear_has_passStory();
  
  ::google::protobuf::UnknownFieldSet _unknown_fields_;
  
  ::std::string* name_;
  int professionType_;
  bool male_;
  bool passStory_;
  ::google::protobuf::int64 roleID_;
  float height_;
  
  mutable int _cached_size_;
  ::google::protobuf::uint32 _has_bits_[(6 + 31) / 32];
  
  friend void  protobuf_AddDesc_BaseInfo_2eproto();
  friend void protobuf_AssignDesc_BaseInfo_2eproto();
  friend void protobuf_ShutdownFile_BaseInfo_2eproto();
  
  void InitAsDefaultInstance();
  static PB_BaseAtt* default_instance_;
};
// ===================================================================


// ===================================================================

// PB_BaseAtt

// optional string name = 1;
inline bool PB_BaseAtt::has_name() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void PB_BaseAtt::set_has_name() {
  _has_bits_[0] |= 0x00000001u;
}
inline void PB_BaseAtt::clear_has_name() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void PB_BaseAtt::clear_name() {
  if (name_ != &::google::protobuf::internal::kEmptyString) {
    name_->clear();
  }
  clear_has_name();
}
inline const ::std::string& PB_BaseAtt::name() const {
  return *name_;
}
inline void PB_BaseAtt::set_name(const ::std::string& value) {
  set_has_name();
  if (name_ == &::google::protobuf::internal::kEmptyString) {
    name_ = new ::std::string;
  }
  name_->assign(value);
}
inline void PB_BaseAtt::set_name(const char* value) {
  set_has_name();
  if (name_ == &::google::protobuf::internal::kEmptyString) {
    name_ = new ::std::string;
  }
  name_->assign(value);
}
inline void PB_BaseAtt::set_name(const char* value, size_t size) {
  set_has_name();
  if (name_ == &::google::protobuf::internal::kEmptyString) {
    name_ = new ::std::string;
  }
  name_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* PB_BaseAtt::mutable_name() {
  set_has_name();
  if (name_ == &::google::protobuf::internal::kEmptyString) {
    name_ = new ::std::string;
  }
  return name_;
}
inline ::std::string* PB_BaseAtt::release_name() {
  clear_has_name();
  if (name_ == &::google::protobuf::internal::kEmptyString) {
    return NULL;
  } else {
    ::std::string* temp = name_;
    name_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
    return temp;
  }
}

// optional .PB_ProfessionInfo.Type professionType = 2;
inline bool PB_BaseAtt::has_professionType() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void PB_BaseAtt::set_has_professionType() {
  _has_bits_[0] |= 0x00000002u;
}
inline void PB_BaseAtt::clear_has_professionType() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void PB_BaseAtt::clear_professionType() {
  professionType_ = 0;
  clear_has_professionType();
}
inline ::PB_ProfessionInfo_Type PB_BaseAtt::professionType() const {
  return static_cast< ::PB_ProfessionInfo_Type >(professionType_);
}
inline void PB_BaseAtt::set_professionType(::PB_ProfessionInfo_Type value) {
  GOOGLE_DCHECK(::PB_ProfessionInfo_Type_IsValid(value));
  set_has_professionType();
  professionType_ = value;
}

// optional bool male = 3;
inline bool PB_BaseAtt::has_male() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void PB_BaseAtt::set_has_male() {
  _has_bits_[0] |= 0x00000004u;
}
inline void PB_BaseAtt::clear_has_male() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void PB_BaseAtt::clear_male() {
  male_ = false;
  clear_has_male();
}
inline bool PB_BaseAtt::male() const {
  return male_;
}
inline void PB_BaseAtt::set_male(bool value) {
  set_has_male();
  male_ = value;
}

// optional int64 roleID = 4;
inline bool PB_BaseAtt::has_roleID() const {
  return (_has_bits_[0] & 0x00000008u) != 0;
}
inline void PB_BaseAtt::set_has_roleID() {
  _has_bits_[0] |= 0x00000008u;
}
inline void PB_BaseAtt::clear_has_roleID() {
  _has_bits_[0] &= ~0x00000008u;
}
inline void PB_BaseAtt::clear_roleID() {
  roleID_ = GOOGLE_LONGLONG(0);
  clear_has_roleID();
}
inline ::google::protobuf::int64 PB_BaseAtt::roleID() const {
  return roleID_;
}
inline void PB_BaseAtt::set_roleID(::google::protobuf::int64 value) {
  set_has_roleID();
  roleID_ = value;
}

// optional float height = 5;
inline bool PB_BaseAtt::has_height() const {
  return (_has_bits_[0] & 0x00000010u) != 0;
}
inline void PB_BaseAtt::set_has_height() {
  _has_bits_[0] |= 0x00000010u;
}
inline void PB_BaseAtt::clear_has_height() {
  _has_bits_[0] &= ~0x00000010u;
}
inline void PB_BaseAtt::clear_height() {
  height_ = 0;
  clear_has_height();
}
inline float PB_BaseAtt::height() const {
  return height_;
}
inline void PB_BaseAtt::set_height(float value) {
  set_has_height();
  height_ = value;
}

// optional bool passStory = 6;
inline bool PB_BaseAtt::has_passStory() const {
  return (_has_bits_[0] & 0x00000020u) != 0;
}
inline void PB_BaseAtt::set_has_passStory() {
  _has_bits_[0] |= 0x00000020u;
}
inline void PB_BaseAtt::clear_has_passStory() {
  _has_bits_[0] &= ~0x00000020u;
}
inline void PB_BaseAtt::clear_passStory() {
  passStory_ = false;
  clear_has_passStory();
}
inline bool PB_BaseAtt::passStory() const {
  return passStory_;
}
inline void PB_BaseAtt::set_passStory(bool value) {
  set_has_passStory();
  passStory_ = value;
}


// @@protoc_insertion_point(namespace_scope)

#ifndef SWIG
namespace google {
namespace protobuf {


}  // namespace google
}  // namespace protobuf
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_BaseInfo_2eproto__INCLUDED