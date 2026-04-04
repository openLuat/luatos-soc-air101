# secboot.bin Decompilation Comparison Report
# secboot.bin 反编译对比报告

Generated from `xt804_secboot.bin` (21468 bytes)

## Summary / 概要

| Metric | Value |
|--------|-------|
| Total functions identified / 已识别函数总数 | 116 |
| Functions with decompiled C / 已反编译函数数 | 113 |
| Decompilation coverage / 反编译覆盖率 | 97% |
| Total binary code size / 二进制代码总大小 | 21468 bytes |

## Per-File Coverage / 各文件覆盖率

| Source File | Functions | Decompiled | Coverage |
|-------------|-----------|------------|----------|
| `secboot_boot.c` | 5 | 5 | 100% |
| `secboot_crypto.c` | 34 | 34 | 100% |
| `secboot_flash.c` | 26 | 26 | 100% |
| `secboot_fwup.c` | 11 | 11 | 100% |
| `secboot_hw_init.c` | 5 | 5 | 100% |
| `secboot_image.c` | 7 | 4 | 57% |
| `secboot_main.c` | 4 | 4 | 100% |
| `secboot_memory.c` | 4 | 4 | 100% |
| `secboot_stdlib.c` | 14 | 14 | 100% |
| `secboot_uart.c` | 4 | 4 | 100% |
| `secboot_vectors.S` | 2 | 2 | 100% |

## Compilation Results / 编译结果

Each decompiled `.c` file was compiled with `csky-elfabiv2-gcc -mcpu=ck804ef -O2`
to check if the pseudo-C produces valid C-SKY assembly.

### `secboot_boot.c` — ✅ Compiles

### `secboot_crypto.c` — ✅ Compiles

### `secboot_flash.c` — ✅ Compiles

### `secboot_fwup.c` — ✅ Compiles

### `secboot_hw_init.c` — ✅ Compiles

### `secboot_image.c` — ✅ Compiles

### `secboot_main.c` — ✅ Compiles

### `secboot_memory.c` — ✅ Compiles

### `secboot_stdlib.c` — ✅ Compiles

### `secboot_uart.c` — ✅ Compiles

### `secboot_vectors.S` — ✅ Compiles

## Function Analysis / 函数分析

Detailed analysis of each identified function in the original binary,
with instruction statistics and decompilation status.

