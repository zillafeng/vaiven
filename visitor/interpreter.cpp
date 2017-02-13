#include "interpreter.h"

#include <iostream>

#include "../ast/all.h"

using std::cout;

using namespace vaiven::visitor;

//int Interpreter::interpret(Node<>& root, vector<int> args, map<string, int>* variablesMap) {
int Interpreter::interpret(Node<>& root) {
  //for(vector<int>::iterator it = args.begin(); it != args.end(); ++it) {
  //  stack.push(*it);
  //}
  //this->variablesMap = variablesMap;

  root.accept(*this);

  if (stack.size()) {
    return stack.top();
  } else {
    return 0;
  }
  //for(int i = 0; i < args.size(); ++i) {
  //  stack.pop();
  //}
  //stack.pop();

  //return val;
}

void Interpreter::visitIfStatement(IfStatement<>& stmt) {
  stmt.condition->accept(*this);
  int condVal = stack.top(); stack.pop();
  if (condVal) {
    for(vector<unique_ptr<Statement<> > >::iterator it = stmt.trueStatements.begin();
        it != stmt.trueStatements.end();
        ++it) {
      if (it != stmt.trueStatements.begin()) {
        stack.pop();
      }
      (*it)->accept(*this);
    }
  } else {
    for(vector<unique_ptr<Statement<> > >::iterator it = stmt.falseStatements.begin();
        it != stmt.falseStatements.end();
        ++it) {
      if (it != stmt.falseStatements.begin()) {
        stack.pop();
      }
      (*it)->accept(*this);
    }
  }
}

void Interpreter::visitReturnStatement(ReturnStatement<>& stmt) {
  stmt.expr->accept(*this);
  throw(stack.top());
}

void Interpreter::visitVarDecl(VarDecl<>& varDecl) {
  throw "not supported";
}

void Interpreter::visitFuncCallExpression(FuncCallExpression<>& expr) {
  if (funcs.funcs.find(expr.name) == funcs.funcs.end()) {
    throw "func not known";
  }

  // TODO check arg counts
  OverkillFunc fptr = funcs.funcs[expr.name]->fptr;
  for(vector<unique_ptr<Expression<> > >::iterator it = expr.parameters.begin();
      it != expr.parameters.end();
      ++it) {
    (*it)->accept(*this);
  }

  int eight;
  int seven;
  int six;
  int five;
  int four;
  int three;
  int two;
  int one;
  switch (expr.parameters.size()) {
    case 8:
      eight = stack.top(); stack.pop();
    case 7:
      seven = stack.top(); stack.pop();
    case 6:
      six = stack.top(); stack.pop();
    case 5:
      five = stack.top(); stack.pop();
    case 4:
      four = stack.top(); stack.pop();
    case 3:
      three = stack.top(); stack.pop();
    case 2:
      two = stack.top(); stack.pop();
    case 1:
      one = stack.top(); stack.pop();
    case 0: break;
    default:
      throw "crap, too many args for my crappy code!";
  }
  
  stack.push(fptr(one, two, three, four, five, six, seven, eight));

}

void Interpreter::visitFuncDecl(FuncDecl<>& decl) {
  // nothing
}

void Interpreter::visitExpressionStatement(ExpressionStatement<>& stmt) {
  stmt.expr->accept(*this);
}

void Interpreter::visitBlock(Block<>& block) {
  for(vector<unique_ptr<Statement<> > >::iterator it = block.statements.begin();
      it != block.statements.end();
      ++it) {
    if (it != block.statements.begin()) {
      stack.pop();
    }
    (*it)->accept(*this);
  }
}

void Interpreter::visitAdditionExpression(AdditionExpression<>& expr) {
  expr.left->accept(*this);
  expr.right->accept(*this);
  int right = stack.top(); stack.pop();
  int left = stack.top(); stack.pop();
  stack.push(left + right);
}
void Interpreter::visitSubtractionExpression(SubtractionExpression<>& expr) {
  expr.left->accept(*this);
  expr.right->accept(*this);
  int right = stack.top(); stack.pop();
  int left = stack.top(); stack.pop();
  stack.push(left - right);
}
void Interpreter::visitMultiplicationExpression(MultiplicationExpression<>& expr) {
  expr.left->accept(*this);
  expr.right->accept(*this);
  int right = stack.top(); stack.pop();
  int left = stack.top(); stack.pop();
  stack.push(left * right);
}
void Interpreter::visitDivisionExpression(DivisionExpression<>& expr) {
  expr.left->accept(*this);
  expr.right->accept(*this);
  int right = stack.top(); stack.pop();
  int left = stack.top(); stack.pop();
  stack.push(left / right);
}
void Interpreter::visitIntegerExpression(IntegerExpression<>& expr) {
  stack.push(expr.value);
}
void Interpreter::visitVariableExpression(VariableExpression<>& expr) {
  int val = stack.c[(*variablesMap)[expr.id]];
  stack.push(val);
}
