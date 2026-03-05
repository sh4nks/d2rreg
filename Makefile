.PHONY: clean build format run help
.DEFAULT_GOAL := help

help: ## Displays this help message.
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {printf "\033[36m%-15s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)

prepare: ## Prepares the build directory
	rm -rf build/ && cmake -B build -DCMAKE_TOOLCHAIN_FILE=./toolchain-mingw.cmake

build: ## Builds the project
	cmake --build build/ -j 6

run: ## Runs the project
	wine ./build/d2rreg.exe

clean: ## Remove unwanted stuff such as __pycache__, etc...
	find . -name '*.pyc' -exec rm -f {} +
	find . -name '*.pyo' -exec rm -f {} +
	find . -name '*~' -exec rm -f {} +
	find . -name '__pycache__' -exec rm -rf {} +
	find . -name '.ruff_cache' -exec rm -rf {} +
