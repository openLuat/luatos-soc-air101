#!/usr/bin/env python3
"""
compare_secboot.py - Compare decompiled C with original secboot.bin assembly

This tool:
1. Disassembles the original secboot.bin using the C-SKY toolchain
2. Compiles individual decompiled C functions with csky-elfabiv2-gcc
3. Compares the generated assembly with the original
4. Outputs a detailed comparison report

Requirements:
  - csky-elfabiv2-gcc in PATH (or set CSKY_GCC env var)
  - csky-elfabiv2-objdump in PATH (or set CSKY_OBJDUMP env var)

Usage:
  python3 compare_secboot.py [--report report.md]
"""

import os
import re
import sys
import struct
import subprocess
import tempfile
import difflib
from pathlib import Path

# ============================================================
# Configuration
# ============================================================

SCRIPT_DIR = Path(__file__).parent
REPO_ROOT = SCRIPT_DIR.parent
SECBOOT_BIN = REPO_ROOT / "tools" / "xt804" / "xt804_secboot.bin"
LOAD_ADDR = 0x08002400

# Toolchain
CSKY_GCC = os.environ.get("CSKY_GCC", "csky-elfabiv2-gcc")
CSKY_OBJDUMP = os.environ.get("CSKY_OBJDUMP", "csky-elfabiv2-objdump")
CSKY_OBJCOPY = os.environ.get("CSKY_OBJCOPY", "csky-elfabiv2-objcopy")

# Find toolchain sysroot
CSKY_SYSROOT = None
for candidate in ["/tmp/csky-tools/csky-elfabiv2/sys-include",
                   "/tmp/csky-tools/csky-elfabiv2/include"]:
    if os.path.isdir(candidate):
        CSKY_SYSROOT = candidate
        break

# Compiler flags matching the original build
CFLAGS = [
    "-mcpu=ck804ef",
    "-O2",
    "-mhard-float",
    "-c",
    "-ffunction-sections",
    "-fdata-sections",
    "-Wall",
    "-Wno-unused-variable",
    "-Wno-unused-function",
    "-fno-builtin",
]
if CSKY_SYSROOT:
    CFLAGS += ["-isystem", CSKY_SYSROOT]

