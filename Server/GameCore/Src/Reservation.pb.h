// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: Reservation.proto

#ifndef PROTOBUF_Reservation_2eproto__INCLUDED
#define PROTOBUF_Reservation_2eproto__INCLUDED

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
void  protobuf_AddDesc_Reservation_2eproto();
void protobuf_AssignDesc_Reservation_2eproto();
void protobuf_ShutdownFile_Reservation_2eproto();

class PB_ReservationTime;
class PB_AllReservationTimes;

// ===================================================================

class PB_ReservationTime : public ::google::protobuf::Message {
 public:
  PB_ReservationTime();
  virtual ~PB_ReservationTime();
  
  PB_ReservationTime(const PB_ReservationTime& from);
  
  inline PB_ReservationTime& operator=(const PB_ReservationTime& from) {
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
  static const PB_ReservationTime& default_instance();
  
  void Swap(PB_ReservationTime* other);
  
  // implements Message ----------------------------------------------
  
  PB_ReservationTime* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const PB_ReservationTime& from);
  void MergeFrom(const PB_ReservationTime& from);
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
  
  // optional int32 id = 1;
  inline bool has_id() const;
  inline void clear_id();
  static const int kIdFieldNumber = 1;
  inline ::google::protobuf::int32 id() const;
  inline void set_id(::google::protobuf::int32 value);
  
  // optional int32 appointmentBeginHour = 2;
  inline bool has_appointmentBeginHour() const;
  inline void clear_appointmentBeginHour();
  static const int kAppointmentBeginHourFieldNumber = 2;
  inline ::google::protobuf::int32 appointmentBeginHour() const;
  inline void set_appointmentBeginHour(::google::protobuf::int32 value);
  
  // optional int32 appointmentBeginMinute = 3;
  inline bool has_appointmentBeginMinute() const;
  inline void clear_appointmentBeginMinute();
  static const int kAppointmentBeginMinuteFieldNumber = 3;
  inline ::google::protobuf::int32 appointmentBeginMinute() const;
  inline void set_appointmentBeginMinute(::google::protobuf::int32 value);
  
  // optional int32 appointmentEndHour = 4;
  inline bool has_appointmentEndHour() const;
  inline void clear_appointmentEndHour();
  static const int kAppointmentEndHourFieldNumber = 4;
  inline ::google::protobuf::int32 appointmentEndHour() const;
  inline void set_appointmentEndHour(::google::protobuf::int32 value);
  
  // optional int32 appointmentEndMinute = 5;
  inline bool has_appointmentEndMinute() const;
  inline void clear_appointmentEndMinute();
  static const int kAppointmentEndMinuteFieldNumber = 5;
  inline ::google::protobuf::int32 appointmentEndMinute() const;
  inline void set_appointmentEndMinute(::google::protobuf::int32 value);
  
  // optional int32 challengeEndHour = 6;
  inline bool has_challengeEndHour() const;
  inline void clear_challengeEndHour();
  static const int kChallengeEndHourFieldNumber = 6;
  inline ::google::protobuf::int32 challengeEndHour() const;
  inline void set_challengeEndHour(::google::protobuf::int32 value);
  
  // optional int32 challengeEndMinute = 7;
  inline bool has_challengeEndMinute() const;
  inline void clear_challengeEndMinute();
  static const int kChallengeEndMinuteFieldNumber = 7;
  inline ::google::protobuf::int32 challengeEndMinute() const;
  inline void set_challengeEndMinute(::google::protobuf::int32 value);
  
  // optional int32 enterBeginHour = 8;
  inline bool has_enterBeginHour() const;
  inline void clear_enterBeginHour();
  static const int kEnterBeginHourFieldNumber = 8;
  inline ::google::protobuf::int32 enterBeginHour() const;
  inline void set_enterBeginHour(::google::protobuf::int32 value);
  
  // optional int32 enterBeginMinute = 9;
  inline bool has_enterBeginMinute() const;
  inline void clear_enterBeginMinute();
  static const int kEnterBeginMinuteFieldNumber = 9;
  inline ::google::protobuf::int32 enterBeginMinute() const;
  inline void set_enterBeginMinute(::google::protobuf::int32 value);
  
  // optional int32 fightBeginHour = 10;
  inline bool has_fightBeginHour() const;
  inline void clear_fightBeginHour();
  static const int kFightBeginHourFieldNumber = 10;
  inline ::google::protobuf::int32 fightBeginHour() const;
  inline void set_fightBeginHour(::google::protobuf::int32 value);
  
