#include "autocompiler.h"

#include "../ast/all.h"
#include "../value.h"
#include "../runtime_error.h"
#include "../optimize.h"
#include "../std.h"
#include "jumping_compiler.h"

#include <iostream>
#include <stdint.h>

using namespace asmjit;
using namespace vaiven::visitor;

void AutoCompiler::compile(Node<TypedLocationInfo>& root) {
  root.accept(*this);
}

void AutoCompiler::visitIfStatement(IfStatement<TypedLocationInfo>& stmt) {
  Label lfalse = cc.newLabel();
  Label lafter = cc.newLabel();
  JumpingCompiler jc(cc, *this, lfalse, true /* jump on false */);
  stmt.condition->accept(jc);

  if (!jc.didJmp) {
    error.typecheckBool(vRegs.top(), stmt.condition->resolvedData);
    cc.cmp(vRegs.top().r32(), 0);
    vRegs.pop();
    if (jc.jmpFalse) {
      cc.je(lfalse);
    } else {
      cc.jne(lfalse);
    }
  }

  for(vector<unique_ptr<Statement<TypedLocationInfo> > >::iterator it = stmt.trueStatements.begin();
      it != stmt.trueStatements.end();
      ++it) {
    (*it)->accept(*this);
    if (vRegs.size()) {
      vRegs.pop();
    }
  }
  cc.jmp(lafter);
  cc.bind(lfalse);
  for(vector<unique_ptr<Statement<TypedLocationInfo> > >::iterator it = stmt.falseStatements.begin();
      it != stmt.falseStatements.end();
      ++it) {
    (*it)->accept(*this);
    if (vRegs.size()) {
      vRegs.pop();
    }
  }
  cc.bind(lafter);
}

void AutoCompiler::visitForCondition(ForCondition<TypedLocationInfo>& stmt) {
  Label lcheck = cc.newLabel();
  Label lafter = cc.newLabel();
  cc.bind(lcheck);

  JumpingCompiler jc(cc, *this, lafter, true /* jump on false */);
  stmt.condition->accept(jc);

  if (!jc.didJmp) {
    error.typecheckBool(vRegs.top(), stmt.condition->resolvedData);
    cc.cmp(vRegs.top().r32(), 0);
    vRegs.pop();
    if (jc.jmpFalse) {
      cc.je(lafter);
    } else {
      cc.jne(lafter);
    }
  }

  for(vector<unique_ptr<Statement<TypedLocationInfo> > >::iterator it = stmt.statements.begin();
      it != stmt.statements.end();
      ++it) {
    (*it)->accept(*this);
    if (vRegs.size()) {
      vRegs.pop();
    }
  }
  cc.jmp(lcheck);
  cc.bind(lafter);
}

void AutoCompiler::visitReturnStatement(ReturnStatement<TypedLocationInfo>& stmt) {
  stmt.expr->accept(*this);
  box(vRegs.top(), stmt.expr->resolvedData);
  cc.ret(vRegs.top());
}

void AutoCompiler::visitVarDecl(VarDecl<TypedLocationInfo>& varDecl) {
  varDecl.expr->accept(*this);

  if (varDecl.resolvedData.location.type == LOCATION_TYPE_ARG) {
    error.dupVarError();
    return;
  }

  X86Gp varReg = cc.newInt64();
  scope.put(varDecl.varname, varReg);
  cc.mov(varReg, vRegs.top());
  box(varReg, varDecl.expr->resolvedData);
  vRegs.pop();
}

void AutoCompiler::visitListLiteralExpression(ListLiteralExpression<TypedLocationInfo>& expr) {
  X86Gp size = cc.newUInt64();
  cc.mov(size, expr.items.size());
  CCFuncCall* alloc = cc.call((uint64_t) newListWithSize, FuncSignature1<uint64_t, uint64_t>());
  alloc->setArg(0, size);
  X86Gp list = cc.newUInt64();
  alloc->setRet(0, list);

  CCFuncCall* getPtr = cc.call((uint64_t) getListContainerUnchecked, FuncSignature1<uint64_t, uint64_t>());
  getPtr->setArg(0, list);
  X86Gp ptr = cc.newUInt64();
  getPtr->setRet(0, ptr);

  for (vector<unique_ptr<Expression<TypedLocationInfo> > >::iterator it = expr.items.begin();
      it != expr.items.end();
      ++it) {
    if (it != expr.items.begin()) {
      cc.add(ptr, sizeof(Value));
    }

    (*it)->accept(*this);
    cc.mov(x86::ptr(ptr), vRegs.top());
    vRegs.pop();
  }

  vRegs.push(list);
}

void AutoCompiler::visitDynamicAccessExpression(DynamicAccessExpression<TypedLocationInfo>& expr) {
  expr.subject->accept(*this);
  expr.property->accept(*this);
  X86Gp propertyReg = vRegs.top(); vRegs.pop();
  X86Gp subjectReg = vRegs.top(); vRegs.pop();
  CCFuncCall* access = cc.call((uint64_t) get, FuncSignature2<uint64_t, uint64_t, uint64_t>());
  access->setArg(0, subjectReg);
  access->setArg(1, propertyReg);

  X86Gp result = cc.newUInt64();
  access->setRet(0, result);
  vRegs.push(result);
}

