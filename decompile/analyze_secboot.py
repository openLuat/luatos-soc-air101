#!/usr/bin/env python3
"""
analyze_secboot.py - Analysis tool for Air101/Air103 secboot.bin

Parses the secure bootloader binary, extracts the vector table,
identifies functions, and generates an annotated disassembly.

Architecture: C-SKY XT804 (CK804EF)
Load Address: 0x08002400
Binary Size:  21,468 bytes (0x53DC)

Usage:
    python3 analyze_secboot.py [path_to_secboot.bin]

Requirements:
    - Python 3.6+
    - csky-elfabiv2-objdump in PATH (for full disassembly mode)

If csky-elfabiv2-objdump is not available, the script will still
parse the vector table, identify data structures, and provide
a hex dump with annotations.
"""

import struct
import sys
import os
import subprocess
import binascii

# ============================================================
# Constants
# ============================================================

BASE_ADDR = 0x08002400
VECTOR_COUNT = 64
SIGNATURE_WORD = 0xA0FFFF9F

# Known functions identified by reverse engineering
# Format: {address: (name, description)}
KNOWN_FUNCTIONS = {
    0x08002506: ("Reset_Handler", "CPU startup entry point"),
    0x08002570: ("Default_Handler", "Trap/exception handler"),
    0x080025C0: ("memcpy_words", "Word-aligned memory copy (r0=dst, r1=src, r2=count, r3=flag)"),
    0x080028F4: ("puts", "Print null-terminated string"),
    0x0800290C: ("printf_simple", "Simple formatted print"),
    0x08002958: ("realloc", "Reallocate memory block"),
    0x08002980: ("calloc", "Allocate zeroed memory"),
    0x080029A0: ("malloc", "Heap allocator (first-fit)"),
    0x08002A3C: ("free", "Free allocated memory"),
    0x08002AB4: ("memcmp_or_recv", "Memory compare or receive buffer"),
    0x08002B54: ("memcpy", "Standard memcpy"),
    0x08002BD4: ("memcmp", "Optimized memory compare (word-aligned fast path)"),
    0x08002D20: ("memset", "Standard memset"),
    0x08002D7C: ("strlen", "String length"),
    0x08002EAC: ("strcmp_or_strncpy", "String compare or copy"),
    0x08002FC8: ("format_number", "Number formatting for printf"),
    0x08003150: ("vsnprintf_core", "Core of vsnprintf"),
    0x08003178: ("snprintf", "Formatted string output"),
    0x080031A8: ("sprintf", "String printf"),
    0x080031DC: ("printf", "Standard printf via uart_putchar"),
    0x08003238: ("flash_spi_cmd", "Send command to flash SPI controller"),
    0x08003298: ("flash_wait_ready", "Wait for flash not busy"),
    0x080032CC: ("flash_write_enable", "Flash write enable (WREN)"),
    0x0800332C: ("flash_erase_sector", "Erase one 4KB flash sector"),
    0x08003390: ("flash_program_page", "Program one flash page"),
    0x08003408: ("flash_read_status", "Read flash status register"),
    0x08003424: ("flash_read_id", "Read flash JEDEC ID"),
    0x080034A0: ("flash_quad_config", "Configure flash quad SPI mode"),
    0x080034E4: ("flash_unlock", "Unlock flash for write"),
    0x08003514: ("flash_lock", "Lock flash after write"),
    0x08003588: ("flash_power_ctrl", "Flash power management"),
    0x08003690: ("flash_controller_init", "Initialize flash SPI controller"),
    0x08003744: ("flash_detect", "Detect flash chip type and size"),
    0x08003774: ("flash_setup", "Full flash setup sequence"),
    0x08003860: ("flash_capacity_check", "Validate flash capacity"),
    0x080039C0: ("rsa_core", "RSA core operation"),
    0x08003A14: ("rsa_step", "RSA computation step"),
    0x08003A8C: ("rsa_modexp", "RSA modular exponentiation"),
    0x08003AE8: ("rsa_init", "RSA engine init"),
    0x08003B3C: ("rsa_process", "RSA process block"),
    0x08003CA4: ("sha_hash_block", "SHA hash one block"),
    0x08003CDC: ("sha_init", "SHA engine init"),
    0x08003CF0: ("sha_update", "SHA update with data"),
    0x08003E1C: ("sha_final", "SHA finalize digest"),
    0x08004258: ("crc32_table_init", "Initialize CRC32 lookup table"),
    0x080042DC: ("crc32_update", "Update CRC32 with data"),
    0x08004324: ("sha1_transform", "SHA-1 block transform via HW engine"),
    0x0800437C: ("hash_ctx_init", "Hash context initialization"),
    0x08004404: ("hw_crypto_setup", "Hardware crypto engine configuration"),
    0x08004410: ("hw_crypto_exec", "Execute hardware crypto operation"),
    0x080044B8: ("hw_crypto_exec2", "Execute hardware crypto operation (variant)"),
    0x08004544: ("hash_finalize", "Finalize hash computation"),
    0x0800459C: ("hash_get_result", "Read hash result"),
    0x080045A4: ("sha1_init", "SHA-1 context initialization"),
    0x080045D8: ("sha1_update", "SHA-1 data feed"),
    0x0800463C: ("sha1_final", "SHA-1 finalize + digest output"),
    0x0800472C: ("sha1_full", "SHA-1 one-shot: init+update+final"),
    0x08004906: ("crypto_subsys_init", "Cryptographic subsystem init"),
    0x080049CC: ("pkey_setup", "Public key setup"),
    0x08004A4C: ("pkey_verify_step", "Public key verify step"),
    0x08004AB4: ("pkey_verify", "Public key verification"),
    0x08004AF8: ("signature_check_init", "Init signature check"),
    0x08004BB0: ("signature_check_data", "Check data signature"),
    0x08004BEC: ("signature_check_final", "Finalize signature check"),
    0x08004C78: ("cert_parse", "Parse certificate structure"),
    0x08004CFC: ("crc_ctx_alloc", "Allocate CRC verification context"),
    0x08004D2C: ("crc_ctx_destroy", "Destroy CRC context"),
    0x08004D50: ("crc_ctx_reset", "Reset CRC internal state"),
    0x08004D98: ("image_decrypt_init", "Image decryption initialization"),
    0x08004E98: ("image_decrypt_block", "Decrypt one image block"),
    0x08004F20: ("image_decrypt_process", "Process encrypted image"),
    0x08005004: ("firmware_update_init", "Firmware update initialization"),
    0x0800509C: ("firmware_update_process", "Process firmware update"),
    0x080051E8: ("flash_read_page", "Read one page (256 bytes) from flash"),
    0x08005338: ("flash_init", "Initialize flash controller (SPI mode)"),
    0x08005358: ("flash_read_raw", "Raw flash read operation"),
    0x080054F8: ("flash_read", "Multi-page flash read"),
    0x0800553C: ("flash_write", "Flash write with sector erase"),
    0x08005670: ("flash_erase_range", "Erase flash address range"),
    0x080056B0: ("flash_protect_config", "Configure flash write protection"),
    0x080057EC: ("flash_param_read", "Read flash parameters"),
    0x08005804: ("flash_param_write", "Write flash parameters"),
    0x08005820: ("flash_param_init", "Initialize flash parameter area"),
    0x0800582C: ("flash_copy_data", "Copy data between flash regions"),
    0x0800588C: ("uart_rx_ready", "Check if UART0 RX FIFO has data"),
    0x080058A0: ("uart_getchar", "Read one byte from UART0 (blocking)"),
    0x080058B8: ("uart_init", "Initialize UART0 baud rate"),
    0x080058EC: ("image_copy_update", "Read image data from flash and verify CRC32 checksum"),
    0x08005988: ("validate_image", "Validate IMAGE_HEADER structure"),
    0x08005A5C: ("xmodem_recv_init", "Initialize xmodem receive"),
    0x08005AD4: ("xmodem_recv_block", "Receive one xmodem block"),
    0x08005B88: ("xmodem_recv_data", "Receive xmodem data stream"),
    0x08005BC8: ("xmodem_process", "Process xmodem transfer"),
    0x08005CFC: ("crc_verify_image", "CRC32 verify entire image"),
    0x08005DF0: ("firmware_apply", "Apply firmware update"),
    0x08005E74: ("firmware_apply_ext", "Extended firmware apply"),
    0x080062D4: ("ota_process", "OTA update process"),
    0x08006494: ("ota_validate", "OTA image validation"),
    0x08006530: ("ota_apply", "Apply OTA update"),
    0x08006880: ("boot_param_setup", "Setup boot parameters"),
    0x08006980: ("boot_param_read", "Read boot parameters from flash"),
    0x08006DEC: ("boot_prepare", "Prepare for application boot"),
    0x08006F94: ("boot_execute_prep", "Pre-execution boot preparation"),
    0x08007058: ("app_boot_sequence", "Application boot sequence"),
    0x080070C4: ("signature_verify", "Verify 128-byte image signature"),
    0x0800712C: ("board_init", "Board initialization (clock + UART)"),
    0x08007148: ("tspend_handler", "Timer/software pending ISR #57"),
    0x0800717C: ("uart_putchar", "UART0 TX one byte (\\n → \\r\\n)"),
    0x080071AC: ("lock_acquire", "Interrupt lock acquire (stub, returns 0)"),
    0x080071B0: ("lock_release", "Interrupt lock release (stub, returns 0)"),
    0x080071B4: ("SystemInit", "System initialization (VBR, cache, VIC)"),
    0x080071F0: ("trap_c", "Trap handler C entry (infinite loop)"),
    0x080071F4: ("image_header_verify", "Verify image header CRC + data integrity"),
    0x08007220: ("boot_uart_check", "Check UART for bootloader mode entry"),
    0x08007278: ("find_valid_image", "Scan flash for valid application image"),
    0x08007304: ("main", "Main bootloader entry point"),
}

