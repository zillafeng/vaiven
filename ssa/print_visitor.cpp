#include "print_visitor.h"

#include <iostream>

using namespace vaiven::ssa;
using namespace std;

int PrintVisitor::varId(Instruction* instr) {
  if (varIds.find(instr) == varIds.end()) {
    varIds[instr] = varIds.size();
  }

  return varIds[instr];
}

void PrintVisitor::printInstruction(string prefix, Instruction* instr) {
  cout << varId(instr) << ": " << prefix;

  for (vector<Instruction*>::iterator it = instr->inputs.begin();
      it != instr->inputs.end();
      ++it) {
    if(it != instr->inputs.begin()) {
      cout << ",";
    }
    cout << " " << varId(*it);
  }

  cout << endl;
}

void PrintVisitor::visitPhiInstr(PhiInstr& instr) {
  printInstruction("phi", &instr);
}

void PrintVisitor::visitArgInstr(ArgInstr& instr) {
  cout << varId(&instr) << ": arg" << instr.argi << endl;
}

void PrintVisitor::visitConstantInstr(ConstantInstr& instr) {
  cout << varId(&instr) << ": val" << instr.val.getRaw() << endl;
}

void PrintVisitor::visitCallInstr(CallInstr& instr) {
  printInstruction(instr.funcName, &instr);
}

void PrintVisitor::visitTypecheckInstr(TypecheckInstr& instr) {
  printInstruction("typecheck", &instr);
}

void PrintVisitor::visitBoxInstr(BoxInstr& instr) {
  printInstruction("box", &instr);
}

void PrintVisitor::visitAddInstr(AddInstr& instr) {
  printInstruction("add", &instr);
}

void PrintVisitor::visitIntAddInstr(IntAddInstr& instr) {
  printInstruction("int add", &instr);
}

void PrintVisitor::visitStrAddInstr(StrAddInstr& instr) {
  printInstruction("str add", &instr);
}

void PrintVisitor::visitSubInstr(SubInstr& instr) {
  printInstruction("sub", &instr);
}

void PrintVisitor::visitMulInstr(MulInstr& instr) {
  printInstruction("mul", &instr);
}

void PrintVisitor::visitDivInstr(DivInstr& instr) {
  printInstruction("div", &instr);
}

void PrintVisitor::visitNotInstr(NotInstr& instr) {
  printInstruction("not", &instr);
}

void PrintVisitor::visitCmpEqInstr(CmpEqInstr& instr) {
  printInstruction("==", &instr);
}

void PrintVisitor::visitCmpIneqInstr(CmpIneqInstr& instr) {
  printInstruction("!=", &instr);
}

void PrintVisitor::visitCmpGtInstr(CmpGtInstr& instr) {
  printInstruction("gt", &instr);
}

void PrintVisitor::visitCmpGteInstr(CmpGteInstr& instr) {
  printInstruction("gte", &instr);
}

void PrintVisitor::visitCmpLtInstr(CmpLtInstr& instr) {
  printInstruction("lt", &instr);
}

void PrintVisitor::visitCmpLteInstr(CmpLteInstr& instr) {
  printInstruction("lte", &instr);
}

void PrintVisitor::visitDynamicAccessInstr(DynamicAccessInstr& instr) {
  printInstruction("dyn acc", &instr);
}

void PrintVisitor::visitDynamicStoreInstr(DynamicStoreInstr& instr) {
  printInstruction("dyn str", &instr);
}

void PrintVisitor::visitListAccessInstr(ListAccessInstr& instr) {
  printInstruction("list acc", &instr);
}

void PrintVisitor::visitListStoreInstr(ListStoreInstr& instr) {
  printInstruction("list str", &instr);
}

void PrintVisitor::visitListInitInstr(ListInitInstr& instr) {
  printInstruction("list init", &instr);
}

void PrintVisitor::visitDynamicObjectAccessInstr(DynamicObjectAccessInstr& instr) {
  printInstruction("dyn obj acc", &instr);
}

void PrintVisitor::visitDynamicObjectStoreInstr(DynamicObjectStoreInstr& instr) {
  printInstruction("dyn obj str", &instr);
}

void PrintVisitor::visitObjectAccessInstr(ObjectAccessInstr& instr) {
  printInstruction("obj acc", &instr);
}

void PrintVisitor::visitObjectStoreInstr(ObjectStoreInstr& instr) {
  printInstruction("obj str", &instr);
}

void PrintVisitor::visitErrInstr(ErrInstr& instr) {
  printInstruction("err", &instr);
}

void PrintVisitor::visitRetInstr(RetInstr& instr) {
  printInstruction("ret", &instr);
}

void PrintVisitor::visitJmpCcInstr(JmpCcInstr& instr) {
  printInstruction("jmpcc", &instr);
}

void PrintVisitor::visitBlock(Block& block) {
  cout << "block " << &block << endl;
  if (block.loopHeaderWithBlocks.size()) {
    cout << "which is a loop header for: ";
    for (set<Block*>::iterator it = block.loopHeaderWithBlocks.begin();
        it != block.loopHeaderWithBlocks.end();
        ++it) {
      cout << *it << ", ";
    }
    cout << endl;
  }
    cout << "with dominators ";
    for (set<Block*>::iterator it = block.dominators.begin();
        it != block.dominators.end();
        ++it) {
      cout << *it << ", ";
    }
    cout << endl;
  ForwardVisitor::visitBlock(block);
}

void PrintVisitor::visitUnconditionalBlockExit(UnconditionalBlockExit& exit) {
  cout << "goto " << exit.toGoTo << endl;
}

void PrintVisitor::visitConditionalBlockExit(ConditionalBlockExit& exit) {
  ForwardVisitor::visitConditionalBlockExit(exit);
  cout << "if " << varId(&*exit.condition) << " goto " << exit.toGoTo << endl;
}
