#include "DerivingShowCheck.h"
#include "Helpers.h"

using clang::ast_matchers::functionDecl;
using clang::ast_matchers::hasAttr;
using clang::ast_matchers::MatchFinder;

namespace clang::tidy::nyub {
const std::string ExpectedLeftHandStreamType = "std::ostream";

static bool isMethod(const clang::FunctionDecl *MatchedDecl) {
  return MatchedDecl->getKind() == Decl::CXXMethod;
}

static bool isMethodDefinitionOutsideClassDeclaration(
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

static SourceRange signatureRange(const clang::FunctionDecl *FunctionDecl) {
  if (FunctionDecl->isThisDeclarationADefinition()) {
    auto Body = FunctionDecl->getBody()->getBeginLoc();
    // Keep leading '{'
    return SourceRange(FunctionDecl->getBeginLoc(), Body.getLocWithOffset(-1));
  }
  return FunctionDecl->getSourceRange();
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
  bool Result = true;

  auto ReturnType =
      dereferencedParamType(MatchedDecl->getReturnType().getUnqualifiedType());
  if (ReturnType.getAsString() != ExpectedLeftHandStreamType) {
    diag(MatchedDecl->getLocation(),
         "function %0 should return %1 instead of %2")
        << MatchedDecl << ExpectedLeftHandStreamType
        << ReturnType.getAsString();
    Result = false;
  }

  if (isMethod(MatchedDecl) &&
      !isMethodDefinitionOutsideClassDeclaration(MatchedDecl) &&
      !MatchedDecl->isStatic()) {
    diag(MatchedDecl->getLocation(), "function %0 should be static")
        << MatchedDecl;
    Result = false;
  }

  if (MatchedDecl->param_size() != 2) {
    diag(MatchedDecl->getLocation(), "function %0 should take 2 parameters")
        << MatchedDecl;
    return false; // so that following code can assume 2 parameters, put
                  // non-parameter checks above
  }

  auto *const OStreamParameter = MatchedDecl->parameters()[0];
  auto *const ThisParameter = MatchedDecl->parameters()[1];

  auto OStreamParameterType = OStreamParameter->getType();
  if (!OStreamParameterType->isLValueReferenceType()) {
    diag(OStreamParameter->getLocation(),
         "parameter %0 should be passed by reference")
        << OStreamParameter;
    Result = false;
  }

  OStreamParameterType = dereferencedParamType(OStreamParameterType);

  if (OStreamParameterType.getUnqualifiedType().getAsString() !=
      ExpectedLeftHandStreamType) {
    diag(OStreamParameter->getLocation(),
         "parameter %0 should be of type std::ostream but is %1")
        << OStreamParameter << OStreamParameterType;
    Result = false;
  }

  return Result;
}

static char lowerCase(char Chr) {
  if (Chr >= 'A' && Chr <= 'Z')
    return 'a' + (Chr - 'A');
  return Chr;
}

static void uncapitalize(std::string *Str) {
  if (Str->empty())
    return;
  Str->at(0) = lowerCase(Str->at(0));
}

std::string
DerivingShowCheck::makeSignature(const clang::FunctionDecl *MatchedDecl) {
  std::string ParamName = "_this";
  std::string ParamType = "T";
  const std::string StaticPrefix =
      (isMethod(MatchedDecl) && !MatchedDecl->isThisDeclarationADefinition())
          ? "static "
          : "";

  if (MatchedDecl->param_size() > 1) {
    ParamName = MatchedDecl->parameters()[1]->getNameAsString();
    ParamType = dereferencedParamType(MatchedDecl->parameters()[1]->getType())
                    .getUnqualifiedType()
                    .getAsString();
  } else if (isMethod(MatchedDecl)) {
    const auto *const MethodDecl =
        static_cast<const CXXMethodDecl *>(MatchedDecl);
    ParamType = MethodDecl->getParent()->getNameAsString();
    ParamName = ParamType;
    uncapitalize(&ParamName);
  }
  const std::string TypePrefix =
      isMethodDefinitionOutsideClassDeclaration(MatchedDecl) ? ParamType + "::"
                                                             : "";

  return StaticPrefix + "std::ostream& " + TypePrefix +
         MatchedDecl->getNameAsString() + "(std::ostream& os, " + ParamType +
         " const& " + ParamName + ")";
}

void DerivingShowCheck::checkBody(const clang::FunctionDecl *MatchedDecl) {
  if (isBodyValid(MatchedDecl))
    return;
  const auto BodyRange = MatchedDecl->getBody()->getSourceRange();
  diag(BodyRange.getBegin(),
       "function %0 body is not suitable for string display")
      << MatchedDecl
      << FixItHint::CreateReplacement(BodyRange, makeBody(MatchedDecl));
}

bool DerivingShowCheck::isBodyValid(const clang::FunctionDecl *MatchedDecl) {

  if (!MatchedDecl->isThisDeclarationADefinition() || !MatchedDecl->getBody())
    return true; // Just a declaration

  const auto *const ReturnStmt = getBodyAsSingleReturnStmt(MatchedDecl);
  if (!ReturnStmt) {
    diag(MatchedDecl->getBody()->getBeginLoc(),
         "function %0 body should consist of a single "
         "return statement")
        << MatchedDecl;
    return false;
  }
  const auto *const ReturnedExpr = getAsCXXOperator(ReturnStmt->getRetValue());
  if (!ReturnedExpr ||
      ReturnedExpr->getOperator() != OverloadedOperatorKind::OO_LessLess) {
    diag(MatchedDecl->getBody()->getBeginLoc(),
         "function %0 body should consist of a single "
         "return statement chaining << operators")
        << MatchedDecl;
    return false;
  }
  return true;
}

static std::string quoted(const std::string &Str) { return '"' + Str + '"'; }

/**
 * Matches annotation with additional content, e.g.
 * `deriving_show::prefix<additionalContent>`
 * - `subAnnotation("deriving_show::prefix",
 * "deriving_show::prefix<additionalContent>") == "additionalContent"`
 * - `subAnnotation("deriving_show::prefix",
 * "deriving_show::oops<additionalContent>") ==
 * {}`
 */
static std::optional<std::string>
subAnnotation(const std::string &AnnotationTag, const std::string &Annotation) {
  if (Annotation.size() < AnnotationTag.size() + 2)
    return {};
  if (Annotation.rfind(AnnotationTag, 0) != 0)
    return {};

  return Annotation.substr(AnnotationTag.size());
}

static std::string fieldRepresentation(const std::string &Receiver,
                                       const FieldDecl *Field) {
  std::string Repr = Receiver + "." + Field->getNameAsString();
  std::string Prefix;
  std::string Suffix;
  for (auto *const Attr : Field->attrs()) {
    if (Attr->getKind() == attr::Kind::Annotate) {
      const std::string Annotation =
          static_cast<const AnnotateAttr *>(Attr)->getAnnotation().str();
      if (const auto SuffixOpt =
              subAnnotation("deriving_show::suffix", Annotation)) {
        Suffix = *SuffixOpt;
      } else if (const auto PrefixOpt =
                     subAnnotation("deriving_show::prefix", Annotation)) {
        Prefix = *PrefixOpt;
      }
    }
  }
  return Prefix + Repr + Suffix;
}

std::string
DerivingShowCheck::makeBody(const clang::FunctionDecl *MatchedDecl) {
  if (MatchedDecl->param_size() != 2)
    return "{ /* TODO */ }";
  auto *const FirstParam = MatchedDecl->parameters()[0];
  auto *const SecondParam = MatchedDecl->parameters()[1];
  const auto *const PrintedType = dereferencedParamType(SecondParam->getType())
                                      ->getUnqualifiedDesugaredType();
  std::string Body;
  if (PrintedType->isRecordType()) {
    auto *const Record = PrintedType->getAsCXXRecordDecl();
    bool First = true;
    for (auto *const Field : Record->fields()) {
      Body += " << ";
      if (!First) {
        Body += quoted(", ." + Field->getNameAsString() + " = ");
      } else {
        Body += quoted("{ ." + Field->getNameAsString() + " = ");
        First = false;
      }
      Body += " << ";
      Body += fieldRepresentation(SecondParam->getNameAsString(), Field);
    }
    Body += " << " + quoted(" }");

  } else {
    Body += " << " + SecondParam->getNameAsString();
  }

  const std::string Prefix = "{ return " + FirstParam->getNameAsString();
  const std::string Suffix = "; }";
  return Prefix + Body + Suffix;
}

} // namespace clang::tidy::nyub