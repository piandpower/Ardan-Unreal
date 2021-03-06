// automatically generated by the FlatBuffers compiler, do not modify

#ifndef FLATBUFFERS_GENERATED_UNREALPKTS_UNREALCOOJAMSG_H_
#define FLATBUFFERS_GENERATED_UNREALPKTS_UNREALCOOJAMSG_H_

#include "flatbuffers/flatbuffers.h"


namespace UnrealCoojaMsg {

struct Vec3;
struct Message;

enum MsgType {
  MsgType_LED = 1,
  MsgType_LOCATION = 2,
  MsgType_RADIO = 3,
  MsgType_PIR = 4,
  MsgType_PAUSE = 5,
  MsgType_RESUME = 6,
  MsgType_SPEED_NORM = 7,
  MsgType_SPEED_SLOW = 8,
  MsgType_SPEED_FAST = 9
};

inline const char **EnumNamesMsgType() {
  static const char *names[] = { "LED", "LOCATION", "RADIO", "PIR", "PAUSE", "RESUME", "SPEED_NORM", "SPEED_SLOW", "SPEED_FAST", nullptr };
  return names;
}

inline const char *EnumNameMsgType(MsgType e) { return EnumNamesMsgType()[static_cast<int>(e) - static_cast<int>(MsgType_LED)]; }

MANUALLY_ALIGNED_STRUCT(4) Vec3 FLATBUFFERS_FINAL_CLASS {
 private:
  float x_;
  float y_;
  float z_;

 public:
  Vec3(float _x, float _y, float _z)
    : x_(flatbuffers::EndianScalar(_x)), y_(flatbuffers::EndianScalar(_y)), z_(flatbuffers::EndianScalar(_z)) { }

  float x() const { return flatbuffers::EndianScalar(x_); }
  float y() const { return flatbuffers::EndianScalar(y_); }
  float z() const { return flatbuffers::EndianScalar(z_); }
};
STRUCT_END(Vec3, 12);

struct Message FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_ID = 4,
    VT_TYPE = 6,
    VT_LOCATION = 8,
  };
  int32_t id() const { return GetField<int32_t>(VT_ID, 0); }
  int32_t type() const { return GetField<int32_t>(VT_TYPE, 0); }
  const Vec3 *location() const { return GetStruct<const Vec3 *>(VT_LOCATION); }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int32_t>(verifier, VT_ID) &&
           VerifyField<int32_t>(verifier, VT_TYPE) &&
           VerifyField<Vec3>(verifier, VT_LOCATION) &&
           verifier.EndTable();
  }
};

struct MessageBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_id(int32_t id) { fbb_.AddElement<int32_t>(Message::VT_ID, id, 0); }
  void add_type(int32_t type) { fbb_.AddElement<int32_t>(Message::VT_TYPE, type, 0); }
  void add_location(const Vec3 *location) { fbb_.AddStruct(Message::VT_LOCATION, location); }
  MessageBuilder(flatbuffers::FlatBufferBuilder &_fbb) : fbb_(_fbb) { start_ = fbb_.StartTable(); }
  MessageBuilder &operator=(const MessageBuilder &);
  flatbuffers::Offset<Message> Finish() {
    auto o = flatbuffers::Offset<Message>(fbb_.EndTable(start_, 3));
    return o;
  }
};

inline flatbuffers::Offset<Message> CreateMessage(flatbuffers::FlatBufferBuilder &_fbb,
   int32_t id = 0,
   int32_t type = 0,
   const Vec3 *location = 0) {
  MessageBuilder builder_(_fbb);
  builder_.add_location(location);
  builder_.add_type(type);
  builder_.add_id(id);
  return builder_.Finish();
}

}  // namespace UnrealCoojaMsg

#endif  // FLATBUFFERS_GENERATED_UNREALPKTS_UNREALCOOJAMSG_H_