  // optional int32 fightBeginMinute = 11;
  inline bool has_fightBeginMinute() const;
  inline void clear_fightBeginMinute();
  static const int kFightBeginMinuteFieldNumber = 11;
  inline ::google::protobuf::int32 fightBeginMinute() const;
  inline void set_fightBeginMinute(::google::protobuf::int32 value);
  
  // optional int32 fightEndHour = 12;
  inline bool has_fightEndHour() const;
  inline void clear_fightEndHour();
  static const int kFightEndHourFieldNumber = 12;
  inline ::google::protobuf::int32 fightEndHour() const;
  inline void set_fightEndHour(::google::protobuf::int32 value);
  
  // optional int32 fightEndMinute = 13;
  inline bool has_fightEndMinute() const;
  inline void clear_fightEndMinute();
  static const int kFightEndMinuteFieldNumber = 13;
  inline ::google::protobuf::int32 fightEndMinute() const;
  inline void set_fightEndMinute(::google::protobuf::int32 value);
  
  // @@protoc_insertion_point(class_scope:PB_ReservationTime)
 private:
  inline void set_has_id();
  inline void clear_has_id();
  inline void set_has_appointmentBeginHour();
  inline void clear_has_appointmentBeginHour();
  inline void set_has_appointmentBeginMinute();
  inline void clear_has_appointmentBeginMinute();
  inline void set_has_appointmentEndHour();
  inline void clear_has_appointmentEndHour();
  inline void set_has_appointmentEndMinute();
  inline void clear_has_appointmentEndMinute();
  inline void set_has_challengeEndHour();
  inline void clear_has_challengeEndHour();
  inline void set_has_challengeEndMinute();
  inline void clear_has_challengeEndMinute();
  inline void set_has_enterBeginHour();
  inline void clear_has_enterBeginHour();
  inline void set_has_enterBeginMinute();
  inline void clear_has_enterBeginMinute();
  inline void set_has_fightBeginHour();
  inline void clear_has_fightBeginHour();
  inline void set_has_fightBeginMinute();
  inline void clear_has_fightBeginMinute();
  inline void set_has_fightEndHour();
  inline void clear_has_fightEndHour();
  inline void set_has_fightEndMinute();
  inline void clear_has_fightEndMinute();
  
  ::google::protobuf::UnknownFieldSet _unknown_fields_;
  
  ::google::protobuf::int32 id_;
  ::google::protobuf::int32 appointmentBeginHour_;
  ::google::protobuf::int32 appointmentBeginMinute_;
  ::google::protobuf::int32 appointmentEndHour_;
  ::google::protobuf::int32 appointmentEndMinute_;
  ::google::protobuf::int32 challengeEndHour_;
  ::google::protobuf::int32 challengeEndMinute_;
  ::google::protobuf::int32 enterBeginHour_;
  ::google::protobuf::int32 enterBeginMinute_;
  ::google::protobuf::int32 fightBeginHour_;
  ::google::protobuf::int32 fightBeginMinute_;
  ::google::protobuf::int32 fightEndHour_;
  ::google::protobuf::int32 fightEndMinute_;
  
  mutable int _cached_size_;
  ::google::protobuf::uint32 _has_bits_[(13 + 31) / 32];
  
  friend void  protobuf_AddDesc_Reservation_2eproto();
  friend void protobuf_AssignDesc_Reservation_2eproto();
  friend void protobuf_ShutdownFile_Reservation_2eproto();
  
  void InitAsDefaultInstance();
  static PB_ReservationTime* default_instance_;
};
// -------------------------------------------------------------------

class PB_AllReservationTimes : public ::google::protobuf::Message {
 public:
  PB_AllReservationTimes();
  virtual ~PB_AllReservationTimes();
  
  PB_AllReservationTimes(const PB_AllReservationTimes& from);
  
  inline PB_AllReservationTimes& operator=(const PB_AllReservationTimes& from) {
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
  static const PB_AllReservationTimes& default_instance();
  
  void Swap(PB_AllReservationTimes* other);
  
  // implements Message ----------------------------------------------
  
  PB_AllReservationTimes* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const PB_AllReservationTimes& from);
  void MergeFrom(const PB_AllReservationTimes& from);
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
  