# Function database: address -> (name, size_hint, source_file)
# These are the 116 functions from analyze_secboot.py
FUNCTIONS = {
    0x08002506: ("Reset_Handler", 0x6A, "secboot_vectors.S"),
    0x08002570: ("Default_Handler", 0x50, "secboot_vectors.S"),
    0x080025C0: ("memcpy_words", 0x334, "secboot_stdlib.c"),
    0x080028F4: ("puts", 0x18, "secboot_stdlib.c"),
    0x0800290C: ("printf_simple", 0x4C, "secboot_stdlib.c"),
    0x08002958: ("realloc", 0x28, "secboot_memory.c"),
    0x08002980: ("calloc", 0x20, "secboot_stdlib.c"),
    0x080029A0: ("malloc", 0x9C, "secboot_memory.c"),
    0x08002A3C: ("free", 0x78, "secboot_memory.c"),
    0x08002AB4: ("memcmp_or_recv", 0xA0, "secboot_stdlib.c"),
    0x08002B54: ("memcpy", 0x180, "secboot_stdlib.c"),
    0x08002BD4: ("memmove", 0x14C, "secboot_stdlib.c"),
    0x08002D20: ("memset", 0x5C, "secboot_stdlib.c"),
    0x08002D7C: ("strlen", 0x130, "secboot_stdlib.c"),
    0x08002EAC: ("strcmp_or_strncpy", 0x11C, "secboot_stdlib.c"),
    0x08002FC8: ("format_number", 0x188, "secboot_stdlib.c"),
    0x08003150: ("vsnprintf_core", 0x28, "secboot_stdlib.c"),
    0x08003178: ("snprintf", 0x30, "secboot_stdlib.c"),
    0x080031A8: ("sprintf", 0x34, "secboot_stdlib.c"),
    0x080031DC: ("printf", 0x5C, "secboot_stdlib.c"),
    0x08003238: ("flash_spi_cmd", 0x60, "secboot_flash.c"),
    0x08003298: ("flash_wait_ready", 0x34, "secboot_flash.c"),
    0x080032CC: ("flash_write_enable", 0x60, "secboot_flash.c"),
    0x0800332C: ("flash_erase_sector", 0x64, "secboot_flash.c"),
    0x08003390: ("flash_program_page", 0x78, "secboot_flash.c"),
    0x08003408: ("flash_read_status", 0x1C, "secboot_flash.c"),
    0x08003424: ("flash_read_id", 0x7C, "secboot_flash.c"),
    0x080034A0: ("flash_quad_config", 0x44, "secboot_flash.c"),
    0x080034E4: ("flash_unlock", 0x30, "secboot_flash.c"),
    0x08003514: ("flash_lock", 0x74, "secboot_flash.c"),
    0x08003588: ("flash_power_ctrl", 0x108, "secboot_flash.c"),
    0x08003690: ("flash_controller_init", 0xB4, "secboot_flash.c"),
    0x08003744: ("flash_detect", 0x30, "secboot_flash.c"),
    0x08003774: ("flash_setup", 0xEC, "secboot_flash.c"),
    0x08003860: ("flash_capacity_check", 0x160, "secboot_flash.c"),
    0x080039C0: ("rsa_core", 0x54, "secboot_crypto.c"),
    0x08003A14: ("rsa_step", 0x78, "secboot_crypto.c"),
    0x08003A8C: ("rsa_modexp", 0x5C, "secboot_crypto.c"),
    0x08003AE8: ("rsa_init", 0x54, "secboot_crypto.c"),
    0x08003B3C: ("rsa_process", 0x168, "secboot_crypto.c"),
    0x08003CA4: ("sha_hash_block", 0x38, "secboot_crypto.c"),
    0x08003CDC: ("sha_init", 0x14, "secboot_crypto.c"),
    0x08003CF0: ("sha_update", 0x12C, "secboot_crypto.c"),
    0x08003E1C: ("sha_final", 0x424, "secboot_crypto.c"),
    0x08004258: ("crc32_table_init", 0x84, "secboot_crypto.c"),
    0x080042DC: ("crc32_update", 0x48, "secboot_crypto.c"),
    0x08004324: ("crc32_finalize", 0x58, "secboot_crypto.c"),
    0x0800437C: ("hash_ctx_init", 0x88, "secboot_crypto.c"),
    0x08004404: ("hash_set_mode", 0x0C, "secboot_crypto.c"),
    0x08004410: ("hash_update_data", 0xA8, "secboot_crypto.c"),
    0x080044B8: ("hash_compress", 0xEC, "secboot_crypto.c"),
    0x08004544: ("hash_finalize", 0x58, "secboot_crypto.c"),
    0x0800459C: ("hash_get_result", 0x08, "secboot_crypto.c"),
    0x080045A4: ("hash_get_result2", 0x34, "secboot_crypto.c"),
    0x080045D8: ("hmac_init", 0x64, "secboot_crypto.c"),
    0x0800463C: ("hmac_update", 0xF0, "secboot_crypto.c"),
    0x0800472C: ("hmac_final", 0x1DA, "secboot_crypto.c"),
    0x08004906: ("crypto_subsys_init", 0xC6, "secboot_crypto.c"),
    0x080049CC: ("pkey_setup", 0x80, "secboot_crypto.c"),
    0x08004A4C: ("pkey_verify_step", 0x68, "secboot_crypto.c"),
    0x08004AB4: ("pkey_verify", 0x44, "secboot_crypto.c"),
    0x08004AF8: ("signature_check_init", 0xB8, "secboot_crypto.c"),
    0x08004BB0: ("signature_check_data", 0x3C, "secboot_crypto.c"),
    0x08004BEC: ("signature_check_final", 0x8C, "secboot_crypto.c"),
    0x08004C78: ("cert_parse", 0x84, "secboot_crypto.c"),
    0x08004CFC: ("cert_validate", 0x30, "secboot_crypto.c"),
    0x08004D2C: ("cert_get_pubkey", 0x24, "secboot_crypto.c"),
    0x08004D50: ("cert_chain_verify", 0x48, "secboot_crypto.c"),
    0x08004D98: ("image_decrypt_init", 0x100, "secboot_image.c"),
    0x08004E98: ("image_decrypt_block", 0x88, "secboot_image.c"),
    0x08004F20: ("image_decrypt_process", 0xE4, "secboot_image.c"),
    0x08005004: ("firmware_update_init", 0x1E4, "secboot_fwup.c"),
    0x0800509C: ("firmware_update_process", 0x14C, "secboot_fwup.c"),
    0x080051E8: ("flash_read_page", 0x150, "secboot_flash.c"),
    0x08005338: ("flash_init", 0x20, "secboot_flash.c"),
    0x08005358: ("flash_read_raw", 0x1A0, "secboot_flash.c"),
    0x080054F8: ("flash_read", 0x44, "secboot_flash.c"),
    0x0800553C: ("flash_write", 0x134, "secboot_flash.c"),
    0x08005670: ("flash_erase_range", 0x40, "secboot_flash.c"),
    0x080056B0: ("flash_protect_config", 0x13C, "secboot_flash.c"),
    0x080057EC: ("flash_param_read", 0x18, "secboot_flash.c"),
    0x08005804: ("flash_param_write", 0x1C, "secboot_flash.c"),
    0x08005820: ("flash_param_init", 0x0C, "secboot_flash.c"),
    0x0800582C: ("flash_copy_data", 0x60, "secboot_flash.c"),
    0x0800588C: ("uart_rx_ready", 0x14, "secboot_uart.c"),
    0x080058A0: ("uart_getchar", 0x18, "secboot_uart.c"),
    0x080058B8: ("uart_init", 0x34, "secboot_uart.c"),
    0x080058EC: ("uart_verify_data", 0x9C, "secboot_uart.c"),
    0x08005988: ("validate_image", 0xD4, "secboot_image.c"),
    0x08005A5C: ("xmodem_recv_init", 0x78, "secboot_fwup.c"),
    0x08005AD4: ("xmodem_recv_block", 0xB4, "secboot_fwup.c"),
    0x08005B88: ("xmodem_recv_data", 0x40, "secboot_fwup.c"),
    0x08005BC8: ("xmodem_process", 0x134, "secboot_fwup.c"),
    0x08005CFC: ("crc_verify_image", 0xF4, "secboot_image.c"),
    0x08005DF0: ("firmware_apply", 0x84, "secboot_fwup.c"),
    0x08005E74: ("firmware_apply_ext", 0x460, "secboot_fwup.c"),
    0x080062D4: ("ota_process", 0x1C0, "secboot_fwup.c"),
    0x08006494: ("ota_validate", 0x9C, "secboot_fwup.c"),
    0x08006530: ("ota_apply", 0x350, "secboot_fwup.c"),
    0x08006880: ("boot_param_setup", 0x100, "secboot_boot.c"),
    0x08006980: ("boot_param_read", 0x46C, "secboot_boot.c"),
    0x08006DEC: ("boot_prepare", 0x1A8, "secboot_boot.c"),
    0x08006F94: ("boot_execute_prep", 0xC4, "secboot_boot.c"),
    0x08007058: ("app_boot_sequence", 0x6C, "secboot_boot.c"),
    0x080070C4: ("signature_verify", 0x68, "secboot_image.c"),
    0x0800712C: ("board_init", 0x1C, "secboot_hw_init.c"),
    0x08007148: ("tspend_handler", 0x34, "secboot_main.c"),
    0x0800717C: ("uart_putchar", 0x30, "secboot_uart.c"),
    0x080071AC: ("lock_acquire", 0x04, "secboot_hw_init.c"),
    0x080071B0: ("lock_release", 0x04, "secboot_hw_init.c"),
    0x080071B4: ("SystemInit", 0x3C, "secboot_hw_init.c"),
    0x080071F0: ("trap_c", 0x04, "secboot_hw_init.c"),
    0x080071F4: ("image_header_verify", 0x2C, "secboot_main.c"),
    0x08007220: ("boot_uart_check", 0x58, "secboot_main.c"),
    0x08007278: ("find_valid_image", 0x8C, "secboot_image.c"),
    0x08007304: ("main", 0x1FC, "secboot_main.c"),
}


