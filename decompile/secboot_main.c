/**
 * secboot_main.c - Decompiled main boot logic
 *
 * Strict instruction-level reconstruction from C-SKY XT804 disassembly
 * of secboot.bin main() at 0x08007304 (0x1FC = 508 bytes).
 *
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

/* ============================================================
 * main (0x08007304)
 *
 * Top-level boot logic. Called from Reset_Handler after
 * SystemInit and board_init.
 *
 * Stack frame: 224 bytes (push r4-r7,r15; subi r14, 224)
 *   sp+0x00 (  0): fota_update_no  (uint32_t, FOTA update counter)
 *   sp+0x04 (  4): boot_params[0]  → total_flash_size
 *   sp+0x08 (  8): boot_params[1]  → primary_hdr.img_attr.w
 *   sp+0x0C ( 12): boot_params[2]  → primary_hdr.img_addr
 *   sp+0x10 ( 16): boot_params[3]  → primary_hdr.img_len
 *   sp+0x14 ( 20): boot_params[4]  → image_hdr_raw.img_attr.w
 *   sp+0x18 ( 24): boot_params[5]  → image_hdr_raw.img_addr
 *   sp+0x1C ( 28): boot_params[6]  → image_hdr_raw.img_len
 *   sp+0x20 ( 32): secondary_hdr   (image_header_t, 64 bytes, FOTA image)
 *   sp+0x60 ( 96): primary_hdr     (image_header_t, 64 bytes, main image)
 *   sp+0xA0 (160): image_hdr_raw   (image_header_t, 64 bytes, original)
 *
 * Callee-saved registers:
 *   r4 = fota_flash_addr (total_size - 0x1000, with bit27 set)
 *   r5 = status code (ASCII char: 'C','N','Y','M')
 *   r6 = param_block_ptr (loaded from *0x2001007C)
 *   r7 = uart_detected flag (0 = no UART)
 *
 * Boot flow:
 *   1. Enable watchdog (WDG_BASE+0x14 |= 0x40)
 *   2. flash_init() + param callback, error 'N' on fail
 *   3. Read IMAGE_HEADER (64 bytes) from 0x08002000 → image_hdr_raw
 *   4. Compute fota_flash_addr = total_size - 0x1000 | (1<<27)
 *   5. Read 4-byte FOTA counter from fota_flash_addr
 *   6. boot_uart_check() → UART mode or normal boot
 *   7. Normal: re-read header, find_valid_image() for primary & secondary
 *   8. Choose image:
 *      - Both valid: compare next/hd_checksum + fota_update_no
 *      - Only secondary valid: use secondary
 *      - Only primary valid: use primary
 *      - Neither: error_exit with primary's status
 *   9. Use secondary path (use_secondary):
 *      - If img_attr bit 16 set: signature_verify(), write to primary_hdr
 *      - If img_attr bit 16 clear: flash_copy_data() header+body,
 *        memcpy secondary→primary hdr, then fota_counter_check
 *  10. Clear FOTA counter if pending (fota_counter_check)
 *  11. image_header_verify(primary_hdr)
 *  12. Build boot_params, call app_entry via *0x200101D0
 *  13. On any failure: call error handler via *0x200101D4, return -1
 *
 * UART mode:
 *   - Print boot message via puts()
 *   - Clear FOTA counter if pending
 *   - Call error handler with 'C' status (handler does UART comm)
 * ============================================================ */
