//===--- DerivingEqCheck.cpp - clang-tidy ---------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "DerivingEqCheck.h"
#include "Helpers.h"
#include "clang/AST/ExprCXX.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/OperatorKinds.h"

#include <utility>

using clang::ast_matchers::cxxMethodDecl;
using clang::ast_matchers::hasAttr;
using clang::ast_matchers::MatchFinder;

namespace clang::tidy::nyub {

void DerivingEqCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      cxxMethodDecl(hasAttr(attr::Kind::Annotate)).bind("function"), this);
}

void DerivingEqCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl = Result.Nodes.getNodeAs<CXXMethodDecl>("function");
  if (!isAnnotatedWith(MatchedDecl, "deriving_eq"))
    return;
  signatureCheck(MatchedDecl);
  bodyCheck(MatchedDecl);
}

void DerivingEqCheck::signatureCheck(const clang::CXXMethodDecl *MatchedDecl) {
  if (!isSignatureValid(MatchedDecl)) {
    SourceRange SignatureToReplace;
    if (MatchedDecl->isThisDeclarationADefinition()) {
      auto Body = MatchedDecl->getBody()->getBeginLoc();
      // Keep leading '{'
      SignatureToReplace =
          SourceRange(MatchedDecl->getBeginLoc(), Body.getLocWithOffset(-1));
    } else {
      SignatureToReplace = MatchedDecl->getSourceRange();
    }
    diag(MatchedDecl->getLocation(),
         "function %0 signature is not suitable for an equality operator")
        << MatchedDecl
        << FixItHint::CreateReplacement(SourceRange(SignatureToReplace),
                                        makeSignature(MatchedDecl));
  }
}

void DerivingEqCheck::bodyCheck(const clang::CXXMethodDecl *MatchedDecl) {
  if (!isBodyValid(MatchedDecl)) {
    diag(MatchedDecl->getBody()->getBeginLoc(),
         "function %0 should consist of a single return statement composed of "
         "binary equality comparisons")
        << MatchedDecl
        << FixItHint::CreateReplacement(
               MatchedDecl->getBody()->getSourceRange(), makeBody(MatchedDecl));
  }
}

bool DerivingEqCheck::isSignatureValid(
    const clang::CXXMethodDecl *MatchedDecl) {
  bool Result = true;
  if (MatchedDecl->param_size() != 1) {
    diag(MatchedDecl->getLocation(),
         "function %0 should have a single argument")
        << MatchedDecl;
    Result = false;
  } else {
    const ParmVarDecl *Param = MatchedDecl->getParamDecl(0);
    const QualType ParamType = Param->getType();

    if (!ParamType->isLValueReferenceType()) {
      diag(Param->getLocation(), "parameter %0 should be passed by reference")
          << Param;
      Result = false;
    }

    const QualType ParamRecord =
        dereferencedParamType(ParamType).getCanonicalType();

    if (!ParamRecord.isConstQualified()) {
      diag(Param->getLocation(), "parameter %0 should be const qualified")
          << Param;
      Result = false;
    }

    const auto *ParentClass = MatchedDecl->getParent();
    const auto ParentRecord =
        MatchedDecl->getASTContext().getCanonicalTypeDeclType(ParentClass);

    if (ParamRecord.getUnqualifiedType() != ParentRecord.getUnqualifiedType()) {
      diag(MatchedDecl->getLocation(),
           "function %0 has invalid argument type %1 for equality with %2")
          << MatchedDecl << ParamRecord.getUnqualifiedType()
          << ParentRecord.getUnqualifiedType();
      Result = false;
    }
  }

  if (!MatchedDecl->getReturnType()->isBooleanType()) {
    diag(MatchedDecl->getLocation(), "function %0 has non boolean return type")
        << MatchedDecl;
    Result = false;
  }

  if (!MatchedDecl->isConst()) {
    diag(MatchedDecl->getLocation(), "function %0 should be const")
        << MatchedDecl;
    Result = false;
  }

  return Result;
}

static bool isSingleReturnStmt(const Stmt *Body) {
  auto First = Body->child_begin();
  if (First->getStmtClass() != Stmt::ReturnStmtClass)
    return false;
  First++;
  if (First != Body->child_end())
    return false;
  return true;
}

bool DerivingEqCheck::isBodyValid(const clang::CXXMethodDecl *MatchedDecl) {
  auto *const Body = MatchedDecl->getBody();
  if (!Body) {
    // Just a declaration
    return true;
  }
  if (Body->children().empty()) {
    diag(Body->getBeginLoc(),
         "function %0 has empty body but should return a boolean value")
        << MatchedDecl;
    return false;
  }
  if (!isSingleReturnStmt(Body)) {
    diag(Body->getBeginLoc(),
         "function %0 should consist of a single return statement")
        << MatchedDecl;
    return false;
  }
  const ReturnStmt *RetStmt =
      static_cast<ReturnStmt *>(*MatchedDecl->getBody()->children().begin());

  if (MatchedDecl->getParent()->field_empty())
    return isReturnTrue(RetStmt);
  return isReturnEqualityConjonction(MatchedDecl, RetStmt);
}

std::vector<const FieldDecl *> static parentFields(
    const clang::CXXMethodDecl *MatchedDecl) {
  std::vector<const FieldDecl *> Fields;
  for (const auto *Field : MatchedDecl->getParent()->fields()) {
    if (isAnnotatedWith(Field, "deriving_eq::ignore"))
      continue;
    Fields.push_back(Field);
  }
  return Fields;
}