void AutoCompiler::visitDynamicStoreExpression(DynamicStoreExpression<TypedLocationInfo>& expr) {
  expr.subject->accept(*this);
  expr.property->accept(*this);
  X86Gp propertyReg = vRegs.top(); vRegs.pop();
  X86Gp subjectReg = vRegs.top(); vRegs.pop();
  X86Gp rhsReg;

  if (expr.preAssignmentOp != ast::kPreAssignmentOpNone) {
    CCFuncCall* access = cc.call((uint64_t) get, FuncSignature2<uint64_t, uint64_t, uint64_t>());
    access->setArg(0, subjectReg);
    access->setArg(1, propertyReg);

    rhsReg = cc.newUInt64();
    access->setRet(0, rhsReg);
    doPreAssignmentOp(rhsReg, expr.preAssignmentOp, *expr.rhs);
  } else {
    expr.rhs->accept(*this);
    rhsReg = vRegs.top(); vRegs.pop();
  }

  CCFuncCall* store = cc.call((uint64_t) set, FuncSignature3<uint64_t, uint64_t, uint64_t, uint64_t>());
  store->setArg(0, subjectReg);
  store->setArg(1, propertyReg);
  store->setArg(2, rhsReg);

  vRegs.push(rhsReg);
}

void AutoCompiler::visitStaticAccessExpression(StaticAccessExpression<TypedLocationInfo>& expr) {
  expr.subject->accept(*this);
  X86Gp propertyReg = cc.newUInt64();
  X86Gp subjectReg = vRegs.top(); vRegs.pop();
  cc.mov(propertyReg, (uint64_t) &expr.property);
  CCFuncCall* access = cc.call((uint64_t) objectAccessChecked, FuncSignature2<uint64_t, uint64_t, uint64_t>());
  access->setArg(0, subjectReg);
  access->setArg(1, propertyReg);

  X86Gp result = cc.newUInt64();
  access->setRet(0, result);
  vRegs.push(result);
}

void AutoCompiler::visitStaticStoreExpression(StaticStoreExpression<TypedLocationInfo>& expr) {
  expr.subject->accept(*this);
  X86Gp propertyReg = cc.newUInt64();
  X86Gp subjectReg = vRegs.top(); vRegs.pop();
  cc.mov(propertyReg, (uint64_t) &expr.property);
  X86Gp rhsReg;

  if (expr.preAssignmentOp != ast::kPreAssignmentOpNone) {
    CCFuncCall* access = cc.call((uint64_t) objectAccessChecked, FuncSignature2<uint64_t, uint64_t, uint64_t>());
    access->setArg(0, subjectReg);
    access->setArg(1, propertyReg);

    rhsReg = cc.newUInt64();
    access->setRet(0, rhsReg);
    doPreAssignmentOp(rhsReg, expr.preAssignmentOp, *expr.rhs);
  } else {
    expr.rhs->accept(*this);
    rhsReg = vRegs.top(); vRegs.pop();
  }

  CCFuncCall* store = cc.call((uint64_t) objectStoreChecked, FuncSignature3<uint64_t, uint64_t, uint64_t, uint64_t>());
  store->setArg(0, subjectReg);
  store->setArg(1, propertyReg);
  store->setArg(2, rhsReg);

  vRegs.push(rhsReg);
}

void AutoCompiler::visitFuncCallExpression(FuncCallExpression<TypedLocationInfo>& expr) {
  auto finder = funcs.funcs.find(expr.name);
  if (finder == funcs.funcs.end()) {
    error.noFuncError();
    vRegs.push(cc.newUInt64());
    return;
  }

  Function& func = *finder->second;

  int argc = func.argc;
  int paramc = expr.parameters.size();

  uint8_t sigArgs[argc];
  vector<X86Gp> paramRegs;

  for (int i = 0; i < paramc; ++i) {
    expr.parameters[i]->accept(*this);
    paramRegs.push_back(vRegs.top());
    if (i < argc) {
      // box those that are actually passed in :)
      box(vRegs.top(), expr.parameters[i]->resolvedData);
    }
    vRegs.pop();
  }

  // unspecified void values
  for (int i = paramc; i < argc; ++i) {
    X86Gp void_ = cc.newUInt64();
    cc.mov(void_, VOID);
    paramRegs.push_back(void_);
  }

  for (int i = 0; i < argc; ++i) {
    sigArgs[i] = TypeIdOf<int64_t>::kTypeId;
  }

  FuncSignature sig;
  sig.init(CallConv::kIdHost, TypeIdOf<int64_t>::kTypeId, sigArgs, argc);

  CCFuncCall* call;
  if (func.isNative) {
    call = cc.call((uint64_t) func.fptr, sig);
  } else {
    // careful that this always handles self-optimization
    X86Gp lookup = cc.newUInt64();
    cc.mov(lookup, (uint64_t) &func.fptr);
    call = cc.call(x86::ptr(lookup), sig);
  }

  for (int i = 0; i < argc; ++i) {
    call->setArg(i, paramRegs[i]);
  }

  X86Gp retReg = cc.newInt64();
  call->setRet(0, retReg);
  vRegs.push(retReg);
}