def read_binary(path):
    """Read the secboot binary."""
    with open(path, "rb") as f:
        return f.read()


def disassemble_original(bin_path):
    """Disassemble original binary using objdump."""
    try:
        result = subprocess.run(
            [CSKY_OBJDUMP, "-D", "-b", "binary", "-m", "csky",
             f"--adjust-vma=0x{LOAD_ADDR:08X}", str(bin_path)],
            capture_output=True, text=True, timeout=30
        )
        return result.stdout
    except (FileNotFoundError, subprocess.TimeoutExpired):
        return None


def parse_disassembly(disasm_text):
    """Parse objdump output into address -> (hex_bytes, mnemonic, operands) dict."""
    instructions = {}
    if not disasm_text:
        return instructions

    for line in disasm_text.split("\n"):
        # Match: " 8002506:\t1410      \tlrw      \tr0, 0xe0000200\t// ..."
        m = re.match(r"\s*([0-9a-f]+):\s+([0-9a-f][0-9a-f ]+?)\s+(\S+)\s*(.*)", line)
        if m:
            addr = int(m.group(1), 16)
            hex_bytes = m.group(2).strip()
            mnemonic = m.group(3).strip()
            operands = m.group(4).split("//")[0].strip()  # Remove comments
            instructions[addr] = (hex_bytes, mnemonic, operands)

    return instructions


