#ifndef VAIVEN_VISITOR_HEADER_VALUE
#define VAIVEN_VISITOR_HEADER_VALUE

#include "inttypes.h"
#include "type_info.h"

namespace vaiven {

// the largest double is nan
// 11111111 11111000 00000000 00000000 00000000 00000000 00000000 00000000
// largest ptr is <= 0x00007FFFFFFFFFFF
// 00000000 00000000 01111111 11111111 11111111 11111111 11111111 11111111
// if we flip NaN, we can say all doubles are >= 0x0007FFFFFFFFFFFF
// 00000000 00000111 11111111 11111111 11111111 11111111 11111111 11111111
// that means we can distinguish some magical values
// 00000000 00000000 10000000 00000000 00000000 00000000 00000000 00000001 true
// 00000000 00000000 10000000 00000000 00000000 00000000 00000000 00000000 false
// 00000000 00000001 00000000 00000000 00000000 00000000 00000000 00000010 void
// 00000000 00000010 00000000 00000000 ........ ........ ........ ........ 32 bit int

const uint64_t MAX_PTR  = 0x00007FFFFFFFFFFF;
const uint64_t MIN_DBL  = 0x0007FFFFFFFFFFFF;
const uint64_t FALSE    = 0x0000800000000000;
const uint64_t TRUE     = 0x0000800000000001;
const uint64_t VOID     = 0x0002000000000000;
const uint64_t INT_TAG  = 0x0001000000000000;
const uint64_t BOOL_TAG = 0x0000800000000000;
const uint64_t VALUE_TAG_SHIFT  = 32;
const uint64_t INT_TAG_SHIFTED  = 0x00010000;
const uint64_t BOOL_TAG_SHIFTED = 0x00008000;

struct ValueAfterHeader {
  int32_t asInt;
  uint32_t header;
};

class Gcable;

class Value {
  private:

  union {
    Gcable* asPtr;
    double asDouble;
    uint64_t raw;
    ValueAfterHeader withHeader;
  } internals;

  public:

  inline Value(int32_t fromInt) {
    internals.withHeader.header = INT_TAG_SHIFTED;
    internals.withHeader.asInt = fromInt;
  }

  inline Value(bool fromBool) {
    internals.raw = fromBool ? TRUE : FALSE;
  }

  inline Value(Gcable* fromPtr) {
    internals.asPtr = fromPtr;
  }

  inline Value(double fromDouble) {
    internals.asDouble = fromDouble;
    internals.raw = ~internals.raw;
  }

  inline Value() {
    internals.raw = VOID;
  }

  inline bool isInt() const {
    return internals.raw >> VALUE_TAG_SHIFT == INT_TAG_SHIFTED;
  }

  inline bool isPtr() const {
    return internals.raw <= MAX_PTR;
  }

  inline bool isDouble() const {
    return internals.raw >= MIN_DBL;
  }

  inline bool isBool() const {
    return internals.raw >> VALUE_TAG_SHIFT == BOOL_TAG_SHIFTED;
  }

  inline bool isTrue() const {
    return internals.raw == TRUE;
  }

  inline bool isFalse() const {
    return internals.raw == FALSE;
  }

  inline bool isVoid() const {
    return internals.raw == VOID;
  }

  inline int32_t getInt() const {
    return internals.withHeader.asInt;
  }

  inline Gcable* getPtr() const {
    return internals.asPtr;
  }
    
  inline double getDouble() const {
    Value val = *this;
    val.internals.raw = ~val.internals.raw;
    return val.internals.asDouble;
  }

  inline bool getBool() const {
    return internals.withHeader.asInt;
  }

  inline uint64_t getRaw() const {
    return internals.raw;
  }

  VaivenStaticType getStaticType() const;
};

}

#endif