| Address | Function | Size | Instructions | Branches | Loads/Stores | Source File | Status |
|---------|----------|------|-------------|----------|-------------|-------------|--------|
| `0x08002506` | `Reset_Handler` | 106 | 48 | 10 | 12/2 | `secboot_vectors.S` | ✅ Decompiled |
| `0x08002570` | `Default_Handler` | 80 | 28 | 2 | 7/8 | `secboot_vectors.S` | ✅ Decompiled |
| `0x080025C0` | `memcpy_words` | 820 | 314 | 52 | 6/0 | `secboot_stdlib.c` | ✅ Decompiled |
| `0x080028F4` | `puts` | 24 | 10 | 2 | 4/0 | `secboot_stdlib.c` | ✅ Decompiled |
| `0x0800290C` | `printf_simple` | 76 | 31 | 8 | 2/0 | `secboot_stdlib.c` | ✅ Decompiled |
| `0x08002958` | `realloc` | 40 | 15 | 5 | 0/0 | `secboot_memory.c` | ✅ Decompiled |
| `0x08002980` | `calloc` | 32 | 12 | 3 | 0/0 | `secboot_stdlib.c` | ✅ Decompiled |
| `0x080029A0` | `malloc` | 156 | 76 | 11 | 14/9 | `secboot_memory.c` | ✅ Decompiled |
| `0x08002A3C` | `free` | 120 | 58 | 11 | 13/7 | `secboot_memory.c` | ✅ Decompiled |
| `0x08002AB4` | `memcmp_or_recv` | 160 | 52 | 16 | 0/11 | `secboot_stdlib.c` | ✅ Decompiled |
| `0x08002B54` | `memcpy` | 128 | 45 | 9 | 7/7 | `secboot_stdlib.c` | ✅ Decompiled |
| `0x08002BD4` | `memcmp` | 332 | 121 | 35 | 24/0 | `secboot_memory.c` | ✅ Decompiled |
| `0x08002D20` | `memset` | 92 | 38 | 5 | 2/3 | `secboot_stdlib.c` | ✅ Decompiled |
| `0x08002D7C` | `strlen` | 304 | 106 | 20 | 13/8 | `secboot_stdlib.c` | ✅ Decompiled |
| `0x08002EAC` | `strcmp_or_strncpy` | 284 | 105 | 18 | 18/14 | `secboot_stdlib.c` | ✅ Decompiled |
| `0x08002FC8` | `format_number` | 392 | 129 | 21 | 16/8 | `secboot_stdlib.c` | ✅ Decompiled |
| `0x08003150` | `vsnprintf_core` | 40 | 15 | 2 | 2/2 | `secboot_stdlib.c` | ✅ Decompiled |
| `0x08003178` | `snprintf` | 48 | 20 | 3 | 0/5 | `secboot_stdlib.c` | ✅ Decompiled |
| `0x080031A8` | `sprintf` | 52 | 21 | 3 | 2/5 | `secboot_stdlib.c` | ✅ Decompiled |
| `0x080031DC` | `printf` | 92 | 33 | 6 | 3/0 | `secboot_stdlib.c` | ✅ Decompiled |
| `0x08003238` | `flash_spi_cmd` | 96 | 41 | 11 | 8/0 | `secboot_flash.c` | ✅ Decompiled |
| `0x08003298` | `flash_wait_ready` | 52 | 21 | 4 | 2/0 | `secboot_flash.c` | ✅ Decompiled |
| `0x080032CC` | `flash_write_enable` | 96 | 38 | 7 | 6/5 | `secboot_flash.c` | ✅ Decompiled |
| `0x0800332C` | `flash_erase_sector` | 100 | 40 | 6 | 6/5 | `secboot_flash.c` | ✅ Decompiled |
| `0x08003390` | `flash_program_page` | 120 | 47 | 10 | 8/4 | `secboot_flash.c` | ✅ Decompiled |
| `0x08003408` | `flash_read_status` | 28 | 11 | 3 | 0/0 | `secboot_flash.c` | ✅ Decompiled |
| `0x08003424` | `flash_read_id` | 124 | 43 | 8 | 5/6 | `secboot_flash.c` | ✅ Decompiled |
| `0x080034A0` | `flash_quad_config` | 68 | 24 | 8 | 4/3 | `secboot_flash.c` | ✅ Decompiled |
| `0x080034E4` | `flash_unlock` | 48 | 19 | 2 | 2/5 | `secboot_flash.c` | ✅ Decompiled |
| `0x08003514` | `flash_lock` | 116 | 46 | 7 | 5/3 | `secboot_flash.c` | ✅ Decompiled |
| `0x08003588` | `flash_power_ctrl` | 264 | 87 | 22 | 9/6 | `secboot_flash.c` | ✅ Decompiled |
| `0x08003690` | `flash_controller_init` | 180 | 68 | 14 | 9/8 | `secboot_flash.c` | ✅ Decompiled |
| `0x08003744` | `flash_detect` | 48 | 18 | 5 | 3/0 | `secboot_flash.c` | ✅ Decompiled |
| `0x08003774` | `flash_setup` | 236 | 84 | 14 | 8/8 | `secboot_flash.c` | ✅ Decompiled |
| `0x08003860` | `flash_capacity_check` | 352 | 122 | 25 | 16/14 | `secboot_flash.c` | ✅ Decompiled |
| `0x080039C0` | `rsa_core` | 84 | 33 | 8 | 3/1 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08003A14` | `rsa_step` | 120 | 48 | 9 | 5/3 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08003A8C` | `rsa_modexp` | 92 | 37 | 5 | 3/5 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08003AE8` | `rsa_init` | 84 | 34 | 4 | 0/5 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08003B3C` | `rsa_process` | 360 | 109 | 17 | 17/12 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08003CA4` | `sha_hash_block` | 56 | 19 | 1 | 5/1 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08003CDC` | `sha_init` | 20 | 8 | 1 | 0/0 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08003CF0` | `sha_update` | 300 | 102 | 17 | 16/14 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08003E1C` | `sha_final` | 1084 | 381 | 85 | 71/46 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08004258` | `crc32_table_init` | 132 | 50 | 9 | 8/7 | `secboot_crypto.c` | ✅ Decompiled |
| `0x080042DC` | `crc32_update` | 72 | 30 | 6 | 0/0 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08004324` | `sha1_transform` | 88 | 38 | 2 | 12/14 | `secboot_crypto.c` | ✅ Decompiled |
| `0x0800437C` | `hash_ctx_init` | 136 | 49 | 8 | 3/0 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08004404` | `hw_crypto_setup` | 12 | 6 | 1 | 0/3 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08004410` | `hw_crypto_exec` | 168 | 63 | 10 | 10/8 | `secboot_crypto.c` | ✅ Decompiled |
| `0x080044B8` | `hw_crypto_exec2` | 140 | 52 | 9 | 7/5 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08004544` | `hash_finalize` | 88 | 32 | 7 | 7/3 | `secboot_crypto.c` | ✅ Decompiled |
| `0x0800459C` | `hash_get_result` | 8 | 4 | 1 | 1/1 | `secboot_crypto.c` | ✅ Decompiled |
| `0x080045A4` | `sha1_init` | 52 | 24 | 2 | 7/9 | `secboot_crypto.c` | ✅ Decompiled |
| `0x080045D8` | `sha1_update` | 100 | 37 | 8 | 4/4 | `secboot_crypto.c` | ✅ Decompiled |
| `0x0800463C` | `sha1_final` | 240 | 91 | 13 | 6/20 | `secboot_crypto.c` | ✅ Decompiled |
| `0x0800472C` | `sha1_full` | 474 | 161 | 33 | 14/8 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08004906` | `crypto_subsys_init` | 198 | 70 | 18 | 7/6 | `secboot_crypto.c` | ✅ Decompiled |
| `0x080049CC` | `pkey_setup` | 128 | 43 | 9 | 3/5 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08004A4C` | `pkey_verify_step` | 104 | 44 | 10 | 7/2 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08004AB4` | `pkey_verify` | 68 | 30 | 5 | 4/2 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08004AF8` | `signature_check_init` | 184 | 76 | 12 | 7/11 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08004BB0` | `signature_check_data` | 60 | 26 | 3 | 3/3 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08004BEC` | `signature_check_final` | 140 | 58 | 9 | 7/4 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08004C78` | `cert_parse` | 132 | 48 | 17 | 6/1 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08004CFC` | `crc_ctx_alloc` | 48 | 19 | 6 | 0/1 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08004D2C` | `crc_ctx_destroy` | 36 | 14 | 6 | 2/0 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08004D50` | `crc_ctx_reset` | 72 | 21 | 9 | 0/0 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08004D98` | `image_decrypt_init` | 256 | 100 | 30 | 6/2 | `secboot_image.c` | ⬜ Identified |
| `0x08004E98` | `image_decrypt_block` | 136 | 58 | 7 | 12/13 | `secboot_image.c` | ⬜ Identified |
| `0x08004F20` | `image_decrypt_process` | 228 | 90 | 18 | 9/5 | `secboot_image.c` | ⬜ Identified |
| `0x08005004` | `firmware_update_init` | 152 | 58 | 4 | 16/9 | `secboot_fwup.c` | ✅ Decompiled |
| `0x0800509C` | `firmware_update_process` | 332 | 124 | 10 | 36/22 | `secboot_fwup.c` | ✅ Decompiled |
| `0x080051E8` | `flash_read_page` | 336 | 118 | 18 | 17/10 | `secboot_flash.c` | ✅ Decompiled |
| `0x08005338` | `flash_init` | 32 | 12 | 1 | 1/2 | `secboot_flash.c` | ✅ Decompiled |
| `0x08005358` | `flash_read_raw` | 416 | 157 | 27 | 24/29 | `secboot_flash.c` | ✅ Decompiled |
| `0x080054F8` | `flash_read` | 68 | 27 | 6 | 0/0 | `secboot_flash.c` | ✅ Decompiled |
| `0x0800553C` | `flash_write` | 308 | 96 | 13 | 12/15 | `secboot_flash.c` | ✅ Decompiled |
| `0x08005670` | `flash_erase_range` | 64 | 26 | 6 | 5/1 | `secboot_flash.c` | ✅ Decompiled |
| `0x080056B0` | `flash_protect_config` | 316 | 114 | 29 | 11/7 | `secboot_flash.c` | ✅ Decompiled |
| `0x080057EC` | `flash_param_read` | 24 | 9 | 3 | 0/0 | `secboot_flash.c` | ✅ Decompiled |
| `0x08005804` | `flash_param_write` | 28 | 10 | 4 | 0/0 | `secboot_flash.c` | ✅ Decompiled |
| `0x08005820` | `flash_param_init` | 12 | 5 | 1 | 0/0 | `secboot_flash.c` | ✅ Decompiled |
| `0x0800582C` | `flash_copy_data` | 96 | 35 | 6 | 0/0 | `secboot_flash.c` | ✅ Decompiled |
| `0x0800588C` | `uart_rx_ready` | 20 | 8 | 2 | 2/0 | `secboot_uart.c` | ✅ Decompiled |
| `0x080058A0` | `uart_getchar` | 24 | 10 | 3 | 3/0 | `secboot_uart.c` | ✅ Decompiled |
| `0x080058B8` | `uart_init` | 52 | 23 | 2 | 3/5 | `secboot_uart.c` | ✅ Decompiled |
| `0x080058EC` | `image_copy_update` | 156 | 67 | 12 | 14/7 | `secboot_image.c` | ✅ Decompiled |
| `0x08005988` | `validate_image` | 212 | 89 | 19 | 16/1 | `secboot_image.c` | ✅ Decompiled |
| `0x08005A5C` | `xmodem_recv_init` | 120 | 50 | 9 | 9/1 | `secboot_fwup.c` | ✅ Decompiled |
| `0x08005AD4` | `xmodem_recv_block` | 180 | 68 | 8 | 20/0 | `secboot_fwup.c` | ✅ Decompiled |
| `0x08005B88` | `xmodem_recv_data` | 64 | 25 | 7 | 2/0 | `secboot_fwup.c` | ✅ Decompiled |
| `0x08005BC8` | `xmodem_process` | 308 | 116 | 29 | 1/1 | `secboot_fwup.c` | ✅ Decompiled |
| `0x08005CFC` | `crc_verify_image` | 244 | 98 | 19 | 6/7 | `secboot_crypto.c` | ✅ Decompiled |
| `0x08005DF0` | `firmware_apply` | 132 | 58 | 5 | 21/6 | `secboot_fwup.c` | ✅ Decompiled |
| `0x08005E74` | `firmware_apply_ext` | 1120 | 394 | 51 | 69/57 | `secboot_fwup.c` | ✅ Decompiled |
| `0x080062D4` | `ota_process` | 448 | 182 | 44 | 52/18 | `secboot_fwup.c` | ✅ Decompiled |
| `0x08006494` | `ota_validate` | 156 | 65 | 11 | 24/5 | `secboot_fwup.c` | ✅ Decompiled |
| `0x08006530` | `ota_apply` | 848 | 328 | 55 | 83/27 | `secboot_fwup.c` | ✅ Decompiled |
| `0x08006880` | `boot_param_setup` | 256 | 97 | 22 | 18/13 | `secboot_boot.c` | ✅ Decompiled |
| `0x08006980` | `boot_param_read` | 1132 | 447 | 91 | 109/34 | `secboot_boot.c` | ✅ Decompiled |
| `0x08006DEC` | `boot_prepare` | 424 | 160 | 23 | 37/10 | `secboot_boot.c` | ✅ Decompiled |
| `0x08006F94` | `boot_execute_prep` | 196 | 82 | 16 | 21/6 | `secboot_boot.c` | ✅ Decompiled |
| `0x08007058` | `app_boot_sequence` | 108 | 48 | 7 | 10/6 | `secboot_boot.c` | ✅ Decompiled |
| `0x080070C4` | `signature_verify` | 104 | 48 | 2 | 13/10 | `secboot_image.c` | ✅ Decompiled |
| `0x0800712C` | `board_init` | 28 | 12 | 1 | 2/1 | `secboot_hw_init.c` | ✅ Decompiled |
| `0x08007148` | `tspend_handler` | 52 | 21 | 1 | 6/2 | `secboot_main.c` | ✅ Decompiled |
| `0x0800717C` | `uart_putchar` | 48 | 19 | 6 | 4/2 | `secboot_uart.c` | ✅ Decompiled |
| `0x080071AC` | `lock_acquire` | 4 | 2 | 1 | 0/0 | `secboot_hw_init.c` | ✅ Decompiled |
| `0x080071B0` | `lock_release` | 4 | 2 | 1 | 0/0 | `secboot_hw_init.c` | ✅ Decompiled |
| `0x080071B4` | `SystemInit` | 60 | 19 | 3 | 2/2 | `secboot_hw_init.c` | ✅ Decompiled |
| `0x080071F0` | `trap_c` | 4 | 2 | 1 | 0/0 | `secboot_hw_init.c` | ✅ Decompiled |
| `0x080071F4` | `image_header_verify` | 44 | 18 | 4 | 0/0 | `secboot_main.c` | ✅ Decompiled |
| `0x08007220` | `boot_uart_check` | 88 | 40 | 8 | 5/4 | `secboot_main.c` | ✅ Decompiled |
| `0x08007278` | `find_valid_image` | 140 | 54 | 14 | 9/0 | `secboot_image.c` | ✅ Decompiled |
| `0x08007304` | `main` | 1240 | 528 | 175 | 192/11 | `secboot_main.c` | ✅ Decompiled |

## Compiled vs Original Comparison / 编译对比

For each compiled function, instruction count and pattern comparison
with the original binary. A higher match indicates better decompilation.