void AutoCompiler::visitFuncDecl(FuncDecl<TypedLocationInfo>& decl) {
  uint8_t sigArgs[decl.args.size()];

  for (int i = 0; i < decl.args.size(); ++i) {
    sigArgs[i] = TypeIdOf<int64_t>::kTypeId;
  }

  FuncSignature sig;
  sig.init(CallConv::kIdHost, TypeIdOf<int64_t>::kTypeId, sigArgs, decl.args.size());
  curFunc = cc.addFunc(sig);
  curFunc->getFrameInfo().addAttributes(asmjit::FuncFrameInfo::kAttrPreserveFP);
  curFuncName = decl.name;

  // allocate a variably sized FunctionUsage with room for shapes
  void* usageMem = malloc(sizeof(FunctionUsage) + sizeof(ArgumentShape) * decl.args.size());
  FunctionUsage* usage = (FunctionUsage*) usageMem;
  unique_ptr<FunctionUsage> savedUsage(new (usage) FunctionUsage());

  // prepare it so it knows how to recurse
  funcs.prepareFunc(decl.name, decl.args.size(), std::move(savedUsage), &decl);

  X86Gp checkReg = cc.newUInt64();
  X86Gp orReg = cc.newUInt64();
  for (int i = 0; i < decl.args.size(); ++i) {
    X86Gp arg = cc.newInt64();
    cc.setArg(i, arg);
    argRegs.push_back(arg);
  }

  generateTypeShapePrelog(decl, usage);

  TypedLocationInfo endType;
  for(vector<unique_ptr<Statement<TypedLocationInfo> > >::iterator it = decl.statements.begin();
      it != decl.statements.end();
      ++it) {
    (*it)->accept(*this);
    endType = (*it)->resolvedData;
  }

  if (vRegs.size()) {
    box(vRegs.top(), endType);
    cc.ret(vRegs.top());
  } else {
    X86Gp voidReg = cc.newInt64();
    cc.mov(voidReg, Value().getRaw());
    cc.ret(voidReg);
  }

  generateOptimizeProlog(decl, sig);
  error.generateTypeErrorProlog();

  cc.endFunc();
  cc.finalize();

  funcs.finalizeFunc(decl.name, &codeHolder);
}

void AutoCompiler::generateTypeShapePrelog(FuncDecl<TypedLocationInfo>& decl, FunctionUsage* usage) {
  optimizeLabel = cc.newLabel();
  X86Gp count = cc.newInt32();
  cc.mov(count, asmjit::x86::dword_ptr((uint64_t) &usage->count));
  cc.add(count, 1);
  cc.mov(asmjit::x86::dword_ptr((uint64_t) &usage->count), count);
  cc.cmp(count, HOT_COUNT);
  cc.je(optimizeLabel);

  X86Gp checkReg = cc.newUInt64();
  X86Gp orReg = cc.newUInt64();
  for (int i = 0; i < decl.args.size(); ++i) {
    usage->argShapes[i].raw = 0; // initialize
    X86Gp arg = argRegs[i];

    Label afterCheck = cc.newLabel();
    Label noPointerCheck = cc.newLabel();

    cc.mov(checkReg, MAX_PTR);
    cc.cmp(arg, checkReg);
    // can't derefence if it isn't a pointer
    cc.jge(noPointerCheck);
    // now we can dereference it
    cc.mov(orReg, x86::ptr(arg));
    cc.shl(orReg, POINTER_TAG_SHIFT);
    cc.jmp(afterCheck);

    cc.bind(noPointerCheck);

    cc.mov(checkReg, MIN_DBL);
    cc.mov(orReg, DOUBLE_SHAPE);
    cc.cmp(arg, checkReg);
    cc.jg(afterCheck);

    cc.mov(orReg, arg);
    cc.shr(orReg, PRIMITIVE_TAG_SHIFT);
    // creates a tag for ints, bools, and void

    cc.bind(afterCheck);
    cc.mov(checkReg.r16(), x86::word_ptr((uint64_t) &usage->argShapes[i]));
    cc.or_(checkReg.r16(), orReg.r16());
    cc.mov(x86::word_ptr((uint64_t) &usage->argShapes[i]), checkReg.r16());
  }
}

void AutoCompiler::generateOptimizeProlog(FuncDecl<TypedLocationInfo>& decl, FuncSignature& sig) {
  cc.bind(optimizeLabel);
  X86Gp funcsReg = cc.newUInt64();
  X86Gp declReg = cc.newUInt64();
  X86Gp optimizedAddr = cc.newUInt64();
  cc.mov(funcsReg, (uint64_t) &funcs);
  cc.mov(declReg, (uint64_t) &decl);
  CCFuncCall* recompileCall = cc.call((size_t) &vaiven::optimize, FuncSignature2<uint64_t, uint64_t, uint64_t>());
  recompileCall->setArg(0, funcsReg);
  recompileCall->setArg(1, declReg);
  recompileCall->setRet(0, optimizedAddr);

  CCFuncCall* optimizedCall = cc.call(optimizedAddr, sig);
  for (int i = 0; i < decl.args.size(); ++i) {
    optimizedCall->setArg(i, argRegs[i]);
  }
  X86Gp optimizedRet = cc.newUInt64();
  optimizedCall->setRet(0, optimizedRet);
  
  cc.ret(optimizedRet);
}

void AutoCompiler::visitExpressionStatement(ExpressionStatement<TypedLocationInfo>& stmt) {
  stmt.expr->accept(*this);
}

void AutoCompiler::visitBlock(Block<TypedLocationInfo>& block) {
  for(vector<unique_ptr<Statement<TypedLocationInfo> > >::iterator it = block.statements.begin();
      it != block.statements.end();
      ++it) {
    (*it)->accept(*this);
  }
}