std::string DerivingEqCheck::makeBody(const CXXMethodDecl *MatchedDecl) {
  const auto Fields = parentFields(MatchedDecl);
  if (Fields.empty())
    return "{ return true; }";
  std::string ParamName;
  if (MatchedDecl->param_size() != 0) {
    ParamName = MatchedDecl->getParamDecl(0)->getNameAsString();
  } else {
    ParamName = "other";
  }
  std::string Result = " { return ";
  bool NoFieldAddedYet = true;
  for (const auto *Field : Fields) {
    const auto Name = Field->getNameAsString();
    if (!NoFieldAddedYet)
      Result += " && ";
    Result += "(";
    Result += Name;
    Result += " == ";
    Result += ParamName;
    Result += ".";
    Result += Name;
    Result += ")";
    NoFieldAddedYet = false;
  }
  Result += "; }";

  return Result;
}

bool DerivingEqCheck::isReturnTrue(const ReturnStmt *Expr) {
  const auto *const Returned = *Expr->child_begin();
  if (Returned->getStmtClass() != Stmt::CXXBoolLiteralExprClass)
    return false;
  const auto *const ReturnedBool =
      static_cast<const CXXBoolLiteralExpr *>(Returned);
  return ReturnedBool->getValue() == true;
}

static bool isFieldBinaryEquality(const FieldDecl *Field,
                                  const BinaryOperator *Binary) {
  if (Binary->getOpcode() != BinaryOperator::Opcode::BO_EQ)
    return false;

  const auto *LHS = unNestImplicitCasts(Binary->getLHS());
  const auto *const LeftMember = getAs<Stmt::MemberExprClass, MemberExpr>(LHS);
  if (!LeftMember ||
      LeftMember->child_begin()->getStmtClass() != Stmt::CXXThisExprClass ||
      LeftMember->getMemberDecl()->getNameAsString() !=
          Field->getNameAsString()) {
    return false;
  }

  const auto *RHS = unNestImplicitCasts(Binary->getRHS());
  const auto *const RightMember = getAs<Stmt::MemberExprClass, MemberExpr>(RHS);
  if (!RightMember ||
      RightMember->child_begin()->getStmtClass() != Stmt::DeclRefExprClass ||
      RightMember->getMemberDecl()->getNameAsString() !=
          Field->getNameAsString()) {
    return false;
  }

  return true;
}

static std::pair<const Stmt *, const Stmt *>
cxxOperatorEqEqSides(const CXXOperatorCallExpr *OperatorCall) {
  assert(operatorCall->getOperator() == OverloadedOperatorKind::OO_EqualEqual);
  // CXXOperatorCall
  // |-- FunctionPtr
  // |-- LHS
  // |-- RHS
  auto ChildrenIterator = OperatorCall->children().begin();
  auto LHS = ++ChildrenIterator;
  auto RHS = ++ChildrenIterator;
  return {*LHS, *RHS};
}

static bool isFieldOperatorEquality(const FieldDecl *Field,
                                    const CXXOperatorCallExpr *OperatorCall) {
  if (OperatorCall->getOperator() != OverloadedOperatorKind::OO_EqualEqual)
    return false;

  auto [LHS, RHS] = cxxOperatorEqEqSides(OperatorCall);

  const auto *LeftMember = getAs<Stmt::MemberExprClass, MemberExpr>(LHS);
  if (!LeftMember)
    return false;
  if (!LeftMember ||
      LeftMember->child_begin()->getStmtClass() != Stmt::CXXThisExprClass ||
      LeftMember->getMemberDecl()->getNameAsString() !=
          Field->getNameAsString()) {
    return false;
  }

  const auto *RightMember = getAs<Stmt::MemberExprClass, MemberExpr>(RHS);
  if (!RightMember)
    return false;
  if (!RightMember ||
      RightMember->child_begin()->getStmtClass() != Stmt::DeclRefExprClass ||
      RightMember->getMemberDecl()->getNameAsString() !=
          Field->getNameAsString()) {
    return false;
  }

  return true;
}

static bool isFieldEquality(const FieldDecl *Field, const Stmt *Stmt) {
  if (auto *Binary = getAsBinaryOperator(Stmt))
    return isFieldBinaryEquality(Field, Binary);
  if (auto *Op = getAsCXXOperator(Stmt))
    return isFieldOperatorEquality(Field, Op);
  return false;
}

bool DerivingEqCheck::isReturnEqualityConjonction(
    const clang::CXXMethodDecl *MatchedDecl, const ReturnStmt *Expr) {
  if (Expr->child_begin() == Expr->child_end())
    return false;

  const auto *Binary = (*Expr->child_begin());

  std::vector<const FieldDecl *> Fields = parentFields(MatchedDecl);
  while (!Fields.empty()) {
    const auto *const Field = Fields.back();
    Fields.pop_back();
    if (Fields.empty())
      return isFieldEquality(Field, Binary);

    const auto *BinaryAnd = getAsBinaryOperator(Binary);
    if (!BinaryAnd ||
        BinaryAnd->getOpcode() != BinaryOperator::Opcode::BO_LAnd) {
      diag((*Expr->child_begin())->getBeginLoc(),
           "function %0 returned expression should be a "
           "boolean conjonction of equalities")
          << MatchedDecl;
      return false;
    }

    const auto *const Eq = BinaryAnd->getRHS();
    if (!isFieldEquality(Field, Eq))
      return false;

    Binary = BinaryAnd->getLHS();
  }

  return true;
}

std::string DerivingEqCheck::makeSignature(const CXXMethodDecl *MatchedDecl) {
  const auto *Parent = MatchedDecl->getParent();
  const auto ParentType = Parent->getName().str();
  std::string ParamName;
  if (MatchedDecl->param_size() != 0) {
    ParamName = MatchedDecl->getParamDecl(0)->getNameAsString();
  } else {
    ParamName = "other";
  }
  return "bool " + MatchedDecl->getNameAsString() + "(" + ParentType +
         " const& " + ParamName + ") const";
}
} // namespace clang::tidy::nyub
