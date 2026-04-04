# secboot.bin Decompilation / 反编译 secboot.bin

## Overview / 概述

This directory contains the reverse-engineered analysis of the Air101/Air103 secure bootloader
(`tools/xt804/xt804_secboot.bin`), a 21,468-byte proprietary binary blob for the C-SKY XT804
(CK804EF) architecture.

本目录包含 Air101/Air103 安全引导加载程序的逆向工程分析。

## Binary Properties / 二进制属性

| Property | Value |
|----------|-------|
| File | `tools/xt804/xt804_secboot.bin` |
| Size | 21,468 bytes (0x53DC) |
| Architecture | C-SKY XT804 (CK804EF), 32-bit, little-endian |
| Load Address | 0x08002400 |
| Entry Point | 0x08002506 (Reset_Handler) |
| Image Magic | 0xA0FFFF9F |
| CRC32 | 0xFF46F553 |
| Flash Partition | 64KB at offset 0x000000 |

## Files / 文件

| File | Description |
|------|-------------|
| `README.md` | This document |
| `analyze_secboot.py` | Python analysis tool - parses binary, generates annotated disassembly |
| `secboot_annotated.S` | Annotated disassembly with function labels and comments |
| `secboot_vectors.S` | Reconstructed vector table and startup code |
| `secboot_main.c` | Decompiled main boot logic (pseudo-C) |
| `secboot_hw_init.c` | Decompiled hardware initialization (pseudo-C) |
| `secboot_uart.c` | Decompiled UART driver and boot detection (pseudo-C) |
| `secboot_flash.c` | Decompiled flash operations (pseudo-C) |
| `secboot_image.c` | Decompiled image validation (pseudo-C) |
| `secboot_crypto.c` | Decompiled CRC/crypto operations (pseudo-C) |
| `secboot_memory.c` | Decompiled memory allocator (pseudo-C) |
| `secboot_stdlib.c` | Decompiled standard library functions (pseudo-C) |
| `secboot_fwup.c` | Decompiled firmware update, OTA, and xmodem (pseudo-C) |
| `secboot_boot.c` | Decompiled boot parameter and app boot sequence (pseudo-C) |
| `compare_secboot.py` | Compilation & comparison tool - compiles C, compares with original asm |
| `comparison_report.md` | Auto-generated comparison report (original vs recompiled assembly) |

## Memory Map / 内存映射

```
Flash:
  0x08000000 - 0x080023FF  : IMAGE_HEADER area (unused by secboot code)
  0x08002400 - 0x080077DB  : secboot.bin code + data (21,468 bytes)
    0x08002400 - 0x080024FF  : Vector table (64 entries × 4 bytes)
    0x08002500 - 0x080025BF  : Reset_Handler + Default_Handler (trap)
    0x080025C0 - 0x080058B7  : Library functions (memcpy, malloc, flash, crypto)
    0x080058B8 - 0x08005A5B  : UART driver + image validation
    0x08005A5C - 0x080070C3  : Additional subsystems (flash ops, crypto, firmware update)
    0x080070C4 - 0x080074FF  : Main boot logic + high-level functions
    0x08007500 - 0x080077DB  : Constant data + literal pools

SRAM:
  0x20000000 - 0x200000FF  : Vector table copy (irq_vectors)
  0x20000100 - 0x200101D7  : Data section (.data)
  0x200101D8 - 0x2001142F  : BSS section (.bss)
  0x20011430 - 0x20027FFF  : Heap (managed by malloc at 0x080029A0)
  0x200111D8                : Initial stack pointer (sp = r14)
  0x200113D8                : Trap handler save area
  0x200113E4                : tspend_handler counter
  0x200113EC                : Boot mode flag

Peripherals:
  0x40000000                : Device base (APB bus)
  0x40010600                : UART0 registers
  0x40011400                : Clock/PMU registers
  0xE000E000                : VIC base (interrupt controller)
  0xE000E100                : VIC interrupt enable registers
```

## Boot Flow / 启动流程