void AutoCompiler::visitAssignmentExpression(AssignmentExpression<TypedLocationInfo>& expr) {
  X86Gp target;
  if (scope.contains(expr.varname)) {
    target = scope.get(expr.varname);
  } else {
    int argIndex = expr.resolvedData.location.data.argIndex;
    if (argIndex == -1) {
      error.noVarError();
      return;
    } else {
      target = argRegs[expr.resolvedData.location.data.argIndex];
    }
  }

  doPreAssignmentOp(target, expr.preAssignmentOp, *expr.expr);

  vRegs.push(target);
}

void AutoCompiler::doPreAssignmentOp(X86Gp currentVal, ast::PreAssignmentOp preAssignmentOp, ast::Expression<TypedLocationInfo>& newVal) {
  if (preAssignmentOp == ast::kPreAssignmentOpAdd) {
    if (newVal.resolvedData.location.type == LOCATION_TYPE_IMM && newVal.resolvedData.type == VAIVEN_STATIC_TYPE_INT) {
      error.typecheckInt(currentVal);
      cc.add(currentVal.r32(), newVal.resolvedData.location.data.imm);
      box(currentVal, VAIVEN_STATIC_TYPE_INT);
    } else {
      if (newVal.resolvedData.type == VAIVEN_STATIC_TYPE_INT) {
        newVal.accept(*this);
        error.typecheckInt(currentVal);
        cc.add(currentVal.r32(), vRegs.top().r32()); // overflow behavior must be 32 bit
        vRegs.pop();
        box(currentVal, VAIVEN_STATIC_TYPE_INT);
      } else if (newVal.resolvedData.type == VAIVEN_STATIC_TYPE_STRING) {
        newVal.accept(*this);
        box(vRegs.top(), newVal.resolvedData);
        // TODO pure string append
        CCFuncCall* call = cc.call((uint64_t) vaiven::add, FuncSignature2<uint64_t, uint64_t, uint64_t>());
        X86Gp result = cc.newUInt64();
        call->setArg(0, currentVal);
        call->setArg(1, vRegs.top());
        call->setRet(0, currentVal);
      } else {
        newVal.accept(*this);
        box(vRegs.top(), newVal.resolvedData);
        CCFuncCall* call = cc.call((uint64_t) vaiven::add, FuncSignature2<uint64_t, uint64_t, uint64_t>());
        X86Gp result = cc.newUInt64();
        call->setArg(0, currentVal);
        call->setArg(1, vRegs.top());
        call->setRet(0, currentVal);
      }
    }
  } else if (preAssignmentOp == ast::kPreAssignmentOpSub) {
    if (newVal.resolvedData.location.type == LOCATION_TYPE_IMM) {
      error.typecheckInt(currentVal);
      cc.sub(currentVal.r32(), newVal.resolvedData.location.data.imm);
      box(currentVal, VAIVEN_STATIC_TYPE_INT);
    } else {
      newVal.accept(*this);
      error.typecheckInt(currentVal);
      cc.sub(currentVal.r32(), vRegs.top().r32()); // overflow behavior must be 32 bit
      vRegs.pop();
      box(currentVal, VAIVEN_STATIC_TYPE_INT);
    }
  } else if (preAssignmentOp == ast::kPreAssignmentOpMul) {
    newVal.accept(*this);
    error.typecheckInt(currentVal);
    error.typecheckInt(vRegs.top(), newVal.resolvedData);
    cc.imul(currentVal.r32(), vRegs.top().r32()); // overflow behavior must be 32 bit
    vRegs.pop();
    box(currentVal, VAIVEN_STATIC_TYPE_INT);
  } else if (preAssignmentOp == ast::kPreAssignmentOpDiv) {
    newVal.accept(*this);
    error.typecheckInt(currentVal);
    error.typecheckInt(vRegs.top(), newVal.resolvedData);
    X86Gp dummy = cc.newInt64();
    cc.xor_(dummy, dummy);
    cc.idiv(dummy.r32(), currentVal.r32(), vRegs.top().r32()); // overflow behavior must be 32 bit
    vRegs.pop();
    box(currentVal, VAIVEN_STATIC_TYPE_INT);
  } else {
    newVal.accept(*this);
    box(vRegs.top(), newVal.resolvedData);
    cc.mov(currentVal, vRegs.top());
    vRegs.pop();
  }
}