| Function | Original Insns | Compiled Insns | Size Ratio | Mnemonic Match |
|----------|---------------|----------------|------------|----------------|
| `memcpy_words` | 314 | 18 | 0.06x | 🔴 4% |
| `puts` | 10 | 21 | 2.10x | 🔴 19% |
| `printf_simple` | 31 | 100 | 3.23x | 🔴 24% |
| `calloc` | 12 | 13 | 1.08x | 🟡 64% |
| `malloc` | 76 | 72 | 0.95x | 🟡 54% |
| `free` | 58 | 57 | 0.98x | 🔴 37% |
| `memcmp_or_recv` | 52 | 11 | 0.21x | 🔴 6% |
| `memcpy` | 45 | 69 | 1.53x | 🔴 23% |
| `memcmp` | 121 | 96 | 0.79x | 🔴 42% |
| `strlen` | 106 | 10 | 0.09x | 🔴 9% |
| `strcmp_or_strncpy` | 105 | 24 | 0.23x | 🔴 11% |
| `snprintf` | 20 | 32 | 1.60x | 🔴 23% |
| `sprintf` | 21 | 24 | 1.14x | 🔴 13% |
| `printf` | 33 | 37 | 1.12x | 🔴 14% |
| `flash_spi_cmd` | 41 | 15 | 0.37x | 🔴 11% |
| `flash_wait_ready` | 21 | 19 | 0.90x | 🔴 10% |
| `flash_write_enable` | 38 | 33 | 0.87x | 🔴 8% |
| `flash_erase_sector` | 40 | 36 | 0.90x | 🔴 16% |
| `flash_program_page` | 47 | 47 | 1.00x | 🔴 26% |
| `flash_read_status` | 11 | 18 | 1.64x | 🔴 7% |
| `flash_read_id` | 43 | 17 | 0.40x | 🔴 10% |
| `flash_quad_config` | 24 | 74 | 3.08x | 🔴 6% |
| `flash_unlock` | 19 | 53 | 2.79x | 🔴 11% |
| `flash_lock` | 46 | 54 | 1.17x | 🔴 14% |
| `flash_power_ctrl` | 87 | 18 | 0.21x | 🔴 8% |
| `flash_controller_init` | 68 | 14 | 0.21x | 🔴 15% |
| `flash_detect` | 18 | 26 | 1.44x | 🔴 18% |
| `flash_setup` | 84 | 46 | 0.55x | 🔴 11% |
| `flash_capacity_check` | 122 | 6 | 0.05x | 🔴 3% |
| `rsa_core` | 33 | 32 | 0.97x | 🟢 98% |
| `rsa_step` | 48 | 48 | 1.00x | 🟢 85% |
| `rsa_modexp` | 37 | 42 | 1.14x | 🟡 71% |
| `rsa_init` | 34 | 37 | 1.09x | 🟡 76% |
| `rsa_process` | 109 | 113 | 1.04x | 🟡 58% |
| `sha_hash_block` | 19 | 20 | 1.05x | 🔴 41% |
| `sha_init` | 8 | 9 | 1.12x | 🟢 94% |
| `sha_update` | 102 | 83 | 0.81x | 🟡 52% |
| `sha_final` | 381 | 167 | 0.44x | 🔴 38% |
| `crc32_table_init` | 50 | 54 | 1.08x | 🟢 87% |
| `crc32_update` | 30 | 30 | 1.00x | 🟡 77% |
| `sha1_transform` | 38 | 37 | 0.97x | 🟢 99% |
| `hash_ctx_init` | 49 | 40 | 0.82x | 🟡 76% |
| `hw_crypto_setup` | 6 | 6 | 1.00x | 🟡 67% |
| `hw_crypto_exec` | 63 | 58 | 0.92x | 🟢 83% |
| `hw_crypto_exec2` | 52 | 47 | 0.90x | 🟢 81% |
| `hash_finalize` | 32 | 23 | 0.72x | 🟡 65% |
| `hash_get_result` | 4 | 4 | 1.00x | 🟢 100% |
| `sha1_init` | 24 | 21 | 0.88x | 🟡 67% |
| `sha1_update` | 37 | 37 | 1.00x | 🟢 97% |
| `sha1_final` | 91 | 92 | 1.01x | 🟡 67% |
| `sha1_full` | 161 | 192 | 1.19x | 🟡 63% |
| `pkey_setup` | 43 | 49 | 1.14x | 🟡 76% |
| `pkey_verify_step` | 44 | 55 | 1.25x | 🟢 83% |
| `pkey_verify` | 30 | 31 | 1.03x | 🟢 92% |
| `signature_check_init` | 76 | 72 | 0.95x | 🟡 80% |
| `signature_check_data` | 26 | 26 | 1.00x | 🟢 100% |
| `signature_check_final` | 58 | 58 | 1.00x | 🟢 88% |
| `cert_parse` | 48 | 54 | 1.12x | 🟡 65% |
| `crc_ctx_alloc` | 19 | 18 | 0.95x | 🟢 97% |
| `crc_ctx_destroy` | 14 | 14 | 1.00x | 🟢 86% |
| `crc_ctx_reset` | 21 | 21 | 1.00x | 🟢 100% |
| `firmware_update_init` | 58 | 46 | 0.79x | 🔴 19% |
| `firmware_update_process` | 124 | 77 | 0.62x | 🔴 24% |
| `flash_init` | 12 | 11 | 0.92x | 🟢 87% |
| `flash_read_raw` | 157 | 25 | 0.16x | 🔴 12% |
| `flash_read` | 27 | 26 | 0.96x | 🟢 87% |
| `flash_write` | 96 | 41 | 0.43x | 🔴 32% |
| `flash_erase_range` | 26 | 19 | 0.73x | 🔴 27% |
| `flash_protect_config` | 114 | 59 | 0.52x | 🔴 24% |
| `flash_param_read` | 9 | 6 | 0.67x | 🔴 40% |
| `flash_param_write` | 10 | 6 | 0.60x | 🔴 38% |
| `flash_param_init` | 5 | 19 | 3.80x | 🔴 33% |
| `flash_copy_data` | 35 | 2 | 0.06x | 🔴 5% |
| `uart_rx_ready` | 8 | 8 | 1.00x | 🟡 75% |
| `uart_getchar` | 10 | 9 | 0.90x | 🟡 74% |
| `uart_init` | 23 | 21 | 0.91x | 🟡 77% |
| `image_copy_update` | 67 | 52 | 0.78x | 🔴 49% |
| `validate_image` | 89 | 67 | 0.75x | 🟡 63% |
| `xmodem_recv_init` | 50 | 51 | 1.02x | 🔴 22% |
| `xmodem_recv_block` | 68 | 68 | 1.00x | 🔴 16% |
| `xmodem_recv_data` | 25 | 31 | 1.24x | 🔴 29% |
| `xmodem_process` | 116 | 37 | 0.32x | 🔴 34% |
| `crc_verify_image` | 98 | 94 | 0.96x | 🟡 66% |
| `firmware_apply` | 58 | 53 | 0.91x | 🔴 23% |
| `firmware_apply_ext` | 394 | 61 | 0.15x | 🔴 11% |
| `ota_process` | 182 | 85 | 0.47x | 🔴 16% |
| `ota_validate` | 65 | 38 | 0.58x | 🔴 19% |
| `ota_apply` | 328 | 57 | 0.17x | 🔴 15% |
| `boot_param_setup` | 97 | 41 | 0.42x | 🔴 23% |
| `boot_param_read` | 447 | 77 | 0.17x | 🔴 15% |
| `boot_prepare` | 160 | 38 | 0.24x | 🔴 11% |
| `boot_execute_prep` | 82 | 31 | 0.38x | 🔴 23% |
| `app_boot_sequence` | 48 | 35 | 0.73x | 🔴 34% |
| `signature_verify` | 48 | 29 | 0.60x | 🟡 55% |
| `board_init` | 12 | 11 | 0.92x | 🟡 78% |
| `tspend_handler` | 21 | 14 | 0.67x | 🔴 46% |
| `uart_putchar` | 19 | 18 | 0.95x | 🟢 92% |
| `lock_acquire` | 2 | 2 | 1.00x | 🟢 100% |
| `lock_release` | 2 | 2 | 1.00x | 🟢 100% |
| `SystemInit` | 19 | 21 | 1.11x | 🔴 40% |
| `trap_c` | 2 | 1 | 0.50x | 🟡 67% |
| `image_header_verify` | 18 | 20 | 1.11x | 🟡 79% |
| `boot_uart_check` | 40 | 37 | 0.93x | 🔴 34% |
| `find_valid_image` | 54 | 52 | 0.96x | 🟡 79% |
| `main` | 528 | 141 | 0.27x | 🔴 33% |

**Average mnemonic similarity: 47.1%** (across 105 compared functions)

### Side-by-Side Examples / 逐行对比示例

#### `uart_putchar` (0x0800717C)