  // repeated .PB_ReservationTime times = 1;
  inline int times_size() const;
  inline void clear_times();
  static const int kTimesFieldNumber = 1;
  inline const ::PB_ReservationTime& times(int index) const;
  inline ::PB_ReservationTime* mutable_times(int index);
  inline ::PB_ReservationTime* add_times();
  inline const ::google::protobuf::RepeatedPtrField< ::PB_ReservationTime >&
      times() const;
  inline ::google::protobuf::RepeatedPtrField< ::PB_ReservationTime >*
      mutable_times();
  
  // @@protoc_insertion_point(class_scope:PB_AllReservationTimes)
 private:
  
  ::google::protobuf::UnknownFieldSet _unknown_fields_;
  
  ::google::protobuf::RepeatedPtrField< ::PB_ReservationTime > times_;
  
  mutable int _cached_size_;
  ::google::protobuf::uint32 _has_bits_[(1 + 31) / 32];
  
  friend void  protobuf_AddDesc_Reservation_2eproto();
  friend void protobuf_AssignDesc_Reservation_2eproto();
  friend void protobuf_ShutdownFile_Reservation_2eproto();
  
  void InitAsDefaultInstance();
  static PB_AllReservationTimes* default_instance_;
};
// ===================================================================


// ===================================================================

// PB_ReservationTime

// optional int32 id = 1;
inline bool PB_ReservationTime::has_id() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void PB_ReservationTime::set_has_id() {
  _has_bits_[0] |= 0x00000001u;
}
inline void PB_ReservationTime::clear_has_id() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void PB_ReservationTime::clear_id() {
  id_ = 0;
  clear_has_id();
}
inline ::google::protobuf::int32 PB_ReservationTime::id() const {
  return id_;
}
inline void PB_ReservationTime::set_id(::google::protobuf::int32 value) {
  set_has_id();
  id_ = value;
}

// optional int32 appointmentBeginHour = 2;
inline bool PB_ReservationTime::has_appointmentBeginHour() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void PB_ReservationTime::set_has_appointmentBeginHour() {
  _has_bits_[0] |= 0x00000002u;
}
inline void PB_ReservationTime::clear_has_appointmentBeginHour() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void PB_ReservationTime::clear_appointmentBeginHour() {
  appointmentBeginHour_ = 0;
  clear_has_appointmentBeginHour();
}
inline ::google::protobuf::int32 PB_ReservationTime::appointmentBeginHour() const {
  return appointmentBeginHour_;
}
inline void PB_ReservationTime::set_appointmentBeginHour(::google::protobuf::int32 value) {
  set_has_appointmentBeginHour();
  appointmentBeginHour_ = value;
}

// optional int32 appointmentBeginMinute = 3;
inline bool PB_ReservationTime::has_appointmentBeginMinute() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void PB_ReservationTime::set_has_appointmentBeginMinute() {
  _has_bits_[0] |= 0x00000004u;
}
inline void PB_ReservationTime::clear_has_appointmentBeginMinute() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void PB_ReservationTime::clear_appointmentBeginMinute() {
  appointmentBeginMinute_ = 0;
  clear_has_appointmentBeginMinute();
}
inline ::google::protobuf::int32 PB_ReservationTime::appointmentBeginMinute() const {
  return appointmentBeginMinute_;
}
inline void PB_ReservationTime::set_appointmentBeginMinute(::google::protobuf::int32 value) {
  set_has_appointmentBeginMinute();
  appointmentBeginMinute_ = value;
}

// optional int32 appointmentEndHour = 4;
inline bool PB_ReservationTime::has_appointmentEndHour() const {
  return (_has_bits_[0] & 0x00000008u) != 0;
}
inline void PB_ReservationTime::set_has_appointmentEndHour() {
  _has_bits_[0] |= 0x00000008u;
}
inline void PB_ReservationTime::clear_has_appointmentEndHour() {
  _has_bits_[0] &= ~0x00000008u;
}
inline void PB_ReservationTime::clear_appointmentEndHour() {
  appointmentEndHour_ = 0;
  clear_has_appointmentEndHour();
}
inline ::google::protobuf::int32 PB_ReservationTime::appointmentEndHour() const {
  return appointmentEndHour_;
}
inline void PB_ReservationTime::set_appointmentEndHour(::google::protobuf::int32 value) {
  set_has_appointmentEndHour();
  appointmentEndHour_ = value;
}

