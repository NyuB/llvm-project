GENERATOR=Ninja
CXX_COMPILER=clang
BUILD_DIR=build

# Without clang, none of clang-query and clang-tidy are available
ENABLED_PROJECTS=clang;clang-tools-extra

# CMake configure phase. This should be run prior to any other action
configure:
	cmake -B $(BUILD_DIR) -DLLVM_ENABLE_PROJECTS=$(ENABLED_PROJECTS) -DCMAKE_BUILD_TYPE=Release -G $(GENERATOR) llvm

# Build clang-tidy and clang-query
rebuild:
	cmake --build $(BUILD_DIR) --target clang-tidy
	cmake --build $(BUILD_DIR) --target clang-query

# List available targets in the current build configuration
$(BUILD_DIR)/target_list.txt:
	cmake --build $(BUILD_DIR) --target help > $(BUILD_DIR)/target_list.txt

# Force eager template parsing to check templates too
CLANG_TIDY_EXTRA_ARGS=--extra-arg=-fno-delayed-template-parsing

demo: rebuild
	build/bin/clang-tidy.exe $(CLANG_TIDY_EXTRA_ARGS) -checks=-*,misc-equalities clang-tools-extra/test/clang-tidy/checkers/misc/equalities.cpp --
demo-fix: rebuild
	build/bin/clang-tidy.exe $(CLANG_TIDY_EXTRA_ARGS) --fix -checks=-*,misc-equalities clang-tools-extra/test/clang-tidy/checkers/misc/equalities.cpp --
ast:
	clang -Xclang -ast-dump clang-tools-extra/test/clang-tidy/checkers/misc/equalities.cpp
query:
	build/bin/clang-query.exe clang-tools-extra/test/clang-tidy/checkers/misc/equalities.cpp --

test: rebuild
	LIT_FILTER="checkers/misc/equalities" ninja -C $(BUILD_DIR) check-clang-tools

# register a new check in the misc module
new-check-%:
	git stash --include-untracked -m "Stash modification before commiting a new check"
	-$(MAKE) -C clang-tools-extra/clang-tidy $@
	-git add .
	-git commit -m "Generated new check '$*'"
	git stash pop