| # | Original | Recompiled |
|---|----------|------------|
| 0 | `cmpnei r0, 10` | `cmpnei r0, 10` |
| 1 | `bf 0x8007196` | `bf 0x1a` |
| 2 | `lrw r2, 0x40010600` | `lrw r2, 0x40010600` |
| 3 | `ld.w r3, (r2, 0x1c)` | `ld.w r3, (r2, 0x1c)` |
| 4 | `andi r3, r3, 63` | `andi r3, r3, 63` |
| 5 | `bnez r3, 0x8007182` | `bnez r3, 0x6` |
| 6 | `andi r0, r0, 255` | `andi r0, r0, 255` |
| 7 | `st.w r0, (r2, 0x20)` | `st.w r0, (r2, 0x20)` |
| 8 | `mov r0, r3` | `mov r0, r3` |
| 9 | `jmp r15` | `jmp r15` |
| 10 | `lrw r2, 0x40010600` | `lrw r2, 0x40010600` |
| 11 | `ld.w r3, (r2, 0x1c)` | `ld.w r3, (r2, 0x1c)` |
| 12 | `andi r3, r3, 63` | `andi r3, r3, 63` |
| 13 | `bnez r3, 0x8007198` | `bnez r3, 0x1c` |
| 14 | `movi r3, 13` | `movi r3, 13` |
| 15 | `st.w r3, (r2, 0x20)` | `st.w r3, (r2, 0x20)` |
| 16 | `br 0x8007180` | `br 0x4` |
| 17 | `br 0x8006da8` | `.long 0x40010600` |
| 18 | `lsli r0, r0, 1` |  |

#### `uart_rx_ready` (0x0800588C)

| # | Original | Recompiled |
|---|----------|------------|
| 0 | `lrw r3, 0x40010600` | `lrw r3, 0x40010600` |
| 1 | `ld.w r3, (r3, 0x1c)` | `ld.w r3, (r3, 0x1c)` |
| 2 | `andi r3, r3, 4032` | `andi r3, r3, 4032` |
| 3 | `cmpnei r3, 0` | `cmpnei r3, 0` |
| 4 | `mvc r0` | `mvc r0` |
| 5 | `jmp r15` | `jmp r15` |
| 6 | `br 0x800549c` | `.short 0x0600` |
| 7 | `lsli r0, r0, 1` | `.short 0x4001` |

#### `flash_init` (0x08005338)

| # | Original | Recompiled |
|---|----------|------------|
| 0 | `movi r3, 8192` | `movi r3, 32771` |
| 1 | `bseti r3, 30` | `rotli r3, r3, 30` |
| 2 | `movi r2, 49311` | `movi r2, 49311` |
| 3 | `bseti r2, 17` | `bseti r2, 17` |
| 4 | `st.w r2, (r3, 0x0)` | `st.w r2, (r3, 0x0)` |
| 5 | `movi r2, 256` | `movi r2, 256` |
| 6 | `st.w r2, (r3, 0x4)` | `st.w r2, (r3, 0x4)` |
| 7 | `movih r3, 16384` | `movih r3, 16384` |
| 8 | `ld.w r0, (r3, 0x0)` | `ld.w r0, (r3, 0x0)` |
| 9 | `zextb r0, r0` | `zextb r0, r0` |
| 10 | `jmp r15` | `jmp r15` |
| 11 | `bkpt ` |  |

#### `SystemInit` (0x080071B4)

| # | Original | Recompiled |
|---|----------|------------|
| 0 | `lrw r3, 0x8002400` | `push r15` |
| 1 | `mtcr r3, cr<1, 0>` | `movi r0, 9216` |
| 2 | `mfcr r3, cr<31, 0>` | `bseti r0, 27` |
| 3 | `ori r3, r3, 16` | `bsr 0x0` |
| 4 | `mtcr r3, cr<31, 0>` | `bsr 0x0` |
| 5 | `mfcr r3, cr<0, 0>` | `ori r0, r0, 16` |
| 6 | `ori r3, r3, 512` | `bsr 0x0` |
| 7 | `mtcr r3, cr<0, 0>` | `bsr 0x0` |
| 8 | `lrw r2, 0xe000e100` | `ori r0, r0, 512` |
| 9 | `movi r3, 0` | `bsr 0x0` |
| 10 | `st.w r3, (r2, 0x200)` | `lrw r3, 0xe000e300` |
| 11 | `subi r3, 1` | `movi r2, 0` |
| 12 | `st.w r3, (r2, 0x180)` | `st.w r2, (r3, 0x0)` |
| 13 | `psrset ee, ie` | `mov r3, r2` |
| 14 | `jmp r15` | `lrw r2, 0xe000e280` |
| 15 | `bkpt ` | `subi r3, 1` |
| 16 | `addi r4, 1` | `st.w r3, (r2, 0x0)` |
| 17 | `bt 0x80071ea` | `bsr 0x0` |
| 18 | `bsr 0xa0231ec` | `pop r15` |
| 19 |  | `.long 0xe000e300` |
| 20 |  | `.long 0xe000e280` |

#### `board_init` (0x0800712C)

| # | Original | Recompiled |
|---|----------|------------|
| 0 | `push r15` | `push r15` |
| 1 | `lrw r2, 0x40011400` | `lrw r2, 0x40011400` |
| 2 | `movi r0, 49664` | `movi r0, 49664` |
| 3 | `bseti r0, 16` | `bseti r0, 16` |
| 4 | `ld.w r3, (r2, 0xc)` | `ld.w r3, (r2, 0xc)` |
| 5 | `bclri r3, 20` | `bclri r3, 20` |
| 6 | `st.w r3, (r2, 0xc)` | `st.w r3, (r2, 0xc)` |
| 7 | `bsr 0x80058b8` | `bsr 0x0` |
| 8 | `pop r15` | `pop r15` |
| 9 | `bkpt ` | `.short 0x0000` |
| 10 | `addi r14, r14, 0` | `.long 0x40011400` |
| 11 | `lsli r0, r0, 1` |  |

#### `lock_acquire` (0x080071AC)

| # | Original | Recompiled |
|---|----------|------------|
| 0 | `movi r0, 0` | `movi r0, 0` |
| 1 | `jmp r15` | `jmp r15` |

#### `lock_release` (0x080071B0)

| # | Original | Recompiled |
|---|----------|------------|
| 0 | `movi r0, 0` | `movi r0, 0` |
| 1 | `jmp r15` | `jmp r15` |

#### `tspend_handler` (0x08007148)

| # | Original | Recompiled |
|---|----------|------------|
| 0 | `nie ` | `subi r14, r14, 4` |
| 1 | `ipush ` | `lrw r3, 0xe000e000` |
| 2 | `subi r14, r14, 56` | `lrw r2, 0x20011380` |
| 3 | `stm r18-r31, (r14)` | `ld.w r3, (r3, 0x10)` |
| 4 | `subi r14, r14, 32` | `st.w r3, (r14, 0x0)` |
| 5 | `fstms fr16-fr19, (r14)` | `ld.w r3, (r14, 0x0)` |
| 6 | `lrw r2, 0x200113e4` | `ld.w r3, (r2, 0x64)` |
| 7 | `lrw r3, 0xe000e000` | `addi r3, 1` |
| 8 | `ld.w r3, (r3, 0x10)` | `st.w r3, (r2, 0x64)` |
| 9 | `ld.w r3, (r2, 0x0)` | `addi r14, r14, 4` |
| 10 | `addi r3, 1` | `jmp r15` |
| 11 | `st.w r3, (r2, 0x0)` | `.short 0x0000` |
| 12 | `fldms fr16-fr19, (r14)` | `.long 0xe000e000` |
| 13 | `addi r14, r14, 32` | `.long 0x20011380` |
| 14 | `ldm r18-r31, (r14)` |  |
| 15 | `addi r14, r14, 56` |  |
| 16 | `ipop ` |  |
| 17 | `nir ` |  |
| 18 | `lrw r7, 0x153814d4` |  |
| 19 | `addi r0, 2` |  |
| 20 | `bsr 0x8023178` |  |

## Detailed Disassembly / 详细反汇编

Original disassembly of key functions for verification against decompiled C.

### `SystemInit` (0x080071B4) — 19 instructions, 60 bytes
Source: `secboot_hw_init.c`

```asm
  080071b4:  6d10                  lrw          r3, 0x8002400
  080071b6:  216403c0              mtcr         r3, cr<1, 0>
  080071ba:  23601fc0              mfcr         r3, cr<31, 0>
  080071be:  100063ec              ori          r3, r3, 16
  080071c2:  3f6403c0              mtcr         r3, cr<31, 0>
  080071c6:  236000c0              mfcr         r3, cr<0, 0>
  080071ca:  000263ec              ori          r3, r3, 512
  080071ce:  206403c0              mtcr         r3, cr<0, 0>
  080071d2:  4710                  lrw          r2, 0xe000e100
  080071d4:  0033                  movi         r3, 0
  080071d6:  802062dc              st.w         r3, (r2, 0x200)
  080071da:  002b                  subi         r3, 1
  080071dc:  602062dc              st.w         r3, (r2, 0x180)
  080071e0:  207480c1              psrset       ee, ie
  080071e4:  3c78                  jmp          r15
  080071e6:  0000                  bkpt         
  080071e8:  0024                  addi         r4, 1
  080071ea:  0008                  bt           0x80071ea
  080071ec:  00e000e1              bsr          0xa0231ec
```

