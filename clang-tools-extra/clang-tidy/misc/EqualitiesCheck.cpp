//===--- EqualitiesCheck.cpp - clang-tidy ---------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "EqualitiesCheck.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <algorithm>
#include <iostream>

using namespace clang::ast_matchers;

namespace clang::tidy::misc {

bool isAnnotatedForEquality(const clang::CXXMethodDecl *MatchedDecl) {

  const auto attrs = MatchedDecl->getAttrs();
  return std::find_if(attrs.begin(), attrs.end(), [](const Attr *attr) {
           return attr->getKind() == attr::Kind::Annotate &&
                  static_cast<const AnnotateAttr *>(attr)->getAnnotation() ==
                      "deriving_eq";
         }) != attrs.end();
}

void EqualitiesCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      cxxMethodDecl(hasAttr(attr::Kind::Annotate)).bind("function"), this);
}

void EqualitiesCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl = Result.Nodes.getNodeAs<CXXMethodDecl>("function");
  if (!isAnnotatedForEquality(MatchedDecl))
    return;
  signatureCheck(MatchedDecl);
  bodyCheck(MatchedDecl);
}

void EqualitiesCheck::signatureCheck(const clang::CXXMethodDecl *MatchedDecl) {
  if (!isSignatureValid(MatchedDecl)) {
    SourceRange signatureToReplace;
    if (MatchedDecl->isThisDeclarationADefinition()) {
      auto body = MatchedDecl->getBody()->getBeginLoc();
      // Keep leading '{'
      signatureToReplace =
          SourceRange(MatchedDecl->getBeginLoc(), body.getLocWithOffset(-1));
    } else {
      signatureToReplace = MatchedDecl->getSourceRange();
    }
    diag(MatchedDecl->getLocation(),
         "function %0 signature is not suitable for an equality operator")
        << MatchedDecl
        << FixItHint::CreateReplacement(SourceRange(signatureToReplace),
                                        makeSignature(MatchedDecl));
  }
}

void EqualitiesCheck::bodyCheck(const clang::CXXMethodDecl *MatchedDecl) {
  if (!isBodyValid(MatchedDecl)) {
    diag(MatchedDecl->getBody()->getBeginLoc(),
         "function %0 should consist of a single return statement composed of "
         "binary equality comparisons")
        << MatchedDecl
        << FixItHint::CreateReplacement(
               MatchedDecl->getBody()->getSourceRange(), makeBody(MatchedDecl));
  }
}

QualType dereferencedParamType(QualType paramType) {
  while (paramType->isPointerOrReferenceType()) {
    if (paramType->isPointerType())
      paramType = paramType->getAs<PointerType>()->getPointeeType();
    else
      paramType = paramType->getAs<ReferenceType>()->getPointeeType();
  }
  return paramType;
}

bool EqualitiesCheck::isSignatureValid(
    const clang::CXXMethodDecl *MatchedDecl) {
  bool result = true;
  if (MatchedDecl->param_size() != 1) {
    diag(MatchedDecl->getLocation(),
         "function %0 should have a single argument")
        << MatchedDecl;
    result = false;
  } else {
    const ParmVarDecl *param = MatchedDecl->getParamDecl(0);
    QualType paramType = param->getType();

    if (!paramType->isLValueReferenceType()) {
      diag(param->getLocation(), "parameter %0 should be passed by reference")
          << param;
      result = false;
    }

    QualType paramRecord = dereferencedParamType(paramType).getCanonicalType();

    if (!paramRecord.isConstQualified()) {
      diag(param->getLocation(), "parameter %0 should be const qualified")
          << param;
      result = false;
    }

    const auto *parentClass = MatchedDecl->getParent();
    const auto parentRecord =
        MatchedDecl->getASTContext()
            .getRecordType(parentClass->getTypeForDecl()->getAsCXXRecordDecl())
            .getCanonicalType();

    if (paramRecord.getUnqualifiedType() != parentRecord.getUnqualifiedType()) {
      diag(MatchedDecl->getLocation(),
           "function %0 has invalid argument type %1 for equality with %2")
          << MatchedDecl << paramRecord.getUnqualifiedType()
          << parentRecord.getUnqualifiedType();
      result = false;
    }
  }

  if (!MatchedDecl->getReturnType()->isBooleanType()) {
    diag(MatchedDecl->getLocation(), "function %0 has non boolean return type")
        << MatchedDecl;
    result = false;
  }

  if (!MatchedDecl->isConst()) {
    diag(MatchedDecl->getLocation(), "function %0 should be const")
        << MatchedDecl;
    result = false;
  }

  return result;
}

bool isSingleReturnStmt(const Stmt *body) {
  auto first = body->child_begin();
  if (first->getStmtClass() != Stmt::ReturnStmtClass)
    return false;
  first++;
  if (first != body->child_end())
    return false;
  return true;
}

bool EqualitiesCheck::isBodyValid(const clang::CXXMethodDecl *MatchedDecl) {
  const auto body = MatchedDecl->getBody();
  if (!body) {
    // Just a declaration
    return true;
  }
  if (body->children().empty()) {
    diag(body->getBeginLoc(),
         "function %0 has empty body but should return a boolean value")
        << MatchedDecl;
    return false;
  }
  if (!isSingleReturnStmt(body)) {
    diag(body->getBeginLoc(),
         "function %0 should consist of a single return statement")
        << MatchedDecl;
    return false;
  }
  const ReturnStmt *returnStmt =
      static_cast<ReturnStmt *>(*MatchedDecl->getBody()->children().begin());
  for (auto child : returnStmt->children()) {
    if (child->getStmtClass() != Stmt::BinaryOperatorClass) {
      diag(child->getBeginLoc(), "function %0 returned expression should be a "
                                 "boolean conjonction of equalities")
          << MatchedDecl;
      return false;
    }
  }
  return true;
}

std::string EqualitiesCheck::makeBody(const CXXMethodDecl *MatchedDecl) {
  const auto *parent = MatchedDecl->getParent();
  std::string paramName;
  if (MatchedDecl->param_size() != 0) {
    paramName = MatchedDecl->getParamDecl(0)->getNameAsString();
  } else {
    paramName = "other";
  }
  std::string result = " { return ";
  bool noFieldAddedYet = true;
  for (const auto *field : parent->fields()) {
    const auto name = field->getNameAsString();
    if (!noFieldAddedYet) {
      result += " && ";
    }
    result += "(";
    result += name;
    result += " == ";
    result += paramName;
    result += ".";
    result += name;
    result += ")";
    noFieldAddedYet = false;
  }
  result += "; }";
  if (noFieldAddedYet)
    return "{ return true; }";
  return result;
}

std::string EqualitiesCheck::makeSignature(const CXXMethodDecl *MatchedDecl) {
  const auto *parent = MatchedDecl->getParent();
  const auto parentType =
      MatchedDecl->getASTContext()
          .getRecordType(parent->getTypeForDecl()->getAsCXXRecordDecl())
          .getUnqualifiedType()
          .getBaseTypeIdentifier()
          ->getName()
          .str();
  std::string paramName;
  if (MatchedDecl->param_size() != 0) {
    paramName = MatchedDecl->getParamDecl(0)->getNameAsString();
  } else {
    paramName = "other";
  }
  return "bool " + MatchedDecl->getNameAsString() + "(" + parentType +
         " const& " + paramName + ") const";
}

} // namespace clang::tidy::misc