def extract_function_asm(all_instrs, func_addr, func_size):
    """Extract instructions belonging to a function."""
    func_instrs = []
    end_addr = func_addr + func_size
    for addr in sorted(all_instrs.keys()):
        if func_addr <= addr < end_addr:
            hex_b, mnem, ops = all_instrs[addr]
            func_instrs.append((addr, hex_b, mnem, ops))
    return func_instrs


def format_func_asm(instrs, base_addr=None):
    """Format function assembly for comparison."""
    lines = []
    for addr, hex_b, mnem, ops in instrs:
        offset = addr - base_addr if base_addr else addr
        lines.append(f"  +{offset:04x}: {mnem:12s} {ops}")
    return lines


def try_compile_function(func_name, source_file, tmpdir):
    """Try to compile a single decompiled C file and extract the function."""
    src_path = SCRIPT_DIR / source_file
    if not src_path.exists():
        return None, f"Source file {source_file} not found"

    if source_file.endswith(".S"):
        return None, "Assembly file (not compiled)"

    obj_path = os.path.join(tmpdir, f"{func_name}.o")

    try:
        # Try compilation
        result = subprocess.run(
            [CSKY_GCC] + CFLAGS + ["-o", obj_path, str(src_path)],
            capture_output=True, text=True, timeout=30
        )

        if result.returncode != 0:
            # Extract first few errors
            errors = result.stderr.strip().split("\n")[:5]
            return None, "Compile error: " + "; ".join(errors)

        # Disassemble the object
        result = subprocess.run(
            [CSKY_OBJDUMP, "-d", obj_path],
            capture_output=True, text=True, timeout=30
        )

        return result.stdout, None

    except (FileNotFoundError, subprocess.TimeoutExpired) as e:
        return None, f"Tool error: {e}"


def compute_function_sizes(binary_data):
    """Compute function sizes from address gaps."""
    sorted_addrs = sorted(FUNCTIONS.keys())
    sizes = {}
    binary_end = LOAD_ADDR + len(binary_data)

    for i, addr in enumerate(sorted_addrs):
        if i + 1 < len(sorted_addrs):
            next_addr = sorted_addrs[i + 1]
        else:
            next_addr = binary_end
        sizes[addr] = next_addr - addr

    return sizes


def analyze_instruction_patterns(orig_instrs):
    """Analyze instruction patterns in original function."""
    stats = {
        "total": len(orig_instrs),
        "branches": 0,
        "loads": 0,
        "stores": 0,
        "arith": 0,
        "other": 0,
        "push_pop": 0,
    }

    for _, _, mnem, _ in orig_instrs:
        ml = mnem.lower()
        if ml in ("bt", "bf", "br", "bsr", "jmp", "jsr", "bnez", "bez",
                   "bnezad", "bsr", "rts", "rte", "jmpix"):
            stats["branches"] += 1
        elif ml.startswith("ld") or ml == "lrw" or ml.startswith("ld."):
            stats["loads"] += 1
        elif ml.startswith("st") and not ml.startswith("stu"):
            stats["stores"] += 1
        elif ml in ("push", "pop", "ipush", "ipop"):
            stats["push_pop"] += 1
        elif ml in ("addi", "addu", "subu", "subi", "movi", "movih",
                     "and", "or", "xor", "not", "lsli", "lsri", "asri",
                     "mult", "divs", "divu", "cmpne", "cmphs", "cmplt",
                     "cmplti", "cmpnei", "cmphsi", "zextb", "zexth",
                     "sextb", "sexth", "bseti", "bclri", "btsti",
                     "incf", "inct", "decf", "dect", "andi", "ori",
                     "andn", "bmaski", "mov", "mvc", "mvcv"):
            stats["arith"] += 1
        else:
            stats["other"] += 1

    return stats


