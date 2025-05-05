#include "Helpers.h"

namespace clang::tidy::nyub {

const Stmt *unwrap(const Stmt *stmt) {
  while (stmt->getStmtClass() == Stmt::ParenExprClass) {
    stmt = *stmt->child_begin();
  }
  return stmt;
}

bool isAnnotatedWith(const clang::Decl *decl, const std::string &label) {

  const auto attrs = decl->getAttrs();
  return std::find_if(attrs.begin(), attrs.end(), [&](const Attr *attr) {
           return attr->getKind() == attr::Kind::Annotate &&
                  static_cast<const AnnotateAttr *>(attr)->getAnnotation() ==
                      label;
         }) != attrs.end();
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

const BinaryOperator *getAsBinaryOperator(const Stmt *expr) {
  return getAs<Stmt::BinaryOperatorClass, BinaryOperator>(expr);
}

const CXXOperatorCallExpr *getAsCXXOperator(const Stmt *expr) {
  return getAs<Stmt::CXXOperatorCallExprClass, CXXOperatorCallExpr>(expr);
}

const ReturnStmt *getBodyAsSingleReturnStmt(const FunctionDecl *decl) {
  auto *const body = decl->getBody();
  if (!body || body->children().empty())
    return nullptr;
  auto first = body->child_begin();
  if (first->getStmtClass() != Stmt::ReturnStmtClass)
    return nullptr;

  const auto *const returnStmt =
      getAs<Stmt::ReturnStmtClass, ReturnStmt>(*first);

  first++;
  if (first != body->child_end())
    return nullptr;

  return returnStmt;
}
} // namespace clang::tidy::nyub