# Known SRAM variables
KNOWN_VARIABLES = {
    0x20010060: "heap_state",
    0x2001007C: "flash_param_ptr",
    0x200101D0: "app_boot_handler",
    0x200101D4: "error_handler",
    0x200111D8: "initial_sp",
    0x200113D8: "trap_save_sp",
    0x200113E4: "tspend_counter",
    0x200113E8: "pre_reset_hook_ptr",
    0x200113EC: "boot_mode_flag",
}

# Peripheral register names
PERIPHERAL_REGS = {
    0x40010600: "UART0_BASE",
    0x4001060C: "UART0_INTR_EN",
    0x40010610: "UART0_BAUD_DIV",
    0x4001061C: "UART0_FIFO_STATUS",
    0x40010620: "UART0_TX_DATA",
    0x40010630: "UART0_RX_DATA",
    0x40011400: "CLK_BASE / PMU_BASE",
    0x4001140C: "CLK_CTRL",
    0xC0002000: "FLASH_SPI_BASE",
    0xE000E000: "VIC_BASE",
    0xE000E100: "VIC_INT_EN",
    0xE000E180: "VIC_INT_CLR (offset 0x180 from VIC_INT_EN)",
    0xE000E300: "VIC_INT_PEND (offset 0x200 from VIC_INT_EN)",
}