void AutoCompiler::visitAdditionExpression(AdditionExpression<TypedLocationInfo>& expr) {
  Location& left_loc = expr.left->resolvedData.location;
  Location& right_loc = expr.right->resolvedData.location;
  bool leftImm = left_loc.type == LOCATION_TYPE_IMM;
  bool rightImm = right_loc.type == LOCATION_TYPE_IMM;

  if (expr.left->resolvedData.type != VAIVEN_STATIC_TYPE_INT
      || expr.right->resolvedData.type != VAIVEN_STATIC_TYPE_INT) {
    expr.left->accept(*this);
    X86Gp lhsReg = vRegs.top(); vRegs.pop();
    expr.right->accept(*this);
    X86Gp rhsReg = vRegs.top(); vRegs.pop();
    box(lhsReg, expr.left->resolvedData);
    box(rhsReg, expr.right->resolvedData);
    CCFuncCall* call = cc.call((uint64_t) vaiven::add, FuncSignature2<uint64_t, uint64_t, uint64_t>());
    X86Gp result = cc.newUInt64();
    call->setArg(0, lhsReg);
    call->setArg(1, rhsReg);
    call->setRet(0, result);
    vRegs.push(result);
    return;
  }

  X86Gp result = cc.newInt64();
  if (leftImm && rightImm) {
    // mov rax, lhsImm or exact reg
    // add rax, rhsImm or exact reg
    cc.mov(result.r32(), left_loc.data.imm);
    cc.add(result.r32(), right_loc.data.imm);
  } else if (leftImm) {
    expr.right->accept(*this);
    X86Gp rhsReg = vRegs.top(); vRegs.pop();
    error.typecheckInt(rhsReg, expr.right->resolvedData);
    if (right_loc.type != LOCATION_TYPE_ARG && right_loc.type != LOCATION_TYPE_LOCAL) {
      result = rhsReg;
    } else {
      cc.mov(result, rhsReg);
    }
    cc.add(result.r32(), left_loc.data.imm);
  } else if (rightImm) {
    expr.left->accept(*this);
    X86Gp lhsReg = vRegs.top(); vRegs.pop();
    error.typecheckInt(lhsReg, expr.left->resolvedData);
    if (left_loc.type != LOCATION_TYPE_ARG && left_loc.type != LOCATION_TYPE_LOCAL) {
      result = lhsReg;
    } else {
      cc.mov(result, lhsReg);
    }
    cc.add(result.r32(), right_loc.data.imm);
  } else {
    expr.left->accept(*this);
    X86Gp lhsReg = vRegs.top(); vRegs.pop();
    expr.right->accept(*this);
    X86Gp rhsReg = vRegs.top(); vRegs.pop();
    error.typecheckInt(lhsReg, expr.left->resolvedData);
    error.typecheckInt(rhsReg, expr.right->resolvedData);
    if (left_loc.type != LOCATION_TYPE_ARG && left_loc.type != LOCATION_TYPE_LOCAL) {
      result = lhsReg;
    } else if (right_loc.type != LOCATION_TYPE_ARG && right_loc.type != LOCATION_TYPE_LOCAL) {
      result = rhsReg;
      rhsReg = lhsReg;
    } else {
      cc.mov(result, lhsReg);
    }
    cc.add(result.r32(), rhsReg.r32());
  }
  vRegs.push(result);
}

