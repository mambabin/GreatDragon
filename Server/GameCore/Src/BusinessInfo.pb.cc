// Generated by the protocol buffer compiler.  DO NOT EDIT!

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "BusinessInfo.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace {

const ::google::protobuf::Descriptor* BusinessUnit_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  BusinessUnit_reflection_ = NULL;
const ::google::protobuf::EnumDescriptor* BusinessUnit_LimitType_descriptor_ = NULL;
const ::google::protobuf::Descriptor* BusinessInfo_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  BusinessInfo_reflection_ = NULL;
const ::google::protobuf::EnumDescriptor* BusinessInfo_CurrencyType_descriptor_ = NULL;
const ::google::protobuf::Descriptor* AllBusiness_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  AllBusiness_reflection_ = NULL;

}  // namespace


void protobuf_AssignDesc_BusinessInfo_2eproto() {
  protobuf_AddDesc_BusinessInfo_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "BusinessInfo.proto");
  GOOGLE_CHECK(file != NULL);
  BusinessUnit_descriptor_ = file->message_type(0);
  static const int BusinessUnit_offsets_[8] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(BusinessUnit, type_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(BusinessUnit, id_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(BusinessUnit, count_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(BusinessUnit, effect_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(BusinessUnit, begin_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(BusinessUnit, end_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(BusinessUnit, limitType_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(BusinessUnit, arg_),
  };
  BusinessUnit_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      BusinessUnit_descriptor_,
      BusinessUnit::default_instance_,
      BusinessUnit_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(BusinessUnit, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(BusinessUnit, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(BusinessUnit));
  BusinessUnit_LimitType_descriptor_ = BusinessUnit_descriptor_->enum_type(0);
  BusinessInfo_descriptor_ = file->message_type(1);
  static const int BusinessInfo_offsets_[3] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(BusinessInfo, id_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(BusinessInfo, currencyType_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(BusinessInfo, items_),
  };
  BusinessInfo_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      BusinessInfo_descriptor_,
      BusinessInfo::default_instance_,
      BusinessInfo_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(BusinessInfo, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(BusinessInfo, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(BusinessInfo));
  BusinessInfo_CurrencyType_descriptor_ = BusinessInfo_descriptor_->enum_type(0);
  AllBusiness_descriptor_ = file->message_type(2);
  static const int AllBusiness_offsets_[1] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(AllBusiness, business_),
  };
  AllBusiness_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      AllBusiness_descriptor_,
      AllBusiness::default_instance_,
      AllBusiness_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(AllBusiness, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(AllBusiness, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(AllBusiness));
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_BusinessInfo_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    BusinessUnit_descriptor_, &BusinessUnit::default_instance());
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    BusinessInfo_descriptor_, &BusinessInfo::default_instance());
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    AllBusiness_descriptor_, &AllBusiness::default_instance());
}

}  // namespace

void protobuf_ShutdownFile_BusinessInfo_2eproto() {
  delete BusinessUnit::default_instance_;
  delete BusinessUnit_reflection_;
  delete BusinessInfo::default_instance_;
  delete BusinessInfo_reflection_;
  delete AllBusiness::default_instance_;
  delete AllBusiness_reflection_;
}

void protobuf_AddDesc_BusinessInfo_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::protobuf_AddDesc_ItemBaseInfo_2eproto();
  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n\022BusinessInfo.proto\032\022ItemBaseInfo.proto"
    "\"\365\001\n\014BusinessUnit\022\037\n\004type\030\001 \001(\0162\021.PB_Ite"
    "mInfo.Type\022\n\n\002id\030\002 \001(\005\022\r\n\005count\030\003 \001(\005\022\016\n"
    "\006effect\030\004 \001(\005\022\r\n\005begin\030\005 \001(\005\022\013\n\003end\030\006 \001("
    "\005\022*\n\tlimitType\030\007 \001(\0162\027.BusinessUnit.Limi"
    "tType\022\013\n\003arg\030\010 \001(\005\"D\n\tLimitType\022\010\n\004NONE\020"
    "\000\022\014\n\010ROLE_DAY\020\001\022\017\n\013ROLE_CAREER\020\002\022\016\n\nSERV"
    "ER_DAY\020\003\"\353\001\n\014BusinessInfo\022\n\n\002id\030\001 \001(\005\0220\n"
    "\014currencyType\030\002 \001(\0162\032.BusinessInfo.Curre"
    "ncyType\022\034\n\005items\030\003 \003(\0132\r.BusinessUnit\"\177\n"
    "\014CurrencyType\022\010\n\004NONE\020\000\022\007\n\003RMB\020\001\022\013\n\007SUB_"
    "RMB\020\002\022\t\n\005MONEY\020\003\022\016\n\nLOVE_POINT\020\004\022\r\n\tPVP_"
    "SCORE\020\005\022\r\n\tGOD_SCORE\020\006\022\026\n\022FACTION_CONTRI"
    "BUTE\020\007\".\n\013AllBusiness\022\037\n\010business\030\001 \003(\0132"
    "\r.BusinessInfo", 574);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "BusinessInfo.proto", &protobuf_RegisterTypes);
  BusinessUnit::default_instance_ = new BusinessUnit();
  BusinessInfo::default_instance_ = new BusinessInfo();
  AllBusiness::default_instance_ = new AllBusiness();
  BusinessUnit::default_instance_->InitAsDefaultInstance();
  BusinessInfo::default_instance_->InitAsDefaultInstance();
  AllBusiness::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_BusinessInfo_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_BusinessInfo_2eproto {
  StaticDescriptorInitializer_BusinessInfo_2eproto() {
    protobuf_AddDesc_BusinessInfo_2eproto();
  }
} static_descriptor_initializer_BusinessInfo_2eproto_;


// ===================================================================

const ::google::protobuf::EnumDescriptor* BusinessUnit_LimitType_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return BusinessUnit_LimitType_descriptor_;
}
bool BusinessUnit_LimitType_IsValid(int value) {
  switch(value) {
    case 0:
    case 1:
    case 2:
    case 3:
      return true;
    default:
      return false;
  }
}

#ifndef _MSC_VER
const BusinessUnit_LimitType BusinessUnit::NONE;
const BusinessUnit_LimitType BusinessUnit::ROLE_DAY;
const BusinessUnit_LimitType BusinessUnit::ROLE_CAREER;
const BusinessUnit_LimitType BusinessUnit::SERVER_DAY;
const BusinessUnit_LimitType BusinessUnit::LimitType_MIN;
const BusinessUnit_LimitType BusinessUnit::LimitType_MAX;
const int BusinessUnit::LimitType_ARRAYSIZE;
#endif  // _MSC_VER
#ifndef _MSC_VER
const int BusinessUnit::kTypeFieldNumber;
const int BusinessUnit::kIdFieldNumber;
const int BusinessUnit::kCountFieldNumber;
const int BusinessUnit::kEffectFieldNumber;
const int BusinessUnit::kBeginFieldNumber;
const int BusinessUnit::kEndFieldNumber;
const int BusinessUnit::kLimitTypeFieldNumber;
const int BusinessUnit::kArgFieldNumber;
#endif  // !_MSC_VER

BusinessUnit::BusinessUnit()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void BusinessUnit::InitAsDefaultInstance() {
}

BusinessUnit::BusinessUnit(const BusinessUnit& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void BusinessUnit::SharedCtor() {
  _cached_size_ = 0;
  type_ = 0;
  id_ = 0;
  count_ = 0;
  effect_ = 0;
  begin_ = 0;
  end_ = 0;
  limitType_ = 0;
  arg_ = 0;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

BusinessUnit::~BusinessUnit() {
  SharedDtor();
}

void BusinessUnit::SharedDtor() {
  if (this != default_instance_) {
  }
}

void BusinessUnit::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* BusinessUnit::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return BusinessUnit_descriptor_;
}

const BusinessUnit& BusinessUnit::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_BusinessInfo_2eproto();  return *default_instance_;
}

BusinessUnit* BusinessUnit::default_instance_ = NULL;

BusinessUnit* BusinessUnit::New() const {
  return new BusinessUnit;
}

void BusinessUnit::Clear() {
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    type_ = 0;
    id_ = 0;
    count_ = 0;
    effect_ = 0;
    begin_ = 0;
    end_ = 0;
    limitType_ = 0;
    arg_ = 0;
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool BusinessUnit::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // optional .PB_ItemInfo.Type type = 1;
      case 1: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
          int value;
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   int, ::google::protobuf::internal::WireFormatLite::TYPE_ENUM>(
                 input, &value)));
          if (::PB_ItemInfo_Type_IsValid(value)) {
            set_type(static_cast< ::PB_ItemInfo_Type >(value));
          } else {
            mutable_unknown_fields()->AddVarint(1, value);
          }
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(16)) goto parse_id;
        break;
      }
      
      // optional int32 id = 2;
      case 2: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_id:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &id_)));
          set_has_id();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(24)) goto parse_count;
        break;
      }
      
      // optional int32 count = 3;
      case 3: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_count:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &count_)));
          set_has_count();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(32)) goto parse_effect;
        break;
      }
      
      // optional int32 effect = 4;
      case 4: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_effect:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &effect_)));
          set_has_effect();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(40)) goto parse_begin;
        break;
      }
      
      // optional int32 begin = 5;
      case 5: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_begin:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &begin_)));
          set_has_begin();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(48)) goto parse_end;
        break;
      }
      
      // optional int32 end = 6;
      case 6: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_end:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &end_)));
          set_has_end();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(56)) goto parse_limitType;
        break;
      }
      
      // optional .BusinessUnit.LimitType limitType = 7;
      case 7: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_limitType:
          int value;
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   int, ::google::protobuf::internal::WireFormatLite::TYPE_ENUM>(
                 input, &value)));
          if (::BusinessUnit_LimitType_IsValid(value)) {
            set_limitType(static_cast< ::BusinessUnit_LimitType >(value));
          } else {
            mutable_unknown_fields()->AddVarint(7, value);
          }
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(64)) goto parse_arg;
        break;
      }
      
      // optional int32 arg = 8;
      case 8: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_arg:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &arg_)));
          set_has_arg();
        } else {
          goto handle_uninterpreted;
        }
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