def read_binary(path):
    """Read the secboot.bin binary file."""
    with open(path, 'rb') as f:
        return f.read()


def parse_vector_table(data):
    """Parse the 64-entry vector table at the start of the binary."""
    vectors = []
    for i in range(VECTOR_COUNT):
        addr = struct.unpack_from('<I', data, i * 4)[0]
        vectors.append(addr)
    return vectors


def parse_image_header(img_path):
    """Parse IMAGE_HEADER from .img file (0x40 bytes prepended to .bin)."""
    with open(img_path, 'rb') as f:
        header_data = f.read(0x40)

    if len(header_data) < 0x40:
        return None

    magic = struct.unpack_from('<I', header_data, 0)[0]
    if magic != SIGNATURE_WORD:
        return None

    img_attr = struct.unpack_from('<I', header_data, 4)[0]
    img_addr = struct.unpack_from('<I', header_data, 8)[0]
    img_len = struct.unpack_from('<I', header_data, 12)[0]
    img_header_addr = struct.unpack_from('<I', header_data, 16)[0]
    upgrade_img_addr = struct.unpack_from('<I', header_data, 20)[0]
    org_checksum = struct.unpack_from('<I', header_data, 24)[0]
    upd_no = struct.unpack_from('<I', header_data, 28)[0]
    ver = header_data[32:48]
    hd_checksum = struct.unpack_from('<I', header_data, 60)[0]

    return {
        'magic': magic,
        'img_attr': img_attr,
        'img_type': img_attr & 0xF,
        'code_encrypt': (img_attr >> 4) & 1,
        'prikey_sel': (img_attr >> 5) & 7,
        'signature': (img_attr >> 8) & 1,
        'zip_type': (img_attr >> 16) & 1,
        'img_addr': img_addr,
        'img_len': img_len,
        'img_header_addr': img_header_addr,
        'upgrade_img_addr': upgrade_img_addr,
        'org_checksum': org_checksum,
        'upd_no': upd_no,
        'ver': ver,
        'hd_checksum': hd_checksum,
    }