void AutoCompiler::visitSubtractionExpression(SubtractionExpression<TypedLocationInfo>& expr) {
  Location& left_loc = expr.left->resolvedData.location;
  Location& right_loc = expr.right->resolvedData.location;
  bool leftImm = left_loc.type == LOCATION_TYPE_IMM;
  bool rightImm = right_loc.type == LOCATION_TYPE_IMM;

  X86Gp result = cc.newInt64();
  if (leftImm && rightImm) {
    // mov rax, lhsImm or exact reg
    // add rax, rhsImm or exact reg
    cc.mov(result.r32(), left_loc.data.imm);
    cc.sub(result.r32(), right_loc.data.imm);
  } else if (leftImm) {
    expr.right->accept(*this);
    X86Gp rhsReg = vRegs.top(); vRegs.pop();
    error.typecheckInt(rhsReg, expr.right->resolvedData);
    if (right_loc.type != LOCATION_TYPE_ARG && right_loc.type != LOCATION_TYPE_LOCAL) {
      result = rhsReg;
    } else {
      cc.mov(result, rhsReg);
    }
    cc.neg(result.r32());
    cc.add(result.r32(), left_loc.data.imm);
  } else if (rightImm) {
    expr.left->accept(*this);
    X86Gp lhsReg = vRegs.top(); vRegs.pop();
    error.typecheckInt(lhsReg, expr.left->resolvedData);
    if (left_loc.type != LOCATION_TYPE_ARG && left_loc.type != LOCATION_TYPE_LOCAL) {
      result = lhsReg;
    } else {
      cc.mov(result, lhsReg);
    }
    cc.sub(result.r32(), right_loc.data.imm);
  } else {
    expr.left->accept(*this);
    X86Gp lhsReg = vRegs.top(); vRegs.pop();
    expr.right->accept(*this);
    X86Gp rhsReg = vRegs.top(); vRegs.pop();
    error.typecheckInt(lhsReg, expr.left->resolvedData);
    error.typecheckInt(rhsReg, expr.right->resolvedData);
    if (left_loc.type != LOCATION_TYPE_ARG && left_loc.type != LOCATION_TYPE_LOCAL) {
      result = lhsReg;
      cc.sub(result.r32(), rhsReg.r32());
    } else if (right_loc.type != LOCATION_TYPE_ARG && right_loc.type != LOCATION_TYPE_LOCAL) {
      result = rhsReg;
      cc.neg(result.r32());
      cc.add(result.r32(), lhsReg.r32());
    } else {
      cc.mov(result, lhsReg);
      cc.add(result.r32(), lhsReg.r32());
    }
  }
  vRegs.push(result);
}
void AutoCompiler::visitMultiplicationExpression(MultiplicationExpression<TypedLocationInfo>& expr) {
  Location& left_loc = expr.left->resolvedData.location;
  Location& right_loc = expr.right->resolvedData.location;
  bool leftImm = left_loc.type == LOCATION_TYPE_IMM;
  bool rightImm = right_loc.type == LOCATION_TYPE_IMM;

  X86Gp result = cc.newInt64();
  if (leftImm && rightImm) {
    // mov rax, lhsImm or exact reg
    // add rax, rhsImm or exact reg
    cc.mov(result.r32(), left_loc.data.imm);
    cc.imul(result.r32(), right_loc.data.imm);
  } else if (leftImm) {
    expr.right->accept(*this);
    X86Gp rhsReg = vRegs.top(); vRegs.pop();
    error.typecheckInt(rhsReg, expr.right->resolvedData);
    if (right_loc.type != LOCATION_TYPE_ARG && right_loc.type != LOCATION_TYPE_LOCAL) {
      result = rhsReg;
    } else {
      cc.mov(result, rhsReg);
    }
    cc.imul(result.r32(), left_loc.data.imm);
  } else if (rightImm) {
    expr.left->accept(*this);
    X86Gp lhsReg = vRegs.top(); vRegs.pop();
    error.typecheckInt(lhsReg, expr.left->resolvedData);
    if (left_loc.type != LOCATION_TYPE_ARG && left_loc.type != LOCATION_TYPE_LOCAL) {
      result = lhsReg;
    } else {
      cc.mov(result, lhsReg);
    }
    cc.imul(result.r32(), right_loc.data.imm);
  } else {
    expr.left->accept(*this);
    X86Gp lhsReg = vRegs.top(); vRegs.pop();
    expr.right->accept(*this);
    X86Gp rhsReg = vRegs.top(); vRegs.pop();
    error.typecheckInt(lhsReg, expr.left->resolvedData);
    error.typecheckInt(rhsReg, expr.right->resolvedData);
    if (left_loc.type != LOCATION_TYPE_ARG && left_loc.type != LOCATION_TYPE_LOCAL) {
      result = lhsReg;
    } else if (right_loc.type != LOCATION_TYPE_ARG && right_loc.type != LOCATION_TYPE_LOCAL) {
      result = rhsReg;
      rhsReg = lhsReg;
    } else {
      cc.mov(result, lhsReg);
    }
    cc.imul(result.r32(), rhsReg.r32());
  }
  vRegs.push(result);
}
void AutoCompiler::visitDivisionExpression(DivisionExpression<TypedLocationInfo>& expr) {
  Location& left_loc = expr.left->resolvedData.location;
  Location& right_loc = expr.right->resolvedData.location;
  bool leftImm = left_loc.type == LOCATION_TYPE_IMM;
  bool rightImm = right_loc.type == LOCATION_TYPE_IMM;

  X86Gp result = cc.newInt64();
  X86Gp divisor = cc.newInt64();
  if (leftImm && rightImm) {
    // mov rax, lhsImm or exact reg
    // add rax, rhsImm or exact reg
    cc.mov(result.r32(), left_loc.data.imm);
    cc.mov(divisor.r32(), right_loc.data.imm);
  } else if (leftImm) {
    expr.right->accept(*this);
    // will this work if its on the stack?
    divisor = vRegs.top(); vRegs.pop();
    error.typecheckInt(divisor, expr.right->resolvedData);
    cc.mov(result.r32(), left_loc.data.imm);
  } else if (rightImm) {
    expr.left->accept(*this);
    X86Gp lhsReg = vRegs.top(); vRegs.pop();
    error.typecheckInt(lhsReg, expr.left->resolvedData);
    if (left_loc.type != LOCATION_TYPE_ARG && left_loc.type != LOCATION_TYPE_LOCAL) {
      result = lhsReg;
    } else {
      cc.mov(result, lhsReg);
    }
    cc.mov(divisor.r32(), right_loc.data.imm);
  } else {
    expr.left->accept(*this);
    X86Gp lhsReg = vRegs.top(); vRegs.pop();
    expr.right->accept(*this);
    divisor = vRegs.top(); vRegs.pop();
    error.typecheckInt(lhsReg, expr.left->resolvedData);
    error.typecheckInt(divisor, expr.right->resolvedData);
    if (left_loc.type != LOCATION_TYPE_ARG && left_loc.type != LOCATION_TYPE_LOCAL) {
      result = lhsReg;
    } else {
      cc.mov(result, lhsReg);
    }
  }
  X86Gp dummy = cc.newInt64();
  cc.xor_(dummy, dummy);
  cc.idiv(dummy.r32(), result.r32(), divisor.r32());
  vRegs.push(result);
}

void AutoCompiler::box(asmjit::X86Gp vReg, TypedLocationInfo& typeInfo) {
  if (typeInfo.isBoxed) {
    return;
  }

  box(vReg, typeInfo.type);
}

void AutoCompiler::box(asmjit::X86Gp vReg, VaivenStaticType type) {
  if (type == VAIVEN_STATIC_TYPE_INT) {
    // can't use 64 bit immediates except with MOV
    X86Gp fullValue = cc.newInt64();
    cc.mov(fullValue, INT_TAG);
    cc.or_(vReg, fullValue);
  } else if (type == VAIVEN_STATIC_TYPE_BOOL) {
    X86Gp fullValue = cc.newInt64();
    cc.mov(fullValue, BOOL_TAG);
    cc.or_(vReg, fullValue);
  } else if (type == VAIVEN_STATIC_TYPE_DOUBLE) {
    X86Gp fullValue = cc.newInt64();
    cc.mov(fullValue, (uint64_t) 0xFFFFFFFFFFFFFFFF);
    // no bitwise negation, so do 1111 XOR ????
    cc.xor_(vReg, fullValue);
  }
  // POINTER nothing to do
  // UNKNOWN should never happen
  // VOID should never happen
}

