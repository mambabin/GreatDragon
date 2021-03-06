// Generated by the protocol buffer compiler.  DO NOT EDIT!

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "MovementInfo.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace {

const ::google::protobuf::Descriptor* PB_MovementAtt_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  PB_MovementAtt_reflection_ = NULL;
const ::google::protobuf::EnumDescriptor* PB_MovementAtt_Status_descriptor_ = NULL;

}  // namespace


void protobuf_AssignDesc_MovementInfo_2eproto() {
  protobuf_AddDesc_MovementInfo_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "MovementInfo.proto");
  GOOGLE_CHECK(file != NULL);
  PB_MovementAtt_descriptor_ = file->message_type(0);
  static const int PB_MovementAtt_offsets_[8] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(PB_MovementAtt, status_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(PB_MovementAtt, mapID_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(PB_MovementAtt, prevNormalMap_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(PB_MovementAtt, prevCoord_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(PB_MovementAtt, logicCoord_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(PB_MovementAtt, position_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(PB_MovementAtt, moveSpeed_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(PB_MovementAtt, radius_),
  };
  PB_MovementAtt_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      PB_MovementAtt_descriptor_,
      PB_MovementAtt::default_instance_,
      PB_MovementAtt_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(PB_MovementAtt, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(PB_MovementAtt, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(PB_MovementAtt));
  PB_MovementAtt_Status_descriptor_ = PB_MovementAtt_descriptor_->enum_type(0);
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_MovementInfo_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    PB_MovementAtt_descriptor_, &PB_MovementAtt::default_instance());
}

}  // namespace

void protobuf_ShutdownFile_MovementInfo_2eproto() {
  delete PB_MovementAtt::default_instance_;
  delete PB_MovementAtt_reflection_;
}

void protobuf_AddDesc_MovementInfo_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::protobuf_AddDesc_Math_2eproto();
  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n\022MovementInfo.proto\032\nMath.proto\"\233\002\n\016PB_"
    "MovementAtt\022&\n\006status\030\001 \001(\0162\026.PB_Movemen"
    "tAtt.Status\022\r\n\005mapID\030\002 \001(\005\022\025\n\rprevNormal"
    "Map\030\003 \001(\005\022\037\n\tprevCoord\030\004 \001(\0132\014.PB_Vector"
    "2i\022 \n\nlogicCoord\030\005 \001(\0132\014.PB_Vector2i\022\036\n\010"
    "position\030\006 \001(\0132\014.PB_Vector3f\022\021\n\tmoveSpee"
    "d\030\007 \001(\005\022\016\n\006radius\030\010 \001(\005\"5\n\006Status\022\010\n\004IDL"
    "E\020\000\022\010\n\004MOVE\020\001\022\n\n\006FOLLOW\020\002\022\013\n\007TALK_TO\020\003", 318);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "MovementInfo.proto", &protobuf_RegisterTypes);
  PB_MovementAtt::default_instance_ = new PB_MovementAtt();
  PB_MovementAtt::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_MovementInfo_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_MovementInfo_2eproto {
  StaticDescriptorInitializer_MovementInfo_2eproto() {
    protobuf_AddDesc_MovementInfo_2eproto();
  }
} static_descriptor_initializer_MovementInfo_2eproto_;


// ===================================================================

const ::google::protobuf::EnumDescriptor* PB_MovementAtt_Status_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return PB_MovementAtt_Status_descriptor_;
}
bool PB_MovementAtt_Status_IsValid(int value) {
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
const PB_MovementAtt_Status PB_MovementAtt::IDLE;
const PB_MovementAtt_Status PB_MovementAtt::MOVE;
const PB_MovementAtt_Status PB_MovementAtt::FOLLOW;
const PB_MovementAtt_Status PB_MovementAtt::TALK_TO;
const PB_MovementAtt_Status PB_MovementAtt::Status_MIN;
const PB_MovementAtt_Status PB_MovementAtt::Status_MAX;
const int PB_MovementAtt::Status_ARRAYSIZE;
#endif  // _MSC_VER
#ifndef _MSC_VER
const int PB_MovementAtt::kStatusFieldNumber;
const int PB_MovementAtt::kMapIDFieldNumber;
const int PB_MovementAtt::kPrevNormalMapFieldNumber;
const int PB_MovementAtt::kPrevCoordFieldNumber;
const int PB_MovementAtt::kLogicCoordFieldNumber;
const int PB_MovementAtt::kPositionFieldNumber;
const int PB_MovementAtt::kMoveSpeedFieldNumber;
const int PB_MovementAtt::kRadiusFieldNumber;
#endif  // !_MSC_VER

PB_MovementAtt::PB_MovementAtt()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void PB_MovementAtt::InitAsDefaultInstance() {
  prevCoord_ = const_cast< ::PB_Vector2i*>(&::PB_Vector2i::default_instance());
  logicCoord_ = const_cast< ::PB_Vector2i*>(&::PB_Vector2i::default_instance());
  position_ = const_cast< ::PB_Vector3f*>(&::PB_Vector3f::default_instance());
}

PB_MovementAtt::PB_MovementAtt(const PB_MovementAtt& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void PB_MovementAtt::SharedCtor() {
  _cached_size_ = 0;
  status_ = 0;
  mapID_ = 0;
  prevNormalMap_ = 0;
  prevCoord_ = NULL;
  logicCoord_ = NULL;
  position_ = NULL;
  moveSpeed_ = 0;
  radius_ = 0;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

PB_MovementAtt::~PB_MovementAtt() {
  SharedDtor();
}

void PB_MovementAtt::SharedDtor() {
  if (this != default_instance_) {
    delete prevCoord_;
    delete logicCoord_;
    delete position_;
  }
}

void PB_MovementAtt::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* PB_MovementAtt::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return PB_MovementAtt_descriptor_;
}

const PB_MovementAtt& PB_MovementAtt::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_MovementInfo_2eproto();  return *default_instance_;
}

PB_MovementAtt* PB_MovementAtt::default_instance_ = NULL;

PB_MovementAtt* PB_MovementAtt::New() const {
  return new PB_MovementAtt;
}

void PB_MovementAtt::Clear() {
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    status_ = 0;
    mapID_ = 0;
    prevNormalMap_ = 0;
    if (has_prevCoord()) {
      if (prevCoord_ != NULL) prevCoord_->::PB_Vector2i::Clear();
    }
    if (has_logicCoord()) {
      if (logicCoord_ != NULL) logicCoord_->::PB_Vector2i::Clear();
    }
    if (has_position()) {
      if (position_ != NULL) position_->::PB_Vector3f::Clear();
    }
    moveSpeed_ = 0;
    radius_ = 0;
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool PB_MovementAtt::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // optional .PB_MovementAtt.Status status = 1;
      case 1: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
          int value;
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   int, ::google::protobuf::internal::WireFormatLite::TYPE_ENUM>(
                 input, &value)));
          if (::PB_MovementAtt_Status_IsValid(value)) {
            set_status(static_cast< ::PB_MovementAtt_Status >(value));
          } else {
            mutable_unknown_fields()->AddVarint(1, value);
          }
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(16)) goto parse_mapID;
        break;
      }
      
      // optional int32 mapID = 2;
      case 2: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_mapID:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &mapID_)));
          set_has_mapID();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(24)) goto parse_prevNormalMap;
        break;
      }
      
      // optional int32 prevNormalMap = 3;
      case 3: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_prevNormalMap:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &prevNormalMap_)));
          set_has_prevNormalMap();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(34)) goto parse_prevCoord;
        break;
      }
      
      // optional .PB_Vector2i prevCoord = 4;
      case 4: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_prevCoord:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_prevCoord()));
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(42)) goto parse_logicCoord;
        break;
      }
      
      // optional .PB_Vector2i logicCoord = 5;
      case 5: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_logicCoord:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_logicCoord()));
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(50)) goto parse_position;
        break;
      }
      
      // optional .PB_Vector3f position = 6;
      case 6: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_position:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_position()));
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(56)) goto parse_moveSpeed;
        break;
      }
      
      // optional int32 moveSpeed = 7;
      case 7: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_moveSpeed:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &moveSpeed_)));
          set_has_moveSpeed();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(64)) goto parse_radius;
        break;
      }
      
      // optional int32 radius = 8;
      case 8: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_radius:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &radius_)));
          set_has_radius();
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