def find_literal_pools(data):
    """Find literal pool constants referenced by LRW instructions."""
    pools = {}
    # Look for 4-byte aligned constant values in the binary
    for offset in range(0, len(data) - 3, 4):
        val = struct.unpack_from('<I', data, offset)[0]
        addr = BASE_ADDR + offset
        # Check if this looks like a known address or constant
        if val in KNOWN_FUNCTIONS:
            pools[addr] = (val, f"→ {KNOWN_FUNCTIONS[val][0]}")
        elif val in KNOWN_VARIABLES:
            pools[addr] = (val, f"→ {KNOWN_VARIABLES[val]}")
        elif val in PERIPHERAL_REGS:
            pools[addr] = (val, f"→ {PERIPHERAL_REGS[val]}")
        elif val == SIGNATURE_WORD:
            pools[addr] = (val, "→ SIGNATURE_WORD")
    return pools


def analyze_data_section(data):
    """Analyze the data section at the end of the binary for CRC table etc."""
    results = []

    # Look for CRC32 table (256 × 4 bytes = 1024 bytes)
    # Standard CRC32 table starts with: 0x00000000, 0x77073096, 0xEE0E612C, ...
    crc32_first_entries = [0x00000000, 0x77073096, 0xEE0E612C]
    for offset in range(0, len(data) - 1024, 4):
        vals = [struct.unpack_from('<I', data, offset + i * 4)[0] for i in range(3)]
        if vals == crc32_first_entries:
            results.append(('CRC32_TABLE', BASE_ADDR + offset, 1024))
            break

    return results


def generate_function_list():
    """Generate sorted list of all identified functions."""
    funcs = sorted(KNOWN_FUNCTIONS.items(), key=lambda x: x[0])
    return funcs


def run_objdump(bin_path):
    """Run csky-elfabiv2-objdump if available."""
    try:
        result = subprocess.run(
            ['csky-elfabiv2-objdump', '-D', '-b', 'binary', '-m', 'csky',
             f'--adjust-vma=0x{BASE_ADDR:08X}', bin_path],
            capture_output=True, text=True, timeout=30
        )
        if result.returncode == 0:
            return result.stdout
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass
    return None


def annotate_disassembly(disasm_text):
    """Add function labels and comments to raw disassembly."""
    lines = disasm_text.split('\n')
    output = []

    for line in lines:
        # Check if this line starts a known function
        for addr, (name, desc) in KNOWN_FUNCTIONS.items():
            addr_str = f' {addr:x}:'
            if addr_str in line:
                output.append(f'\n; ========================================')
                output.append(f'; {name} - {desc}')
                output.append(f'; Address: 0x{addr:08X}')
                output.append(f'; ========================================')
                break

        # Annotate known constants
        for reg_addr, reg_name in PERIPHERAL_REGS.items():
            hex_str = f'0x{reg_addr:x}'
            if hex_str in line.lower():
                line = f'{line}  ; << {reg_name}'
                break

        for var_addr, var_name in KNOWN_VARIABLES.items():
            hex_str = f'0x{var_addr:x}'
            if hex_str in line.lower():
                line = f'{line}  ; << {var_name}'
                break

        if '0xa0ffff9f' in line.lower():
            line = f'{line}  ; << SIGNATURE_WORD'

        output.append(line)

    return '\n'.join(output)


