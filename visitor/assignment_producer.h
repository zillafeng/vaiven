#ifndef VAIVEN_VISITOR_HEADER_ASSIGNMENT_PRODUCER
#define VAIVEN_VISITOR_HEADER_ASSIGNMENT_PRODUCER

#include "../ast/visitor.h"
#include "../ast/all.h"

#include <memory>

namespace vaiven { namespace visitor {

using namespace vaiven::ast;
using std::unique_ptr;

class AssignmentProducer : public Visitor<> {

  public:
  AssignmentProducer(unique_ptr<Expression<>> rhs, ast::PreAssignmentOp preAssignmentOp)
      : rhs(std::move(rhs)), preAssignmentOp(preAssignmentOp) {};

  virtual void visitAssignmentExpression(AssignmentExpression<>& expr);
  virtual void visitAdditionExpression(AdditionExpression<>& expr);
  virtual void visitSubtractionExpression(SubtractionExpression<>& expr);
  virtual void visitMultiplicationExpression(MultiplicationExpression<>& expr);
  virtual void visitDivisionExpression(DivisionExpression<>& expr);
  virtual void visitIntegerExpression(IntegerExpression<>& expr);
  virtual void visitDoubleExpression(DoubleExpression<>& expr);
  virtual void visitStringExpression(StringExpression<>& expr);
  virtual void visitVariableExpression(VariableExpression<>& expr);
  virtual void visitBoolLiteral(BoolLiteral<>& expr);
  virtual void visitVoidLiteral(VoidLiteral<>& expr);
  virtual void visitNotExpression(NotExpression<>& expr);
  virtual void visitEqualityExpression(EqualityExpression<>& expr);
  virtual void visitInequalityExpression(InequalityExpression<>& expr);
  virtual void visitGtExpression(GtExpression<>& expr);
  virtual void visitGteExpression(GteExpression<>& expr);
  virtual void visitLtExpression(LtExpression<>& expr);
  virtual void visitLteExpression(LteExpression<>& expr);
  virtual void visitFuncCallExpression(FuncCallExpression<>& expr);
  virtual void visitListLiteralExpression(ListLiteralExpression<>& expr);
  virtual void visitDynamicAccessExpression(DynamicAccessExpression<>& expr);
  virtual void visitDynamicStoreExpression(DynamicStoreExpression<>& expr);
  virtual void visitStaticAccessExpression(StaticAccessExpression<>& expr);
  virtual void visitStaticStoreExpression(StaticStoreExpression<>& expr);
  virtual void visitExpressionStatement(ExpressionStatement<>& stmt);
  virtual void visitIfStatement(IfStatement<>& stmt);
  virtual void visitForCondition(ForCondition<>& stmt);
  virtual void visitReturnStatement(ReturnStatement<>& stmt);
  virtual void visitBlock(Block<>& expr);
  virtual void visitFuncDecl(FuncDecl<>& funcDecl);
  virtual void visitVarDecl(VarDecl<>& varDecl);

  ast::PreAssignmentOp preAssignmentOp;
  unique_ptr<Expression<>> rhs;
  unique_ptr<Expression<>> result;

};

}}

#endif