void BusinessUnit::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // optional .PB_ItemInfo.Type type = 1;
  if (has_type()) {
    ::google::protobuf::internal::WireFormatLite::WriteEnum(
      1, this->type(), output);
  }
  
  // optional int32 id = 2;
  if (has_id()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(2, this->id(), output);
  }
  
  // optional int32 count = 3;
  if (has_count()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(3, this->count(), output);
  }
  
  // optional int32 effect = 4;
  if (has_effect()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(4, this->effect(), output);
  }
  
  // optional int32 begin = 5;
  if (has_begin()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(5, this->begin(), output);
  }
  
  // optional int32 end = 6;
  if (has_end()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(6, this->end(), output);
  }
  
  // optional .BusinessUnit.LimitType limitType = 7;
  if (has_limitType()) {
    ::google::protobuf::internal::WireFormatLite::WriteEnum(
      7, this->limitType(), output);
  }
  
  // optional int32 arg = 8;
  if (has_arg()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(8, this->arg(), output);
  }
  
  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* BusinessUnit::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // optional .PB_ItemInfo.Type type = 1;
  if (has_type()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteEnumToArray(
      1, this->type(), target);
  }
  
  // optional int32 id = 2;
  if (has_id()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(2, this->id(), target);
  }
  
  // optional int32 count = 3;
  if (has_count()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(3, this->count(), target);
  }
  
  // optional int32 effect = 4;
  if (has_effect()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(4, this->effect(), target);
  }
  
  // optional int32 begin = 5;
  if (has_begin()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(5, this->begin(), target);
  }
  
  // optional int32 end = 6;
  if (has_end()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(6, this->end(), target);
  }
  
  // optional .BusinessUnit.LimitType limitType = 7;
  if (has_limitType()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteEnumToArray(
      7, this->limitType(), target);
  }
  
  // optional int32 arg = 8;
  if (has_arg()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(8, this->arg(), target);
  }
  
  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int BusinessUnit::ByteSize() const {
  int total_size = 0;
  
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // optional .PB_ItemInfo.Type type = 1;
    if (has_type()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::EnumSize(this->type());
    }
    
    // optional int32 id = 2;
    if (has_id()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->id());
    }
    
    // optional int32 count = 3;
    if (has_count()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->count());
    }
    
    // optional int32 effect = 4;
    if (has_effect()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->effect());
    }
    
    // optional int32 begin = 5;
    if (has_begin()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->begin());
    }
    
    // optional int32 end = 6;
    if (has_end()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->end());
    }
    
    // optional .BusinessUnit.LimitType limitType = 7;
    if (has_limitType()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::EnumSize(this->limitType());
    }
    
    // optional int32 arg = 8;
    if (has_arg()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->arg());
    }
    
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

