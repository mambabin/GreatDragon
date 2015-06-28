// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: BoxInfo.proto

#ifndef PROTOBUF_BoxInfo_2eproto__INCLUDED
#define PROTOBUF_BoxInfo_2eproto__INCLUDED

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
void  protobuf_AddDesc_BoxInfo_2eproto();
void protobuf_AssignDesc_BoxInfo_2eproto();
void protobuf_ShutdownFile_BoxInfo_2eproto();

class BoxInfo;
class AllBoxes;

enum BoxInfo_Type {
  BoxInfo_Type_NONE = 0,
  BoxInfo_Type_EXP = 1,
  BoxInfo_Type_MONEY = 2,
  BoxInfo_Type_GOODS = 3,
  BoxInfo_Type_EQUIPMENT = 4,
  BoxInfo_Type_RMB = 5,
  BoxInfo_Type_SOUL = 6,
  BoxInfo_Type_SOULJADE = 7,
  BoxInfo_Type_SOULSTONE = 8,
  BoxInfo_Type_DESIGNATION = 9,
  BoxInfo_Type_FASHION = 10,
  BoxInfo_Type_SUBRMB = 11,
  BoxInfo_Type_WING = 12,
  BoxInfo_Type_GODSCORE = 13,
  BoxInfo_Type_TRANSFORM = 14,
  BoxInfo_Type_GODSHIP = 15,
  BoxInfo_Type_RIDES_FOOD = 16,
  BoxInfo_Type_RIDES = 17
};
bool BoxInfo_Type_IsValid(int value);
const BoxInfo_Type BoxInfo_Type_Type_MIN = BoxInfo_Type_NONE;
const BoxInfo_Type BoxInfo_Type_Type_MAX = BoxInfo_Type_RIDES;
const int BoxInfo_Type_Type_ARRAYSIZE = BoxInfo_Type_Type_MAX + 1;

const ::google::protobuf::EnumDescriptor* BoxInfo_Type_descriptor();
inline const ::std::string& BoxInfo_Type_Name(BoxInfo_Type value) {
  return ::google::protobuf::internal::NameOfEnum(
    BoxInfo_Type_descriptor(), value);
}
inline bool BoxInfo_Type_Parse(
    const ::std::string& name, BoxInfo_Type* value) {
  return ::google::protobuf::internal::ParseNamedEnum<BoxInfo_Type>(
    BoxInfo_Type_descriptor(), name, value);
}
// ===================================================================

class BoxInfo : public ::google::protobuf::Message {
 public:
  BoxInfo();
  virtual ~BoxInfo();
  
  BoxInfo(const BoxInfo& from);
  
  inline BoxInfo& operator=(const BoxInfo& from) {
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
  static const BoxInfo& default_instance();
  
  void Swap(BoxInfo* other);
  
  // implements Message ----------------------------------------------
  
  BoxInfo* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const BoxInfo& from);
  void MergeFrom(const BoxInfo& from);
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
  
  typedef BoxInfo_Type Type;
  static const Type NONE = BoxInfo_Type_NONE;
  static const Type EXP = BoxInfo_Type_EXP;
  static const Type MONEY = BoxInfo_Type_MONEY;
  static const Type GOODS = BoxInfo_Type_GOODS;
  static const Type EQUIPMENT = BoxInfo_Type_EQUIPMENT;
  static const Type RMB = BoxInfo_Type_RMB;
  static const Type SOUL = BoxInfo_Type_SOUL;
  static const Type SOULJADE = BoxInfo_Type_SOULJADE;
  static const Type SOULSTONE = BoxInfo_Type_SOULSTONE;
  static const Type DESIGNATION = BoxInfo_Type_DESIGNATION;
  static const Type FASHION = BoxInfo_Type_FASHION;
  static const Type SUBRMB = BoxInfo_Type_SUBRMB;
  static const Type WING = BoxInfo_Type_WING;
  static const Type GODSCORE = BoxInfo_Type_GODSCORE;
  static const Type TRANSFORM = BoxInfo_Type_TRANSFORM;
  static const Type GODSHIP = BoxInfo_Type_GODSHIP;
  static const Type RIDES_FOOD = BoxInfo_Type_RIDES_FOOD;
  static const Type RIDES = BoxInfo_Type_RIDES;
  static inline bool Type_IsValid(int value) {
    return BoxInfo_Type_IsValid(value);
  }
  static const Type Type_MIN =
    BoxInfo_Type_Type_MIN;
  static const Type Type_MAX =
    BoxInfo_Type_Type_MAX;
  static const int Type_ARRAYSIZE =
    BoxInfo_Type_Type_ARRAYSIZE;
  static inline const ::google::protobuf::EnumDescriptor*
  Type_descriptor() {
    return BoxInfo_Type_descriptor();
  }
  static inline const ::std::string& Type_Name(Type value) {
    return BoxInfo_Type_Name(value);
  }
  static inline bool Type_Parse(const ::std::string& name,
      Type* value) {
    return BoxInfo_Type_Parse(name, value);
  }
  
