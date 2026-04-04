# Makefile for secboot.bin decompilation build
#
# Usage:
#   make                    - Build secboot.elf and secboot.bin
#   make compare            - Compare with original binary
#   make disasm             - Generate disassembly of built binary
#   make clean              - Remove build artifacts
#   make report             - Run full comparison report
#
# Toolchain:
#   Set CROSS_COMPILE to point to C-SKY toolchain prefix
#   Default: looks in /tmp/csky-tools/gcc/bin/

# Toolchain prefix
CROSS_COMPILE ?= csky-elfabiv2-

# Try to find toolchain in common locations
ifneq ($(wildcard /tmp/csky-tools/gcc/bin/$(CROSS_COMPILE)gcc),)
    TOOLCHAIN_PATH := /tmp/csky-tools/gcc/bin/
else
    TOOLCHAIN_PATH :=
endif

CC      := $(TOOLCHAIN_PATH)$(CROSS_COMPILE)gcc
AS      := $(TOOLCHAIN_PATH)$(CROSS_COMPILE)gcc
LD      := $(TOOLCHAIN_PATH)$(CROSS_COMPILE)gcc
OBJCOPY := $(TOOLCHAIN_PATH)$(CROSS_COMPILE)objcopy
OBJDUMP := $(TOOLCHAIN_PATH)$(CROSS_COMPILE)objdump
SIZE    := $(TOOLCHAIN_PATH)$(CROSS_COMPILE)size

# Directories
SRCDIR  := .
BUILDDIR := build
REPO_ROOT := ..

# Original binary for comparison
ORIG_BIN := $(REPO_ROOT)/tools/xt804/xt804_secboot.bin

# Source files
C_SRCS := \
    secboot_hw_init.c \
    secboot_uart.c \
    secboot_stdlib.c \
    secboot_memory.c \
    secboot_flash.c \
    secboot_crypto.c \
    secboot_image.c \
    secboot_fwup.c \
    secboot_boot.c \
    secboot_main.c

ASM_SRCS := \
    secboot_vectors.S

# Object files
C_OBJS   := $(addprefix $(BUILDDIR)/,$(C_SRCS:.c=.o))
ASM_OBJS := $(addprefix $(BUILDDIR)/,$(ASM_SRCS:.S=.o))
OBJS     := $(ASM_OBJS) $(C_OBJS)

# Output files
TARGET_ELF := $(BUILDDIR)/secboot.elf
TARGET_BIN := $(BUILDDIR)/secboot.bin
TARGET_DIS := $(BUILDDIR)/secboot.dis

# Linker script
LDSCRIPT := secboot.ld

# Find sysroot
SYSROOT := $(wildcard /tmp/csky-tools/gcc/csky-elfabiv2/sys-include)

# Compiler flags (matching original build as closely as possible)
CFLAGS := \
    -mcpu=ck804ef \
    -O2 \
    -mhard-float \
    -ffunction-sections \
    -fdata-sections \
    -fno-builtin \
    -Wall \
    -Wno-unused-variable \
    -Wno-unused-function \
    -I$(SRCDIR)

ifneq ($(SYSROOT),)
    CFLAGS += -isystem $(SYSROOT)
endif

# Assembler flags
ASFLAGS := \
    -mcpu=ck804ef \
    -mhard-float

# Linker flags
LDFLAGS := \
    -mcpu=ck804ef \
    -mhard-float \
    -nostdlib \
    -nostartfiles \
    -T$(LDSCRIPT) \
    -Wl,--gc-sections \
    -Wl,-Map=$(BUILDDIR)/secboot.map

# ============================================================
# Targets
# ============================================================

.PHONY: all clean disasm compare report size install-toolchain

all: $(TARGET_BIN) $(TARGET_DIS) size

# Create build directory
$(BUILDDIR):
	@mkdir -p $(BUILDDIR)

# Compile C sources
$(BUILDDIR)/%.o: $(SRCDIR)/%.c secboot_common.h | $(BUILDDIR)
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

# Assemble ASM sources
$(BUILDDIR)/%.o: $(SRCDIR)/%.S | $(BUILDDIR)
	@echo "  AS    $<"
	@$(AS) $(ASFLAGS) -c -o $@ $<

# Link
$(TARGET_ELF): $(OBJS) $(LDSCRIPT)
	@echo "  LD    $@"
	@$(LD) $(LDFLAGS) -o $@ $(OBJS)

# Generate binary
$(TARGET_BIN): $(TARGET_ELF)
	@echo "  BIN   $@"
	@$(OBJCOPY) -O binary $< $@
	@echo "  Size: $$(wc -c < $@) bytes (original: $$(wc -c < $(ORIG_BIN)) bytes)"

# Generate disassembly
$(TARGET_DIS): $(TARGET_ELF)
	@echo "  DIS   $@"
	@$(OBJDUMP) -d $< > $@

disasm: $(TARGET_DIS)

# Size report
size: $(TARGET_ELF)
	@$(SIZE) $<

# Compare with original binary
compare: $(TARGET_BIN)
	@echo ""
	@echo "=== Binary Comparison ==="
	@echo "Original:    $$(wc -c < $(ORIG_BIN)) bytes"
	@echo "Recompiled:  $$(wc -c < $(TARGET_BIN)) bytes"
	@echo ""
	@if cmp -s $(ORIG_BIN) $(TARGET_BIN); then \
		echo "✅ IDENTICAL - binaries match exactly!"; \
	else \
		echo "❌ DIFFERENT - binaries do not match"; \
		echo ""; \
		echo "Byte differences:"; \
		cmp -l $(ORIG_BIN) $(TARGET_BIN) 2>/dev/null | head -20; \
		DIFF_COUNT=$$(cmp -l $(ORIG_BIN) $(TARGET_BIN) 2>/dev/null | wc -l); \
		echo "Total different bytes: $$DIFF_COUNT"; \
	fi

# Full comparison report using compare_secboot.py
report: all
	@echo ""
	@echo "=== Running comparison report ==="
	@CSKY_GCC=$(CC) CSKY_OBJDUMP=$(OBJDUMP) python3 compare_secboot.py

# Clean build artifacts
clean:
	@rm -rf $(BUILDDIR)
	@echo "Cleaned build directory"

# Install toolchain helper
install-toolchain:
	@if [ ! -f "/tmp/csky-tools/gcc/bin/csky-elfabiv2-gcc" ]; then \
		echo "Downloading C-SKY toolchain..."; \
		cd /tmp && \
		curl -sL -o csky-tools.zip \
			"https://github.com/openLuat/luatos-soc-air101/releases/download/v2001.gcc/csky-elfabiv2-tools-x86_64-minilibc-20230301.zip" && \
		unzip -q -o csky-tools.zip -d /tmp/csky-tools && \
		echo "Toolchain installed to /tmp/csky-tools/gcc/bin/"; \
	else \
		echo "Toolchain already installed"; \
	fi
