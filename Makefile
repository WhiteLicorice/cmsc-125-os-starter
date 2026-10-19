# SPDX-License-Identifier: MIT
#
# Build, run, and test the CMSC 125 RV32 operating system.
#
# The file adapts on its own. It builds the user-mode shell and the disk image
# only when the tree carries shell.c.
#
# Override any tool on the command line, for example:
#   make CC=/opt/homebrew/opt/llvm/bin/clang QEMU=qemu-system-riscv32
#
# Targets:
#   make                 build the kernel
#   make run             boot the kernel interactively
#   make disk            build the disk image
#   make test STAGE=N    run every test case of stages 1 to N
#   make test-harness    run the controller self-tests
#   make test-disk       build the pristine test image
#   make clean           remove the build directory
#
# macOS targets. Install the toolchain with Homebrew:
#   brew install llvm lld qemu
# Each target sets CC to Homebrew's clang, because Apple's clang has no 32-bit
# RISC-V target. Each target also puts Homebrew's LLVM bin folder and Homebrew's
# bin folder ahead of PATH, so ld.lld and llvm-objcopy resolve. A target stops
# at once when brew is not on PATH.
#
#   make build-mac           build the kernel
#   make run-mac             boot the kernel interactively
#   make disk-mac            build the disk image
#   make test-mac STAGE=N    run every test case of stages 1 to N
#   make test-harness-mac    run the controller self-tests
#   make test-disk-mac       build the pristine test image

# GNU Make defines CC=cc as a built-in default, so "CC ?= clang" would keep
# cc. Raise clang only over that built-in default. A caller still wins, from
# the command line or the environment:
#   make CC=/opt/homebrew/opt/llvm/bin/clang
ifeq ($(origin CC),default)
CC       := clang
endif
OBJCOPY  ?= llvm-objcopy
QEMU     ?= qemu-system-riscv32

# The Homebrew prefix, the Homebrew LLVM bin folder, and the PATH a -mac target
# exports. These are recursive, so Homebrew runs only when a -mac target reads
# the value. A prefix that is empty is the same as a missing brew.
BREW_PREFIX  = $(shell brew --prefix 2>/dev/null)
MAC_LLVM_BIN = $(BREW_PREFIX)/opt/llvm/bin
MAC_PATH     = $(MAC_LLVM_BIN):$(BREW_PREFIX)/bin:$(PATH)

# Ubuntu 24.04's QEMU ships no riscv32 OpenSBI image, so the firmware is
# vendored and named explicitly.
BIOS     := firmware/opensbi-riscv32-generic-fw_dynamic.bin

CFLAGS   ?= -std=c11 -O2 -g3 -Wall -Wextra \
            --target=riscv32-unknown-elf -fuse-ld=lld \
            -fno-stack-protector -ffreestanding -nostdlib

BUILD    := build
KERNEL_SRCS := kernel.c common.c
# The production kernel has no case. The test rule adds the case and its flags.
# An empty identity means the dependency scan failed. The production kernel
# embeds the identity through KERNEL_DEFS, so the kernel recipe stops before the
# compile when the identity is empty. The check lives in the recipe, not at
# parse time, so it covers a BUILD override and every goal that reaches the
# kernel. The print-* targets, the disk image, and clean stay independent.
BASE_ID      = $(shell sh tests/build-id.sh '' '$(CFLAGS)' '$(CC)' 2>/dev/null)
KERNEL_DEFS = -DKTEST_BUILD_ID=\"$(BASE_ID)\"

HAS_USER := $(if $(wildcard shell.c),1,)