void AutoCompiler::visitIntegerExpression(IntegerExpression<TypedLocationInfo>& expr) {
  // should only happen when in a stmt by itself
  X86Gp var = cc.newInt64();
  cc.mov(var, Value(expr.value).getRaw());
  vRegs.push(var);
}

void AutoCompiler::visitDoubleExpression(DoubleExpression<TypedLocationInfo>& expr) {
  // should only happen when in a stmt by itself
  X86Gp var = cc.newInt64();
  cc.mov(var, Value(expr.value).getRaw());
  vRegs.push(var);
}

void AutoCompiler::visitStringExpression(StringExpression<TypedLocationInfo>& expr) {
  // should only happen when in a stmt by itself
  X86Gp var = cc.newInt64();
  cc.mov(var, (uint64_t) expr.value);
  vRegs.push(var);
}

void AutoCompiler::visitVariableExpression(VariableExpression<TypedLocationInfo>& expr) {
  if (scope.contains(expr.id)) {
    vRegs.push(scope.get(expr.id));
  } else {
    int argIndex = expr.resolvedData.location.data.argIndex;
    if (argIndex == -1) {
      error.noVarError();
      vRegs.push(cc.newUInt64());
    } else {
      vRegs.push(argRegs[expr.resolvedData.location.data.argIndex]);
    }
  }
}

void AutoCompiler::visitBoolLiteral(BoolLiteral<TypedLocationInfo>& expr) {
  // should only happen when in a stmt by itself
  X86Gp var = cc.newInt64();
  cc.mov(var, Value(expr.value).getRaw());
  vRegs.push(var);
}

void AutoCompiler::doCmpNotExpression(NotExpression<TypedLocationInfo>& expr) {
  Location& inner_loc = expr.expr->resolvedData.location;
  bool innerImm = inner_loc.type == LOCATION_TYPE_IMM;

  if (innerImm) {
    X86Gp tmp = cc.newInt64();
    cc.mov(tmp.r32(), inner_loc.data.imm);
    cc.test(tmp, tmp);
  } else {
    expr.expr->accept(*this);
    X86Gp valReg = vRegs.top(); vRegs.pop();
    error.typecheckBool(valReg, expr.expr->resolvedData);
    cc.test(valReg.r32(), valReg.r32());
  }
}

void AutoCompiler::visitNotExpression(NotExpression<TypedLocationInfo>& expr) {
  X86Gp result = cc.newInt64();
  cc.xor_(result, result);
  doCmpNotExpression(expr);
  cc.setz(result);
  vRegs.push(result);
}

void AutoCompiler::visitInequalityExpression(InequalityExpression<TypedLocationInfo>& expr) {
  X86Gp result = cc.newInt64();
  // TODO only xor this when we use setne
  cc.xor_(result, result);
  if (doCmpEqualityExpression(*expr.left, *expr.right, false)) {
    cc.setne(result);
    vRegs.push(result);
  }
}

bool AutoCompiler::doCmpEqualityExpression(Expression<TypedLocationInfo>& left, Expression<TypedLocationInfo>& right, bool checkTrue) {
  Location& left_loc = left.resolvedData.location;
  Location& right_loc = right.resolvedData.location;
  bool leftImm = left_loc.type == LOCATION_TYPE_IMM;
  bool rightImm = right_loc.type == LOCATION_TYPE_IMM;

  if (left.resolvedData.type == VAIVEN_STATIC_TYPE_STRING
      || left.resolvedData.type == VAIVEN_STATIC_TYPE_UNKNOWN
      || right.resolvedData.type == VAIVEN_STATIC_TYPE_STRING
      || right.resolvedData.type == VAIVEN_STATIC_TYPE_UNKNOWN) {
    left.accept(*this);
    X86Gp lhsReg = vRegs.top(); vRegs.pop();
    right.accept(*this);
    X86Gp rhsReg = vRegs.top(); vRegs.pop();
    box(lhsReg, left.resolvedData);
    box(rhsReg, right.resolvedData);
    uint64_t funcaddr = (uint64_t) (checkTrue ? vaiven::cmp : vaiven::inverseCmp);
    CCFuncCall* call = cc.call(funcaddr, FuncSignature2<uint64_t, uint64_t, uint64_t>());
    X86Gp result = cc.newUInt64();
    call->setArg(0, lhsReg);
    call->setArg(1, rhsReg);
    call->setRet(0, result);
    vRegs.push(result);
    return false; // ended as vReg, not cmp
  }

  if (leftImm && rightImm) {
    // mov rax, lhsImm or exact reg
    // add rax, rhsImm or exact reg
    X86Gp tmp = cc.newInt64();
    cc.mov(tmp.r32(), left_loc.data.imm);
    cc.cmp(tmp.r32(), right_loc.data.imm);
  } else if (leftImm) {
    right.accept(*this);
    X86Gp rhsReg = vRegs.top(); vRegs.pop();
    if (left.resolvedData.type == VAIVEN_STATIC_TYPE_INT
        || left.resolvedData.type == VAIVEN_STATIC_TYPE_BOOL) {
      cc.cmp(rhsReg.r32(), left_loc.data.imm);
    }
    // TODO objects, doubles
  } else if (rightImm) {
    left.accept(*this);
    X86Gp lhsReg = vRegs.top(); vRegs.pop();
    if (right.resolvedData.type == VAIVEN_STATIC_TYPE_INT
        || right.resolvedData.type == VAIVEN_STATIC_TYPE_BOOL) {
      cc.cmp(lhsReg.r32(), right_loc.data.imm);
    }
    // TODO objects, doubles
  } else {
    left.accept(*this);
    X86Gp lhsReg = vRegs.top(); vRegs.pop();
    right.accept(*this);
    X86Gp rhsReg = vRegs.top(); vRegs.pop();
    cc.cmp(lhsReg, rhsReg);
  }

  return true; // ended in cmp
}

