#include "DerivingShowCheck.h"
#include "Helpers.h"
#include <algorithm>
#include <iostream>

using clang::ast_matchers::functionDecl;
using clang::ast_matchers::hasAttr;
using clang::ast_matchers::MatchFinder;

namespace clang::tidy::nyub {

void DerivingShowCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      functionDecl(hasAttr(attr::Kind::Annotate)).bind("function"), this);
}

void DerivingShowCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl = Result.Nodes.getNodeAs<FunctionDecl>("function");
  if (!isAnnotatedWith(MatchedDecl, "deriving_show"))
    return;
  checkSignature(MatchedDecl);
}

void DerivingShowCheck::checkSignature(const clang::FunctionDecl *MatchedDecl) {
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
         "function %0 signature is not suitable for string display")
        << MatchedDecl
        << FixItHint::CreateReplacement(signatureToReplace,
                                        makeSignature(MatchedDecl));
  }
}

bool DerivingShowCheck::isSignatureValid(
    const clang::FunctionDecl *MatchedDecl) {

  if (MatchedDecl->param_size() != 2) {
    diag(MatchedDecl->getLocation(), "function %0 should take 2 parameters")
        << MatchedDecl;
    return false;
  }

  const auto ostreamParameter = MatchedDecl->parameters()[0];
  const auto thisParameter = MatchedDecl->parameters()[1];

  bool result = true;

  auto ostreamParameterType = ostreamParameter->getType();
  if (!ostreamParameterType->isLValueReferenceType()) {
    diag(ostreamParameter->getLocation(),
         "parameter %0 should be passed by reference")
        << ostreamParameter;
    result = false;
  }

  ostreamParameterType = dereferencedParamType(ostreamParameterType);

  if (ostreamParameterType.getUnqualifiedType().getAsString() !=
      "std::ostream") {
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
  bool isMethod = MatchedDecl->getKind() == Decl::CXXMethod;
  std::string staticPrefix = isMethod ? "static " : "";

  if (MatchedDecl->param_size() > 1) {
    paramName = MatchedDecl->parameters()[1]->getNameAsString();
    paramType = dereferencedParamType(MatchedDecl->parameters()[1]->getType())
                    .getUnqualifiedType()
                    .getAsString();
  } else if (isMethod) {
    const auto methodDecl = static_cast<const CXXMethodDecl *>(MatchedDecl);
    paramType = methodDecl->getParent()->getNameAsString();
    paramName = paramType;
    uncapitalize(&paramName);
  }

  return staticPrefix + "std::ostream& " + MatchedDecl->getNameAsString() +
         "(std::ostream& os, " + paramType + " const& " + paramName + ")";
}

} // namespace clang::tidy::nyub