# Copyright (c) 2018, Sergiy Yevtushenko
# Copyright (c) 2018-2019, Niklas Hauser
#
# This file is part of the modm project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

BUILD_DIR = build/spincoater

CMAKE_GENERATOR = Unix Makefiles
CMAKE_FLAGS = -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON -DCMAKE_RULE_MESSAGES:BOOL=ON -DCMAKE_VERBOSE_MAKEFILE:BOOL=OFF

.PHONY: cmake build clean cleanall program program-bmp debug debug-bmp debug-coredump log-itm

.DEFAULT_GOAL := all

### Targets
all: cmake build

cmake:
	@cmake -E make_directory $(BUILD_DIR)/cmake-build-debug
	@cmake -E make_directory $(BUILD_DIR)/cmake-build-release
	@cd $(BUILD_DIR)/cmake-build-debug && cmake $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=Debug -G "$(CMAKE_GENERATOR)" ../../..
	@cd $(BUILD_DIR)/cmake-build-release && cmake $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=Release -G "$(CMAKE_GENERATOR)" ../../..

clean:
	@cmake --build $(BUILD_DIR)/cmake-build-release --target clean
	@cmake --build $(BUILD_DIR)/cmake-build-debug --target clean

cleanall:
	@rm -rf $(BUILD_DIR)/cmake-build-release
	@rm -rf $(BUILD_DIR)/cmake-build-debug

profile?=release
build:
	@cmake --build $(BUILD_DIR)/cmake-build-$(profile)

port?=auto
ELF_FILE=$(BUILD_DIR)/cmake-build-$(profile)/spincoater.elf
MEMORIES = "[{'name': 'flash', 'access': 'rx', 'start': 134217728, 'size': 131072}, {'name': 'ccm', 'access': 'rwx', 'start': 268435456, 'size': 10240}, {'name': 'sram1', 'access': 'rwx', 'start': 536870912, 'size': 26624}, {'name': 'sram2', 'access': 'rwx', 'start': 536897536, 'size': 6144}]"
size: build
	@python3 modm/modm_tools/size.py $(ELF_FILE) $(MEMORIES)

program: build
	@python3 modm/modm_tools/openocd.py -f modm/openocd.cfg $(ELF_FILE)

program-bmp: build
	@python3 modm/modm_tools/bmp.py -p $(port) $(ELF_FILE)

ui?=tui
debug: build
	@python3 modm/modm_tools/gdb.py -x modm/gdbinit -x modm/openocd_gdbinit \
			$(ELF_FILE) -ui=$(ui) \
			openocd -f modm/openocd.cfg

debug-bmp: build
	@python3 modm/modm_tools/bmp.py -x modm/gdbinit \
			$(ELF_FILE) -ui=$(ui) \
			bmp -p $(port)

debug-coredump: build
	@python3 modm/modm_tools/gdb.py -x modm/gdbinit \
			$(ELF_FILE) -ui=$(ui) \
			crashdebug --binary-path modm/ext/crashcatcher/bins

fcpu?=0
log-itm:
	@python3 modm/modm_tools/log.py itm openocd -f modm/openocd.cfg -fcpu $(fcpu)