```
Power-On / Reset
      │
      ▼
Reset_Handler (0x08002506)
      │ - Set PSR (privilege mode, EE enabled)
      │ - Set stack pointer (sp = 0x200111D8)
      │ - Copy .data from flash to SRAM
      │ - Clear .bss section
      │
      ├──► SystemInit (0x080071B4)
      │      - Set VBR = 0x08002400 (vector base)
      │      - Enable cache (cr31 bit 4)
      │      - Enable interrupt controller
      │      - Enable EE + IE in PSR
      │
      ├──► board_init (0x0800712C)
      │      - Configure clock/PMU registers
      │      - Call uart_init(115200) via 0x080058B8
      │
      └──► main (0x08007304)
             │ - Initialize watchdog (0x40000E00 + 0x14)
             │ - Read flash_param from 0x2001007C
             │ - Call flash_init (0x08005338)
             │ - Read FOTA header from flash
             │
             ├──► boot_uart_check (0x08007220)
             │      Poll UART0 for ~196606 cycles
             │      If 3+ non-ESC bytes received → bootloader mode
             │      Sets flag at 0x200113EC
             │
             ├─[UART detected]──► UART Download Mode
             │      │ - Print boot message via uart_putchar
             │      │ - Receive firmware via xmodem-like protocol
             │      │ - Validate received image
             │      │ - Write to flash
             │      │ - Jump to result handler at [0x200101D4]
             │
             └─[No UART]──► Normal Boot
                    │ - flash_read IMAGE_HEADER from 0x080D0000
                    │ - find_valid_image → validate_image
                    │ - Check FOTA update header
                    │ - Verify image CRC via image_header_verify
                    │ - If signature bit set: verify signature
                    │ - Prepare boot params structure
                    │ - Jump to app via function pointer at [0x200101D0]
                    │
                    ├─[Success, return 0]──► App runs
                    └─[Fail]──► Error code → jump to [0x200101D4]

Return codes (r5):
  'C' (67) = OK/Success
  'N' (78) = Flash init failed
  'Y' (89) = App boot handler returned error
  'M' (77) = Image CRC mismatch
  'J' (74) = Invalid image address
  'L' (76) = Bad magic / validation failed
  'I' (73) = Invalid image length
  'K' (75) = Image address not aligned
  'Z' (90) = CRC read error
```

## Toolchain / 工具链

To reproduce the disassembly:

```bash
# Download C-SKY GCC toolchain (zip format, from project releases)
wget https://github.com/openLuat/luatos-soc-air101/releases/download/v2001.gcc/csky-elfabiv2-tools-x86_64-minilibc-20230301.zip
unzip csky-elfabiv2-tools-x86_64-minilibc-20230301.zip -d csky-tools
export PATH=$PWD/csky-tools/gcc/bin:$PATH

# Disassemble
csky-elfabiv2-objdump -D -b binary -m csky --adjust-vma=0x08002400 tools/xt804/xt804_secboot.bin

# Or use the analysis script
python3 decompile/analyze_secboot.py
```

## Compilation & Comparison / 编译与对比

The decompiled C files can be compiled with the C-SKY toolchain and compared
against the original binary assembly:

```bash
# Run the comparison tool (requires csky-elfabiv2-gcc in PATH)
python3 decompile/compare_secboot.py

# This produces:
#   - Compiles each .c file with csky-elfabiv2-gcc -mcpu=ck804ef -O2
#   - Extracts per-function assembly from compiled .o files
#   - Compares instruction sequences with the original binary
#   - Outputs comparison_report.md with detailed results
```

### Compilation Status / 编译状态

| File | Compiles | Notes |
|------|----------|-------|
| secboot_boot.c | ✅ | All 5 boot functions compile |
| secboot_crypto.c | ✅ | All crypto functions compile |
| secboot_flash.c | ✅ | All 26 flash functions compile |
| secboot_fwup.c | ✅ | All firmware update functions compile |
| secboot_hw_init.c | ✅ | Hardware init functions compile |
| secboot_image.c | ✅ | Image validation functions compile |
| secboot_main.c | ✅ | Main boot logic compiles |
| secboot_memory.c | ✅ | Memory allocator compiles |
| secboot_stdlib.c | ✅ | Standard library functions compile |
| secboot_uart.c | ✅ | UART driver functions compile |
| secboot_vectors.S | ✅ | Startup code and vector table, C-SKY assembly |