int main(void)
{
    /*
     * Stack variables - ordered to match the sp-relative layout.
     * The compiler may reorder these, but field access via typed structs
     * ensures correct offsets regardless.
     */
    uint32_t fota_update_no;              /* sp+0x00 */
    uint32_t boot_params[7];              /* sp+0x04 .. sp+0x1C */
    image_header_t secondary_hdr;         /* sp+0x20 (64 bytes) */
    image_header_t primary_hdr;           /* sp+0x60 (64 bytes) */
    image_header_t image_hdr_raw;         /* sp+0xA0 (64 bytes) */

    int status;                           /* r5 */
    uint32_t fota_flash_addr;             /* r4 */
    int uart_detected;                    /* r7 */
    volatile uint32_t *param_block_ptr;   /* r6 */

    /* ---- 0x08007304: push r4-r7,r15; subi r14, 224 ---- */

    /* ---- 0x08007308: Step 1: Initialize ---- */
    /*
     * 8007308: movi  r5, 0
     * 800730a: movih r2, 16384       ; r2 = 0x40000000
     * 800730e: addi  r2, r2, 3584    ; r2 = 0x40000E00 (WDG_BASE)
     * 8007312: subi  r5, 1           ; r5 = 0xFFFFFFFF
     * 8007314: st.w  r5, (r14, 0x0)  ; fota_update_no = 0xFFFFFFFF
     * 8007316: ld.w  r3, (r2, 0x14)  ; read WDG_CTRL
     * 8007318: ori   r3, r3, 64      ; set bit 6 (enable)
     * 800731c: lrw   r6, 0x2001007c  ; load param block pointer address
     * 800731e: st.w  r3, (r2, 0x14)  ; write WDG_CTRL
     */
    fota_update_no = 0xFFFFFFFF;
    WDG_CTRL_REG |= 0x40;

    /* ---- 0x08007320: Step 2: Flash init ---- */
    /*
     * 8007320: ld.w  r3, (r6, 0x0)   ; r3 = *PARAM_BLOCK_PTR
     * 8007322: ld.w  r4, (r3, 0x10)  ; r4 = param_block[4] (callback)
     * 8007324: bsr   0x8005338       ; flash_init()
     * 8007328: jsr   r4              ; call param's flash init callback
     * 800732a: bnez  r0, 0x8007478   ; if non-zero → error 'N'
     */
    param_block_ptr = *(volatile uint32_t **)PARAM_BLOCK_PTR_ADDR;
    typedef int (*flash_init_fn_t)(void);
    flash_init_fn_t flash_init_fn = (flash_init_fn_t)param_block_ptr[4];
    flash_init();
    if (flash_init_fn() != 0) {
        status = 'N';   /* STATUS_FLASH_FAIL = 78 */
        goto error_exit;
    }

    /* ---- 0x0800732E: Step 3: Read IMAGE_HEADER area (64 bytes) ---- */
    /*
     * 800732e: movi  r2, 64
     * 8007330: addi  r1, r14, 160    ; r1 = &image_hdr_raw
     * 8007332: movi  r0, 8192        ; r0 = 0x2000
     * 8007336: bseti r0, 27          ; r0 = 0x08002000
     * 8007338: bsr   0x80054f8       ; flash_read()
     */
    flash_read(0x08002000, (uint8_t *)&image_hdr_raw, 64);

    /* ---- 0x0800733C: Step 4: Compute FOTA flash address ---- */
    /*
     * 800733c: ld.w  r3, (r6, 0x0)   ; r3 = *PARAM_BLOCK_PTR
     * 800733e: movi  r2, 4
     * 8007340: ld.w  r4, (r3, 0xc)   ; r4 = param_block[3] (total_size)
     * 8007342: subi  r4, r4, 4096    ; r4 -= 0x1000
     * 8007346: bseti r4, 27          ; set flash base bit
     */
    fota_flash_addr = param_block_ptr[3] - 0x1000;
    fota_flash_addr |= (1 << 27);

    /* ---- 0x08007348: Step 5: Read 4-byte FOTA update counter ---- */
    /*
     * 8007348: mov   r1, r14         ; r1 = sp = &fota_update_no
     * 800734a: mov   r0, r4          ; r0 = fota_flash_addr
     * 800734c: bsr   0x80054f8       ; flash_read()
     */
    flash_read(fota_flash_addr, (uint8_t *)&fota_update_no, 4);

    /* ---- 0x08007350: Step 6: Check for UART boot request ---- */
    /*
     * 8007350: bsr   0x8007220       ; boot_uart_check()
     * 8007354: mov   r7, r0          ; r7 = uart_detected
     * 8007356: bnez  r0, 0x8007446   ; → uart_mode
     */
    uart_detected = boot_uart_check();
    if (uart_detected)
        goto uart_mode;

    /* ---- 0x0800735A: Step 7: Normal boot path ---- */
    /*
     * 800735a-8007364: Re-read IMAGE_HEADER from 0x08002000
     */
    flash_read(0x08002000, (uint8_t *)&image_hdr_raw, 64);

    /*
     * 8007368: mov   r2, r7          ; r2 = 0 (flags = normal)
     * 800736a: addi  r1, r14, 96     ; r1 = &primary_hdr
     * 800736c-8007372: find_valid_image(0x08002000, &primary_hdr, 0)
     * 8007376: mov   r5, r0          ; r5 = primary status
     */
    status = find_valid_image(0x08002000, (uint8_t *)&primary_hdr, 0);

    /*
     * 8007378: movi  r2, 1
     * 800737a: addi  r1, r14, 32     ; r1 = &secondary_hdr
     * 800737c-8007380: find_valid_image(0x08010000, &secondary_hdr, 1)
     */
    int fota_status = find_valid_image(0x08010000, (uint8_t *)&secondary_hdr, 1);

    /* ---- 0x08007384: Step 8: Choose which image to boot ---- */
    /*
     * 8007384: cmpnei r5, 67        ; primary == 'C'?
     * 8007388: bf     0x8007430     ; if primary == 'C' → both_valid_check
     * 800738a: cmpnei r0, 67        ; secondary == 'C'?
     * 800738e: bt     0x8007460     ; if secondary != 'C' → error_exit
     *
     * Fall through: primary != 'C', secondary == 'C' → use_secondary
     */
    if (status == 'C')
        goto both_valid_check;
    if (fota_status != 'C')
        goto error_exit;

    /* -------- use_secondary (0x08007390) -------- */
    /*
     * Reached when:
     *  - Primary invalid + secondary valid, OR
     *  - Both valid + secondary preferred (via br 0x8007390 at 0x8007444)
     *
     * Check img_attr bit 16 (byte 2 of img_attr, bit 0):
     *   If set → signature_verify path (signed/special image)
     *   If clear → flash_copy_data path (simple FOTA copy)
     */
use_secondary:
    /*
     * 8007390: ld.b  r3, (r14, 0x26) ; secondary_hdr.img_attr byte 2 (bit 16)
     * 8007394: andi  r3, r3, 1
     * 8007398: bez   r3, 0x8007400   ; if clear → fota_copy (unsigned)
     */
    if (((uint8_t *)&secondary_hdr.img_attr)[2] & 0x01) {
        /* ---- Signed/special image: signature verify path ---- */
        /*
         * 800739c: ld.w  r0, (r14, 0x34)   ; r0 = secondary_hdr.upgrade_img_addr
         * 800739e: addi  r2, r14, 96        ; r2 = &primary_hdr (output buffer)
         * 80073a0: ld.w  r1, (r14, 0x2c)   ; r1 = secondary_hdr.img_len
         * 80073a2: addi  r0, 64             ; r0 += 64 (skip to signature data)
         * 80073a4: bsr   0x80070c4          ; signature_verify()
         * 80073a8: blz   r0, 0x8007484      ; if result < 0 → sig_fail 'M'
         */
        int sig_result = signature_verify(
            secondary_hdr.upgrade_img_addr + 64,
            secondary_hdr.img_len,
            (uint32_t)&primary_hdr
        );
        if (sig_result < 0) {
            status = 'M';   /* STATUS_SIG_FAIL = 77 */
            goto error_exit;
        }
        /* Fall through to fota_counter_check */
    } else {
        /* ---- Unsigned image: FOTA data copy path (0x08007400) ---- */
        /*
         * Copy image header from secondary to primary partition:
         * 8007400: movi  r2, 64
         * 8007402: ld.w  r1, (r14, 0x34)   ; r1 = secondary_hdr.upgrade_img_addr
         * 8007404: ld.w  r0, (r14, 0x30)   ; r0 = secondary_hdr.img_header_addr
         * 8007406: bsr   0x800582c          ; flash_copy_data(src, dst, 64)
         */
        flash_copy_data(secondary_hdr.img_header_addr,
                        secondary_hdr.upgrade_img_addr, 64);

        /*
         * Copy image body (+ signature if present):
         * 800740a: ld.b  r3, (r14, 0x25)   ; img_attr byte 1 (bit 8 = signature)
         * 800740e: andi  r3, r3, 1
         * 8007412: ld.w  r1, (r14, 0x34)   ; r1 = upgrade_img_addr
         * 8007414: ld.w  r0, (r14, 0x28)   ; r0 = secondary_hdr.img_addr
         * 8007416: addi  r1, 64             ; r1 = upgrade_img_addr + 64
         * 8007418: ld.w  r2, (r14, 0x2c)   ; r2 = secondary_hdr.img_len
         * 800741a: bez   r3, 0x8007420      ; if no signature → skip
         * 800741e: addi  r2, 128            ; r2 += 128 (signature data)
         * 8007420: bsr   0x800582c          ; flash_copy_data()
         */
        {
            uint32_t copy_len = secondary_hdr.img_len;
            if (((uint8_t *)&secondary_hdr.img_attr)[1] & 0x01)
                copy_len += 128;
            flash_copy_data(secondary_hdr.img_addr,
                            secondary_hdr.upgrade_img_addr + 64,
                            copy_len);
        }

        /*
         * Copy secondary header to primary header in memory:
         * 8007424: movi  r2, 64
         * 8007426: addi  r1, r14, 32       ; r1 = &secondary_hdr (src)
         * 8007428: addi  r0, r14, 96       ; r0 = &primary_hdr (dst)
         * 800742a: bsr   0x8002b54         ; memcpy(dst, src, 64)
         * 800742e: br    0x80073ac         ; → fota_counter_check
         */
        memcpy(&primary_hdr, &secondary_hdr, 64);
        /* Fall through to fota_counter_check */
    }

    /* ---- 0x080073AC: fota_counter_check ---- */
    /*
     * Clear FOTA update counter if an update was pending.
     *
     * 80073ac: movi  r3, 0
     * 80073ae: ld.w  r2, (r14, 0x0)    ; r2 = fota_update_no
     * 80073b0: subi  r3, 1              ; r3 = 0xFFFFFFFF
     * 80073b2: cmpne r2, r3             ; fota_update_no != -1?
     * 80073b4: bf    0x80073c2          ; if equal → skip
     * 80073b6: movi  r2, 4
     * 80073b8: mov   r1, r14            ; r1 = &fota_update_no
     * 80073ba: mov   r0, r4             ; r0 = fota_flash_addr
     * 80073bc: st.w  r3, (r14, 0x0)    ; fota_update_no = 0xFFFFFFFF
     * 80073be: bsr   0x800553c          ; flash_write()
     */
fota_counter_check:
    if (fota_update_no != 0xFFFFFFFF) {
        fota_update_no = 0xFFFFFFFF;
        flash_write(fota_flash_addr, (const uint8_t *)&fota_update_no, 4);
    }

    /* ---- 0x080073C2: Verify image header CRC ---- */
    /*
     * 80073c2: ld.w  r1, (r14, 0x68)   ; r1 = primary_hdr.img_addr (offset 0x08)
     * 80073c4: addi  r0, r14, 96       ; r0 = &primary_hdr
     * 80073c6: bsr   0x80071f4         ; image_header_verify()
     * 80073ca: cmpnei r0, 67           ; == 'C'?
     * 80073ce: mov   r5, r0            ; r5 = status
     * 80073d0: bt    0x8007460         ; if != 'C' → error_exit
     */
    status = image_header_verify((const uint8_t *)&primary_hdr,
                                 primary_hdr.img_addr);
    if (status != 'C')
        goto error_exit;

    /* ---- 0x080073D2: boot_app - Build boot params and jump to app ---- */
    /*
     * 80073d2: ld.w  r3, (r6, 0x0)     ; *PARAM_BLOCK_PTR
     * 80073d4: addi  r0, r14, 4        ; r0 = &boot_params
     * 80073d6: ld.w  r3, (r3, 0xc)     ; param_block[3] (total_size)
     * 80073d8: st.w  r3, (r14, 0x4)    ; boot_params[0] = total_size
     * 80073da: ld.w  r3, (r14, 0x68)   ; primary_hdr + 0x08 = img_addr
     * 80073dc: st.w  r3, (r14, 0xc)    ; boot_params[2] = img_addr
     * 80073de: ld.w  r3, (r14, 0x64)   ; primary_hdr + 0x04 = img_attr.w
     * 80073e0: st.w  r3, (r14, 0x8)    ; boot_params[1] = img_attr.w
     * 80073e2: ld.w  r3, (r14, 0x6c)   ; primary_hdr + 0x0C = img_len
     * 80073e4: st.w  r3, (r14, 0x10)   ; boot_params[3] = img_len
     * 80073e6: ld.w  r3, (r14, 0xa4)   ; image_hdr_raw + 0x04 = img_attr.w
     * 80073e8: st.w  r3, (r14, 0x14)   ; boot_params[4]
     * 80073ea: ld.w  r3, (r14, 0xa8)   ; image_hdr_raw + 0x08 = img_addr
     * 80073ec: st.w  r3, (r14, 0x18)   ; boot_params[5]
     * 80073ee: ld.w  r3, (r14, 0xac)   ; image_hdr_raw + 0x0C = img_len
     * 80073f0: st.w  r3, (r14, 0x1c)   ; boot_params[6]
     */
boot_app:
    boot_params[0] = param_block_ptr[3];         /* total_flash_size */
    boot_params[2] = primary_hdr.img_addr;       /* offset 0x08 */
    boot_params[1] = primary_hdr.img_attr.w;     /* offset 0x04 */
    boot_params[3] = primary_hdr.img_len;        /* offset 0x0C */
    boot_params[4] = image_hdr_raw.img_attr.w;   /* offset 0x04 */
    boot_params[5] = image_hdr_raw.img_addr;     /* offset 0x08 */
    boot_params[6] = image_hdr_raw.img_len;      /* offset 0x0C */

    /*
     * 80073f2: lrw   r3, 0x200101d0    ; APP_ENTRY_PTR address
     * 80073f4: ld.w  r3, (r3, 0x0)     ; r3 = *APP_ENTRY_PTR
     * 80073f6: jsr   r3                ; call app_entry(boot_params)
     * 80073f8: bnez  r0, 0x800747c     ; if != 0 → error 'Y'
     * 80073fc: addi  r14, r14, 224     ; restore stack
     * 80073fe: pop   r4-r7, r15        ; return (r0 = 0 from app_entry)
     */
    {
        typedef int (*app_entry_t)(uint32_t *params);
        app_entry_t app_entry = *(app_entry_t *)APP_ENTRY_PTR_ADDR;
        int result = app_entry(boot_params);
        if (result != 0) {
            status = 'Y';   /* STATUS_APP_FAIL = 89 */
            goto error_exit;
        }
    }
    return 0;

    /* ---- 0x08007430: both_valid_check ---- */
    /*
     * Reached when primary_status == 'C'.
     * Decide whether to use primary or secondary (FOTA) image.
     *
     * Logic:
     *   - If secondary != 'C': use primary (boot_app)
     *   - If both 'C':
     *     a. If next pointers differ AND secondary.next == fota_update_no:
     *        → use_secondary (FOTA update ready)
     *     b. If next pointers same AND hd_checksum differs AND
     *        secondary.next == fota_update_no:
     *        → use_secondary
     *     c. Otherwise → use primary (boot_app)
     */
both_valid_check:
    /*
     * 8007430: cmpnei r0, 67          ; fota_status == 'C'?
     * 8007434: bt     0x80073d2       ; if secondary != 'C' → boot_app
     */
    if (fota_status != 'C')
        goto boot_app;

    /*
     * 8007436: ld.w  r3, (r14, 0x38)  ; r3 = secondary_hdr.org_checksum (offset 0x18)
     * 8007438: ld.w  r2, (r14, 0x78)  ; r2 = primary_hdr.org_checksum (offset 0x18)
     * 800743a: cmpne r2, r3           ; same org_checksum?
     * 800743c: bf    0x800746e        ; if equal → detailed_compare
     */
    if (secondary_hdr.org_checksum == primary_hdr.org_checksum) {
        /*
         * detailed_compare (0x0800746E):
         * Same org_checksum; compare header CRC checksums.
         *
         * 800746e: ld.w  r1, (r14, 0x9c) ; primary_hdr.hd_checksum (offset 0x3C)
         * 8007470: ld.w  r2, (r14, 0x5c) ; secondary_hdr.hd_checksum (offset 0x3C)
         * 8007472: cmpne r1, r2
         * 8007474: bt    0x800743e       ; if different → check fota_update_no
         * 8007476: br    0x80073d2       ; if same → boot_app (prefer primary)
         */
        if (primary_hdr.hd_checksum == secondary_hdr.hd_checksum)
            goto boot_app;
        /* Different hd_checksums, fall through to fota check */
    }

    /*
     * fota_update_no check (0x0800743E):
     * If secondary.org_checksum matches the FOTA update counter,
     * the secondary partition has a pending update to apply.
     *
     * 800743e: ld.w  r2, (r14, 0x0)    ; r2 = fota_update_no
     * 8007440: cmpne r3, r2             ; secondary.org_checksum != fota_update_no?
     * 8007442: bt    0x80073d2          ; if not equal → boot_app (use primary)
     * 8007444: br    0x8007390          ; if equal → use_secondary
     */
    if (secondary_hdr.org_checksum != fota_update_no)
        goto boot_app;
    goto use_secondary;

    /* ---- 0x08007446: uart_mode ---- */
    /*
     * UART bootloader detected. Print boot message, clear any pending
     * FOTA counter, then call the error/result handler with status 'C'.
     * The handler at *0x200101D4 implements the actual UART communication
     * protocol (xmodem download, flash programming, etc.).
     *
     * r5 = 0xFFFFFFFF at this point (initial sentinel value).
     */
uart_mode:
    /*
     * 8007446: lrw   r0, 0x80075f4     ; boot message string
     * 8007448: bsr   0x80028f4         ; puts()
     */
    puts((const char *)0x080075F4);

    /*
     * 800744c: ld.w  r3, (r14, 0x0)    ; r3 = fota_update_no
     * 800744e: cmpne r3, r5             ; compare with 0xFFFFFFFF (r5)
     * 8007450: bf    0x8007480          ; if equal → skip (no FOTA pending)
     * 8007452: movi  r2, 4
     * 8007454: mov   r1, r14            ; r1 = &fota_update_no
     * 8007456: mov   r0, r4             ; r0 = fota_flash_addr
     * 8007458: st.w  r5, (r14, 0x0)    ; fota_update_no = 0xFFFFFFFF
     * 800745a: bsr   0x800553c         ; flash_write()
     * 800745e: movi  r5, 67            ; r5 = 'C'
     *   (at 0x8007480: movi r5, 67; br 0x8007460 — same for skip path)
     */
    if (fota_update_no != 0xFFFFFFFF) {
        fota_update_no = 0xFFFFFFFF;
        flash_write(fota_flash_addr, (const uint8_t *)&fota_update_no, 4);
    }
    status = 'C';   /* STATUS_OK */
    /* Fall through to error_exit (handler does UART bootloader work) */

    /* ---- 0x08007460: error_exit ---- */
    /*
     * Call the error/result handler function at *0x200101D4.
     * Status code in r5 determines handler behavior:
     *   'C' = OK/UART mode, 'N' = flash fail, 'Y' = app fail,
     *   'M' = signature fail, etc.
     *
     * 8007460: lrw   r3, 0x200101d4
     * 8007462: mov   r0, r5             ; status code
     * 8007464: ld.w  r3, (r3, 0x0)
     * 8007466: jsr   r3                 ; call error handler
     * 8007468: movi  r0, 0
     * 800746a: subi  r0, 1              ; r0 = -1
     * 800746c: br    0x80073fc          ; → stack cleanup and return
     */
error_exit:
    {
        typedef void (*error_handler_t)(int status_code);
        error_handler_t handler = *(error_handler_t *)ERROR_HANDLER_PTR_ADDR;
        handler(status);
    }
    return -1;
}