### `board_init` (0x0800712C) — 12 instructions, 28 bytes
Source: `secboot_hw_init.c`

```asm
  0800712c:  d014                  push         r15
  0800712e:  4610                  lrw          r2, 0x40011400
  08007130:  00c200ea              movi         r0, 49664
  08007134:  b038                  bseti        r0, 16
  08007136:  6392                  ld.w         r3, (r2, 0xc)
  08007138:  943b                  bclri        r3, 20
  0800713a:  63b2                  st.w         r3, (r2, 0xc)
  0800713c:  bef3ffe3              bsr          0x80058b8
  08007140:  9014                  pop          r15
  08007142:  0000                  bkpt         
  08007144:  0014                  addi         r14, r14, 0
  08007146:  0140                  lsli         r0, r0, 1
```

### `main` (0x08007304) — 528 instructions, 1240 bytes
Source: `secboot_main.c`

```asm
  08007304:  d414                  push         r4-r7, r15
  08007306:  3815                  subi         r14, r14, 224
  08007308:  0035                  movi         r5, 0
  0800730a:  004022ea              movih        r2, 16384
  0800730e:  ff0d42e4              addi         r2, r2, 3584
  08007312:  002d                  subi         r5, 1
  08007314:  a0b8                  st.w         r5, (r14, 0x0)
  08007316:  6592                  ld.w         r3, (r2, 0x14)
  08007318:  400063ec              ori          r3, r3, 64
  0800731c:  db12                  lrw          r6, 0x2001007c
  0800731e:  65b2                  st.w         r3, (r2, 0x14)
  08007320:  6096                  ld.w         r3, (r6, 0x0)
  08007322:  8493                  ld.w         r4, (r3, 0x10)
  08007324:  0af0ffe3              bsr          0x8005338
  08007328:  d17b                  jsr          r4
  0800732a:  a70020e9              bnez         r0, 0x8007478
  0800732e:  4032                  movi         r2, 64
  08007330:  2819                  addi         r1, r14, 160
  08007332:  002000ea              movi         r0, 8192
  08007336:  bb38                  bseti        r0, 27
  08007338:  e0f0ffe3              bsr          0x80054f8
  0800733c:  6096                  ld.w         r3, (r6, 0x0)
  0800733e:  0432                  movi         r2, 4
  08007340:  8393                  ld.w         r4, (r3, 0xc)
  08007342:  ff1f84e4              subi         r4, r4, 4096
  08007346:  bb3c                  bseti        r4, 27
  08007348:  7b6c                  mov          r1, r14
  0800734a:  136c                  mov          r0, r4
  0800734c:  d6f0ffe3              bsr          0x80054f8
  08007350:  68ffffe3              bsr          0x8007220
  08007354:  c36d                  mov          r7, r0
  08007356:  780020e9              bnez         r0, 0x8007446
  0800735a:  4032                  movi         r2, 64
  0800735c:  2819                  addi         r1, r14, 160
  0800735e:  002000ea              movi         r0, 8192
  08007362:  bb38                  bseti        r0, 27
  08007364:  caf0ffe3              bsr          0x80054f8
  08007368:  9f6c                  mov          r2, r7
  0800736a:  1819                  addi         r1, r14, 96
  0800736c:  002000ea              movi         r0, 8192
  08007370:  bb38                  bseti        r0, 27
  08007372:  83ffffe3              bsr          0x8007278
  08007376:  436d                  mov          r5, r0
  08007378:  0132                  movi         r2, 1
  0800737a:  0819                  addi         r1, r14, 32
  0800737c:  010820ea              movih        r0, 2049
  08007380:  7cffffe3              bsr          0x8007278
  08007384:  430045eb              cmpnei       r5, 67
  08007388:  540c                  bf           0x8007430
  0800738a:  430040eb              cmpnei       r0, 67
  ... (478 more instructions)
```

### `boot_uart_check` (0x08007220) — 40 instructions, 88 bytes
Source: `secboot_main.c`

```asm
  08007220:  d414                  push         r4-r7, r15
  08007222:  2114                  subi         r14, r14, 4
  08007224:  0035                  movi         r5, 0
  08007226:  030024ea              movih        r4, 3
  0800722a:  a0b8                  st.w         r5, (r14, 0x0)
  0800722c:  012c                  subi         r4, 2
  0800722e:  976d                  mov          r6, r5
  08007230:  0237                  movi         r7, 2
  08007232:  6098                  ld.w         r3, (r14, 0x0)
  08007234:  d164                  cmplt        r4, r3
  08007236:  0b08                  bt           0x800724c
  08007238:  2af3ffe3              bsr          0x800588c
  0800723c:  0b0020e9              bnez         r0, 0x8007252
  08007240:  6098                  ld.w         r3, (r14, 0x0)
  08007242:  0023                  addi         r3, 1
  08007244:  60b8                  st.w         r3, (r14, 0x0)
  08007246:  6098                  ld.w         r3, (r14, 0x0)
  08007248:  d164                  cmplt        r4, r3
  0800724a:  f70f                  bf           0x8007238
  0800724c:  0030                  movi         r0, 0
  0800724e:  0114                  addi         r14, r14, 4
  08007250:  9414                  pop          r4-r7, r15
  08007252:  27f3ffe3              bsr          0x80058a0
  08007256:  5b38                  cmpnei       r0, 27
  08007258:  030c                  bf           0x800725e
  0800725a:  0035                  movi         r5, 0
  0800725c:  eb07                  br           0x8007232
  0800725e:  0025                  addi         r5, 1
  08007260:  5475                  zextb        r5, r5
  08007262:  5c65                  cmphs        r7, r5
  08007264:  c0b8                  st.w         r6, (r14, 0x0)
  08007266:  e60b                  bt           0x8007232
  08007268:  6310                  lrw          r3, 0x200113ec
  0800726a:  0130                  movi         r0, 1
  0800726c:  c0a3                  st.b         r6, (r3, 0x0)
  0800726e:  0114                  addi         r14, r14, 4
  08007270:  9414                  pop          r4-r7, r15
  08007272:  0000                  bkpt         
  08007274:  ec13                  lrw          r7, 0x19083240
  08007276:  0120                  addi         r0, 2
```

### `image_header_verify` (0x080071F4) — 18 instructions, 44 bytes
Source: `secboot_main.c`

```asm
  080071f4:  d214                  push         r4-r5, r15
  080071f6:  036d                  mov          r4, r0
  080071f8:  476d                  mov          r5, r1
  080071fa:  81f5ffe3              bsr          0x8005cfc
  080071fe:  6a58                  addi         r3, r0, 3
  08007200:  013b                  cmphsi       r3, 2
  08007202:  070c                  bf           0x8007210
  08007204:  040080e9              blz          r0, 0x800720c
  08007208:  4330                  movi         r0, 67
  0800720a:  9214                  pop          r4-r5, r15
  0800720c:  5a30                  movi         r0, 90
  0800720e:  9214                  pop          r4-r5, r15
  08007210:  576c                  mov          r1, r5
  08007212:  136c                  mov          r0, r4
  08007214:  6cf3ffe3              bsr          0x80058ec
  08007218:  f8ff20e9              bnez         r0, 0x8007208
  0800721c:  4d30                  movi         r0, 77
  0800721e:  9214                  pop          r4-r5, r15
```

### `validate_image` (0x08005988) — 89 instructions, 212 bytes
Source: `secboot_image.c`

```asm
  08005988:  d114                  push         r4, r15
  0800598a:  2314                  subi         r14, r14, 12
  0800598c:  0033                  movi         r3, 0
  0800598e:  60b8                  st.w         r3, (r14, 0x0)
  08005990:  4090                  ld.w         r2, (r0, 0x0)
  08005992:  7011                  lrw          r3, 0xa0ffff9f
  08005994:  ca64                  cmpne        r2, r3
  08005996:  036d                  mov          r4, r0
  08005998:  040c                  bf           0x80059a0
  0800599a:  4c30                  movi         r0, 76
  0800599c:  0314                  addi         r14, r14, 12
  0800599e:  9114                  pop          r4, r15
  080059a0:  6680                  ld.b         r3, (r0, 0x6)
  080059a2:  012063e4              andi         r3, r3, 1
  080059a6:  400003e9              bez          r3, 0x8005a26
  080059aa:  6290                  ld.w         r3, (r0, 0x8)
  080059ac:  225080c7              bmaski       r2, 29
  080059b0:  c864                  cmphs        r2, r3
  080059b2:  090c                  bf           0x80059c4
  080059b4:  4811                  lrw          r2, 0x2001007c
  080059b6:  000821ea              movih        r1, 2048
  080059ba:  4092                  ld.w         r2, (r2, 0x0)
  080059bc:  4392                  ld.w         r2, (r2, 0xc)
  080059be:  8460                  addu         r2, r1
  080059c0:  8c64                  cmphs        r3, r2
  080059c2:  4408                  bt           0x8005a4a
  080059c4:  fbdf22ea              movih        r2, 57339
  080059c8:  af3a                  bseti        r2, 15
  080059ca:  285b                  addu         r1, r3, r2
  080059cc:  4311                  lrw          r2, 0xffb7fff
  080059ce:  4864                  cmphs        r2, r1
  080059d0:  3d08                  bt           0x8005a4a
  080059d2:  4394                  ld.w         r2, (r4, 0xc)
  080059d4:  390002e9              bez          r2, 0x8005a46
  080059d8:  002021ea              movih        r1, 8192
  080059dc:  4e60                  subu         r1, r3
  080059de:  4864                  cmphs        r2, r1
  080059e0:  2a0c                  bf           0x8005a34
  080059e2:  003021ea              movih        r1, 12288
  080059e6:  4e60                  subu         r1, r3
  080059e8:  4864                  cmphs        r2, r1
  080059ea:  0808                  bt           0x80059fa
  080059ec:  128001ea              movi         r1, 32786
  080059f0:  0149c1c5              rotli        r1, r1, 14
  080059f4:  6d59                  subu         r3, r1, r3
  080059f6:  c864                  cmphs        r2, r3
  080059f8:  2708                  bt           0x8005a46
  080059fa:  0333                  movi         r3, 3
  080059fc:  0031                  movi         r1, 0
  080059fe:  8f6c                  mov          r2, r3
  ... (39 more instructions)
```

