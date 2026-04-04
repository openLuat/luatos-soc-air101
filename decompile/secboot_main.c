/**
 * secboot_main.c - Decompiled main boot logic
 *
 * Pseudo-C reconstruction from C-SKY XT804 disassembly of secboot.bin
 * Cross-referenced with: include/platform/wm_fwup.h, include/driver/wm_flash_map.h,
 *                         tools/xt804/wm_tool.c
 *
 * Functions:
 *   main()              - 0x08007304  (top-level boot logic)
 *
 * Note: boot_uart_check is defined in secboot_uart.c,
 *       image_header_verify is defined in secboot_image.c,
 *       tspend_handler is defined in secboot_vectors.S.
 */

#include "secboot_common.h"

/* Local aliases for main-specific SRAM addresses */
#define WDG_CTRL            WDG_CTRL_REG

/* Flash addresses */
#define FLASH_BASE_OTP      0x08000000
#define IMAGE_AREA_ADDR     (0x2000 | (1 << 27))   /* 0x08002000 -> bseti r0,27 = 0x08002000 */
#define FOTA_HEADER_ADDR    (0x080D0000)            /* movih(2049) = secondary partition */
#define USER_IMAGE_ADDR     IMAGE_AREA_ADDR

/* Image header offsets (byte offsets into header buffer, used in disassembly) */
#define HDR_IMG_ATTR_OFF    0x04
#define HDR_SIGNATURE_BIT   0x26    /* byte offset in header buf for signature flag */
#define HDR_ENCRYPT_BIT     0x25    /* byte offset for encryption flag */
#define HDR_IMG_ADDR_OFF    0x28
#define HDR_IMG_LEN_OFF     0x2C
#define HDR_SIG_ADDR_OFF    0x30
#define HDR_NEXT_OFF        0x38

/* boot_uart_check, tspend_handler, and image_header_verify are defined
 * in secboot_uart.c, secboot_vectors.S, and secboot_image.c respectively.
 * They are declared in secboot_common.h as extern. */

/* ============================================================
 * main (0x08007304)
 *
 * Top-level boot logic. Called from Reset_Handler after
 * SystemInit and board_init.
 *
 * Stack frame: 224 bytes (subi r14, 224)
 *   sp+0x00 (  0): fota_update_no  (persistent update counter)
 *   sp+0x04 (  4): boot_params[0]  → flash_param.total_size
 *   sp+0x08 (  8): boot_params[1]  → img_addr
 *   sp+0x0C ( 12): boot_params[2]  → img_len
 *   sp+0x10 ( 16): boot_params[3]  → next header ptr
 *   sp+0x14 ( 20): boot_params[4]  → fota upd_no from primary
 *   sp+0x18 ( 24): boot_params[5]  → fota upd_no from secondary
 *   sp+0x1C ( 28): boot_params[6]  → fota upd_no from secondary+4
 *   sp+0x20 ( 32): secondary_hdr[64]  (FOTA header buffer)
 *   sp+0x60 ( 96): primary_hdr[64]    (main image header buffer)
 *   sp+0xA0 (160): image_hdr_raw[64]  (IMAGE_HEADER area buffer)
 *
 * Registers:
 *   r4 = fota_flash_addr (param->total_size - 4096, with bit27 set)
 *   r5 = status code (return value)
 *   r6 = &PARAM_BLOCK_PTR (0x2001007C)
 *   r7 = uart_detected flag
 *
 * Flow:
 *   1. Enable watchdog
 *   2. Call flash_init via param->callback
 *   3. Read IMAGE_HEADER (64 bytes) from flash at 0x08002000
 *   4. Compute FOTA flash address = param->total_size - 0x1000
 *   5. Read 4 bytes of FOTA header from fota_flash_addr
 *   6. Call boot_uart_check()
 *   7. If UART detected → print boot message, go to UART handler
 *   8. If no UART:
 *      a. Re-read IMAGE_HEADER from 0x08002000
 *      b. find_valid_image(0x08002000, primary_hdr, flags=0)
 *      c. find_valid_image(0x08010000, secondary_hdr, flags=1)
 *      d. Compare results, choose best image
 *      e. Optional: check signature bit → signature_verify()
 *      f. Check/update FOTA counter
 *      g. image_header_verify()
 *      h. Build boot_params structure
 *      i. Call app entry via APP_ENTRY_PTR
 *   9. On any failure: set status code, call ERROR_HANDLER_PTR
 *
 * Disassembly: see secboot_annotated.S for full listing
 * ============================================================ */
