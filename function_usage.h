#ifndef VAIVEN_HEADER_FUNCTION_USAGE
#define VAIVEN_HEADER_FUNCTION_USAGE

#include "inttypes.h"

#include "value.h"
#include "type_info.h"

namespace vaiven {

const short INT_SHAPE = INT_TAG >> 48;
const short BOOL_SHAPE = BOOL_TAG >> 48;
const short VOID_SHAPE = VOID >> 48;
const short DOUBLE_SHAPE = 0x08;
const short LIST_SHAPE = 0x10;
const short STRING_SHAPE = 0x20;
const short OBJECT_SHAPE = 0x40;
const short PRIMITIVE_TAG_SHIFT = 48;
const short POINTER_TAG_SHIFT = 4;

const int HOT_COUNT = 10;

class ArgumentShape {
  public:
  short raw;

  inline bool isPure() {
    // is power of two trick
    return raw && !(raw & (raw - 1));
  }

  inline bool isPureInt() {
    return (raw & ~INT_SHAPE) == 0;
  }

  inline bool isPureBool() {
    return (raw & ~BOOL_SHAPE) == 0;
  }

  inline bool isPureVoid() {
    return (raw & ~VOID_SHAPE) == 0;
  }

  inline bool isPureDouble() {
    return (raw & ~DOUBLE_SHAPE) == 0;
  }

  inline bool isPureObject() {
    return (raw & ~OBJECT_SHAPE) == 0;
  }

  inline bool isPureList() {
    return (raw & ~LIST_SHAPE) == 0;
  }

  inline bool isPureString() {
    return (raw & ~STRING_SHAPE) == 0;
  }

  inline VaivenStaticType getStaticType() {
    if (!isPure()) {
      return VAIVEN_STATIC_TYPE_UNKNOWN;
    } else if (isPureInt()) {
      return VAIVEN_STATIC_TYPE_INT;
    } else if (isPureVoid()) {
      return VAIVEN_STATIC_TYPE_VOID;
    } else if (isPureDouble()) {
      return VAIVEN_STATIC_TYPE_DOUBLE;
    } else if (isPureObject()) {
      return VAIVEN_STATIC_TYPE_OBJECT;
    } else if (isPureList()) {
      return VAIVEN_STATIC_TYPE_LIST;
    } else if (isPureString()) {
      return VAIVEN_STATIC_TYPE_STRING;
    }

    return VAIVEN_STATIC_TYPE_UNKNOWN;
  }
  
};

class FunctionUsage {
  public:
  int32_t count;
  // variably sized -- allocate with new (alloca(...)) FunctionUsage
  ArgumentShape argShapes[0];
};

}

#endif