def check_decompiled_coverage(source_file, func_name):
    """Check if a function is actually decompiled in the source file."""
    src_path = SCRIPT_DIR / source_file
    if not src_path.exists():
        return False

    content = src_path.read_text()

    # Look for the function name in the file
    # Check for function definition or address reference
    if func_name in content:
        return True

    return False


def generate_report(binary_data, all_instrs, func_sizes, compile_results,
                    compiled_funcs=None):
    """Generate the comparison report as markdown."""
    if compiled_funcs is None:
        compiled_funcs = {}
    lines = []
    lines.append("# secboot.bin Decompilation Comparison Report")
    lines.append("# secboot.bin 反编译对比报告")
    lines.append("")
    lines.append(f"Generated from `{SECBOOT_BIN.name}` ({len(binary_data)} bytes)")
    lines.append("")

    # Summary statistics
    total_funcs = len(FUNCTIONS)
    decompiled = 0
    compilable = 0
    not_decompiled = 0

    by_file = {}
    for addr, (name, _, src) in sorted(FUNCTIONS.items()):
        if src not in by_file:
            by_file[src] = {"total": 0, "decompiled": 0, "compilable": 0}
        by_file[src]["total"] += 1

        has_code = check_decompiled_coverage(src, name)
        if has_code:
            decompiled += 1
            by_file[src]["decompiled"] += 1

    lines.append("## Summary / 概要")
    lines.append("")
    lines.append(f"| Metric | Value |")
    lines.append(f"|--------|-------|")
    lines.append(f"| Total functions identified / 已识别函数总数 | {total_funcs} |")
    lines.append(f"| Functions with decompiled C / 已反编译函数数 | {decompiled} |")
    lines.append(f"| Decompilation coverage / 反编译覆盖率 | {decompiled*100//total_funcs}% |")
    lines.append(f"| Total binary code size / 二进制代码总大小 | {len(binary_data)} bytes |")
    lines.append("")

    # Per-file breakdown
    lines.append("## Per-File Coverage / 各文件覆盖率")
    lines.append("")
    lines.append("| Source File | Functions | Decompiled | Coverage |")
    lines.append("|-------------|-----------|------------|----------|")
    for src_file in sorted(by_file.keys()):
        info = by_file[src_file]
        cov = info["decompiled"] * 100 // max(info["total"], 1)
        lines.append(f"| `{src_file}` | {info['total']} | {info['decompiled']} | {cov}% |")
    lines.append("")

    # Compilation attempt results
    lines.append("## Compilation Results / 编译结果")
    lines.append("")
    lines.append("Each decompiled `.c` file was compiled with `csky-elfabiv2-gcc -mcpu=ck804ef -O2`")
    lines.append("to check if the pseudo-C produces valid C-SKY assembly.")
    lines.append("")

    for src_file, (success, error_msg) in sorted(compile_results.items()):
        status = "✅ Compiles" if success else "❌ Errors"
        lines.append(f"### `{src_file}` — {status}")
        lines.append("")
        if not success and error_msg:
            # Limit error message length
            err_lines = error_msg.strip().split("\n")[:10]
            lines.append("```")
            for el in err_lines:
                lines.append(el[:200])
            if len(error_msg.strip().split("\n")) > 10:
                lines.append(f"... ({len(error_msg.strip().split(chr(10)))} total error lines)")
            lines.append("```")
            lines.append("")

    # Function-by-function analysis
    lines.append("## Function Analysis / 函数分析")
    lines.append("")
    lines.append("Detailed analysis of each identified function in the original binary,")
    lines.append("with instruction statistics and decompilation status.")
    lines.append("")

    lines.append("| Address | Function | Size | Instructions | Branches | Loads/Stores | Source File | Status |")
    lines.append("|---------|----------|------|-------------|----------|-------------|-------------|--------|")

    for addr in sorted(FUNCTIONS.keys()):
        name, _, src = FUNCTIONS[addr]
        size = func_sizes.get(addr, 0)
        orig = extract_function_asm(all_instrs, addr, size)
        stats = analyze_instruction_patterns(orig)
        has_code = check_decompiled_coverage(src, name)
        status = "✅ Decompiled" if has_code else "⬜ Identified"

        lines.append(
            f"| `0x{addr:08X}` | `{name}` | {size} | {stats['total']} | "
            f"{stats['branches']} | {stats['loads']}/{stats['stores']} | "
            f"`{src}` | {status} |"
        )

    lines.append("")

    # ============================================================
    # Compiled vs Original Comparison
    # ============================================================
    lines.append("## Compiled vs Original Comparison / 编译对比")
    lines.append("")
    lines.append("For each compiled function, instruction count and pattern comparison")
    lines.append("with the original binary. A higher match indicates better decompilation.")
    lines.append("")

    if compiled_funcs:
        lines.append("| Function | Original Insns | Compiled Insns | Size Ratio | Mnemonic Match |")
        lines.append("|----------|---------------|----------------|------------|----------------|")

        match_total = 0
        match_count = 0

        for addr in sorted(FUNCTIONS.keys()):
            name, _, src = FUNCTIONS[addr]
            if name not in compiled_funcs:
                continue

            size = func_sizes.get(addr, 0)
            orig = extract_function_asm(all_instrs, addr, size)
            comp = compiled_funcs[name]

            orig_count = len(orig)
            comp_count = len(comp)

            if orig_count == 0:
                continue

            ratio = comp_count / max(orig_count, 1)

            # Compare mnemonic sequences
            orig_mnems = [m for _, _, m, _ in orig]
            comp_mnems = [m for _, _, m, _ in comp]

            # Use SequenceMatcher for mnemonic pattern similarity
            matcher = difflib.SequenceMatcher(None, orig_mnems, comp_mnems)
            similarity = matcher.ratio() * 100

            match_total += similarity
            match_count += 1

            ratio_str = f"{ratio:.2f}x"
            sim_str = f"{similarity:.0f}%"

            # Color coding via emoji
            if similarity >= 80:
                indicator = "🟢"
            elif similarity >= 50:
                indicator = "🟡"
            else:
                indicator = "🔴"

            lines.append(
                f"| `{name}` | {orig_count} | {comp_count} | {ratio_str} | {indicator} {sim_str} |"
            )

        if match_count > 0:
            avg_match = match_total / match_count
            lines.append("")
            lines.append(f"**Average mnemonic similarity: {avg_match:.1f}%** (across {match_count} compared functions)")
        lines.append("")

        # Show detailed side-by-side for a few key functions
        lines.append("### Side-by-Side Examples / 逐行对比示例")
        lines.append("")

        sample_funcs = ["uart_putchar", "uart_rx_ready", "flash_init",
                        "SystemInit", "board_init", "lock_acquire",
                        "lock_release", "tspend_handler"]

        for func_name in sample_funcs:
            if func_name not in compiled_funcs:
                continue

            # Find the address
            func_addr = None
            for a, (n, _, _) in FUNCTIONS.items():
                if n == func_name:
                    func_addr = a
                    break
            if func_addr is None:
                continue

            size = func_sizes.get(func_addr, 0)
            orig = extract_function_asm(all_instrs, func_addr, size)
            comp = compiled_funcs[func_name]

            if not orig:
                continue

            lines.append(f"#### `{func_name}` (0x{func_addr:08X})")
            lines.append("")
            lines.append("| # | Original | Recompiled |")
            lines.append("|---|----------|------------|")

            max_lines = max(len(orig), len(comp))
            for i in range(min(max_lines, 30)):
                orig_line = ""
                comp_line = ""
                if i < len(orig):
                    _, _, mn, ops = orig[i]
                    orig_line = f"`{mn} {ops}`"
                if i < len(comp):
                    _, _, mn, ops = comp[i]
                    comp_line = f"`{mn} {ops}`"
                lines.append(f"| {i} | {orig_line} | {comp_line} |")

            if max_lines > 30:
                lines.append(f"| ... | ({len(orig)} total) | ({len(comp)} total) |")
            lines.append("")

    else:
        lines.append("*No compiled functions available for comparison.*")
        lines.append("")

    # Detailed function disassembly comparison for key functions
    lines.append("## Detailed Disassembly / 详细反汇编")
    lines.append("")
    lines.append("Original disassembly of key functions for verification against decompiled C.")
    lines.append("")

    key_functions = [
        0x080071B4,  # SystemInit
        0x0800712C,  # board_init
        0x08007304,  # main
        0x08007220,  # boot_uart_check
        0x080071F4,  # image_header_verify
        0x08005988,  # validate_image
        0x0800717C,  # uart_putchar
        0x0800588C,  # uart_rx_ready
        0x080058A0,  # uart_getchar
        0x080058B8,  # uart_init
        0x08005338,  # flash_init
        0x080054F8,  # flash_read
        0x080029A0,  # malloc
        0x08002A3C,  # free
        0x080045A4,  # hash_get_result2 (sha1_init)
        0x08004404,  # hash_set_mode
        0x08005CFC,  # crc_verify_image
        0x08007278,  # find_valid_image
        0x08007148,  # tspend_handler
        0x080070C4,  # signature_verify
    ]

    for addr in key_functions:
        if addr not in FUNCTIONS:
            continue
        name, _, src = FUNCTIONS[addr]
        size = func_sizes.get(addr, 0)
        orig = extract_function_asm(all_instrs, addr, size)
        if not orig:
            continue

        lines.append(f"### `{name}` (0x{addr:08X}) — {len(orig)} instructions, {size} bytes")
        lines.append(f"Source: `{src}`")
        lines.append("")
        lines.append("```asm")
        for a, hx, mn, ops in orig[:50]:  # Limit to 50 instructions per function
            lines.append(f"  {a:08x}:  {hx:20s}  {mn:12s} {ops}")
        if len(orig) > 50:
            lines.append(f"  ... ({len(orig) - 50} more instructions)")
        lines.append("```")
        lines.append("")

    # Address map for cross-reference
    lines.append("## Address Map / 地址映射")
    lines.append("")
    lines.append("Complete mapping of all 116 identified functions with their addresses,")
    lines.append("sizes, and decompilation status.")
    lines.append("")

    code_start = min(FUNCTIONS.keys())
    code_end = max(addr + func_sizes.get(addr, 0) for addr in FUNCTIONS.keys())
    total_code = sum(func_sizes.get(a, 0) for a in FUNCTIONS.keys())
    decompiled_code = sum(
        func_sizes.get(a, 0) for a in FUNCTIONS.keys()
        if check_decompiled_coverage(FUNCTIONS[a][2], FUNCTIONS[a][0])
    )

    lines.append(f"- Code region: `0x{code_start:08X}` — `0x{code_end:08X}`")
    lines.append(f"- Total function code: {total_code} bytes")
    lines.append(f"- Decompiled code: {decompiled_code} bytes ({decompiled_code*100//max(total_code,1)}%)")
    lines.append("")

    # Data section analysis
    data_start = 0x08007500
    data_end = LOAD_ADDR + len(binary_data)
    lines.append("## Constant Data / 常量数据")
    lines.append("")
    lines.append(f"The region `0x{data_start:08X}` — `0x{data_end:08X}` ({data_end-data_start} bytes)")
    lines.append("contains constant data including:")
    lines.append("")
    lines.append("- Literal pool entries (32-bit constants loaded via `lrw`)")
    lines.append("- String constants (boot messages)")
    lines.append("- CRC32 lookup table (256 × 4 bytes = 1024 bytes)")
    lines.append("- Padding/alignment bytes")
    lines.append("")

    # Extract some string constants from the data area
    data_offset = data_start - LOAD_ADDR
    if data_offset < len(binary_data):
        lines.append("### String Constants Found / 发现的字符串常量")
        lines.append("")
        lines.append("```")
        data_region = binary_data[data_offset:]
        i = 0
        str_count = 0
        while i < len(data_region) and str_count < 20:
            # Look for printable strings of length >= 4
            start = i
            while i < len(data_region) and 0x20 <= data_region[i] < 0x7F:
                i += 1
            if i - start >= 4 and i < len(data_region) and data_region[i] == 0:
                s = data_region[start:i].decode("ascii", errors="replace")
                addr = data_start + start
                lines.append(f"  0x{addr:08X}: \"{s}\"")
                str_count += 1
            i += 1
        lines.append("```")
        lines.append("")

    return "\n".join(lines)