int main(void)
{
    uint8_t stack_frame[224];
    /* Overlay the stack frame with typed views */
    uint32_t *fota_update_no = (uint32_t *)&stack_frame[0];
    uint32_t *boot_params    = (uint32_t *)&stack_frame[4];
    uint8_t  *secondary_hdr  = &stack_frame[32];
    uint8_t  *primary_hdr    = &stack_frame[96];
    uint8_t  *image_hdr_raw  = &stack_frame[160];

    int status = 0;
    uint32_t fota_flash_addr;
    int uart_detected;

    /* ---- Step 1: Initialize watchdog ---- */
    /*
     * r2 = 0x40000000 + 0xE00 = 0x40000E00  (WDG base)
     * r3 = *(r2 + 0x14)  ; read WDG_CTRL
     * r3 |= 0x40          ; set bit 6 (enable)
     * *(r2 + 0x14) = r3   ; write back
     */
    *fota_update_no = 0xFFFFFFFF;  /* sentinel: no update pending */
    WDG_CTRL |= 0x40;

    /* ---- Step 2: Flash init ---- */
    /*
     * r6 = *(0x2001007C)   ; param block pointer
     * r4 = *(r6 + 0x10)    ; callback function pointer (flash_init_fn)
     * bsr flash_init        ; 0x08005338
     * jsr r4                ; call param's flash init callback
     * bnez r0 → error 'N'
     */
    volatile uint32_t *param_block = (volatile uint32_t *)PARAM_BLOCK_PTR;
    typedef int (*flash_init_fn_t)(void);
    flash_init_fn_t flash_init_fn = (flash_init_fn_t)param_block[4]; /* offset 0x10 */
    flash_init();
    if (flash_init_fn() != 0) {
        status = STATUS_FLASH_FAIL;  /* 'N' = 78 */
        goto error_exit;
    }

    /* ---- Step 3: Read IMAGE_HEADER area (64 bytes from 0x08002000) ---- */
    /*
     * r0 = 0x2000 | bseti(27) = 0x08002000
     * r1 = sp + 160 (image_hdr_raw)
     * r2 = 64
     * bsr flash_read  ; 0x080054F8
     */
    flash_read(0x08002000, image_hdr_raw, 64);

    /* ---- Step 4: Compute FOTA flash address ---- */
    /*
     * r3 = *(param_block + 0x0C) ; total flash size
     * r4 = r3 - 4096
     * r4 = bseti(r4, 27)  ; set flash base bit
     */
    fota_flash_addr = param_block[3] - 0x1000; /* offset 0x0C = total_size */
    fota_flash_addr |= (1 << 27);

    /* ---- Step 5: Read 4 bytes of FOTA update header ---- */
    /*
     * r0 = fota_flash_addr
     * r1 = sp (fota_update_no)
     * r2 = 4
     * bsr flash_read
     */
    flash_read(fota_flash_addr, fota_update_no, 4);

    /* ---- Step 6: Check for UART boot request ---- */
    /*
     * bsr boot_uart_check  ; 0x08007220
     * r7 = r0
     * bnez r0 → uart_mode (0x8007446)
     */
    uart_detected = boot_uart_check();
    if (uart_detected) {
        goto uart_mode;
    }

    /* ---- Step 7: Normal boot path ---- */

    /* Re-read IMAGE_HEADER from 0x08002000 (may have changed if FOTA) */
    flash_read(0x08002000, image_hdr_raw, 64);

    /* Find valid image in primary partition */
    /*
     * r0 = 0x08002000  ; primary partition
     * r1 = sp + 96     ; primary_hdr buffer
     * r2 = r7 (= 0)    ; flags = no UART
     * bsr find_valid_image  ; 0x08007278
     * r5 = r0           ; primary status
     */
    status = find_valid_image(0x08002000, primary_hdr, 0);

    /* Find valid image in secondary (FOTA) partition */
    /*
     * r0 = movih(2049) = 0x08010000
     * r1 = sp + 32     ; secondary_hdr buffer
     * r2 = 1           ; flags = FOTA candidate
     * bsr find_valid_image
     */
    int fota_status = find_valid_image(0x08010000, secondary_hdr, 1);

    /* ---- Step 8: Choose which image to boot ---- */
    /*
     * cmpnei r5, 'C'    ; primary valid?
     * bf     both_valid_check (0x8007430)
     * cmpnei r0, 'C'    ; secondary valid?
     * bt     error_exit  ; neither valid
     *
     * At 0x8007430 (both_valid_check):
     *   cmpnei r0, 'C'
     *   bt     use_primary (0x80073D2)
     *   ; Both valid: compare update sequence numbers
     *   r3 = *(sp + 0x38)  ; primary next ptr
     *   r2 = *(sp + 0x78)  ; secondary next ptr
     *   cmpne r2, r3
     *   bf    compare_upd_no (0x800746E)
     *   ; ... compare fota_update_no with sequence ...
     */
    if (status != STATUS_OK) {
        /* Primary not valid */
        if (fota_status != STATUS_OK)
            goto error_exit;    /* Neither valid */

        /* Use secondary image: copy its header to primary_hdr */
        /* Check signature bit on secondary image header */
        goto check_signature;
    }

    if (fota_status == STATUS_OK) {
        /* Both valid: compare update sequence numbers */
        uint32_t primary_next = *(uint32_t *)&primary_hdr[0x38 - 96 + 96];
        uint32_t secondary_next = *(uint32_t *)&secondary_hdr[0x78 - 32 + 32 - 64];
        /* Simplified: compare fields to decide which is newer */
        if (primary_next == secondary_next) {
            /* Compare upd_no fields */
            uint32_t primary_upd = *(uint32_t *)&primary_hdr[0x9C - 96 + 96 - 96];
            uint32_t secondary_upd = *(uint32_t *)&secondary_hdr[0x5C - 32 + 32 - 32];
            if (primary_upd == secondary_upd) {
                goto use_primary;
            }
            /* Fall through to check update counter */
        }
        if (*fota_update_no == *(uint32_t *)&primary_hdr[0x38 - 96 + 96]) {
            goto use_primary;
        }
        /* Use secondary - fall through to signature check */
    }

use_primary:
    /* ---- Step 9: Check signature if required ---- */
check_signature:
    /*
     * ld.b  r3, (sp + 0x26)   ; img_attr signature bit
     * andi  r3, r3, 1
     * bez   r3, read_unsigned_image  ; no signature -> skip
     *
     * ; Signed image path:
     * r0 = *(sp + 0x34) + 64    ; signature address (header addr + 64)
     * r1 = *(sp + 0x2C)         ; image length
     * r2 = sp + 96              ; primary_hdr buffer
     * bsr signature_verify       ; 0x080070C4
     * blz r0 → sig_fail ('M')
     */
    {
        uint8_t sig_flag = primary_hdr[0x26 - 96 + 96];
        if (sig_flag & 0x01) {
            /* Signature verification required */
            uint32_t header_addr = *(uint32_t *)&primary_hdr[0x34 - 96 + 96];
            uint32_t img_len     = *(uint32_t *)&primary_hdr[0x2C - 96 + 96];
            int sig_result = signature_verify(header_addr + 64, img_len, primary_hdr);
            if (sig_result < 0) {
                status = STATUS_SIG_FAIL;  /* 'M' = 77 */
                goto error_exit;
            }
        } else {
            /* ---- Read unsigned image header data ---- */
            /*
             * At 0x08007400:
             * r1 = *(sp + 0x34)  ; img_header_addr
             * r0 = *(sp + 0x30)  ; img_addr
             * r2 = 64
             * bsr flash_read_raw ; 0x0800582C
             *
             * Then read image body:
             * r0 = *(sp + 0x28)  ; img load addr
             * r1 = *(sp + 0x34) + 64
             * r2 = *(sp + 0x2C)  ; img_len
             * Check encrypt bit: if set, r2 += 128
             * bsr flash_read_raw
             *
             * Copy secondary_hdr to primary_hdr:
             * memcpy(primary_hdr, secondary_hdr, 64)
             * br back to fota_check
             */
            uint32_t header_addr = *(uint32_t *)&primary_hdr[0x34 - 96 + 96];
            uint32_t img_addr    = *(uint32_t *)&primary_hdr[0x30 - 96 + 96];
            flash_read_raw(img_addr, header_addr, 64);

            uint8_t encrypt_flag = primary_hdr[0x25 - 96 + 96];
            uint32_t load_addr   = *(uint32_t *)&primary_hdr[0x28 - 96 + 96];
            uint32_t img_len     = *(uint32_t *)&primary_hdr[0x2C - 96 + 96];
            if (encrypt_flag & 0x01)
                img_len += 128;  /* signature data appended */
            flash_read_raw(load_addr, header_addr + 64, img_len);

            /* Copy headers */
            memcpy(primary_hdr, secondary_hdr, 64);
        }
    }

    /* ---- Step 10: Check/update FOTA counter ---- */
    /*
     * At 0x080073AC:
     * r2 = *(sp + 0x00)       ; fota_update_no
     * r3 = -1
     * cmpne r2, r3            ; if fota_update_no != 0xFFFFFFFF
     *   r2 = 4
     *   r1 = sp
     *   r0 = fota_flash_addr
     *   st.w r3, (sp, 0x0)   ; fota_update_no = -1 (clear)
     *   bsr flash_write        ; 0x0800553C
     */
    if (*fota_update_no != 0xFFFFFFFF) {
        *fota_update_no = 0xFFFFFFFF;  /* Clear FOTA pending flag */
        flash_write(fota_flash_addr, fota_update_no, 4);
    }

    /* ---- Step 11: Verify image CRC ---- */
    /*
     * r1 = *(sp + 0x68)       ; primary_hdr img_len
     * r0 = sp + 96            ; primary_hdr buffer
     * bsr image_header_verify  ; 0x080071F4
     * cmpnei r0, 'C'
     * r5 = r0
     * bt error_exit
     */
    {
        uint32_t img_len = *(uint32_t *)&primary_hdr[0x68 - 96 + 96];
        status = image_header_verify(primary_hdr, img_len);
        if (status != STATUS_OK)
            goto error_exit;
    }

    /* ---- Step 12: Build boot params and jump to app ---- */
    /*
     * r3 = *(param_block + 0x0C) ; total_size
     * st.w r3, (sp, 0x04)
     * ... load fields from primary_hdr into boot_params ...
     *
     * r3 = *(0x200101D0)        ; APP_ENTRY_PTR
     * r0 = sp + 4               ; &boot_params
     * jsr r3                    ; call app entry
     * bnez r0 → app_fail 'Y'
     */
    boot_params[0] = param_block[3];                            /* total_size */
    boot_params[1] = *(uint32_t *)&primary_hdr[0x64 - 96 + 96]; /* img_addr */
    boot_params[2] = *(uint32_t *)&primary_hdr[0x68 - 96 + 96]; /* img_len */
    boot_params[3] = *(uint32_t *)&primary_hdr[0x6C - 96 + 96]; /* next hdr */
    boot_params[4] = *(uint32_t *)&primary_hdr[0xA4 - 96 + 96]; /* fota upd_no */
    boot_params[5] = *(uint32_t *)&primary_hdr[0xA8 - 96 + 96]; /* fota upd_no2 */
    boot_params[6] = *(uint32_t *)&primary_hdr[0xAC - 96 + 96]; /* fota upd_no3 */

    {
        typedef int (*app_entry_t)(uint32_t *params);
        app_entry_t app_entry = (app_entry_t)APP_ENTRY_PTR;
        int result = app_entry(boot_params);
        if (result != 0) {
            status = STATUS_APP_FAIL;  /* 'Y' = 89 */
            goto error_exit;
        }
    }

    /* Success: return from main */
    return 0;

uart_mode:
    /* ---- UART bootloader mode ---- */
    /*
     * At 0x08007446:
     * lrw r0, boot_message_str  ; 0x080075F4
     * bsr puts                   ; 0x080028F4
     *
     * ; Check if FOTA update was pending
     * ld.w r3, (sp, 0x0)        ; fota_update_no
     * cmpne r3, r5              ; compare with saved
     * bf    uart_no_fota
     *
     * ; Write cleared fota_update_no
     * r2 = 4
     * r1 = sp
     * r0 = fota_flash_addr
     * st.w r5, (sp, 0x0)       ; clear counter
     * bsr flash_write
     *
     * uart_no_fota:
     * r5 = 'C'
     * fall through to error_exit  (which calls ERROR_HANDLER_PTR)
     */
    puts((const char *)0x080075F4);  /* Print boot message */
    if (*fota_update_no != (uint32_t)status) {
        *fota_update_no = status;
        flash_write(fota_flash_addr, fota_update_no, 4);
    }
    status = STATUS_OK;
    /* Fall through to error_exit to invoke handler */

error_exit:
    /* ---- Call error/result handler ---- */
    /*
     * At 0x08007460:
     * lrw r3, 0x200101D4
     * mov r0, r5               ; status code
     * ld.w r3, (r3, 0x0)
     * jsr r3                   ; call error handler
     * movi r0, 0
     * subi r0, 1               ; return -1
     */
    {
        typedef void (*error_handler_t)(int status);
        error_handler_t handler = (error_handler_t)ERROR_HANDLER_PTR;
        handler(status);
    }
    return -1;
}
