#include "constant_propagation.h"

#include "../std.h"

using namespace vaiven::ssa;
using namespace std;
using namespace asmjit;

void ConstantPropagator::visitPhiInstr(PhiInstr& instr) {
}

void ConstantPropagator::visitArgInstr(ArgInstr& instr) {
}

void ConstantPropagator::visitConstantInstr(ConstantInstr& instr) {
}

void ConstantPropagator::visitCallInstr(CallInstr& instr) {
}

void ConstantPropagator::visitTypecheckInstr(TypecheckInstr& instr) {
}

void ConstantPropagator::visitBoxInstr(BoxInstr& instr) {
}

void ConstantPropagator::replaceWithConstant(Instruction& instr, Value newVal) {
  Instruction* newConstant = new ConstantInstr(newVal);
  replace(instr, newConstant);
}

void append(Instruction& instr, Instruction* toAppend) {
  Instruction* next = instr.next;
  toAppend->next = next;
  instr.next = toAppend;
}

void ConstantPropagator::replace(Instruction& oldInstr, Instruction* newInstr) {
  if (oldInstr.usages.size() == 0) {
    return;
  }

  // add the constant that will replace this to the instruction list. Let dead code
  // elim remove this node later.
  oldInstr.append(newInstr);

  oldInstr.replaceUsagesWith(newInstr);
  performedWork = true;
}

bool ConstantPropagator::isConstantBinIntInstruction(Instruction& instr) {
  return instr.inputs[0]->tag == INSTR_CONSTANT
      && instr.inputs[1]->tag == INSTR_CONSTANT
      && instr.inputs[0]->type == VAIVEN_STATIC_TYPE_INT
      && instr.inputs[1]->type == VAIVEN_STATIC_TYPE_INT;
}

void ConstantPropagator::visitAddInstr(AddInstr& instr) {
  // TODO can this have constant values?
}

void ConstantPropagator::visitStrAddInstr(StrAddInstr& instr) {
  if (instr.inputs[0]->tag == INSTR_CONSTANT
      && instr.inputs[1]->tag == INSTR_CONSTANT
      && instr.inputs[0]->type == VAIVEN_STATIC_TYPE_STRING
      && instr.inputs[1]->type == VAIVEN_STATIC_TYPE_STRING) {
    Value valA = static_cast<ConstantInstr*>(instr.inputs[0])->val;
    Value valB = static_cast<ConstantInstr*>(instr.inputs[1])->val;
    string strA = ((GcableString*) valA.getPtr())->str;
    string strB = ((GcableString*) valB.getPtr())->str;

    // TODO generate a constant that's collected properly
    replaceWithConstant(instr, Value(new GcableString(strA + strB)));
  }
}

void ConstantPropagator::visitIntAddInstr(IntAddInstr& instr) {
  if (isConstantBinIntInstruction(instr)) {
    int newval = static_cast<ConstantInstr*>(instr.inputs[0])->val.getInt()
        + static_cast<ConstantInstr*>(instr.inputs[1])->val.getInt();

    replaceWithConstant(instr, Value(newval));
  }
}

void ConstantPropagator::visitSubInstr(SubInstr& instr) {
  if (isConstantBinIntInstruction(instr)) {
    int newval = static_cast<ConstantInstr*>(instr.inputs[0])->val.getInt()
        - static_cast<ConstantInstr*>(instr.inputs[1])->val.getInt();

    replaceWithConstant(instr, Value(newval));
  }
}

void ConstantPropagator::visitMulInstr(MulInstr& instr) {
  if (isConstantBinIntInstruction(instr)) {
    int newval = static_cast<ConstantInstr*>(instr.inputs[0])->val.getInt()
        * static_cast<ConstantInstr*>(instr.inputs[1])->val.getInt();

    replaceWithConstant(instr, Value(newval));
  }
}

void ConstantPropagator::visitDivInstr(DivInstr& instr) {
  if (isConstantBinIntInstruction(instr)) {
    int newval = static_cast<ConstantInstr*>(instr.inputs[0])->val.getInt()
        / static_cast<ConstantInstr*>(instr.inputs[1])->val.getInt();

    replaceWithConstant(instr, Value(newval));
  }
}

