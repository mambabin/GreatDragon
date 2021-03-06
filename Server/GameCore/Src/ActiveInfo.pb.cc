// Generated by the protocol buffer compiler.  DO NOT EDIT!

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "ActiveInfo.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace {

const ::google::protobuf::Descriptor* ActiveInfo_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  ActiveInfo_reflection_ = NULL;
const ::google::protobuf::Descriptor* AllActives_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  AllActives_reflection_ = NULL;

}  // namespace


void protobuf_AssignDesc_ActiveInfo_2eproto() {
  protobuf_AddDesc_ActiveInfo_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "ActiveInfo.proto");
  GOOGLE_CHECK(file != NULL);
  ActiveInfo_descriptor_ = file->message_type(0);
  static const int ActiveInfo_offsets_[3] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(ActiveInfo, id_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(ActiveInfo, beginTime_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(ActiveInfo, endTime_),
  };
  ActiveInfo_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      ActiveInfo_descriptor_,
      ActiveInfo::default_instance_,
      ActiveInfo_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(ActiveInfo, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(ActiveInfo, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(ActiveInfo));
  AllActives_descriptor_ = file->message_type(1);
  static const int AllActives_offsets_[1] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(AllActives, allActives_),
  };
  AllActives_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      AllActives_descriptor_,
      AllActives::default_instance_,
      AllActives_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(AllActives, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(AllActives, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(AllActives));
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_ActiveInfo_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    ActiveInfo_descriptor_, &ActiveInfo::default_instance());
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    AllActives_descriptor_, &AllActives::default_instance());
}

}  // namespace

void protobuf_ShutdownFile_ActiveInfo_2eproto() {
  delete ActiveInfo::default_instance_;
  delete ActiveInfo_reflection_;
  delete AllActives::default_instance_;
  delete AllActives_reflection_;
}

void protobuf_AddDesc_ActiveInfo_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n\020ActiveInfo.proto\"<\n\nActiveInfo\022\n\n\002id\030\001"
    " \001(\005\022\021\n\tbeginTime\030\002 \003(\005\022\017\n\007endTime\030\003 \003(\005"
    "\"-\n\nAllActives\022\037\n\nallActives\030\001 \003(\0132\013.Act"
    "iveInfo", 127);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "ActiveInfo.proto", &protobuf_RegisterTypes);
  ActiveInfo::default_instance_ = new ActiveInfo();
  AllActives::default_instance_ = new AllActives();
  ActiveInfo::default_instance_->InitAsDefaultInstance();
  AllActives::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_ActiveInfo_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_ActiveInfo_2eproto {
  StaticDescriptorInitializer_ActiveInfo_2eproto() {
    protobuf_AddDesc_ActiveInfo_2eproto();
  }
} static_descriptor_initializer_ActiveInfo_2eproto_;


// ===================================================================

#ifndef _MSC_VER
const int ActiveInfo::kIdFieldNumber;
const int ActiveInfo::kBeginTimeFieldNumber;
const int ActiveInfo::kEndTimeFieldNumber;
#endif  // !_MSC_VER

ActiveInfo::ActiveInfo()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void ActiveInfo::InitAsDefaultInstance() {
}

ActiveInfo::ActiveInfo(const ActiveInfo& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void ActiveInfo::SharedCtor() {
  _cached_size_ = 0;
  id_ = 0;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

ActiveInfo::~ActiveInfo() {
  SharedDtor();
}

void ActiveInfo::SharedDtor() {
  if (this != default_instance_) {
  }
}

void ActiveInfo::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* ActiveInfo::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return ActiveInfo_descriptor_;
}

const ActiveInfo& ActiveInfo::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_ActiveInfo_2eproto();  return *default_instance_;
}

ActiveInfo* ActiveInfo::default_instance_ = NULL;

ActiveInfo* ActiveInfo::New() const {
  return new ActiveInfo;
}

void ActiveInfo::Clear() {
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    id_ = 0;
  }
  beginTime_.Clear();
  endTime_.Clear();
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool ActiveInfo::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // optional int32 id = 1;
      case 1: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &id_)));
          set_has_id();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(16)) goto parse_beginTime;
        break;
      }
      
      // repeated int32 beginTime = 2;
      case 2: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_beginTime:
          DO_((::google::protobuf::internal::WireFormatLite::ReadRepeatedPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 1, 16, input, this->mutable_beginTime())));
        } else if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag)
                   == ::google::protobuf::internal::WireFormatLite::
                      WIRETYPE_LENGTH_DELIMITED) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPackedPrimitiveNoInline<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, this->mutable_beginTime())));
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(16)) goto parse_beginTime;
        if (input->ExpectTag(24)) goto parse_endTime;
        break;
      }
      
      // repeated int32 endTime = 3;
      case 3: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_endTime:
          DO_((::google::protobuf::internal::WireFormatLite::ReadRepeatedPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 1, 24, input, this->mutable_endTime())));
        } else if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag)
                   == ::google::protobuf::internal::WireFormatLite::
                      WIRETYPE_LENGTH_DELIMITED) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPackedPrimitiveNoInline<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, this->mutable_endTime())));
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(24)) goto parse_endTime;
        if (input->ExpectAtEnd()) return true;
        break;
      }
      
      default: {
      handle_uninterpreted:
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          return true;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
  return true;
#undef DO_
}