  // accessors -------------------------------------------------------
  
  // optional int32 id = 1;
  inline bool has_id() const;
  inline void clear_id();
  static const int kIdFieldNumber = 1;
  inline ::google::protobuf::int32 id() const;
  inline void set_id(::google::protobuf::int32 value);
  
  // optional int32 num = 2;
  inline bool has_num() const;
  inline void clear_num();
  static const int kNumFieldNumber = 2;
  inline ::google::protobuf::int32 num() const;
  inline void set_num(::google::protobuf::int32 value);
  
  // optional bool prof = 3;
  inline bool has_prof() const;
  inline void clear_prof();
  static const int kProfFieldNumber = 3;
  inline bool prof() const;
  inline void set_prof(bool value);
  
  // repeated .BoxInfo.Type type = 4;
  inline int type_size() const;
  inline void clear_type();
  static const int kTypeFieldNumber = 4;
  inline ::BoxInfo_Type type(int index) const;
  inline void set_type(int index, ::BoxInfo_Type value);
  inline void add_type(::BoxInfo_Type value);
  inline const ::google::protobuf::RepeatedField<int>& type() const;
  inline ::google::protobuf::RepeatedField<int>* mutable_type();
  
  // repeated int32 arg1 = 5;
  inline int arg1_size() const;
  inline void clear_arg1();
  static const int kArg1FieldNumber = 5;
  inline ::google::protobuf::int32 arg1(int index) const;
  inline void set_arg1(int index, ::google::protobuf::int32 value);
  inline void add_arg1(::google::protobuf::int32 value);
  inline const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&
      arg1() const;
  inline ::google::protobuf::RepeatedField< ::google::protobuf::int32 >*
      mutable_arg1();
  
  // repeated int32 arg2 = 6;
  inline int arg2_size() const;
  inline void clear_arg2();
  static const int kArg2FieldNumber = 6;
  inline ::google::protobuf::int32 arg2(int index) const;
  inline void set_arg2(int index, ::google::protobuf::int32 value);
  inline void add_arg2(::google::protobuf::int32 value);
  inline const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&
      arg2() const;
  inline ::google::protobuf::RepeatedField< ::google::protobuf::int32 >*
      mutable_arg2();
  
  // repeated int32 arg3 = 7;
  inline int arg3_size() const;
  inline void clear_arg3();
  static const int kArg3FieldNumber = 7;
  inline ::google::protobuf::int32 arg3(int index) const;
  inline void set_arg3(int index, ::google::protobuf::int32 value);
  inline void add_arg3(::google::protobuf::int32 value);
  inline const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&
      arg3() const;
  inline ::google::protobuf::RepeatedField< ::google::protobuf::int32 >*
      mutable_arg3();
  
  // @@protoc_insertion_point(class_scope:BoxInfo)
 private:
  inline void set_has_id();
  inline void clear_has_id();
  inline void set_has_num();
  inline void clear_has_num();
  inline void set_has_prof();
  inline void clear_has_prof();
  
  ::google::protobuf::UnknownFieldSet _unknown_fields_;
  
  ::google::protobuf::int32 id_;
  ::google::protobuf::int32 num_;
  ::google::protobuf::RepeatedField<int> type_;
  ::google::protobuf::RepeatedField< ::google::protobuf::int32 > arg1_;
  ::google::protobuf::RepeatedField< ::google::protobuf::int32 > arg2_;
  ::google::protobuf::RepeatedField< ::google::protobuf::int32 > arg3_;
  bool prof_;
  
