// Generated by the protocol buffer compiler.  DO NOT EDIT!

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "Combat.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace {

const ::google::protobuf::Descriptor* CombatTurn_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  CombatTurn_reflection_ = NULL;
const ::google::protobuf::EnumDescriptor* CombatTurn_Type_descriptor_ = NULL;
const ::google::protobuf::Descriptor* CombatRecord_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  CombatRecord_reflection_ = NULL;

}  // namespace


void protobuf_AssignDesc_Combat_2eproto() {
  protobuf_AddDesc_Combat_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "Combat.proto");
  GOOGLE_CHECK(file != NULL);
  CombatTurn_descriptor_ = file->message_type(0);
  static const int CombatTurn_offsets_[7] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CombatTurn, type_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CombatTurn, time_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CombatTurn, coord_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CombatTurn, skillId_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CombatTurn, status_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CombatTurn, targetID_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CombatTurn, damage_),
  };
  CombatTurn_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      CombatTurn_descriptor_,
      CombatTurn::default_instance_,
      CombatTurn_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CombatTurn, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CombatTurn, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(CombatTurn));
  CombatTurn_Type_descriptor_ = CombatTurn_descriptor_->enum_type(0);
  CombatRecord_descriptor_ = file->message_type(1);
  static const int CombatRecord_offsets_[1] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CombatRecord, combatTurns_),
  };
  CombatRecord_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      CombatRecord_descriptor_,
      CombatRecord::default_instance_,
      CombatRecord_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CombatRecord, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(CombatRecord, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(CombatRecord));
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_Combat_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    CombatTurn_descriptor_, &CombatTurn::default_instance());
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    CombatRecord_descriptor_, &CombatRecord::default_instance());
}

}  // namespace

void protobuf_ShutdownFile_Combat_2eproto() {
  delete CombatTurn::default_instance_;
  delete CombatTurn_reflection_;
  delete CombatRecord::default_instance_;
  delete CombatRecord_reflection_;
}

void protobuf_AddDesc_Combat_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::protobuf_AddDesc_Math_2eproto();
  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n\014Combat.proto\032\nMath.proto\"\221\002\n\nCombatTur"
    "n\022\036\n\004type\030\001 \001(\0162\020.CombatTurn.Type\022\014\n\004tim"
    "e\030\002 \001(\003\022\033\n\005coord\030\003 \001(\0132\014.PB_Vector2i\022\017\n\007"
    "skillId\030\004 \001(\005\022\016\n\006status\030\005 \001(\005\022\020\n\010targetI"
    "D\030\006 \001(\005\022\016\n\006damage\030\007 \001(\005\"u\n\004Type\022\017\n\013ROLE_"
    "ATTACK\020\000\022\016\n\nPET_ATTACK\020\001\022\023\n\017ATTACKED_ENE"
    "RMY\020\002\022\026\n\022ATTACKED_ENERMYPET\020\003\022\020\n\014ATTACKE"
    "D_NPC\020\004\022\r\n\tTRANSFORM\020\005\"0\n\014CombatRecord\022 "
    "\n\013combatTurns\030\002 \003(\0132\013.CombatTurn", 352);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "Combat.proto", &protobuf_RegisterTypes);
  CombatTurn::default_instance_ = new CombatTurn();
  CombatRecord::default_instance_ = new CombatRecord();
  CombatTurn::default_instance_->InitAsDefaultInstance();
  CombatRecord::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_Combat_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_Combat_2eproto {
  StaticDescriptorInitializer_Combat_2eproto() {
    protobuf_AddDesc_Combat_2eproto();
  }
} static_descriptor_initializer_Combat_2eproto_;


// ===================================================================

const ::google::protobuf::EnumDescriptor* CombatTurn_Type_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return CombatTurn_Type_descriptor_;
}
bool CombatTurn_Type_IsValid(int value) {
  switch(value) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
      return true;
    default:
      return false;
  }
}

#ifndef _MSC_VER
const CombatTurn_Type CombatTurn::ROLE_ATTACK;
const CombatTurn_Type CombatTurn::PET_ATTACK;
const CombatTurn_Type CombatTurn::ATTACKED_ENERMY;
const CombatTurn_Type CombatTurn::ATTACKED_ENERMYPET;
const CombatTurn_Type CombatTurn::ATTACKED_NPC;
const CombatTurn_Type CombatTurn::TRANSFORM;
const CombatTurn_Type CombatTurn::Type_MIN;
const CombatTurn_Type CombatTurn::Type_MAX;
const int CombatTurn::Type_ARRAYSIZE;
#endif  // _MSC_VER
#ifndef _MSC_VER
const int CombatTurn::kTypeFieldNumber;
const int CombatTurn::kTimeFieldNumber;
const int CombatTurn::kCoordFieldNumber;
const int CombatTurn::kSkillIdFieldNumber;
const int CombatTurn::kStatusFieldNumber;
const int CombatTurn::kTargetIDFieldNumber;
const int CombatTurn::kDamageFieldNumber;
#endif  // !_MSC_VER

