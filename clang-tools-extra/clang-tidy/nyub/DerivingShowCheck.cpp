#include "DerivingShowCheck.h"
#include "Helpers.h"
#include <algorithm>
#include <iostream>

using clang::ast_matchers::functionDecl;
using clang::ast_matchers::hasAttr;
using clang::ast_matchers::MatchFinder;

namespace clang::tidy::nyub {
const std::string expectedLeftHandStreamType = "std::ostream";

bool isMethod(const clang::FunctionDecl *MatchedDecl) {
  return MatchedDecl->getKind() == Decl::CXXMethod;
}

bool isMethodDefinitionOutsideClassDeclaration(
    const clang::FunctionDecl *MatchedDecl) {
  return isMethod(MatchedDecl) && MatchedDecl->isThisDeclarationADefinition() &&
         (MatchedDecl->getFirstDecl() != MatchedDecl);
}

void DerivingShowCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      functionDecl(hasAttr(attr::Kind::Annotate)).bind("function"), this);
}

void DerivingShowCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl = Result.Nodes.getNodeAs<FunctionDecl>("function");
  if (!isAnnotatedWith(MatchedDecl, "deriving_show"))
    return;
  checkSignature(MatchedDecl);
  checkBody(MatchedDecl);
}

SourceRange signatureRange(const clang::FunctionDecl *functionDecl) {
  if (functionDecl->isThisDeclarationADefinition()) {
    auto body = functionDecl->getBody()->getBeginLoc();
    // Keep leading '{'
    return SourceRange(functionDecl->getBeginLoc(), body.getLocWithOffset(-1));
  }
  return functionDecl->getSourceRange();
}

void DerivingShowCheck::checkSignature(const clang::FunctionDecl *MatchedDecl) {
  if (!isSignatureValid(MatchedDecl)) {
    diag(MatchedDecl->getLocation(),
         "function %0 signature is not suitable for string display")
        << MatchedDecl
        << FixItHint::CreateReplacement(signatureRange(MatchedDecl),
                                        makeSignature(MatchedDecl));
  }
}

bool DerivingShowCheck::isSignatureValid(
    const clang::FunctionDecl *MatchedDecl) {
  bool result = true;

  auto returnType =
      dereferencedParamType(MatchedDecl->getReturnType().getUnqualifiedType());
  if (returnType.getAsString() != expectedLeftHandStreamType) {
    diag(MatchedDecl->getLocation(),
         "function %0 should return %1 instead of %2")
        << MatchedDecl << expectedLeftHandStreamType
        << returnType.getAsString();
    result = false;
  }

  if (isMethod(MatchedDecl) &&
      !isMethodDefinitionOutsideClassDeclaration(MatchedDecl) &&
      !MatchedDecl->isStatic()) {
    diag(MatchedDecl->getLocation(), "function %0 should be static")
        << MatchedDecl;
    result = false;
  }

  if (MatchedDecl->param_size() != 2) {
    diag(MatchedDecl->getLocation(), "function %0 should take 2 parameters")
        << MatchedDecl;
    return false; // so that following code can assume 2 parameters, put
                  // non-parameter checks above
  }

  auto *const ostreamParameter = MatchedDecl->parameters()[0];
  auto *const thisParameter = MatchedDecl->parameters()[1];

  auto ostreamParameterType = ostreamParameter->getType();
  if (!ostreamParameterType->isLValueReferenceType()) {
    diag(ostreamParameter->getLocation(),
         "parameter %0 should be passed by reference")
        << ostreamParameter;
    result = false;
  }

  ostreamParameterType = dereferencedParamType(ostreamParameterType);

  if (ostreamParameterType.getUnqualifiedType().getAsString() !=
      expectedLeftHandStreamType) {
    diag(ostreamParameter->getLocation(),
         "parameter %0 should be of type std::ostream but is %1")
        << ostreamParameter << ostreamParameterType;
    result = false;
  }

  return result;
}

char lowerCase(char c) {
  if (c >= 'A' && c <= 'Z') {
    return 'a' + (c - 'A');
  }
  return c;
}