### `uart_putchar` (0x0800717C) — 19 instructions, 48 bytes
Source: `secboot_uart.c`

```asm
  0800717c:  4a38                  cmpnei       r0, 10
  0800717e:  0c0c                  bf           0x8007196
  08007180:  4a10                  lrw          r2, 0x40010600
  08007182:  6792                  ld.w         r3, (r2, 0x1c)
  08007184:  3f2063e4              andi         r3, r3, 63
  08007188:  fdff23e9              bnez         r3, 0x8007182
  0800718c:  ff2000e4              andi         r0, r0, 255
  08007190:  08b2                  st.w         r0, (r2, 0x20)
  08007192:  0f6c                  mov          r0, r3
  08007194:  3c78                  jmp          r15
  08007196:  4510                  lrw          r2, 0x40010600
  08007198:  6792                  ld.w         r3, (r2, 0x1c)
  0800719a:  3f2063e4              andi         r3, r3, 63
  0800719e:  fdff23e9              bnez         r3, 0x8007198
  080071a2:  0d33                  movi         r3, 13
  080071a4:  68b2                  st.w         r3, (r2, 0x20)
  080071a6:  ed07                  br           0x8007180
  080071a8:  0006                  br           0x8006da8
  080071aa:  0140                  lsli         r0, r0, 1
```

### `uart_rx_ready` (0x0800588C) — 8 instructions, 20 bytes
Source: `secboot_uart.c`

```asm
  0800588c:  6410                  lrw          r3, 0x40010600
  0800588e:  6793                  ld.w         r3, (r3, 0x1c)
  08005890:  c02f63e4              andi         r3, r3, 4032
  08005894:  403b                  cmpnei       r3, 0
  08005896:  000500c4              mvc          r0
  0800589a:  3c78                  jmp          r15
  0800589c:  0006                  br           0x800549c
  0800589e:  0140                  lsli         r0, r0, 1
```

### `uart_getchar` (0x080058A0) — 10 instructions, 24 bytes
Source: `secboot_uart.c`

```asm
  080058a0:  4510                  lrw          r2, 0x40010600
  080058a2:  6792                  ld.w         r3, (r2, 0x1c)
  080058a4:  c02f63e4              andi         r3, r3, 4032
  080058a8:  fdff03e9              bez          r3, 0x80058a2
  080058ac:  0c92                  ld.w         r0, (r2, 0x30)
  080058ae:  0074                  zextb        r0, r0
  080058b0:  3c78                  jmp          r15
  080058b2:  0000                  bkpt         
  080058b4:  0006                  br           0x80054b4
  080058b6:  0140                  lsli         r0, r0, 1
```

### `uart_init` (0x080058B8) — 23 instructions, 52 bytes
Source: `secboot_uart.c`

```asm
  080058b8:  2440                  lsli         r1, r0, 4
  080058ba:  6b10                  lrw          r3, 0x2625a00
  080058bc:  428023c4              divs         r2, r3, r1
  080058c0:  208422c4              mult         r0, r2, r1
  080058c4:  c260                  subu         r3, r0
  080058c6:  0443                  lsli         r0, r3, 4
  080058c8:  408020c4              divs         r0, r0, r1
  080058cc:  6710                  lrw          r3, 0x40010600
  080058ce:  002a                  subi         r2, 1
  080058d0:  1040                  lsli         r0, r0, 16
  080058d2:  086c                  or           r0, r2
  080058d4:  c332                  movi         r2, 195
  080058d6:  04b3                  st.w         r0, (r3, 0x10)
  080058d8:  40b3                  st.w         r2, (r3, 0x0)
  080058da:  0032                  movi         r2, 0
  080058dc:  41b3                  st.w         r2, (r3, 0x4)
  080058de:  42b3                  st.w         r2, (r3, 0x8)
  080058e0:  43b3                  st.w         r2, (r3, 0xc)
  080058e2:  3c78                  jmp          r15
  080058e4:  005a                  addu         r0, r2, r0
  080058e6:  6202                  lrw          r3, 0x29c461
  080058e8:  0006                  br           0x80054e8
  080058ea:  0140                  lsli         r0, r0, 1
```

### `flash_init` (0x08005338) — 12 instructions, 32 bytes
Source: `secboot_flash.c`

```asm
  08005338:  002003ea              movi         r3, 8192
  0800533c:  be3b                  bseti        r3, 30
  0800533e:  9fc002ea              movi         r2, 49311
  08005342:  b13a                  bseti        r2, 17
  08005344:  40b3                  st.w         r2, (r3, 0x0)
  08005346:  000102ea              movi         r2, 256
  0800534a:  41b3                  st.w         r2, (r3, 0x4)
  0800534c:  004023ea              movih        r3, 16384
  08005350:  0093                  ld.w         r0, (r3, 0x0)
  08005352:  0074                  zextb        r0, r0
  08005354:  3c78                  jmp          r15
  08005356:  0000                  bkpt         
```

### `flash_read` (0x080054F8) — 27 instructions, 68 bytes
Source: `secboot_flash.c`

```asm
  080054f8:  d614                  push         r4-r9, r15
  080054fa:  684a                  lsri         r3, r2, 8
  080054fc:  436e                  mov          r9, r0
  080054fe:  ff2002e5              andi         r8, r2, 255
  08005502:  1b0003e9              bez          r3, 0x8005538
  08005506:  e843                  lsli         r7, r3, 8
  08005508:  bc59                  addu         r5, r1, r7
  0800550a:  076d                  mov          r4, r1
  0800550c:  c558                  subu         r6, r0, r1
  0800550e:  536c                  mov          r1, r4
  08005510:  185c                  addu         r0, r4, r6
  08005512:  0133                  movi         r3, 1
  08005514:  000102ea              movi         r2, 256
  08005518:  ff24                  addi         r4, 256
  0800551a:  67feffe3              bsr          0x80051e8
  0800551e:  5265                  cmpne        r4, r5
  08005520:  f70b                  bt           0x800550e
  08005522:  200027c5              addu         r0, r7, r9
  08005526:  070008e9              bez          r8, 0x8005534
  0800552a:  0133                  movi         r3, 1
  0800552c:  a36c                  mov          r2, r8
  0800552e:  576c                  mov          r1, r5
  08005530:  5cfeffe3              bsr          0x80051e8
  08005534:  0030                  movi         r0, 0
  08005536:  9614                  pop          r4-r9, r15
  08005538:  476d                  mov          r5, r1
  0800553a:  f607                  br           0x8005526
```

### `malloc` (0x080029A0) — 76 instructions, 156 bytes
Source: `secboot_memory.c`