void BusinessUnit::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const BusinessUnit* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const BusinessUnit*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void BusinessUnit::MergeFrom(const BusinessUnit& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_type()) {
      set_type(from.type());
    }
    if (from.has_id()) {
      set_id(from.id());
    }
    if (from.has_count()) {
      set_count(from.count());
    }
    if (from.has_effect()) {
      set_effect(from.effect());
    }
    if (from.has_begin()) {
      set_begin(from.begin());
    }
    if (from.has_end()) {
      set_end(from.end());
    }
    if (from.has_limitType()) {
      set_limitType(from.limitType());
    }
    if (from.has_arg()) {
      set_arg(from.arg());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void BusinessUnit::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void BusinessUnit::CopyFrom(const BusinessUnit& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool BusinessUnit::IsInitialized() const {
  
  return true;
}

void BusinessUnit::Swap(BusinessUnit* other) {
  if (other != this) {
    std::swap(type_, other->type_);
    std::swap(id_, other->id_);
    std::swap(count_, other->count_);
    std::swap(effect_, other->effect_);
    std::swap(begin_, other->begin_);
    std::swap(end_, other->end_);
    std::swap(limitType_, other->limitType_);
    std::swap(arg_, other->arg_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata BusinessUnit::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = BusinessUnit_descriptor_;
  metadata.reflection = BusinessUnit_reflection_;
  return metadata;
}


// ===================================================================

const ::google::protobuf::EnumDescriptor* BusinessInfo_CurrencyType_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return BusinessInfo_CurrencyType_descriptor_;
}
bool BusinessInfo_CurrencyType_IsValid(int value) {
  switch(value) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
      return true;
    default:
      return false;
  }
}

#ifndef _MSC_VER
const BusinessInfo_CurrencyType BusinessInfo::NONE;
const BusinessInfo_CurrencyType BusinessInfo::RMB;
const BusinessInfo_CurrencyType BusinessInfo::SUB_RMB;
const BusinessInfo_CurrencyType BusinessInfo::MONEY;
const BusinessInfo_CurrencyType BusinessInfo::LOVE_POINT;
const BusinessInfo_CurrencyType BusinessInfo::PVP_SCORE;
const BusinessInfo_CurrencyType BusinessInfo::GOD_SCORE;
const BusinessInfo_CurrencyType BusinessInfo::FACTION_CONTRIBUTE;
const BusinessInfo_CurrencyType BusinessInfo::CurrencyType_MIN;
const BusinessInfo_CurrencyType BusinessInfo::CurrencyType_MAX;
const int BusinessInfo::CurrencyType_ARRAYSIZE;
#endif  // _MSC_VER
#ifndef _MSC_VER
const int BusinessInfo::kIdFieldNumber;
const int BusinessInfo::kCurrencyTypeFieldNumber;
const int BusinessInfo::kItemsFieldNumber;
#endif  // !_MSC_VER

BusinessInfo::BusinessInfo()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void BusinessInfo::InitAsDefaultInstance() {
}

BusinessInfo::BusinessInfo(const BusinessInfo& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void BusinessInfo::SharedCtor() {
  _cached_size_ = 0;
  id_ = 0;
  currencyType_ = 0;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

BusinessInfo::~BusinessInfo() {
  SharedDtor();
}

void BusinessInfo::SharedDtor() {
  if (this != default_instance_) {
  }
}

void BusinessInfo::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* BusinessInfo::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return BusinessInfo_descriptor_;
}

const BusinessInfo& BusinessInfo::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_BusinessInfo_2eproto();  return *default_instance_;
}

BusinessInfo* BusinessInfo::default_instance_ = NULL;

BusinessInfo* BusinessInfo::New() const {
  return new BusinessInfo;
}

void BusinessInfo::Clear() {
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    id_ = 0;
    currencyType_ = 0;
  }
  items_.Clear();
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool BusinessInfo::MergePartialFromCodedStream(
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
        if (input->ExpectTag(16)) goto parse_currencyType;
        break;
      }
      
      // optional .BusinessInfo.CurrencyType currencyType = 2;
      case 2: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_currencyType:
          int value;
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   int, ::google::protobuf::internal::WireFormatLite::TYPE_ENUM>(
                 input, &value)));
          if (::BusinessInfo_CurrencyType_IsValid(value)) {
            set_currencyType(static_cast< ::BusinessInfo_CurrencyType >(value));
          } else {
            mutable_unknown_fields()->AddVarint(2, value);
          }
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(26)) goto parse_items;
        break;
      }
      
      // repeated .BusinessUnit items = 3;
      case 3: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_items:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
                input, add_items()));
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(26)) goto parse_items;
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

