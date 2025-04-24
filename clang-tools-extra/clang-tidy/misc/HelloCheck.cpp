//===--- HelloCheck.cpp - clang-tidy --------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "HelloCheck.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang::tidy::misc {

void HelloCheck::registerMatchers(MatchFinder *Finder) {
  // Find functions in current file that are annotated with clang::annotate
  Finder->addMatcher(
      functionDecl(isExpansionInMainFile(), hasAttr(attr::Kind::Annotate))
          .bind("x"),
      this);
}

void HelloCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl = Result.Nodes.getNodeAs<FunctionDecl>("x");

  // Only check functions annotated with [[clang::annotate("Hello")]]
  bool IsHelloAnnotated = false;
  for (const AnnotateAttr *attr : MatchedDecl->specific_attrs<AnnotateAttr>()) {
    if (attr->getAnnotation() == "Hello") {
      IsHelloAnnotated = true;
      break;
    }
  }

  if (!IsHelloAnnotated || MatchedDecl->getName().starts_with("hello_"))
    return;
  diag(MatchedDecl->getLocation(), "function %0 is annotated with Hello and "
                                   "should therefore be prefixed with 'hello_'")
      << MatchedDecl
      << FixItHint::CreateInsertion(MatchedDecl->getLocation(), "hello_");
  diag(MatchedDecl->getLocation(), "insert 'hello'", DiagnosticIDs::Note);
}

} // namespace clang::tidy::misc
