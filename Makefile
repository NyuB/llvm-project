GENERATOR=Ninja
CXX_COMPILER=clang
BUILD_DIR=build

# Without clang, none of clang-query and clang-tidy are available
ENABLED_PROJECTS=clang;clang-tools-extra
RELEASE_WITH_PDB=-DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PDB=ON
# CMake configure phase. This should be run prior to any other action
configure:
	cmake $(RELEASE_WITH_PDB) -DCMAKE_CXX_COMPILER=$(CXX_COMPILER) -B $(BUILD_DIR) -DLLVM_ENABLE_PROJECTS=$(ENABLED_PROJECTS) -G $(GENERATOR) llvm

.PHONY: build
# Build clang-tidy and clang-query
build:
	cmake --build $(BUILD_DIR) --target clang-tidy
	cmake --build $(BUILD_DIR) --target clang-query
	cmake --build $(BUILD_DIR) --target clangd

# List available targets in the current build configuration
$(BUILD_DIR)/target_list.txt:
	cmake --build $(BUILD_DIR) --target help > $(BUILD_DIR)/target_list.txt

# Force eager template parsing to check templates too
CLANG_TIDY_EXTRA_ARGS=--extra-arg=-fno-delayed-template-parsing
ONLY_MY_CHECKS=-checks=-*,nyub-deriving-*
DEMO_FILE=clang-tools-extra/test/clang-tidy/checkers/nyub/deriving_show.cpp
CLANG_TIDY=$(BUILD_DIR)/bin/clang-tidy.exe
CLANG_QUERY=$(BUILD_DIR)/bin/clang-query.exe

demo: build
	$(CLANG_TIDY) $(CLANG_TIDY_EXTRA_ARGS) $(ONLY_MY_CHECKS) $(DEMO_FILE) --
demo-fix: build
	$(CLANG_TIDY) $(CLANG_TIDY_EXTRA_ARGS) $(ONLY_MY_CHECKS) --fix $(DEMO_FILE) --
ast:
	clang -Xclang -ast-dump $(DEMO_FILE)
query:
	$(CLANG_QUERY) $(DEMO_FILE) --

test: export LIT_FILTER=checkers/nyub/
test: build
	ninja -C $(BUILD_DIR) check-clang-tools

SOURCES=$(wildcard clang-tools-extra/clang-tidy/nyub/*.cpp)
SOURCES += $(wildcard clang-tools-extra/clang-tidy/nyub/*.h)
CHECKS = -*
CHECKS := $(CHECKS),bugprone-*
CHECKS := $(CHECKS),cppcoreguidelines-*
CHECKS := $(CHECKS),-cppcoreguidelines-pro-type-static-cast-downcast
CHECKS := $(CHECKS),google-*
CHECKS := $(CHECKS),-google-readability-braces-around-statements
CHECKS := $(CHECKS),llvm-*
CHECKS := $(CHECKS),misc-const-correctness
tidy: build
	$(CLANG_TIDY) -p $(BUILD_DIR) -checks=$(CHECKS) $(SOURCES)
tidy-fix: build
	$(CLANG_TIDY) -fix -p $(BUILD_DIR) -checks=$(CHECKS) $(SOURCES)

# register a new check in the misc module
new-check-%:
	git stash --include-untracked -m "Stash modification before commiting a new check"
	-$(MAKE) -C clang-tools-extra/clang-tidy $@
	-git add .
	-git commit -m "Generated new check '$*'"
	git stash pop
