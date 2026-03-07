# Sample Makefile For FujiNet Applications

TARGETS = atari coco
PROGRAM := killzone

# Set this to the version of FN-LIB you wish to use in this project:
export FUJINET_LIB_VERSION := 4.9.0

SUB_TASKS := clean disk test release
.PHONY: all help apple2 apple2enh apple2-disk apple2-release apple2enh-disk apple2enh-release $(SUB_TASKS)

all:
	@for target in $(TARGETS); do \
		echo "-------------------------------------"; \
		echo "Building $$target"; \
		echo "-------------------------------------"; \
		$(MAKE) --no-print-directory -f ./makefiles/build.mk CURRENT_TARGET=$$target PROGRAM=$(PROGRAM) $(MAKECMDGOALS); \
	done
	
# if disk images were built show them
	@if [ "$(DEBUG)" = "true" ] && [ -d "./dist" ]; then \
		echo "Contents of dist:"; \
		ls -1 ./dist; \
	fi

$(SUB_TASKS): _do_all
$(SUB_TASKS):
	@:

_do_all: all

apple2:
	@$(MAKE) --no-print-directory -f ./makefiles/build.mk CURRENT_TARGET=apple2 PROGRAM=$(PROGRAM) $(filter-out $@,$(MAKECMDGOALS))

apple2enh:
	@$(MAKE) --no-print-directory -f ./makefiles/build.mk CURRENT_TARGET=apple2enh PROGRAM=$(PROGRAM) $(filter-out $@,$(MAKECMDGOALS))

apple2-disk:
	@$(MAKE) --no-print-directory -f ./makefiles/build.mk CURRENT_TARGET=apple2 PROGRAM=$(PROGRAM) disk

apple2-release:
	@$(MAKE) --no-print-directory -f ./makefiles/build.mk CURRENT_TARGET=apple2 PROGRAM=$(PROGRAM) release

apple2enh-disk:
	@$(MAKE) --no-print-directory -f ./makefiles/build.mk CURRENT_TARGET=apple2enh PROGRAM=$(PROGRAM) disk

apple2enh-release:
	@$(MAKE) --no-print-directory -f ./makefiles/build.mk CURRENT_TARGET=apple2enh PROGRAM=$(PROGRAM) release

help:
	@echo "Makefile for $(PROGRAM)"
	@echo ""
	@echo "Available tasks:"
	@echo "all       - do all compilation tasks, create app in build directory"
	@echo "clean     - remove all build artifacts"
	@echo "release   - create a release of the executable in the build/ dir"
	@echo "disk      - generate platform specific disk images in dist/ dir"
	@echo "test      - run application in emulator for given platform."
	@echo "            specific platforms may expose additional variables to run with"
	@echo "            different emulators, see makefiles/custom-<platform>.mk"
	@echo "apple2    - build only the apple2 target"
	@echo "apple2enh - build only the apple2enh target"
	@echo "apple2-disk / apple2-release"
	@echo "          - create apple2 disk image or release"
	@echo "apple2enh-disk / apple2enh-release"
	@echo "          - create apple2enh disk image or release"
	