void ActiveInfo::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // optional int32 id = 1;
  if (has_id()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(1, this->id(), output);
  }
  
  // repeated int32 beginTime = 2;
  for (int i = 0; i < this->beginTime_size(); i++) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(
      2, this->beginTime(i), output);
  }
  
  // repeated int32 endTime = 3;
  for (int i = 0; i < this->endTime_size(); i++) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(
      3, this->endTime(i), output);
  }
  
  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* ActiveInfo::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // optional int32 id = 1;
  if (has_id()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(1, this->id(), target);
  }
  
  // repeated int32 beginTime = 2;
  for (int i = 0; i < this->beginTime_size(); i++) {
    target = ::google::protobuf::internal::WireFormatLite::
      WriteInt32ToArray(2, this->beginTime(i), target);
  }
  
  // repeated int32 endTime = 3;
  for (int i = 0; i < this->endTime_size(); i++) {
    target = ::google::protobuf::internal::WireFormatLite::
      WriteInt32ToArray(3, this->endTime(i), target);
  }
  
  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int ActiveInfo::ByteSize() const {
  int total_size = 0;
  
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // optional int32 id = 1;
    if (has_id()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->id());
    }
    
  }
  // repeated int32 beginTime = 2;
  {
    int data_size = 0;
    for (int i = 0; i < this->beginTime_size(); i++) {
      data_size += ::google::protobuf::internal::WireFormatLite::
        Int32Size(this->beginTime(i));
    }
    total_size += 1 * this->beginTime_size() + data_size;
  }
  
  // repeated int32 endTime = 3;
  {
    int data_size = 0;
    for (int i = 0; i < this->endTime_size(); i++) {
      data_size += ::google::protobuf::internal::WireFormatLite::
        Int32Size(this->endTime(i));
    }
    total_size += 1 * this->endTime_size() + data_size;
  }
  
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void ActiveInfo::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const ActiveInfo* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const ActiveInfo*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void ActiveInfo::MergeFrom(const ActiveInfo& from) {
  GOOGLE_CHECK_NE(&from, this);
  beginTime_.MergeFrom(from.beginTime_);
  endTime_.MergeFrom(from.endTime_);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_id()) {
      set_id(from.id());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void ActiveInfo::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void ActiveInfo::CopyFrom(const ActiveInfo& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool ActiveInfo::IsInitialized() const {
  
  return true;
}

void ActiveInfo::Swap(ActiveInfo* other) {
  if (other != this) {
    std::swap(id_, other->id_);
    beginTime_.Swap(&other->beginTime_);
    endTime_.Swap(&other->endTime_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata ActiveInfo::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = ActiveInfo_descriptor_;
  metadata.reflection = ActiveInfo_reflection_;
  return metadata;
}


// ===================================================================

#ifndef _MSC_VER
const int AllActives::kAllActivesFieldNumber;
#endif  // !_MSC_VER

AllActives::AllActives()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void AllActives::InitAsDefaultInstance() {
}

AllActives::AllActives(const AllActives& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void AllActives::SharedCtor() {
  _cached_size_ = 0;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

AllActives::~AllActives() {
  SharedDtor();
}

void AllActives::SharedDtor() {
  if (this != default_instance_) {
  }
}

void AllActives::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* AllActives::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return AllActives_descriptor_;
}

const AllActives& AllActives::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_ActiveInfo_2eproto();  return *default_instance_;
}

AllActives* AllActives::default_instance_ = NULL;

AllActives* AllActives::New() const {
  return new AllActives;
}

void AllActives::Clear() {
  allActives_.Clear();
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool AllActives::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // repeated .ActiveInfo allActives = 1;
      case 1: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_allActives:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
                input, add_allActives()));
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(10)) goto parse_allActives;
        if (input->ExpectAtEnd()) return true;
        break;
      }
      
      default: {
      handle_uninterpreted:
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          return true;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
  return true;
#undef DO_
}

void AllActives::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // repeated .ActiveInfo allActives = 1;
  for (int i = 0; i < this->allActives_size(); i++) {
    ::google::protobuf::internal::WireFormatLite::WriteMessageMaybeToArray(
      1, this->allActives(i), output);
  }
  
  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* AllActives::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // repeated .ActiveInfo allActives = 1;
  for (int i = 0; i < this->allActives_size(); i++) {
    target = ::google::protobuf::internal::WireFormatLite::
      WriteMessageNoVirtualToArray(
        1, this->allActives(i), target);
  }
  
  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int AllActives::ByteSize() const {
  int total_size = 0;
  
  // repeated .ActiveInfo allActives = 1;
  total_size += 1 * this->allActives_size();
  for (int i = 0; i < this->allActives_size(); i++) {
    total_size +=
      ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
        this->allActives(i));
  }
  
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void AllActives::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const AllActives* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const AllActives*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void AllActives::MergeFrom(const AllActives& from) {
  GOOGLE_CHECK_NE(&from, this);
  allActives_.MergeFrom(from.allActives_);
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void AllActives::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void AllActives::CopyFrom(const AllActives& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool AllActives::IsInitialized() const {
  
  return true;
}

void AllActives::Swap(AllActives* other) {
  if (other != this) {
    allActives_.Swap(&other->allActives_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata AllActives::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = AllActives_descriptor_;
  metadata.reflection = AllActives_reflection_;
  return metadata;
}


// @@protoc_insertion_point(namespace_scope)

// @@protoc_insertion_point(global_scope)