def try_compile_file(source_file, tmpdir):
    """Try to compile a single source file."""
    src_path = SCRIPT_DIR / source_file
    if not src_path.exists():
        return False, "File not found"

    if source_file.endswith(".S"):
        obj_path = os.path.join(tmpdir, source_file.replace(".S", ".o"))
        try:
            result = subprocess.run(
                [CSKY_GCC, "-mcpu=ck804ef", "-c", "-o", obj_path, str(src_path)],
                capture_output=True, text=True, timeout=30
            )
            if result.returncode == 0:
                return True, None
            return False, result.stderr
        except Exception as e:
            return False, str(e)

    obj_path = os.path.join(tmpdir, source_file.replace(".c", ".o"))
    try:
        result = subprocess.run(
            [CSKY_GCC] + CFLAGS + ["-o", obj_path, str(src_path)],
            capture_output=True, text=True, timeout=30
        )
        if result.returncode == 0:
            return True, None
        return False, result.stderr
    except Exception as e:
        return False, str(e)


def main():
    print("=== secboot.bin Decompilation Comparison Tool ===")
    print()

    # Check toolchain
    try:
        subprocess.run([CSKY_GCC, "--version"], capture_output=True, timeout=10)
    except FileNotFoundError:
        print(f"ERROR: C-SKY toolchain not found. Set PATH or CSKY_GCC env var.")
        print(f"  Looking for: {CSKY_GCC}")
        sys.exit(1)

    # Read binary
    print(f"Reading {SECBOOT_BIN}...")
    binary_data = read_binary(SECBOOT_BIN)
    print(f"  Size: {len(binary_data)} bytes")

    # Disassemble original
    print("Disassembling original binary...")
    disasm = disassemble_original(SECBOOT_BIN)
    if not disasm:
        print("ERROR: Failed to disassemble binary")
        sys.exit(1)

    all_instrs = parse_disassembly(disasm)
    print(f"  Parsed {len(all_instrs)} instructions")

    # Compute function sizes
    func_sizes = compute_function_sizes(binary_data)
    print(f"  {len(FUNCTIONS)} functions mapped")

    # Try compiling each source file and extract per-function assembly
    print("\nCompiling decompiled source files...")
    source_files = set()
    for _, (_, _, src) in FUNCTIONS.items():
        source_files.add(src)

    compile_results = {}
    compiled_funcs = {}  # name -> [(addr, hex, mnemonic, operands)]

    with tempfile.TemporaryDirectory() as tmpdir:
        for src_file in sorted(source_files):
            print(f"  Compiling {src_file}...", end=" ")
            success, error = try_compile_file(src_file, tmpdir)
            compile_results[src_file] = (success, error)
            print("✅" if success else "❌")

            if success and src_file.endswith(".c"):
                # Extract per-function disassembly from compiled object
                obj_path = os.path.join(tmpdir, src_file.replace(".c", ".o"))
                if os.path.exists(obj_path):
                    try:
                        result = subprocess.run(
                            [CSKY_OBJDUMP, "-d", obj_path],
                            capture_output=True, text=True, timeout=30
                        )
                        if result.returncode == 0:
                            # Parse function sections from objdump output
                            current_func = None
                            current_instrs = []
                            for line in result.stdout.split("\n"):
                                # Function header: "00000000 <func_name>:"
                                fm = re.match(r"[0-9a-f]+ <(\w+)>:", line)
                                if fm:
                                    if current_func and current_instrs:
                                        compiled_funcs[current_func] = current_instrs
                                    current_func = fm.group(1)
                                    current_instrs = []
                                    continue

                                # Instruction line
                                im = re.match(r"\s*([0-9a-f]+):\s+([0-9a-f][0-9a-f ]+?)\s+(\S+)\s*(.*)", line)
                                if im and current_func:
                                    addr = int(im.group(1), 16)
                                    hex_b = im.group(2).strip()
                                    mnem = im.group(3).strip()
                                    ops = im.group(4).split("//")[0].strip()
                                    current_instrs.append((addr, hex_b, mnem, ops))

                            if current_func and current_instrs:
                                compiled_funcs[current_func] = current_instrs
                    except Exception:
                        pass

        print(f"\n  Extracted {len(compiled_funcs)} compiled functions for comparison")

    # Generate report
    print("\nGenerating comparison report...")
    report = generate_report(binary_data, all_instrs, func_sizes, compile_results,
                             compiled_funcs)

    # Determine output path
    report_path = SCRIPT_DIR / "comparison_report.md"
    if "--report" in sys.argv:
        idx = sys.argv.index("--report")
        if idx + 1 < len(sys.argv):
            report_path = Path(sys.argv[idx + 1])

    report_path.write_text(report)
    print(f"\nReport written to: {report_path}")
    print(f"  Total functions: {len(FUNCTIONS)}")

    decompiled = sum(1 for addr in FUNCTIONS
                     if check_decompiled_coverage(
                         FUNCTIONS[addr][2], FUNCTIONS[addr][0]))
    print(f"  Decompiled: {decompiled}")
    print(f"  Coverage: {decompiled*100//len(FUNCTIONS)}%")


if __name__ == "__main__":
    main()
