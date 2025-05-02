//===--- DerivingEqCheck.h - clang-tidy -------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_NYUB_DERIVINGEQCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_NYUB_DERIVINGEQCHECK_H

#include "../ClangTidyCheck.h"
#include <vector>

namespace clang::tidy::nyub {

/// FIXME: Write a short description.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/misc/equalities.html
class DerivingEqCheck : public ClangTidyCheck {
public:
  DerivingEqCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
  bool isLanguageVersionSupported(const LangOptions &LangOpts) const override {
    return LangOpts.CPlusPlus;
  }

private:
  void signatureCheck(const clang::CXXMethodDecl *MatchedDecl);
  bool isSignatureValid(const clang::CXXMethodDecl *MatchedDecl);
  static std::string makeSignature(const clang::CXXMethodDecl *MatchedDecl);

  void bodyCheck(const clang::CXXMethodDecl *MatchedDecl);
  bool isBodyValid(const clang::CXXMethodDecl *MatchedDecl);
  static std::string makeBody(const clang::CXXMethodDecl *MatchedDecl);

  bool isReturnTrue(const ReturnStmt *expr);
  bool isReturnEqualityConjonction(const clang::CXXMethodDecl *MatchedDecl,
                                   const ReturnStmt *expr);
};

std::vector<const FieldDecl *>
parentFields(const clang::CXXMethodDecl *MatchedDecl);

} // namespace clang::tidy::nyub

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_NYUB_DERIVINGEQCHECK_H