void BusinessInfo::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // optional int32 id = 1;
  if (has_id()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(1, this->id(), output);
  }
  
  // optional .BusinessInfo.CurrencyType currencyType = 2;
  if (has_currencyType()) {
    ::google::protobuf::internal::WireFormatLite::WriteEnum(
      2, this->currencyType(), output);
  }
  
  // repeated .BusinessUnit items = 3;
  for (int i = 0; i < this->items_size(); i++) {
    ::google::protobuf::internal::WireFormatLite::WriteMessageMaybeToArray(
      3, this->items(i), output);
  }
  
  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* BusinessInfo::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // optional int32 id = 1;
  if (has_id()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(1, this->id(), target);
  }
  
  // optional .BusinessInfo.CurrencyType currencyType = 2;
  if (has_currencyType()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteEnumToArray(
      2, this->currencyType(), target);
  }
  
  // repeated .BusinessUnit items = 3;
  for (int i = 0; i < this->items_size(); i++) {
    target = ::google::protobuf::internal::WireFormatLite::
      WriteMessageNoVirtualToArray(
        3, this->items(i), target);
  }
  
  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int BusinessInfo::ByteSize() const {
  int total_size = 0;
  
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // optional int32 id = 1;
    if (has_id()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->id());
    }
    
    // optional .BusinessInfo.CurrencyType currencyType = 2;
    if (has_currencyType()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::EnumSize(this->currencyType());
    }
    
  }
  // repeated .BusinessUnit items = 3;
  total_size += 1 * this->items_size();
  for (int i = 0; i < this->items_size(); i++) {
    total_size +=
      ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
        this->items(i));
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