void PB_MovementAtt::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // optional .PB_MovementAtt.Status status = 1;
  if (has_status()) {
    ::google::protobuf::internal::WireFormatLite::WriteEnum(
      1, this->status(), output);
  }
  
  // optional int32 mapID = 2;
  if (has_mapID()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(2, this->mapID(), output);
  }
  
  // optional int32 prevNormalMap = 3;
  if (has_prevNormalMap()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(3, this->prevNormalMap(), output);
  }
  
  // optional .PB_Vector2i prevCoord = 4;
  if (has_prevCoord()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessageMaybeToArray(
      4, this->prevCoord(), output);
  }
  
  // optional .PB_Vector2i logicCoord = 5;
  if (has_logicCoord()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessageMaybeToArray(
      5, this->logicCoord(), output);
  }
  
  // optional .PB_Vector3f position = 6;
  if (has_position()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessageMaybeToArray(
      6, this->position(), output);
  }
  
  // optional int32 moveSpeed = 7;
  if (has_moveSpeed()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(7, this->moveSpeed(), output);
  }
  
  // optional int32 radius = 8;
  if (has_radius()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(8, this->radius(), output);
  }
  
  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* PB_MovementAtt::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // optional .PB_MovementAtt.Status status = 1;
  if (has_status()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteEnumToArray(
      1, this->status(), target);
  }
  
  // optional int32 mapID = 2;
  if (has_mapID()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(2, this->mapID(), target);
  }
  
  // optional int32 prevNormalMap = 3;
  if (has_prevNormalMap()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(3, this->prevNormalMap(), target);
  }
  
  // optional .PB_Vector2i prevCoord = 4;
  if (has_prevCoord()) {
    target = ::google::protobuf::internal::WireFormatLite::
      WriteMessageNoVirtualToArray(
        4, this->prevCoord(), target);
  }
  
  // optional .PB_Vector2i logicCoord = 5;
  if (has_logicCoord()) {
    target = ::google::protobuf::internal::WireFormatLite::
      WriteMessageNoVirtualToArray(
        5, this->logicCoord(), target);
  }
  
  // optional .PB_Vector3f position = 6;
  if (has_position()) {
    target = ::google::protobuf::internal::WireFormatLite::
      WriteMessageNoVirtualToArray(
        6, this->position(), target);
  }
  
  // optional int32 moveSpeed = 7;
  if (has_moveSpeed()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(7, this->moveSpeed(), target);
  }
  
  // optional int32 radius = 8;
  if (has_radius()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(8, this->radius(), target);
  }
  
  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int PB_MovementAtt::ByteSize() const {
  int total_size = 0;
  
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // optional .PB_MovementAtt.Status status = 1;
    if (has_status()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::EnumSize(this->status());
    }
    
    // optional int32 mapID = 2;
    if (has_mapID()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->mapID());
    }
    
    // optional int32 prevNormalMap = 3;
    if (has_prevNormalMap()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->prevNormalMap());
    }
    
    // optional .PB_Vector2i prevCoord = 4;
    if (has_prevCoord()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->prevCoord());
    }
    
    // optional .PB_Vector2i logicCoord = 5;
    if (has_logicCoord()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->logicCoord());
    }
    
    // optional .PB_Vector3f position = 6;
    if (has_position()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->position());
    }
    
    // optional int32 moveSpeed = 7;
    if (has_moveSpeed()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->moveSpeed());
    }
    
    // optional int32 radius = 8;
    if (has_radius()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int32Size(
          this->radius());
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

void PB_MovementAtt::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const PB_MovementAtt* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const PB_MovementAtt*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void PB_MovementAtt::MergeFrom(const PB_MovementAtt& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_status()) {
      set_status(from.status());
    }
    if (from.has_mapID()) {
      set_mapID(from.mapID());
    }
    if (from.has_prevNormalMap()) {
      set_prevNormalMap(from.prevNormalMap());
    }
    if (from.has_prevCoord()) {
      mutable_prevCoord()->::PB_Vector2i::MergeFrom(from.prevCoord());
    }
    if (from.has_logicCoord()) {
      mutable_logicCoord()->::PB_Vector2i::MergeFrom(from.logicCoord());
    }
    if (from.has_position()) {
      mutable_position()->::PB_Vector3f::MergeFrom(from.position());
    }
    if (from.has_moveSpeed()) {
      set_moveSpeed(from.moveSpeed());
    }
    if (from.has_radius()) {
      set_radius(from.radius());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void PB_MovementAtt::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void PB_MovementAtt::CopyFrom(const PB_MovementAtt& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool PB_MovementAtt::IsInitialized() const {
  
  return true;
}

void PB_MovementAtt::Swap(PB_MovementAtt* other) {
  if (other != this) {
    std::swap(status_, other->status_);
    std::swap(mapID_, other->mapID_);
    std::swap(prevNormalMap_, other->prevNormalMap_);
    std::swap(prevCoord_, other->prevCoord_);
    std::swap(logicCoord_, other->logicCoord_);
    std::swap(position_, other->position_);
    std::swap(moveSpeed_, other->moveSpeed_);
    std::swap(radius_, other->radius_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata PB_MovementAtt::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = PB_MovementAtt_descriptor_;
  metadata.reflection = PB_MovementAtt_reflection_;
  return metadata;
}


// @@protoc_insertion_point(namespace_scope)

// @@protoc_insertion_point(global_scope)
