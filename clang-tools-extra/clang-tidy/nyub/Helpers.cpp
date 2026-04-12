#include "Helpers.h"

namespace clang::tidy::nyub {

const Stmt *unwrap(const Stmt *Stmt) {
  while (Stmt->getStmtClass() == Stmt::ParenExprClass)
    Stmt = *Stmt->child_begin();
  return Stmt;
}

bool isAnnotatedWith(const clang::Decl *Decl, const std::string &Label) {
  const auto &Attrs = Decl->getAttrs();
  return std::find_if(Attrs.begin(), Attrs.end(), [&](const Attr *Attr) {
           return Attr->getKind() == attr::Kind::Annotate &&
                  static_cast<const AnnotateAttr *>(Attr)->getAnnotation() ==
                      Label;
         }) != Attrs.end();
}

QualType dereferencedParamType(QualType ParamType) {
  while (ParamType->isPointerOrReferenceType())
    if (ParamType->isPointerType())
      ParamType = ParamType->getAs<PointerType>()->getPointeeType();
    else
      ParamType = ParamType->getAs<ReferenceType>()->getPointeeType();
  return ParamType;
}

const BinaryOperator *getAsBinaryOperator(const Stmt *Expr) {
  return getAs<Stmt::BinaryOperatorClass, BinaryOperator>(Expr);
}

const CXXOperatorCallExpr *getAsCXXOperator(const Stmt *Expr) {
  return getAs<Stmt::CXXOperatorCallExprClass, CXXOperatorCallExpr>(Expr);
}

const ReturnStmt *getBodyAsSingleReturnStmt(const FunctionDecl *Decl) {
  auto *const Body = Decl->getBody();
  if (!Body || Body->children().empty())
    return nullptr;
  auto First = Body->child_begin();
  if (First->getStmtClass() != Stmt::ReturnStmtClass)
    return nullptr;

  const auto *const RetStmt = getAs<Stmt::ReturnStmtClass, ReturnStmt>(*First);

  First++;
  if (First != Body->child_end())
    return nullptr;

  return RetStmt;
}
} // namespace clang::tidy::nyub