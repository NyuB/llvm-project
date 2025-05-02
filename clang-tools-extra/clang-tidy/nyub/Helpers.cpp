#include "Helpers.h"

namespace clang::tidy::nyub {

const Stmt *unwrap(const Stmt *stmt) {
  while (stmt->getStmtClass() == Stmt::ParenExprClass) {
    stmt = *stmt->child_begin();
  }
  return stmt;
}

bool isAnnotatedFor(const clang::CXXMethodDecl *MatchedDecl,
                    const std::string &label) {

  const auto attrs = MatchedDecl->getAttrs();
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
} // namespace clang::tidy::nyub