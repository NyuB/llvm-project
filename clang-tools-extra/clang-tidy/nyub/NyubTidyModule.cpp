#include "../ClangTidy.h"
#include "../ClangTidyModule.h"
#include "../ClangTidyModuleRegistry.h"
#include "EqualitiesCheck.h"

namespace clang::tidy {
namespace nyub {

class NyubTidyModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<EqualitiesCheck>("nyub-equalities");
  }
};
static ClangTidyModuleRegistry::Add<NyubTidyModule> X("nyub-module",
                                                      "Adds nyub checks");
} // namespace nyub
// This anchor is used to force the linker to link in the generated object file
// and thus register the NyubModule.
volatile int NyubModuleAnchorSource = 0;
} // namespace clang::tidy