void uncapitalize(std::string *s) {
  if (s->empty())
    return;
  s->at(0) = lowerCase(s->at(0));
}

std::string
DerivingShowCheck::makeSignature(const clang::FunctionDecl *MatchedDecl) {
  std::string paramName = "_this";
  std::string paramType = "T";
  const std::string staticPrefix =
      (isMethod && !MatchedDecl->isThisDeclarationADefinition()) ? "static "
                                                                 : "";

  if (MatchedDecl->param_size() > 1) {
    paramName = MatchedDecl->parameters()[1]->getNameAsString();
    paramType = dereferencedParamType(MatchedDecl->parameters()[1]->getType())
                    .getUnqualifiedType()
                    .getAsString();
  } else if (isMethod(MatchedDecl)) {
    const auto *const methodDecl =
        static_cast<const CXXMethodDecl *>(MatchedDecl);
    paramType = methodDecl->getParent()->getNameAsString();
    paramName = paramType;
    uncapitalize(&paramName);
  }
  const std::string typePrefix =
      isMethodDefinitionOutsideClassDeclaration(MatchedDecl) ? paramType + "::"
                                                             : "";

  return staticPrefix + "std::ostream& " + typePrefix +
         MatchedDecl->getNameAsString() + "(std::ostream& os, " + paramType +
         " const& " + paramName + ")";
}

void DerivingShowCheck::checkBody(const clang::FunctionDecl *MatchedDecl) {
  if (isBodyValid(MatchedDecl))
    return;
  const auto bodyRange = MatchedDecl->getBody()->getSourceRange();
  diag(bodyRange.getBegin(),
       "function %0 body is not suitable for string display")
      << MatchedDecl
      << FixItHint::CreateReplacement(bodyRange, makeBody(MatchedDecl));
}

bool DerivingShowCheck::isBodyValid(const clang::FunctionDecl *MatchedDecl) {

  if (!MatchedDecl->isThisDeclarationADefinition() || !MatchedDecl->getBody())
    return true; // Just a declaration

  const auto *const returnStmt = getBodyAsSingleReturnStmt(MatchedDecl);
  if (!returnStmt) {
    diag(MatchedDecl->getBody()->getBeginLoc(),
         "function %0 body should consist of a single "
         "return statement")
        << MatchedDecl;
    return false;
  }
  const auto *const returnedExpr = getAsCXXOperator(returnStmt->getRetValue());
  if (!returnedExpr ||
      returnedExpr->getOperator() != OverloadedOperatorKind::OO_LessLess) {
    diag(MatchedDecl->getBody()->getBeginLoc(),
         "function %0 body should consist of a single "
         "return statement chaining << operators")
        << MatchedDecl;
    return false;
  }
  return true;
}

std::string quoted(std::string s) { return '"' + s + '"'; }

std::string
DerivingShowCheck::makeBody(const clang::FunctionDecl *MatchedDecl) {
  if (MatchedDecl->param_size() != 2)
    return "{ /* TODO */ }";
  auto *const firstParam = MatchedDecl->parameters()[0];
  auto *const secondParam = MatchedDecl->parameters()[1];
  const auto *const printedType = dereferencedParamType(secondParam->getType())
                                      ->getUnqualifiedDesugaredType();
  std::string body = "";
  if (printedType->isRecordType()) {
    auto *const record = printedType->getAsCXXRecordDecl();
    bool first = true;
    for (auto *const field : record->fields()) {
      body += " << ";
      if (!first) {
        body += quoted(", ." + field->getNameAsString() + " = ");
      } else {
        body += quoted("{ ." + field->getNameAsString() + " = ");
        first = false;
      }
      body += " << ";
      body += secondParam->getNameAsString();
      body += ".";
      body += field->getNameAsString();
    }
    body += " << " + quoted(" }");

  } else {
    body += " << " + secondParam->getNameAsString();
  }
  body += " << " + quoted(" }");
  const std::string prefix = "{ return " + firstParam->getNameAsString();
  const std::string suffix = "; }";
  return prefix + body + suffix;
}

} // namespace clang::tidy::nyub