#ifndef VAIVEN_VISITOR_HEADER_LOCATION
#define VAIVEN_VISITOR_HEADER_LOCATION

#include "asmjit/src/asmjit/asmjit.h"

namespace vaiven {

enum LocationType {
  LOCATION_TYPE_REG,
  LOCATION_TYPE_ARG,
  LOCATION_TYPE_IMM,
  LOCATION_TYPE_SPILLED,
  LOCATION_TYPE_VOID,
};

typedef union {
    asmjit::X86Gp* reg;
    int imm;
    int argIndex;
} LocationDataUnion;

class Location {

  public:
  static Location imm(int val);
  static Location arg(int val);
  static Location void_();
  static Location spilled();

  Location(asmjit::X86Gp* reg) : data(), type(LOCATION_TYPE_REG) {
    data.reg = reg;
  }

  Location(LocationType type, LocationDataUnion data) : data(data), type(type) { }
  Location() : type(LOCATION_TYPE_VOID), data() { }

  LocationType type;
  LocationDataUnion data;

  const asmjit::X86Gp* getReg();

  asmjit::X86Mem getArgPtr();

};

}

#endif