CombatTurn::CombatTurn()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void CombatTurn::InitAsDefaultInstance() {
  coord_ = const_cast< ::PB_Vector2i*>(&::PB_Vector2i::default_instance());
}

CombatTurn::CombatTurn(const CombatTurn& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void CombatTurn::SharedCtor() {
  _cached_size_ = 0;
  type_ = 0;
  time_ = GOOGLE_LONGLONG(0);
  coord_ = NULL;
  skillId_ = 0;
  status_ = 0;
  targetID_ = 0;
  damage_ = 0;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

CombatTurn::~CombatTurn() {
  SharedDtor();
}

void CombatTurn::SharedDtor() {
  if (this != default_instance_) {
    delete coord_;
  }
}

void CombatTurn::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* CombatTurn::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return CombatTurn_descriptor_;
}

const CombatTurn& CombatTurn::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_Combat_2eproto();  return *default_instance_;
}

CombatTurn* CombatTurn::default_instance_ = NULL;

CombatTurn* CombatTurn::New() const {
  return new CombatTurn;
}

void CombatTurn::Clear() {
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    type_ = 0;
    time_ = GOOGLE_LONGLONG(0);
    if (has_coord()) {
      if (coord_ != NULL) coord_->::PB_Vector2i::Clear();
    }
    skillId_ = 0;
    status_ = 0;
    targetID_ = 0;
    damage_ = 0;
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool CombatTurn::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // optional .CombatTurn.Type type = 1;
      case 1: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
          int value;
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   int, ::google::protobuf::internal::WireFormatLite::TYPE_ENUM>(
                 input, &value)));
          if (::CombatTurn_Type_IsValid(value)) {
            set_type(static_cast< ::CombatTurn_Type >(value));
          } else {
            mutable_unknown_fields()->AddVarint(1, value);
          }
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(16)) goto parse_time;
        break;
      }
      
      // optional int64 time = 2;
      case 2: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_time:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int64, ::google::protobuf::internal::WireFormatLite::TYPE_INT64>(
                 input, &time_)));
          set_has_time();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(26)) goto parse_coord;
        break;
      }
      
      // optional .PB_Vector2i coord = 3;
      case 3: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_coord:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_coord()));
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(32)) goto parse_skillId;
        break;
      }
      
      // optional int32 skillId = 4;
      case 4: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_skillId:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &skillId_)));
          set_has_skillId();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(40)) goto parse_status;
        break;
      }
      
      // optional int32 status = 5;
      case 5: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_status:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &status_)));
          set_has_status();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(48)) goto parse_targetID;
        break;
      }
      
      // optional int32 targetID = 6;
      case 6: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_targetID:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &targetID_)));
          set_has_targetID();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(56)) goto parse_damage;
        break;
      }
      
      // optional int32 damage = 7;
      case 7: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_damage:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &damage_)));
          set_has_damage();
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

void CombatTurn::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // optional .CombatTurn.Type type = 1;
  if (has_type()) {
    ::google::protobuf::internal::WireFormatLite::WriteEnum(
      1, this->type(), output);
  }
  
  // optional int64 time = 2;
  if (has_time()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt64(2, this->time(), output);
  }
  
  // optional .PB_Vector2i coord = 3;
  if (has_coord()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessageMaybeToArray(
      3, this->coord(), output);
  }
  
  // optional int32 skillId = 4;
  if (has_skillId()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(4, this->skillId(), output);
  }
  
  // optional int32 status = 5;
  if (has_status()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(5, this->status(), output);
  }
  
  // optional int32 targetID = 6;
  if (has_targetID()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(6, this->targetID(), output);
  }
  
  // optional int32 damage = 7;
  if (has_damage()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(7, this->damage(), output);
  }
  
  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* CombatTurn::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // optional .CombatTurn.Type type = 1;
  if (has_type()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteEnumToArray(
      1, this->type(), target);
  }
  
  // optional int64 time = 2;
  if (has_time()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt64ToArray(2, this->time(), target);
  }
  
  // optional .PB_Vector2i coord = 3;
  if (has_coord()) {
    target = ::google::protobuf::internal::WireFormatLite::
      WriteMessageNoVirtualToArray(
        3, this->coord(), target);
  }
  
  // optional int32 skillId = 4;
  if (has_skillId()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(4, this->skillId(), target);
  }
  
  // optional int32 status = 5;
  if (has_status()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(5, this->status(), target);
  }
  
  // optional int32 targetID = 6;
  if (has_targetID()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(6, this->targetID(), target);
  }
  
  // optional int32 damage = 7;
  if (has_damage()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(7, this->damage(), target);
  }
  
  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int CombatTurn::ByteSize() const {
  int total_size = 0;
  
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // optional .CombatTurn.Type type = 1;
    if (has_type()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::EnumSize(this->type());
    }
    
    // optional int64 time = 2;
    if (has_time()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int64Size(
          this->time());
    }
    
    // optional .PB_Vector2i coord = 3;
    if (has_coord()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->coord());
    }
    
    // optional int32 skillId = 4;
    if (has_skillId()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->skillId());
    }
    
    // optional int32 status = 5;
    if (has_status()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->status());
    }
    
    // optional int32 targetID = 6;
    if (has_targetID()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->targetID());
    }
    
    // optional int32 damage = 7;
    if (has_damage()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->damage());
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