void BusinessInfo::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const BusinessInfo* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const BusinessInfo*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void BusinessInfo::MergeFrom(const BusinessInfo& from) {
  GOOGLE_CHECK_NE(&from, this);
  items_.MergeFrom(from.items_);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_id()) {
      set_id(from.id());
    }
    if (from.has_currencyType()) {
      set_currencyType(from.currencyType());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void BusinessInfo::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void BusinessInfo::CopyFrom(const BusinessInfo& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool BusinessInfo::IsInitialized() const {
  
  return true;
}

void BusinessInfo::Swap(BusinessInfo* other) {
  if (other != this) {
    std::swap(id_, other->id_);
    std::swap(currencyType_, other->currencyType_);
    items_.Swap(&other->items_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata BusinessInfo::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = BusinessInfo_descriptor_;
  metadata.reflection = BusinessInfo_reflection_;
  return metadata;
}


// ===================================================================

#ifndef _MSC_VER
const int AllBusiness::kBusinessFieldNumber;
#endif  // !_MSC_VER

AllBusiness::AllBusiness()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void AllBusiness::InitAsDefaultInstance() {
}

AllBusiness::AllBusiness(const AllBusiness& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void AllBusiness::SharedCtor() {
  _cached_size_ = 0;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

AllBusiness::~AllBusiness() {
  SharedDtor();
}

void AllBusiness::SharedDtor() {
  if (this != default_instance_) {
  }
}

void AllBusiness::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* AllBusiness::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return AllBusiness_descriptor_;
}

const AllBusiness& AllBusiness::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_BusinessInfo_2eproto();  return *default_instance_;
}

AllBusiness* AllBusiness::default_instance_ = NULL;

AllBusiness* AllBusiness::New() const {
  return new AllBusiness;
}

void AllBusiness::Clear() {
  business_.Clear();
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool AllBusiness::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // repeated .BusinessInfo business = 1;
      case 1: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_business:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
                input, add_business()));
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(10)) goto parse_business;
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

void AllBusiness::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // repeated .BusinessInfo business = 1;
  for (int i = 0; i < this->business_size(); i++) {
    ::google::protobuf::internal::WireFormatLite::WriteMessageMaybeToArray(
      1, this->business(i), output);
  }
  
  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* AllBusiness::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // repeated .BusinessInfo business = 1;
  for (int i = 0; i < this->business_size(); i++) {
    target = ::google::protobuf::internal::WireFormatLite::
      WriteMessageNoVirtualToArray(
        1, this->business(i), target);
  }
  
  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int AllBusiness::ByteSize() const {
  int total_size = 0;
  
  // repeated .BusinessInfo business = 1;
  total_size += 1 * this->business_size();
  for (int i = 0; i < this->business_size(); i++) {
    total_size +=
      ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
        this->business(i));
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

void AllBusiness::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const AllBusiness* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const AllBusiness*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void AllBusiness::MergeFrom(const AllBusiness& from) {
  GOOGLE_CHECK_NE(&from, this);
  business_.MergeFrom(from.business_);
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void AllBusiness::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void AllBusiness::CopyFrom(const AllBusiness& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool AllBusiness::IsInitialized() const {
  
  return true;
}

void AllBusiness::Swap(AllBusiness* other) {
  if (other != this) {
    business_.Swap(&other->business_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata AllBusiness::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = AllBusiness_descriptor_;
  metadata.reflection = AllBusiness_reflection_;
  return metadata;
}


// @@protoc_insertion_point(namespace_scope)

// @@protoc_insertion_point(global_scope)