void ConstantPropagator::visitNotInstr(NotInstr& instr) {
  if (instr.inputs[0]->tag == INSTR_CONSTANT) {
    bool newval = static_cast<ConstantInstr*>(instr.inputs[0])->val.getBool();

    replaceWithConstant(instr, Value(!newval));
  }
}

void ConstantPropagator::visitCmpEqInstr(CmpEqInstr& instr) {
  if (isConstantBinIntInstruction(instr)) {
    bool newval = cmpUnboxed(static_cast<ConstantInstr*>(instr.inputs[0])->val,
        static_cast<ConstantInstr*>(instr.inputs[1])->val);

    replaceWithConstant(instr, Value(newval));
  }
}

void ConstantPropagator::visitCmpIneqInstr(CmpIneqInstr& instr) {
  if (isConstantBinIntInstruction(instr)) {
    bool newval = inverseCmpUnboxed(static_cast<ConstantInstr*>(instr.inputs[0])->val,
        static_cast<ConstantInstr*>(instr.inputs[1])->val);

    replaceWithConstant(instr, Value(newval));
  }
}

void ConstantPropagator::visitCmpGtInstr(CmpGtInstr& instr) {
  if (isConstantBinIntInstruction(instr)) {
    bool newval = static_cast<ConstantInstr*>(instr.inputs[0])->val.getInt()
        > static_cast<ConstantInstr*>(instr.inputs[1])->val.getInt();

    replaceWithConstant(instr, Value(newval));
  }
}

void ConstantPropagator::visitCmpGteInstr(CmpGteInstr& instr) {
  if (isConstantBinIntInstruction(instr)) {
    bool newval = static_cast<ConstantInstr*>(instr.inputs[0])->val.getInt()
        >= static_cast<ConstantInstr*>(instr.inputs[1])->val.getInt();

    replaceWithConstant(instr, Value(newval));
  }
}

void ConstantPropagator::visitCmpLtInstr(CmpLtInstr& instr) {
  if (isConstantBinIntInstruction(instr)) {
    bool newval = static_cast<ConstantInstr*>(instr.inputs[0])->val.getInt()
        < static_cast<ConstantInstr*>(instr.inputs[1])->val.getInt();

    replaceWithConstant(instr, Value(newval));
  }
}

void ConstantPropagator::visitCmpLteInstr(CmpLteInstr& instr) {
  if (isConstantBinIntInstruction(instr)) {
    bool newval = static_cast<ConstantInstr*>(instr.inputs[0])->val.getInt()
        <= static_cast<ConstantInstr*>(instr.inputs[1])->val.getInt();

    replaceWithConstant(instr, Value(newval));
  }
}

void ConstantPropagator::visitDynamicAccessInstr(DynamicAccessInstr& instr) {
  // todo track "constant" lists/objects
}

void ConstantPropagator::visitDynamicStoreInstr(DynamicStoreInstr& instr) {
  // todo track "constant" lists/objects
}

void ConstantPropagator::visitListAccessInstr(ListAccessInstr& instr) {
  // todo track "constant" lists
}

void ConstantPropagator::visitListStoreInstr(ListStoreInstr& instr) {
  // todo track "constant" lists
}

void ConstantPropagator::visitListInitInstr(ListInitInstr& instr) {
  // todo track "constant" lists
}

void ConstantPropagator::visitDynamicObjectAccessInstr(DynamicObjectAccessInstr& instr) {
  // todo track "constant" objects
}

void ConstantPropagator::visitDynamicObjectStoreInstr(DynamicObjectStoreInstr& instr) {
  // todo track "constant" objects
}

void ConstantPropagator::visitObjectAccessInstr(ObjectAccessInstr& instr) {
  // todo track "constant" objects
}

void ConstantPropagator::visitObjectStoreInstr(ObjectStoreInstr& instr) {
  // todo track "constant" objects
}

void ConstantPropagator::visitErrInstr(ErrInstr& instr) {
}

void ConstantPropagator::visitRetInstr(RetInstr& instr) {
}

void ConstantPropagator::visitJmpCcInstr(JmpCcInstr& instr) {
}