void CombatTurn::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const CombatTurn* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const CombatTurn*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void CombatTurn::MergeFrom(const CombatTurn& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_type()) {
      set_type(from.type());
    }
    if (from.has_time()) {
      set_time(from.time());
    }
    if (from.has_coord()) {
      mutable_coord()->::PB_Vector2i::MergeFrom(from.coord());
    }
    if (from.has_skillId()) {
      set_skillId(from.skillId());
    }
    if (from.has_status()) {
      set_status(from.status());
    }
    if (from.has_targetID()) {
      set_targetID(from.targetID());
    }
    if (from.has_damage()) {
      set_damage(from.damage());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void CombatTurn::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void CombatTurn::CopyFrom(const CombatTurn& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool CombatTurn::IsInitialized() const {
  
  return true;
}

void CombatTurn::Swap(CombatTurn* other) {
  if (other != this) {
    std::swap(type_, other->type_);
    std::swap(time_, other->time_);
    std::swap(coord_, other->coord_);
    std::swap(skillId_, other->skillId_);
    std::swap(status_, other->status_);
    std::swap(targetID_, other->targetID_);
    std::swap(damage_, other->damage_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata CombatTurn::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = CombatTurn_descriptor_;
  metadata.reflection = CombatTurn_reflection_;
  return metadata;
}


// ===================================================================

#ifndef _MSC_VER
const int CombatRecord::kCombatTurnsFieldNumber;
#endif  // !_MSC_VER

CombatRecord::CombatRecord()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void CombatRecord::InitAsDefaultInstance() {
}

CombatRecord::CombatRecord(const CombatRecord& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void CombatRecord::SharedCtor() {
  _cached_size_ = 0;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

CombatRecord::~CombatRecord() {
  SharedDtor();
}

void CombatRecord::SharedDtor() {
  if (this != default_instance_) {
  }
}

void CombatRecord::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* CombatRecord::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return CombatRecord_descriptor_;
}

const CombatRecord& CombatRecord::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_Combat_2eproto();  return *default_instance_;
}

CombatRecord* CombatRecord::default_instance_ = NULL;

CombatRecord* CombatRecord::New() const {
  return new CombatRecord;
}

void CombatRecord::Clear() {
  combatTurns_.Clear();
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool CombatRecord::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // repeated .CombatTurn combatTurns = 2;
      case 2: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_combatTurns:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
                input, add_combatTurns()));
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(18)) goto parse_combatTurns;
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

void CombatRecord::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // repeated .CombatTurn combatTurns = 2;
  for (int i = 0; i < this->combatTurns_size(); i++) {
    ::google::protobuf::internal::WireFormatLite::WriteMessageMaybeToArray(
      2, this->combatTurns(i), output);
  }
  
  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* CombatRecord::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // repeated .CombatTurn combatTurns = 2;
  for (int i = 0; i < this->combatTurns_size(); i++) {
    target = ::google::protobuf::internal::WireFormatLite::
      WriteMessageNoVirtualToArray(
        2, this->combatTurns(i), target);
  }
  
  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int CombatRecord::ByteSize() const {
  int total_size = 0;
  
  // repeated .CombatTurn combatTurns = 2;
  total_size += 1 * this->combatTurns_size();
  for (int i = 0; i < this->combatTurns_size(); i++) {
    total_size +=
      ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
        this->combatTurns(i));
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

void CombatRecord::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const CombatRecord* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const CombatRecord*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void CombatRecord::MergeFrom(const CombatRecord& from) {
  GOOGLE_CHECK_NE(&from, this);
  combatTurns_.MergeFrom(from.combatTurns_);
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void CombatRecord::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void CombatRecord::CopyFrom(const CombatRecord& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool CombatRecord::IsInitialized() const {
  
  return true;
}

void CombatRecord::Swap(CombatRecord* other) {
  if (other != this) {
    combatTurns_.Swap(&other->combatTurns_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata CombatRecord::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = CombatRecord_descriptor_;
  metadata.reflection = CombatRecord_reflection_;
  return metadata;
}


// @@protoc_insertion_point(namespace_scope)

// @@protoc_insertion_point(global_scope)