// optional int32 appointmentEndMinute = 5;
inline bool PB_ReservationTime::has_appointmentEndMinute() const {
  return (_has_bits_[0] & 0x00000010u) != 0;
}
inline void PB_ReservationTime::set_has_appointmentEndMinute() {
  _has_bits_[0] |= 0x00000010u;
}
inline void PB_ReservationTime::clear_has_appointmentEndMinute() {
  _has_bits_[0] &= ~0x00000010u;
}
inline void PB_ReservationTime::clear_appointmentEndMinute() {
  appointmentEndMinute_ = 0;
  clear_has_appointmentEndMinute();
}
inline ::google::protobuf::int32 PB_ReservationTime::appointmentEndMinute() const {
  return appointmentEndMinute_;
}
inline void PB_ReservationTime::set_appointmentEndMinute(::google::protobuf::int32 value) {
  set_has_appointmentEndMinute();
  appointmentEndMinute_ = value;
}

// optional int32 challengeEndHour = 6;
inline bool PB_ReservationTime::has_challengeEndHour() const {
  return (_has_bits_[0] & 0x00000020u) != 0;
}
inline void PB_ReservationTime::set_has_challengeEndHour() {
  _has_bits_[0] |= 0x00000020u;
}
inline void PB_ReservationTime::clear_has_challengeEndHour() {
  _has_bits_[0] &= ~0x00000020u;
}
inline void PB_ReservationTime::clear_challengeEndHour() {
  challengeEndHour_ = 0;
  clear_has_challengeEndHour();
}
inline ::google::protobuf::int32 PB_ReservationTime::challengeEndHour() const {
  return challengeEndHour_;
}
inline void PB_ReservationTime::set_challengeEndHour(::google::protobuf::int32 value) {
  set_has_challengeEndHour();
  challengeEndHour_ = value;
}

// optional int32 challengeEndMinute = 7;
inline bool PB_ReservationTime::has_challengeEndMinute() const {
  return (_has_bits_[0] & 0x00000040u) != 0;
}
inline void PB_ReservationTime::set_has_challengeEndMinute() {
  _has_bits_[0] |= 0x00000040u;
}
inline void PB_ReservationTime::clear_has_challengeEndMinute() {
  _has_bits_[0] &= ~0x00000040u;
}
inline void PB_ReservationTime::clear_challengeEndMinute() {
  challengeEndMinute_ = 0;
  clear_has_challengeEndMinute();
}
inline ::google::protobuf::int32 PB_ReservationTime::challengeEndMinute() const {
  return challengeEndMinute_;
}
inline void PB_ReservationTime::set_challengeEndMinute(::google::protobuf::int32 value) {
  set_has_challengeEndMinute();
  challengeEndMinute_ = value;
}

// optional int32 enterBeginHour = 8;
inline bool PB_ReservationTime::has_enterBeginHour() const {
  return (_has_bits_[0] & 0x00000080u) != 0;
}
inline void PB_ReservationTime::set_has_enterBeginHour() {
  _has_bits_[0] |= 0x00000080u;
}
inline void PB_ReservationTime::clear_has_enterBeginHour() {
  _has_bits_[0] &= ~0x00000080u;
}
inline void PB_ReservationTime::clear_enterBeginHour() {
  enterBeginHour_ = 0;
  clear_has_enterBeginHour();
}
inline ::google::protobuf::int32 PB_ReservationTime::enterBeginHour() const {
  return enterBeginHour_;
}
inline void PB_ReservationTime::set_enterBeginHour(::google::protobuf::int32 value) {
  set_has_enterBeginHour();
  enterBeginHour_ = value;
}

// optional int32 enterBeginMinute = 9;
inline bool PB_ReservationTime::has_enterBeginMinute() const {
  return (_has_bits_[0] & 0x00000100u) != 0;
}
inline void PB_ReservationTime::set_has_enterBeginMinute() {
  _has_bits_[0] |= 0x00000100u;
}
inline void PB_ReservationTime::clear_has_enterBeginMinute() {
  _has_bits_[0] &= ~0x00000100u;
}
inline void PB_ReservationTime::clear_enterBeginMinute() {
  enterBeginMinute_ = 0;
  clear_has_enterBeginMinute();
}
inline ::google::protobuf::int32 PB_ReservationTime::enterBeginMinute() const {
  return enterBeginMinute_;
}
inline void PB_ReservationTime::set_enterBeginMinute(::google::protobuf::int32 value) {
  set_has_enterBeginMinute();
  enterBeginMinute_ = value;
}