### Comparison Highlights / 对比要点

Several functions show near-perfect instruction match when compiled:
- `uart_putchar` — 17/19 instructions identical (branches differ only in address encoding)
- `uart_rx_ready` — 6/8 instructions identical
- `flash_init` — 10/12 instructions near-identical
- `flash_read` — 87% mnemonic similarity
- `calloc` — 64% similarity with matched instruction count

## Function Map / 函数映射 (116 functions identified, 113 decompiled)

See `secboot_annotated.S` for complete annotated disassembly.

Remaining 3 undecompiled functions are `image_decrypt_init`, `image_decrypt_block`,
`image_decrypt_process` — all crypto-dependent and deferred.

### Key Functions / 核心函数

| Address | Name | Description |
|---------|------|-------------|
| 0x08002506 | Reset_Handler | CPU startup, .data/.bss init, calls SystemInit→board_init→main |
| 0x08002570 | Default_Handler | Trap/exception handler, saves all regs, calls trap_c |
| 0x080025C0 | memcpy_words | Word-aligned memory copy |
| 0x080028F4 | puts | Print string via uart_putchar |
| 0x0800290C | printf_simple | Simple formatted output |
| 0x080029A0 | malloc | Heap allocator (first-fit, heap at 0x20011430-0x20028000) |
| 0x08002A3C | free | Heap deallocator |
| 0x08002B54 | memcpy | Standard memcpy |
| 0x08002BD4 | memcmp | Optimized memory compare (word-aligned fast path) |
| 0x08002D20 | memset | Standard memset |
| 0x08005338 | flash_init | Flash controller init (SPI flash at 0xC0002000) |
| 0x080051E8 | flash_read_page | Read one flash page (256 bytes) |
| 0x080054F8 | flash_read | Multi-page flash read |
| 0x0800553C | flash_write | Flash write with erase |
| 0x0800588C | uart_rx_ready | Check UART0 RX FIFO non-empty |
| 0x080058A0 | uart_getchar | Blocking UART0 read one byte |
| 0x080058B8 | uart_init | UART0 baud rate config (default 115200) |
| 0x080058EC | image_copy_update | Read image data from flash and verify CRC32 checksum |
| 0x08005988 | validate_image | Validate IMAGE_HEADER (magic, addr, len, CRC) |
| 0x08005CFC | crc_verify_image | CRC32 verification of image data |
| 0x080070C4 | signature_verify | Optional 128-byte signature verification |
| 0x0800712C | board_init | Clock + UART init |
| 0x08007148 | tspend_handler | Timer/software pending interrupt handler (ISR #57) |
| 0x0800717C | uart_putchar | UART0 TX one byte (handles \\n → \\r\\n) |
| 0x080071AC | lock_acquire | Interrupt lock (returns 0) |
| 0x080071B0 | lock_release | Interrupt unlock (returns 0) |
| 0x080071B4 | SystemInit | VBR, cache, VIC, interrupt setup |
| 0x080071F0 | trap_c | Trap handler (infinite loop) |
| 0x080071F4 | image_header_verify | CRC verify + optional data integrity check |
| 0x08007220 | boot_uart_check | Detect UART activity for bootloader mode |
| 0x08007278 | find_valid_image | Scan flash for valid app image |
| 0x08007304 | main | Main boot logic |

## Cross-References / 交叉参考

The following SDK source files were used to identify functions and register definitions:

- `platform/arch/xt804/bsp/startup.S` — Vector table layout, Reset_Handler pattern
- `include/wm_regs.h` — All peripheral register addresses
- `include/driver/wm_flash_map.h` — Flash addresses, SIGNATURE_WORD
- `include/platform/wm_fwup.h` — IMAGE_HEADER_PARAM_ST structure
- `tools/xt804/wm_tool.c` — CRC32 table, image header format
- `platform/common/fwup/wm_fwup.c` — Firmware update patterns

## Legal Notice / 法律声明

This decompilation is provided for educational and security audit purposes only.
The original `secboot.bin` is a proprietary binary from Winner Microelectronics (WinnerMicro).
The repository is MIT-licensed, but the binary blob's license may differ.