def main():
    # Determine binary path
    if len(sys.argv) > 1:
        bin_path = sys.argv[1]
    else:
        # Default path relative to repository root
        script_dir = os.path.dirname(os.path.abspath(__file__))
        repo_root = os.path.dirname(script_dir)
        bin_path = os.path.join(repo_root, 'tools', 'xt804', 'xt804_secboot.bin')

    if not os.path.exists(bin_path):
        print(f"Error: Cannot find {bin_path}")
        print(f"Usage: {sys.argv[0]} [path_to_secboot.bin]")
        sys.exit(1)

    data = read_binary(bin_path)
    print(f"=== secboot.bin Analysis ===")
    print(f"File: {bin_path}")
    print(f"Size: {len(data)} bytes (0x{len(data):X})")
    print(f"Load Address: 0x{BASE_ADDR:08X}")
    print(f"End Address:  0x{BASE_ADDR + len(data):08X}")
    print()

    # Parse vector table
    print("=== Vector Table (64 entries) ===")
    vectors = parse_vector_table(data)
    default_handler = vectors[1]
    special_vectors = {}
    for i, addr in enumerate(vectors):
        label = ""
        if i == 0:
            label = "Reset_Handler"
        elif addr == default_handler:
            label = "Default_Handler"
        else:
            label = f"SPECIAL_ISR (vector #{i})"
            special_vectors[i] = addr

        if addr != default_handler or i <= 1 or i in special_vectors:
            print(f"  [{i:2d}] 0x{addr:08X}  {label}")

    if not special_vectors:
        print(f"  [2-63] All point to Default_Handler at 0x{default_handler:08X}")
    else:
        non_special = [i for i in range(2, 64) if i not in special_vectors]
        if non_special:
            print(f"  [{non_special[0]}-{non_special[-1]}] "
                  f"(except specials) → Default_Handler at 0x{default_handler:08X}")
    print()

    # Parse .img header if available
    img_path = bin_path.replace('.bin', '.img')
    if os.path.exists(img_path):
        print("=== Image Header (.img) ===")
        header = parse_image_header(img_path)
        if header:
            print(f"  Magic:          0x{header['magic']:08X} {'✓' if header['magic'] == SIGNATURE_WORD else '✗'}")
            print(f"  Image Type:     {header['img_type']} (0=secboot, 1=app)")
            print(f"  Code Encrypt:   {header['code_encrypt']}")
            print(f"  Signature:      {header['signature']}")
            print(f"  Image Address:  0x{header['img_addr']:08X}")
            print(f"  Image Length:   0x{header['img_len']:X} ({header['img_len']} bytes)")
            print(f"  Header Address: 0x{header['img_header_addr']:08X}")
            print(f"  Checksum:       0x{header['org_checksum']:08X}")
            print(f"  Update No:      {header['upd_no']}")
            print(f"  HD Checksum:    0x{header['hd_checksum']:08X}")
        print()

    # Identify functions
    print(f"=== Identified Functions ({len(KNOWN_FUNCTIONS)} total) ===")
    for addr, (name, desc) in generate_function_list():
        print(f"  0x{addr:08X}  {name:30s}  {desc}")
    print()

    # Find data structures
    print("=== Data Structures ===")
    data_sections = analyze_data_section(data)
    for name, addr, size in data_sections:
        print(f"  {name}: 0x{addr:08X} ({size} bytes)")
    print()

    # Try running objdump for full disassembly
    print("=== Disassembly ===")
    disasm = run_objdump(bin_path)
    if disasm:
        annotated = annotate_disassembly(disasm)
        output_path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                   'secboot_annotated.S')
        with open(output_path, 'w') as f:
            f.write('; secboot.bin annotated disassembly\n')
            f.write(f'; Generated by analyze_secboot.py\n')
            f.write(f'; Architecture: C-SKY XT804 (CK804EF)\n')
            f.write(f'; Load Address: 0x{BASE_ADDR:08X}\n')
            f.write(f'; Binary Size: {len(data)} bytes\n\n')
            f.write(annotated)
        print(f"  Annotated disassembly written to: {output_path}")
    else:
        print("  csky-elfabiv2-objdump not found in PATH.")
        print("  Install the C-SKY toolchain to generate full disassembly.")
        print("  See README.md for download instructions.")
    print()

    print("=== Analysis Complete ===")


if __name__ == '__main__':
    main()