// optional int32 fightBeginHour = 10;
inline bool PB_ReservationTime::has_fightBeginHour() const {
  return (_has_bits_[0] & 0x00000200u) != 0;
}
inline void PB_ReservationTime::set_has_fightBeginHour() {
  _has_bits_[0] |= 0x00000200u;
}
inline void PB_ReservationTime::clear_has_fightBeginHour() {
  _has_bits_[0] &= ~0x00000200u;
}
inline void PB_ReservationTime::clear_fightBeginHour() {
  fightBeginHour_ = 0;
  clear_has_fightBeginHour();
}
inline ::google::protobuf::int32 PB_ReservationTime::fightBeginHour() const {
  return fightBeginHour_;
}
inline void PB_ReservationTime::set_fightBeginHour(::google::protobuf::int32 value) {
  set_has_fightBeginHour();
  fightBeginHour_ = value;
}

// optional int32 fightBeginMinute = 11;
inline bool PB_ReservationTime::has_fightBeginMinute() const {
  return (_has_bits_[0] & 0x00000400u) != 0;
}
inline void PB_ReservationTime::set_has_fightBeginMinute() {
  _has_bits_[0] |= 0x00000400u;
}
inline void PB_ReservationTime::clear_has_fightBeginMinute() {
  _has_bits_[0] &= ~0x00000400u;
}
inline void PB_ReservationTime::clear_fightBeginMinute() {
  fightBeginMinute_ = 0;
  clear_has_fightBeginMinute();
}
inline ::google::protobuf::int32 PB_ReservationTime::fightBeginMinute() const {
  return fightBeginMinute_;
}
inline void PB_ReservationTime::set_fightBeginMinute(::google::protobuf::int32 value) {
  set_has_fightBeginMinute();
  fightBeginMinute_ = value;
}

// optional int32 fightEndHour = 12;
inline bool PB_ReservationTime::has_fightEndHour() const {
  return (_has_bits_[0] & 0x00000800u) != 0;
}
inline void PB_ReservationTime::set_has_fightEndHour() {
  _has_bits_[0] |= 0x00000800u;
}
inline void PB_ReservationTime::clear_has_fightEndHour() {
  _has_bits_[0] &= ~0x00000800u;
}
inline void PB_ReservationTime::clear_fightEndHour() {
  fightEndHour_ = 0;
  clear_has_fightEndHour();
}
inline ::google::protobuf::int32 PB_ReservationTime::fightEndHour() const {
  return fightEndHour_;
}
inline void PB_ReservationTime::set_fightEndHour(::google::protobuf::int32 value) {
  set_has_fightEndHour();
  fightEndHour_ = value;
}

// optional int32 fightEndMinute = 13;
inline bool PB_ReservationTime::has_fightEndMinute() const {
  return (_has_bits_[0] & 0x00001000u) != 0;
}
inline void PB_ReservationTime::set_has_fightEndMinute() {
  _has_bits_[0] |= 0x00001000u;
}
inline void PB_ReservationTime::clear_has_fightEndMinute() {
  _has_bits_[0] &= ~0x00001000u;
}
inline void PB_ReservationTime::clear_fightEndMinute() {
  fightEndMinute_ = 0;
  clear_has_fightEndMinute();
}
inline ::google::protobuf::int32 PB_ReservationTime::fightEndMinute() const {
  return fightEndMinute_;
}
inline void PB_ReservationTime::set_fightEndMinute(::google::protobuf::int32 value) {
  set_has_fightEndMinute();
  fightEndMinute_ = value;
}

// -------------------------------------------------------------------

// PB_AllReservationTimes

// repeated .PB_ReservationTime times = 1;
inline int PB_AllReservationTimes::times_size() const {
  return times_.size();
}
inline void PB_AllReservationTimes::clear_times() {
  times_.Clear();
}
inline const ::PB_ReservationTime& PB_AllReservationTimes::times(int index) const {
  return times_.Get(index);
}
inline ::PB_ReservationTime* PB_AllReservationTimes::mutable_times(int index) {
  return times_.Mutable(index);
}
inline ::PB_ReservationTime* PB_AllReservationTimes::add_times() {
  return times_.Add();
}
inline const ::google::protobuf::RepeatedPtrField< ::PB_ReservationTime >&
PB_AllReservationTimes::times() const {
  return times_;
}
inline ::google::protobuf::RepeatedPtrField< ::PB_ReservationTime >*
PB_AllReservationTimes::mutable_times() {
  return &times_;
}


// @@protoc_insertion_point(namespace_scope)

#ifndef SWIG
namespace google {
namespace protobuf {


}  // namespace google
}  // namespace protobuf
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_Reservation_2eproto__INCLUDED
