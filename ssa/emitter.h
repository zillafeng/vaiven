#ifndef VAIVEN_VISITOR_HEADER_SSA_EMITTER
#define VAIVEN_VISITOR_HEADER_SSA_EMITTER
#include "forward_visitor.h"
#include "../functions.h"

namespace vaiven { namespace ssa {

class Emitter : public ForwardVisitor {

  public:
  Emitter(asmjit::X86Compiler& cc, Functions& funcs) : cc(cc), funcs(funcs) {};

  void visitPhiInstr(PhiInstr& instr);
  void visitArgInstr(ArgInstr& instr);
  void visitConstantInstr(ConstantInstr& instr);
  void visitCallInstr(CallInstr& instr);
  void visitTypecheckInstr(TypecheckInstr& instr);
  void visitBoxInstr(BoxInstr& instr);
  void visitAddInstr(AddInstr& instr);
  void visitSubInstr(SubInstr& instr);
  void visitMulInstr(MulInstr& instr);
  void visitDivInstr(DivInstr& instr);
  void visitNotInstr(NotInstr& instr);
  void visitCmpEqInstr(CmpEqInstr& instr);
  void visitCmpIneqInstr(CmpIneqInstr& instr);
  void visitCmpGtInstr(CmpGtInstr& instr);
  void visitCmpGteInstr(CmpGteInstr& instr);
  void visitCmpLtInstr(CmpLtInstr& instr);
  void visitCmpLteInstr(CmpLteInstr& instr);
  void doCmpEqInstr(CmpEqInstr& instr);
  void doCmpIneqInstr(CmpIneqInstr& instr);
  void doCmpGtInstr(CmpGtInstr& instr);
  void doCmpGteInstr(CmpGteInstr& instr);
  void doCmpLtInstr(CmpLtInstr& instr);
  void doCmpLteInstr(CmpLteInstr& instr);
  void visitErrInstr(ErrInstr& instr);
  void visitRetInstr(RetInstr& instr);
  void visitJmpCcInstr(JmpCcInstr& instr);

  void visitUnconditionalBlockExit(UnconditionalBlockExit& exit);
  void visitConditionalBlockExit(ConditionalBlockExit& exit);
  void visitBlock(Block& block);

  asmjit::X86Compiler& cc;
  Functions& funcs;
};

}}

#endif