  mutable int _cached_size_;
  ::google::protobuf::uint32 _has_bits_[(7 + 31) / 32];
  
  friend void  protobuf_AddDesc_BoxInfo_2eproto();
  friend void protobuf_AssignDesc_BoxInfo_2eproto();
  friend void protobuf_ShutdownFile_BoxInfo_2eproto();
  
  void InitAsDefaultInstance();
  static BoxInfo* default_instance_;
};
// -------------------------------------------------------------------

class AllBoxes : public ::google::protobuf::Message {
 public:
  AllBoxes();
  virtual ~AllBoxes();
  
  AllBoxes(const AllBoxes& from);
  
  inline AllBoxes& operator=(const AllBoxes& from) {
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
  static const AllBoxes& default_instance();
  
  void Swap(AllBoxes* other);
  
  // implements Message ----------------------------------------------
  
  AllBoxes* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const AllBoxes& from);
  void MergeFrom(const AllBoxes& from);
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
  
  // repeated .BoxInfo boxes = 1;
  inline int boxes_size() const;
  inline void clear_boxes();
  static const int kBoxesFieldNumber = 1;
  inline const ::BoxInfo& boxes(int index) const;
  inline ::BoxInfo* mutable_boxes(int index);
  inline ::BoxInfo* add_boxes();
  inline const ::google::protobuf::RepeatedPtrField< ::BoxInfo >&
      boxes() const;
  inline ::google::protobuf::RepeatedPtrField< ::BoxInfo >*
      mutable_boxes();
  
  // @@protoc_insertion_point(class_scope:AllBoxes)
 private:
  
  ::google::protobuf::UnknownFieldSet _unknown_fields_;
  
  ::google::protobuf::RepeatedPtrField< ::BoxInfo > boxes_;
  
  mutable int _cached_size_;
  ::google::protobuf::uint32 _has_bits_[(1 + 31) / 32];
  
  friend void  protobuf_AddDesc_BoxInfo_2eproto();
  friend void protobuf_AssignDesc_BoxInfo_2eproto();
  friend void protobuf_ShutdownFile_BoxInfo_2eproto();
  
  void InitAsDefaultInstance();
  static AllBoxes* default_instance_;
};
// ===================================================================


// ===================================================================

// BoxInfo

// optional int32 id = 1;
inline bool BoxInfo::has_id() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void BoxInfo::set_has_id() {
  _has_bits_[0] |= 0x00000001u;
}
inline void BoxInfo::clear_has_id() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void BoxInfo::clear_id() {
  id_ = 0;
  clear_has_id();
}
inline ::google::protobuf::int32 BoxInfo::id() const {
  return id_;
}
inline void BoxInfo::set_id(::google::protobuf::int32 value) {
  set_has_id();
  id_ = value;
}

// optional int32 num = 2;
inline bool BoxInfo::has_num() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void BoxInfo::set_has_num() {
  _has_bits_[0] |= 0x00000002u;
}
inline void BoxInfo::clear_has_num() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void BoxInfo::clear_num() {
  num_ = 0;
  clear_has_num();
}
inline ::google::protobuf::int32 BoxInfo::num() const {
  return num_;
}
inline void BoxInfo::set_num(::google::protobuf::int32 value) {
  set_has_num();
  num_ = value;
}

// optional bool prof = 3;
inline bool BoxInfo::has_prof() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void BoxInfo::set_has_prof() {
  _has_bits_[0] |= 0x00000004u;
}
inline void BoxInfo::clear_has_prof() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void BoxInfo::clear_prof() {
  prof_ = false;
  clear_has_prof();
}
inline bool BoxInfo::prof() const {
  return prof_;
}
inline void BoxInfo::set_prof(bool value) {
  set_has_prof();
  prof_ = value;
}