```asm
  080029a0:  c314                  push         r4-r6
  080029a2:  036d                  mov          r4, r0
  080029a4:  7f6d                  mov          r5, r15
  080029a6:  0211                  lrw          r0, 0x2001142c
  080029a8:  022400e0              bsr          0x80071ac
  080029ac:  136c                  mov          r0, r4
  080029ae:  d76f                  mov          r15, r5
  080029b0:  2011                  lrw          r1, 0x20010060
  080029b2:  8091                  ld.w         r4, (r1, 0x0)
  080029b4:  403c                  cmpnei       r4, 0
  080029b6:  0a08                  bt           0x80029ca
  080029b8:  5f10                  lrw          r2, 0x20011430
  080029ba:  41b1                  st.w         r2, (r1, 0x4)
  080029bc:  80b2                  st.w         r4, (r2, 0x0)
  080029be:  7f10                  lrw          r3, 0x20028000
  080029c0:  ca60                  subu         r3, r2
  080029c2:  072b                  subi         r3, 8
  080029c4:  61b2                  st.w         r3, (r2, 0x4)
  080029c6:  0134                  movi         r4, 1
  080029c8:  80b1                  st.w         r4, (r1, 0x0)
  080029ca:  9a58                  addi         r4, r0, 7
  080029cc:  0730                  movi         r0, 7
  080029ce:  0169                  andn         r4, r0
  080029d0:  0030                  movi         r0, 0
  080029d2:  ae59                  addi         r5, r1, 4
  080029d4:  2191                  ld.w         r1, (r1, 0x4)
  080029d6:  0d04                  br           0x80029f0
  080029d8:  4191                  ld.w         r2, (r1, 0x4)
  080029da:  0865                  cmphs        r2, r4
  080029dc:  080c                  bf           0x80029ec
  080029de:  4038                  cmpnei       r0, 0
  080029e0:  030c                  bf           0x80029e6
  080029e2:  c864                  cmphs        r2, r3
  080029e4:  0408                  bt           0x80029ec
  080029e6:  cb6c                  mov          r3, r2
  080029e8:  976d                  mov          r6, r5
  080029ea:  076c                  mov          r0, r1
  080029ec:  476d                  mov          r5, r1
  080029ee:  2091                  ld.w         r1, (r1, 0x0)
  080029f0:  4039                  cmpnei       r1, 0
  080029f2:  f30b                  bt           0x80029d8
  080029f4:  4038                  cmpnei       r0, 0
  080029f6:  120c                  bf           0x8002a1a
  080029f8:  2190                  ld.w         r1, (r0, 0x4)
  080029fa:  5260                  subu         r1, r4
  080029fc:  2e39                  cmplti       r1, 15
  080029fe:  2090                  ld.w         r1, (r0, 0x0)
  08002a00:  0b08                  bt           0x8002a16
  08002a02:  5e5c                  addi         r2, r4, 8
  08002a04:  8060                  addu         r2, r0
  ... (26 more instructions)
```

### `free` (0x08002A3C) — 58 instructions, 120 bytes
Source: `secboot_memory.c`

```asm
  08002a3c:  c214                  push         r4-r5
  08002a3e:  036d                  mov          r4, r0
  08002a40:  7f6d                  mov          r5, r15
  08002a42:  1b10                  lrw          r0, 0x2001142c
  08002a44:  b42300e0              bsr          0x80071ac
  08002a48:  136c                  mov          r0, r4
  08002a4a:  d76f                  mov          r15, r5
  08002a4c:  4038                  cmpnei       r0, 0
  08002a4e:  290c                  bf           0x8002aa0
  08002a50:  5f58                  subi         r2, r0, 8
  08002a52:  6192                  ld.w         r3, (r2, 0x4)
  08002a54:  0c60                  addu         r0, r3
  08002a56:  9710                  lrw          r4, 0x20010064
  08002a58:  6094                  ld.w         r3, (r4, 0x0)
  08002a5a:  c264                  cmpne        r0, r3
  08002a5c:  0708                  bt           0x8002a6a
  08002a5e:  6190                  ld.w         r3, (r0, 0x4)
  08002a60:  2192                  ld.w         r1, (r2, 0x4)
  08002a62:  4c60                  addu         r1, r3
  08002a64:  0721                  addi         r1, 8
  08002a66:  21b2                  st.w         r1, (r2, 0x4)
  08002a68:  6090                  ld.w         r3, (r0, 0x0)
  08002a6a:  403b                  cmpnei       r3, 0
  08002a6c:  050c                  bf           0x8002a76
  08002a6e:  2193                  ld.w         r1, (r3, 0x4)
  08002a70:  0721                  addi         r1, 8
  08002a72:  4c60                  addu         r1, r3
  08002a74:  0204                  br           0x8002a78
  08002a76:  4f6c                  mov          r1, r3
  08002a78:  8664                  cmpne        r1, r2
  08002a7a:  0b08                  bt           0x8002a90
  08002a7c:  0193                  ld.w         r0, (r3, 0x4)
  08002a7e:  4191                  ld.w         r2, (r1, 0x4)
  08002a80:  0860                  addu         r0, r2
  08002a82:  0720                  addi         r0, 8
  08002a84:  01b3                  st.w         r0, (r3, 0x4)
  08002a86:  0032                  movi         r2, 0
  08002a88:  41b1                  st.w         r2, (r1, 0x4)
  08002a8a:  60b1                  st.w         r3, (r1, 0x0)
  08002a8c:  60b4                  st.w         r3, (r4, 0x0)
  08002a8e:  0904                  br           0x8002aa0
  08002a90:  8464                  cmphs        r1, r2
  08002a92:  0408                  bt           0x8002a9a
  08002a94:  40b4                  st.w         r2, (r4, 0x0)
  08002a96:  60b2                  st.w         r3, (r2, 0x0)
  08002a98:  0404                  br           0x8002aa0
  08002a9a:  0f6d                  mov          r4, r3
  08002a9c:  6093                  ld.w         r3, (r3, 0x0)
  08002a9e:  de07                  br           0x8002a5a
  08002aa0:  0310                  lrw          r0, 0x2001142c
  ... (8 more instructions)
```

### `sha1_init` (0x080045A4) — 24 instructions, 52 bytes
Source: `secboot_crypto.c`

```asm
  080045a4:  6810                  lrw          r3, 0x67452301
  080045a6:  62b0                  st.w         r3, (r0, 0x8)
  080045a8:  6810                  lrw          r3, 0xefcdab89
  080045aa:  63b0                  st.w         r3, (r0, 0xc)
  080045ac:  6810                  lrw          r3, 0x98badcfe
  080045ae:  64b0                  st.w         r3, (r0, 0x10)
  080045b0:  6810                  lrw          r3, 0x10325476
  080045b2:  65b0                  st.w         r3, (r0, 0x14)
  080045b4:  6810                  lrw          r3, 0xc3d2e1f0
  080045b6:  66b0                  st.w         r3, (r0, 0x18)
  080045b8:  0033                  movi         r3, 0
  080045ba:  67b0                  st.w         r3, (r0, 0x1c)
  080045bc:  60b0                  st.w         r3, (r0, 0x0)
  080045be:  61b0                  st.w         r3, (r0, 0x4)
  080045c0:  3c78                  jmp          r15
  080045c2:  0000                  bkpt         
  080045c4:  0123                  addi         r3, 2
  080045c6:  4567                  cmplt        r1, r13
  080045c8:  89ab                  st.h         r4, (r3, 0x12)
  080045ca:  fedccdef              ori          r30, r13, 56574
  080045ce:  ba98                  ld.w         r5, (r14, 0x68)
  080045d0:  7654                  asri         r3, r4, 22
  080045d2:  3210                  lrw          r1, 0x6c1b0bed
  080045d4:  d2c3f0e1              bsr          0xbe1cd78
```

### `hw_crypto_setup` (0x08004404) — 6 instructions, 12 bytes
Source: `secboot_crypto.c`

```asm
  08004404:  20b0                  st.w         r1, (r0, 0x0)
  08004406:  44a0                  st.b         r2, (r0, 0x4)
  08004408:  65a0                  st.b         r3, (r0, 0x5)
  0800440a:  0030                  movi         r0, 0
  0800440c:  3c78                  jmp          r15
  0800440e:  0000                  bkpt         
```

### `crc_verify_image` (0x08005CFC) — 98 instructions, 244 bytes
Source: `secboot_crypto.c`

```asm
  08005cfc:  d514                  push         r4-r8, r15
  08005cfe:  3115                  subi         r14, r14, 196
  08005d00:  836d                  mov          r6, r0
  08005d02:  0033                  movi         r3, 0
  08005d04:  000100ea              movi         r0, 256
  08005d08:  c76d                  mov          r7, r1
  08005d0a:  65b8                  st.w         r3, (r14, 0x14)
  08005d0c:  4ae6ffe3              bsr          0x80029a0
  08005d10:  036d                  mov          r4, r0
  08005d12:  660000e9              bez          r0, 0x8005dde
  08005d16:  436c                  mov          r1, r0
  08005d18:  000102ea              movi         r2, 256
  08005d1c:  1b6c                  mov          r0, r6
  08005d1e:  35ffffe3              bsr          0x8005b88
  08005d22:  436d                  mov          r5, r0
  08005d24:  0b0080e9              blz          r0, 0x8005d3a
  08005d28:  1b6c                  mov          r0, r6
  08005d2a:  0b1a                  addi         r2, r14, 44
  08005d2c:  5f6c                  mov          r1, r7
  08005d2e:  4dffffe3              bsr          0x8005bc8
  08005d32:  836d                  mov          r6, r0
  08005d34:  090000e9              bez          r0, 0x8005d46
  08005d38:  436d                  mov          r5, r0
  08005d3a:  136c                  mov          r0, r4
  08005d3c:  80e6ffe3              bsr          0x8002a3c
  08005d40:  176c                  mov          r0, r5
  08005d42:  1115                  addi         r14, r14, 196
  08005d44:  9514                  pop          r4-r8, r15
  08005d46:  dbf7ffe3              bsr          0x8004cfc
  08005d4a:  c36d                  mov          r7, r0
  08005d4c:  4c0000e9              bez          r0, 0x8005de4
  08005d50:  576c                  mov          r1, r5
  08005d52:  031a                  addi         r2, r14, 12
  08005d54:  0518                  addi         r0, r14, 20
  08005d56:  85b8                  st.w         r4, (r14, 0x14)
  08005d58:  aef6ffe3              bsr          0x8004ab4
  08005d5c:  176e                  mov          r8, r5
  08005d5e:  436d                  mov          r5, r0
  08005d60:  360080e9              blz          r0, 0x8005dcc
  08005d64:  2598                  ld.w         r1, (r14, 0x14)
  08005d66:  1062                  addu         r8, r4
  08005d68:  031b                  addi         r3, r14, 12
  08005d6a:  041a                  addi         r2, r14, 16
  08005d6c:  810028c4              subu         r1, r8, r1
  08005d70:  0518                  addi         r0, r14, 20
  08005d72:  1ff7ffe3              bsr          0x8004bb0
  08005d76:  436d                  mov          r5, r0
  08005d78:  2a0080e9              blz          r0, 0x8005dcc
  08005d7c:  4598                  ld.w         r2, (r14, 0x14)
  08005d7e:  6097                  ld.w         r3, (r7, 0x0)
  ... (48 more instructions)
```