void AutoCompiler::visitEqualityExpression(EqualityExpression<TypedLocationInfo>& expr) {
  X86Gp result = cc.newInt64();
  // TODO only xor when we use sete
  cc.xor_(result, result);
  if (doCmpEqualityExpression(*expr.left, *expr.right, true)) {
    cc.sete(result);
    vRegs.push(result);
  }
}

void AutoCompiler::visitGtExpression(GtExpression<TypedLocationInfo>& expr) {
  Location& left_loc = expr.left->resolvedData.location;
  Location& right_loc = expr.right->resolvedData.location;
  bool backwardsCmp = left_loc.type != LOCATION_TYPE_IMM && right_loc.type == LOCATION_TYPE_IMM;
  X86Gp result = cc.newUInt64();
  cc.xor_(result, result);
  doCmpIntExpression(*expr.left, *expr.right);
  if (backwardsCmp) {
    cc.setl(result);
  } else {
    cc.setg(result);
  }
  vRegs.push(result);
}

void AutoCompiler::doCmpIntExpression(Expression<TypedLocationInfo>& left, Expression<TypedLocationInfo>& right) {
  Location& left_loc = left.resolvedData.location;
  Location& right_loc = right.resolvedData.location;
  bool leftImm = left_loc.type == LOCATION_TYPE_IMM;
  bool rightImm = right_loc.type == LOCATION_TYPE_IMM;

  if (leftImm && rightImm) {
    // mov rax, lhsImm or exact reg
    // add rax, rhsImm or exact reg
    X86Gp tmp = cc.newInt64();
    cc.mov(tmp.r32(), left_loc.data.imm);
    cc.cmp(tmp.r32(), right_loc.data.imm);
  } else if (leftImm) {
    right.accept(*this);
    X86Gp rhsReg = vRegs.top(); vRegs.pop();
    error.typecheckInt(rhsReg, right.resolvedData);
    cc.cmp(rhsReg.r32(), left_loc.data.imm);
  } else if (rightImm) {
    left.accept(*this);
    X86Gp lhsReg = vRegs.top(); vRegs.pop();
    error.typecheckInt(lhsReg, left.resolvedData);
    cc.cmp(lhsReg.r32(), right_loc.data.imm);
  } else {
    left.accept(*this);
    X86Gp lhsReg = vRegs.top(); vRegs.pop();
    right.accept(*this);
    X86Gp rhsReg = vRegs.top(); vRegs.pop();
    error.typecheckInt(rhsReg, right.resolvedData);
    error.typecheckInt(lhsReg, left.resolvedData);
    cc.cmp(lhsReg, rhsReg);
  }
}

void AutoCompiler::visitGteExpression(GteExpression<TypedLocationInfo>& expr) {
  Location& left_loc = expr.left->resolvedData.location;
  Location& right_loc = expr.right->resolvedData.location;
  bool backwardsCmp = left_loc.type != LOCATION_TYPE_IMM && right_loc.type == LOCATION_TYPE_IMM;
  X86Gp result = cc.newUInt64();
  cc.xor_(result, result);
  doCmpIntExpression(*expr.left, *expr.right);
  if (backwardsCmp) {
    cc.setle(result);
  } else {
    cc.setge(result);
  }
  vRegs.push(result);
}

void AutoCompiler::visitLtExpression(LtExpression<TypedLocationInfo>& expr) {
  Location& left_loc = expr.left->resolvedData.location;
  Location& right_loc = expr.right->resolvedData.location;
  bool backwardsCmp = left_loc.type != LOCATION_TYPE_IMM && right_loc.type == LOCATION_TYPE_IMM;
  X86Gp result = cc.newUInt64();
  cc.xor_(result, result);
  doCmpIntExpression(*expr.left, *expr.right);
  if (backwardsCmp) {
    cc.setg(result);
  } else {
    cc.setl(result);
  }
  vRegs.push(result);
}

void AutoCompiler::visitLteExpression(LteExpression<TypedLocationInfo>& expr) {
  Location& left_loc = expr.left->resolvedData.location;
  Location& right_loc = expr.right->resolvedData.location;
  bool backwardsCmp = left_loc.type != LOCATION_TYPE_IMM && right_loc.type == LOCATION_TYPE_IMM;
  X86Gp result = cc.newUInt64();
  cc.xor_(result, result);
  doCmpIntExpression(*expr.left, *expr.right);
  if (backwardsCmp) {
    cc.setge(result);
  } else {
    cc.setle(result);
  }
  vRegs.push(result);
}
