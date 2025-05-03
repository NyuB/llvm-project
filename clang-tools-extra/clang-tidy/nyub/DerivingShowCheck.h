#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_NYUB_DERIVINGSHOWCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_NYUB_DERIVINGSHOWCHECK_H

#include "../ClangTidyCheck.h"
#include <vector>

namespace clang::tidy::nyub {

class DerivingShowCheck : public ClangTidyCheck {
public:
  DerivingShowCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
  bool isLanguageVersionSupported(const LangOptions &LangOpts) const override {
    return LangOpts.CPlusPlus;
  }

private:
  void checkSignature(const clang::FunctionDecl *MatchedDecl);
  bool isSignatureValid(const clang::FunctionDecl *MatchedDecl);
  static std::string makeSignature(const clang::FunctionDecl *MatchedDecl);
};
} // namespace clang::tidy::nyub
#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_NYUB_DERIVINGSHOWCHECK_H