// repeated .BoxInfo.Type type = 4;
inline int BoxInfo::type_size() const {
  return type_.size();
}
inline void BoxInfo::clear_type() {
  type_.Clear();
}
inline ::BoxInfo_Type BoxInfo::type(int index) const {
  return static_cast< ::BoxInfo_Type >(type_.Get(index));
}
inline void BoxInfo::set_type(int index, ::BoxInfo_Type value) {
  GOOGLE_DCHECK(::BoxInfo_Type_IsValid(value));
  type_.Set(index, value);
}
inline void BoxInfo::add_type(::BoxInfo_Type value) {
  GOOGLE_DCHECK(::BoxInfo_Type_IsValid(value));
  type_.Add(value);
}
inline const ::google::protobuf::RepeatedField<int>&
BoxInfo::type() const {
  return type_;
}
inline ::google::protobuf::RepeatedField<int>*
BoxInfo::mutable_type() {
  return &type_;
}

// repeated int32 arg1 = 5;
inline int BoxInfo::arg1_size() const {
  return arg1_.size();
}
inline void BoxInfo::clear_arg1() {
  arg1_.Clear();
}
inline ::google::protobuf::int32 BoxInfo::arg1(int index) const {
  return arg1_.Get(index);
}
inline void BoxInfo::set_arg1(int index, ::google::protobuf::int32 value) {
  arg1_.Set(index, value);
}
inline void BoxInfo::add_arg1(::google::protobuf::int32 value) {
  arg1_.Add(value);
}
inline const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&
BoxInfo::arg1() const {
  return arg1_;
}
inline ::google::protobuf::RepeatedField< ::google::protobuf::int32 >*
BoxInfo::mutable_arg1() {
  return &arg1_;
}

// repeated int32 arg2 = 6;
inline int BoxInfo::arg2_size() const {
  return arg2_.size();
}
inline void BoxInfo::clear_arg2() {
  arg2_.Clear();
}
inline ::google::protobuf::int32 BoxInfo::arg2(int index) const {
  return arg2_.Get(index);
}
inline void BoxInfo::set_arg2(int index, ::google::protobuf::int32 value) {
  arg2_.Set(index, value);
}
inline void BoxInfo::add_arg2(::google::protobuf::int32 value) {
  arg2_.Add(value);
}
inline const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&
BoxInfo::arg2() const {
  return arg2_;
}
inline ::google::protobuf::RepeatedField< ::google::protobuf::int32 >*
BoxInfo::mutable_arg2() {
  return &arg2_;
}

// repeated int32 arg3 = 7;
inline int BoxInfo::arg3_size() const {
  return arg3_.size();
}
inline void BoxInfo::clear_arg3() {
  arg3_.Clear();
}
inline ::google::protobuf::int32 BoxInfo::arg3(int index) const {
  return arg3_.Get(index);
}
inline void BoxInfo::set_arg3(int index, ::google::protobuf::int32 value) {
  arg3_.Set(index, value);
}
inline void BoxInfo::add_arg3(::google::protobuf::int32 value) {
  arg3_.Add(value);
}
inline const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&
BoxInfo::arg3() const {
  return arg3_;
}
inline ::google::protobuf::RepeatedField< ::google::protobuf::int32 >*
BoxInfo::mutable_arg3() {
  return &arg3_;
}

// -------------------------------------------------------------------

// AllBoxes

// repeated .BoxInfo boxes = 1;
inline int AllBoxes::boxes_size() const {
  return boxes_.size();
}
inline void AllBoxes::clear_boxes() {
  boxes_.Clear();
}
inline const ::BoxInfo& AllBoxes::boxes(int index) const {
  return boxes_.Get(index);
}
inline ::BoxInfo* AllBoxes::mutable_boxes(int index) {
  return boxes_.Mutable(index);
}
inline ::BoxInfo* AllBoxes::add_boxes() {
  return boxes_.Add();
}
inline const ::google::protobuf::RepeatedPtrField< ::BoxInfo >&
AllBoxes::boxes() const {
  return boxes_;
}
inline ::google::protobuf::RepeatedPtrField< ::BoxInfo >*
AllBoxes::mutable_boxes() {
  return &boxes_;
}


// @@protoc_insertion_point(namespace_scope)

#ifndef SWIG
namespace google {
namespace protobuf {

template <>
inline const EnumDescriptor* GetEnumDescriptor< ::BoxInfo_Type>() {
  return ::BoxInfo_Type_descriptor();
}

}  // namespace google
}  // namespace protobuf
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_BoxInfo_2eproto__INCLUDED
