#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_NYUB_HELPERS_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_NYUB_HELPERS_H

#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/TypeBase.h"

namespace clang::tidy::nyub {
bool isAnnotatedWith(const clang::Decl *Decl, const std::string &label);
QualType dereferencedParamType(QualType ParamType);
const Stmt *unwrap(const Stmt *Stmt);
/**
 * @brief safe match + cast for a given ast node
 * @note @p stmt is unwrapped first if nested in parenthesis expressions
 * @return @p stmt casted as @p AstClass or nullptr if @p stmt is not a node of
 * class @p AstClassTag
 */
template <Stmt::StmtClass AstClassTag, typename AstClass>
const AstClass *getAs(const Stmt *Stmt) {
  if (Stmt == nullptr)
    return nullptr;
  Stmt = unwrap(Stmt);
  if (Stmt->getStmtClass() != AstClassTag)
    return nullptr;
  return static_cast<const AstClass *>(Stmt);
}
const BinaryOperator *getAsBinaryOperator(const Stmt *Expr);
const CXXOperatorCallExpr *getAsCXXOperator(const Stmt *Expr);
const ReturnStmt *getBodyAsSingleReturnStmt(const FunctionDecl *Decl);
const Stmt *unNestImplicitCasts(const Stmt *Expr);
} // namespace clang::tidy::nyub
#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_NYUB_HELPERS_H