DISK_SRCS := $(wildcard disk/*.txt)
DISK_ARGS := -drive id=drive0,file=$(BUILD)/disk.tar,format=raw,if=none \
             -device virtio-blk-device,drive=drive0,bus=virtio-mmio-bus.0
RUN_DRIVE := $(if $(HAS_USER),$(DISK_ARGS),)
SHELL_OBJ := $(if $(HAS_USER),$(BUILD)/shell.bin.o,)

.PHONY: all run disk test test-harness test-disk print-test-flags print-test-cc clean FORCE \
        require-brew build-mac run-mac disk-mac test-mac test-harness-mac test-disk-mac

all: $(BUILD)/kernel.elf

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/kernel.elf: $(KERNEL_SRCS) kernel.h common.h assert.h kernel.ld $(SHELL_OBJ) | $(BUILD)
	@if [ -z "$(BASE_ID)" ]; then \
	    echo "error: the build identity is empty: the compiler cannot list the dependencies (CC=$(CC))" >&2; \
	    exit 1; \
	fi
	$(CC) $(CFLAGS) -I. $(KERNEL_DEFS) -Wl,-Tkernel.ld \
	    -Wl,-Map=$(BUILD)/kernel.map -o $@ $(KERNEL_SRCS) $(SHELL_OBJ)

# The user-mode shell. The starter has no shell.c, so this chain never runs
# there. objcopy runs inside the build directory so the generated symbols keep
# the symbols the kernel expects.
$(BUILD)/shell.elf: shell.c user.c common.c user.h common.h user.ld | $(BUILD)
	$(CC) $(CFLAGS) -I. -Wl,-Tuser.ld -Wl,-Map=$(BUILD)/shell.map \
	    -o $@ shell.c user.c common.c

$(BUILD)/shell.bin: $(BUILD)/shell.elf
	$(OBJCOPY) --set-section-flags .bss=alloc,contents -O binary $< $@

$(BUILD)/shell.bin.o: $(BUILD)/shell.bin
	cd $(BUILD) && $(OBJCOPY) -I binary -O elf32-littleriscv shell.bin shell.bin.o

# The test kernel links the production kernel objects, the record writer, and one
# test case. The controller sets CASE, SRC, and BOOT.
#
# FORCE rebuilds the executable on every controller call, so a case change can
# never be served from a stale object. The identity covers the compiler
# dependency list, the case path, and the flags, so a stale image also fails the
# identity check.
$(BUILD)/ktest/$(CASE).boot$(BOOT).elf: FORCE
	@mkdir -p $(BUILD)/ktest
	@id=$$(sh tests/build-id.sh '$(SRC)' '$(CFLAGS) -DKTEST -DKTEST_BOOT=$(BOOT)' '$(CC)') || \
	    { echo "error: $(CASE) has no build identity: the dependency scan failed" >&2; exit 1; }; \
	$(CC) $(CFLAGS) -I. -DKTEST -DKTEST_BOOT=$(BOOT) -DKTEST_BUILD_ID=\"$$id\" \
	    -Wl,-Tkernel.ld -Wl,-Map=$(BUILD)/ktest/$(CASE).boot$(BOOT).map -o $@ \
	    $(KERNEL_SRCS) tests/ktest.c $(SRC)

# The disk image. make run does not recreate an existing image.
$(BUILD)/disk.tar: $(DISK_SRCS) | $(BUILD)
	@if [ -z "$(DISK_SRCS)" ]; then echo "error: no disk/*.txt files to pack"; exit 2; fi
	cd disk && tar cf ../$(BUILD)/disk.tar --format=ustar *.txt

disk: $(BUILD)/disk.tar

# The pristine test image. The controller copies it for each case, so a case
# never writes this file. make run never writes it either.
$(BUILD)/test-disk.tar: $(DISK_SRCS) | $(BUILD)
	@if [ -z "$(DISK_SRCS)" ]; then echo "error: no disk/*.txt files to pack"; exit 2; fi
	cd disk && tar cf ../$(BUILD)/test-disk.tar --format=ustar *.txt

test-disk: $(BUILD)/test-disk.tar

# print-test-flags prints the compiler flags and the test defines. The
# controller hashes the same string into the build identity, so the two sides
# always agree.
print-test-flags:
	@printf '%s' '$(CFLAGS) -DKTEST -DKTEST_BOOT=$(BOOT)'

# print-test-cc prints the compiler. The controller reads it and passes the
# same compiler to tests/build-id.sh, so both sides scan the same dependencies.
print-test-cc:
	@printf '%s' '$(CC)'

FORCE:

run: all $(if $(HAS_USER),$(BUILD)/disk.tar)
	$(QEMU) -machine virt -bios $(BIOS) -nographic -serial mon:stdio \
	    --no-reboot $(RUN_DRIVE) -kernel $(BUILD)/kernel.elf

test:
	bash tests/run-tests.sh $(if $(STAGE),--stage $(STAGE),)

test-harness:
	bash tests/selftest/run-self-tests.sh

clean:
	rm -rf $(BUILD)

# The -mac targets. require-brew stops a target at once, with one message, when
# brew is missing. Each target then runs the base target as a sub-make with the
# Homebrew compiler and the Homebrew PATH.
require-brew:
	@if [ -z "$(BREW_PREFIX)" ]; then \
	    echo "error: brew is not on PATH. Install Homebrew, then run 'brew install llvm lld qemu'." >&2; \
	    exit 1; \
	fi

build-mac: require-brew
	@export PATH="$(MAC_PATH)"; \
	    $(MAKE) all CC="$(MAC_LLVM_BIN)/clang"

run-mac: require-brew
	@export PATH="$(MAC_PATH)"; \
	    $(MAKE) run CC="$(MAC_LLVM_BIN)/clang"

disk-mac: require-brew
	@export PATH="$(MAC_PATH)"; \
	    $(MAKE) disk CC="$(MAC_LLVM_BIN)/clang"

test-mac: require-brew
	@export PATH="$(MAC_PATH)"; \
	    $(MAKE) test STAGE="$(STAGE)" CC="$(MAC_LLVM_BIN)/clang"

test-harness-mac: require-brew
	@export PATH="$(MAC_PATH)"; \
	    $(MAKE) test-harness CC="$(MAC_LLVM_BIN)/clang"

test-disk-mac: require-brew
	@export PATH="$(MAC_PATH)"; \
	    $(MAKE) test-disk CC="$(MAC_LLVM_BIN)/clang"