### `find_valid_image` (0x08007278) — 54 instructions, 140 bytes
Source: `secboot_image.c`

```asm
  08007278:  d514                  push         r4-r8, r15
  0800727a:  036d                  mov          r4, r0
  0800727c:  476d                  mov          r5, r1
  0800727e:  8b6d                  mov          r6, r2
  08007280:  2f0000e9              bez          r0, 0x80072de
  08007284:  275040c7              bmaski       r7, 27
  08007288:  1e0088ea              lrw          r8, 0x2001007c
  0800728c:  1404                  br           0x80072b4
  0800728e:  6395                  ld.w         r3, (r5, 0xc)
  08007290:  3f23                  addi         r3, 64
  08007292:  0c61                  addu         r4, r3
  08007294:  6585                  ld.b         r3, (r5, 0x5)
  08007296:  012063e4              andi         r3, r3, 1
  0800729a:  030003e9              bez          r3, 0x80072a0
  0800729e:  7f24                  addi         r4, 128
  080072a0:  1c65                  cmphs        r7, r4
  080072a2:  1e08                  bt           0x80072de
  080072a4:  002068d8              ld.w         r3, (r8, 0x0)
  080072a8:  000822ea              movih        r2, 2048
  080072ac:  6393                  ld.w         r3, (r3, 0xc)
  080072ae:  c860                  addu         r3, r2
  080072b0:  d064                  cmphs        r4, r3
  080072b2:  1608                  bt           0x80072de
  080072b4:  4032                  movi         r2, 64
  080072b6:  576c                  mov          r1, r5
  080072b8:  136c                  mov          r0, r4
  080072ba:  1ff1ffe3              bsr          0x80054f8
  080072be:  176c                  mov          r0, r5
  080072c0:  64f3ffe3              bsr          0x8005988
  080072c4:  430040eb              cmpnei       r0, 67
  080072c8:  0d08                  bt           0x80072e2
  080072ca:  6485                  ld.b         r3, (r5, 0x4)
  080072cc:  0f2063e4              andi         r3, r3, 15
  080072d0:  413b                  cmpnei       r3, 1
  080072d2:  0a0c                  bf           0x80072e6
  080072d4:  ddff26e9              bnez         r6, 0x800728e
  080072d8:  8e95                  ld.w         r4, (r5, 0x38)
  080072da:  1c65                  cmphs        r7, r4
  080072dc:  e40f                  bf           0x80072a4
  080072de:  4a30                  movi         r0, 74
  080072e0:  9514                  pop          r4-r8, r15
  080072e2:  4c30                  movi         r0, 76
  080072e4:  9514                  pop          r4-r8, r15
  080072e6:  080006e9              bez          r6, 0x80072f6
  080072ea:  3f0024e4              addi         r1, r4, 64
  080072ee:  176c                  mov          r0, r5
  080072f0:  82ffffe3              bsr          0x80071f4
  080072f4:  9514                  pop          r4-r8, r15
  080072f6:  2295                  ld.w         r1, (r5, 0x8)
  080072f8:  176c                  mov          r0, r5
  ... (4 more instructions)
```

### `tspend_handler` (0x08007148) — 21 instructions, 52 bytes
Source: `secboot_main.c`

```asm
  08007148:  6014                  nie          
  0800714a:  6214                  ipush        
  0800714c:  2e14                  subi         r14, r14, 56
  0800714e:  2d1c4ed6              stm          r18-r31, (r14)
  08007152:  2814                  subi         r14, r14, 32
  08007154:  0034eef4              fstms        fr16-fr19, (r14)
  08007158:  4710                  lrw          r2, 0x200113e4
  0800715a:  6810                  lrw          r3, 0xe000e000
  0800715c:  6493                  ld.w         r3, (r3, 0x10)
  0800715e:  6092                  ld.w         r3, (r2, 0x0)
  08007160:  0023                  addi         r3, 1
  08007162:  60b2                  st.w         r3, (r2, 0x0)
  08007164:  0030eef4              fldms        fr16-fr19, (r14)
  08007168:  0814                  addi         r14, r14, 32
  0800716a:  2d1c4ed2              ldm          r18-r31, (r14)
  0800716e:  0e14                  addi         r14, r14, 56
  08007170:  6314                  ipop         
  08007172:  6114                  nir          
  08007174:  e413                  lrw          r7, 0x153814d4
  08007176:  0120                  addi         r0, 2
  08007178:  00e000e0              bsr          0x8023178
```

### `signature_verify` (0x080070C4) — 48 instructions, 104 bytes
Source: `secboot_image.c`

```asm
  080070c4:  d314                  push         r4-r6, r15
  080070c6:  7110                  lrw          r3, 0x200113fc
  080070c8:  0034                  movi         r4, 0
  080070ca:  80b3                  st.w         r4, (r3, 0x0)
  080070cc:  d010                  lrw          r6, 0x200113e0
  080070ce:  7110                  lrw          r3, 0x20011424
  080070d0:  b110                  lrw          r5, 0x20011418
  080070d2:  80b3                  st.w         r4, (r3, 0x0)
  080070d4:  40b6                  st.w         r2, (r6, 0x0)
  080070d6:  80b5                  st.w         r4, (r5, 0x0)
  080070d8:  7010                  lrw          r3, 0x200113f8
  080070da:  80b3                  st.w         r4, (r3, 0x0)
  080070dc:  7010                  lrw          r3, 0x20011414
  080070de:  80b3                  st.w         r4, (r3, 0x0)
  080070e0:  7010                  lrw          r3, 0x2001140c
  080070e2:  80b3                  st.w         r4, (r3, 0x0)
  080070e4:  7010                  lrw          r3, 0x20011410
  080070e6:  20b3                  st.w         r1, (r3, 0x0)
  080070e8:  7010                  lrw          r3, 0x2001141c
  080070ea:  00b3                  st.w         r0, (r3, 0x0)
  080070ec:  136c                  mov          r0, r4
  080070ee:  f3f8ffe3              bsr          0x80062d4
  080070f2:  b3ffffe3              bsr          0x8007058
  080070f6:  4095                  ld.w         r2, (r5, 0x0)
  080070f8:  235040c7              bmaski       r3, 27
  080070fc:  8c64                  cmphs        r3, r2
  080070fe:  000500c4              mvc          r0
  08007102:  015c                  subu         r0, r4, r0
  08007104:  80b6                  st.w         r4, (r6, 0x0)
  08007106:  9314                  pop          r4-r6, r15
  08007108:  fc13                  lrw          r7, 0xe3ff6c17
  0800710a:  0120                  addi         r0, 2
  0800710c:  e013                  lrw          r7, 0x95630414
  0800710e:  0120                  addi         r0, 2
  08007110:  2414                  subi         r14, r14, 16
  08007112:  0120                  addi         r0, 2
  08007114:  1814                  addi         r14, r14, 96
  08007116:  0120                  addi         r0, 2
  08007118:  f813                  lrw          r7, 0xe3ff6c17
  0800711a:  0120                  addi         r0, 2
  0800711c:  1414                  addi         r14, r14, 80
  0800711e:  0120                  addi         r0, 2
  08007120:  0c14                  addi         r14, r14, 48
  08007122:  0120                  addi         r0, 2
  08007124:  1014                  addi         r14, r14, 64
  08007126:  0120                  addi         r0, 2
  08007128:  1c14                  addi         r14, r14, 112
  0800712a:  0120                  addi         r0, 2
```

## Address Map / 地址映射

Complete mapping of all 116 identified functions with their addresses,
sizes, and decompilation status.

- Code region: `0x08002506` — `0x080077DC`
- Total function code: 21206 bytes
- Decompiled code: 20586 bytes (97%)

## Constant Data / 常量数据

The region `0x08007500` — `0x080077DC` (732 bytes)
contains constant data including:

- Literal pool entries (32-bit constants loaded via `lrw`)
- String constants (boot messages)
- CRC32 lookup table (256 × 4 bytes = 1024 bytes)
- Padding/alignment bytes

### String Constants Found / 发现的字符串常量

```
  0x080075F4: "Secboot V1.1"
```
