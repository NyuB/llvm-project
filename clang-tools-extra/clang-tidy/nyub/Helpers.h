#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_NYUB_HELPERS_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_NYUB_HELPERS_H

#include "clang/ASTMatchers/ASTMatchFinder.h"

namespace clang::tidy::nyub {
bool isAnnotatedFor(const clang::CXXMethodDecl *MatchedDecl,
                    const std::string &label);
QualType dereferencedParamType(QualType paramType);
const Stmt *unwrap(const Stmt *stmt);
/**
 * @brief safe match + cast for a given ast node
 * @note @p stmt is unwrap ped first if nested in parenthesis expressions
 * @return @p stmt casted as @p AstClass or nullptr if @p stmt is not a node of
 * class @p AstClassTag
 */
template <Stmt::StmtClass AstClassTag, typename AstClass>
const AstClass *getAs(const Stmt *stmt) {
  stmt = unwrap(stmt);
  if (stmt->getStmtClass() != AstClassTag)
    return nullptr;
  return static_cast<const AstClass *>(stmt);
}
} // namespace clang::tidy::nyub
#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_NYUB_HELPERS_H