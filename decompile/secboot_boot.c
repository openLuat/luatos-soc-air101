/**
 * secboot_boot.c - Decompiled boot parameter and application boot logic
 *
 * Pseudo-C reconstruction from C-SKY XT804 disassembly of secboot.bin
 * Cross-referenced with: include/platform/wm_fwup.h, include/driver/wm_flash_map.h,
 *                         platform/common/fwup/wm_fwup.c, include/wm_regs.h
 *
 * Functions:
 *   boot_param_setup()    - 0x08006880  (setup boot parameters from flash)
 *   boot_param_read()     - 0x08006980  (read boot parameters / image chain)
 *   boot_prepare()        - 0x08006DEC  (prepare for application boot)
 *   boot_execute_prep()   - 0x08006F94  (pre-execution boot preparation)
 *   app_boot_sequence()   - 0x08007058  (application boot sequence)
 */

#include <stdint.h>

/* ============================================================
 * Constants
 * ============================================================ */

#define FLASH_BASE_ADDR         0x08000000
#define CODE_UPD_START_ADDR     0x08010000
#define CODE_RUN_START_ADDR     0x080D0000
#define INSIDE_FLS_SECTOR_SIZE  0x1000
#define IMAGE_HEADER_SIZE       64
#define SIGNATURE_SIZE          128
#define SIGNATURE_WORD          0xA0FFFF9F

/* SRAM global state addresses (from disassembly literal pools) */
#define PARAM_BLOCK_PTR_ADDR    0x2001007C
#define BOOT_PARAMS_BASE        0x200101A0
#define APP_ENTRY_PTR           0x200101D0
#define ERROR_HANDLER_PTR       0x200101D4

/* Watchdog */
#define WDG_BASE                0x40000E00
#define WDG_FEED_REG            (*(volatile uint32_t *)(WDG_BASE + 0x08))
#define WDG_CTRL_REG            (*(volatile uint32_t *)(WDG_BASE + 0x14))

/* VIC / Interrupt controller */
#define VIC_BASE                0xE000E000
#define VIC_INT_EN_BASE         0xE000E100

/* ============================================================
 * External functions
 * ============================================================ */
extern int   flash_read(uint32_t addr, void *buf, uint32_t len);    /* 0x080054F8 */
extern int   flash_write(uint32_t addr, void *buf, uint32_t len);   /* 0x0800553C */
extern void  flash_copy_data(uint32_t dst, uint32_t src,
                             uint32_t len);                          /* 0x0800582C */
extern int   flash_erase_range(uint32_t addr, uint32_t len);        /* 0x08005670 */
extern void  flash_protect_config(uint32_t mode);                    /* 0x080056B0 */
extern int   validate_image(const uint8_t *header);                  /* 0x08005988 */
extern int   crc_verify_image(void *hdr, void *param);               /* 0x08005CFC */
extern int   signature_verify(uint32_t data_addr, uint32_t data_len,
                              uint32_t sig_addr);                    /* 0x080070C4 */
extern void *memcpy(void *dst, const void *src, uint32_t n);        /* 0x08002B54 */
extern void *memset(void *s, int c, uint32_t n);                    /* 0x08002D20 */
extern int   puts(const char *s);                                    /* 0x080028F4 */


/* ============================================================
 * IMAGE_HEADER_PARAM_ST - Image Header Structure
 *
 * 64-byte structure stored in flash at the start of each image.
 * Layout from include/platform/wm_fwup.h:
 *
 *   Offset  Size  Field              Description
 *   0x00    4     magic_no           0xA0FFFF9F
 *   0x04    4     img_attr           Image attributes (bitfield)
 *                   [3:0]   img_type      (0=SECBOOT, 1=FLASHBIN0)
 *                   [4]     code_encrypt  (1=encrypted)
 *                   [7:5]   prikey_sel    (key selection)
 *                   [8]     signature     (1=128-byte sig appended)
 *                   [15:9]  reserved
 *                   [16]    zip_type      (1=compressed)
 *                   [17]    psram_io
 *                   [18]    erase_block_en
 *                   [19]    erase_always
 *   0x08    4     img_addr           Flash address of image code
 *   0x0C    4     img_len            Image code length in bytes
 *   0x10    4     img_header_addr    Address of this header in flash
 *   0x14    4     upgrade_img_addr   Address where upgrade image is stored
 *   0x18    4     org_checksum       CRC32 of image code
 *   0x1C    4     upd_no             Update sequence number
 *   0x20    16    ver[16]            Version string
 *   0x30    4     _reserved0
 *   0x34    4     _reserved1
 *   0x38    4     next               Pointer to next image header
 *   0x3C    4     hd_checksum        CRC32 of first 60 bytes
 * ============================================================ */


/* ============================================================
 * boot_param_setup (0x08006880)
 *
 * Setup boot parameters by reading the image header chain
 * from flash. Constructs the parameter block that will be
 * passed to the application entry point.
 *
 * This function reads the primary IMAGE_HEADER at
 * CODE_RUN_START_ADDR and follows the linked list of headers
 * (via the 'next' field) to discover all image components.
 *
 * r0 = output buffer for boot params (uint32_t[8])
 * r1 = flash param block pointer
 *
 * Returns: 0 on success, -1 on failure
 *
 * Disassembly:
 *   8006880: push  r4-r10, r15
 *   8006882: subi  r14, 96             ; stack frame
 *   8006884: mov   r4, r0              ; boot_params output
 *   8006886: mov   r5, r1              ; flash_param
 *
 *   ; Read primary image header from CODE_RUN_START_ADDR
 *   8006888: lrw   r0, 0x080D0000      ; CODE_RUN_START_ADDR
 *   800688c: bseti r0, 27             ; set flash base bit
 *   800688e: addi  r1, r14, 32         ; local header buf
 *   8006890: movi  r2, 64
 *   8006892: bsr   flash_read          ; 0x080054F8
 *
 *   ; Validate header
 *   8006896: addi  r0, r14, 32
 *   8006898: bsr   validate_image      ; 0x08005988
 *   800689c: cmpnei r0, 67            ; 'C'?
 *   800689e: bt    setup_fail
 *
 *   ; Extract key fields from header
 *   80068a2: ld.w  r6, (r14+32, 0x8)   ; img_addr
 *   80068a4: bseti r6, 27             ; with flash base
 *   80068a8: ld.w  r7, (r14+32, 0xc)   ; img_len
 *   80068ac: ld.w  r8, (r14+32, 0x1c)  ; upd_no
 *
 *   ; Store flash total_size into boot_params[0]
 *   80068b0: ld.w  r3, (r5, 0xc)       ; flash_param->total_size
 *   80068b2: st.w  r3, (r4, 0x0)       ; params[0] = total_size
 *
 *   ; Store img_addr, img_len
 *   80068b4: st.w  r6, (r4, 0x4)       ; params[1] = img_addr
 *   80068b6: st.w  r7, (r4, 0x8)       ; params[2] = img_len
 *
 *   ; Store upd_no
 *   80068ba: st.w  r8, (r4, 0x0c)      ; params[3] = upd_no
 *
 *   ; Store image attributes
 *   80068be: ld.w  r3, (r14+32, 0x4)   ; img_attr
 *   80068c0: st.w  r3, (r4, 0x10)      ; params[4] = img_attr
 *
 *   ; Store header address
 *   80068c4: ld.w  r3, (r14+32, 0x10)  ; img_header_addr
 *   80068c6: bseti r3, 27
 *   80068c8: st.w  r3, (r4, 0x14)      ; params[5] = img_header_addr
 *
 *   ; Store upgrade_img_addr
 *   80068cc: ld.w  r3, (r14+32, 0x14)  ; upgrade_img_addr
 *   80068ce: bseti r3, 27
 *   80068d0: st.w  r3, (r4, 0x18)      ; params[6] = upgrade_img_addr
 *
 *   ; Store next header pointer
 *   80068d4: ld.w  r3, (r14+32, 0x38)  ; next
 *   80068d6: st.w  r3, (r4, 0x1c)      ; params[7] = next
 *
 *   ; Feed watchdog during long operations
 *   80068da: lrw   r3, 0x40000E00
 *   80068dc: ld.w  r2, (r3, 0x8)
 *   80068de: st.w  r2, (r3, 0x8)       ; feed WDG
 *
 *   8006900: movi  r0, 0               ; return success
 *   8006902: addi  r14, 96
 *   8006904: pop   r4-r10, r15
 *
 * setup_fail:
 *   8006908: movi  r0, -1
 *   800690a: addi  r14, 96
 *   800690c: pop   r4-r10, r15
 * ============================================================ */
int boot_param_setup(uint32_t *boot_params, const uint32_t *flash_param)
{
    uint8_t hdr[IMAGE_HEADER_SIZE];

    /* Read primary image header from CODE_RUN_START_ADDR */
    uint32_t hdr_addr = CODE_RUN_START_ADDR | (1 << 27);
    flash_read(hdr_addr, hdr, IMAGE_HEADER_SIZE);

    /* Validate header */
    if (validate_image(hdr) != 'C')
        return -1;

    /* Extract and store boot parameters */
    uint32_t img_addr = *(uint32_t *)(hdr + 0x08) | (1 << 27);
    uint32_t img_len  = *(uint32_t *)(hdr + 0x0C);
    uint32_t upd_no   = *(uint32_t *)(hdr + 0x1C);
    uint32_t img_attr = *(uint32_t *)(hdr + 0x04);
    uint32_t hdr_self = *(uint32_t *)(hdr + 0x10) | (1 << 27);
    uint32_t upg_addr = *(uint32_t *)(hdr + 0x14) | (1 << 27);
    uint32_t next_ptr = *(uint32_t *)(hdr + 0x38);

    boot_params[0] = flash_param[3];    /* total_size (offset 0x0C) */
    boot_params[1] = img_addr;
    boot_params[2] = img_len;
    boot_params[3] = upd_no;
    boot_params[4] = img_attr;
    boot_params[5] = hdr_self;
    boot_params[6] = upg_addr;
    boot_params[7] = next_ptr;

    /* Feed watchdog */
    WDG_FEED_REG = WDG_FEED_REG;

    return 0;
}


/* ============================================================
 * boot_param_read (0x08006980)
 *
 * Read boot parameters from flash, following the image header
 * chain. Handles the upgrade/FOTA detection logic by reading
 * both the primary and upgrade partition headers.
 *
 * r0 = output boot_params (uint32_t[])
 * r1 = flags (0 = primary only, 1 = check upgrade too)
 *
 * Returns: 'C' (67) on success, 'L' (76) on invalid image,
 *          'J' (74) on not found
 *
 * This function is the core of the bootloader's image
 * discovery logic. It:
 *   1. Reads the image header at CODE_RUN_START_ADDR
 *   2. Validates it and extracts boot parameters
 *   3. If flags == 1, also checks the upgrade partition
 *   4. Compares upd_no to decide which image is newer
 *   5. Returns the parameters for the chosen image
 *
 * Disassembly:
 *   8006980: push  r4-r11, r15
 *   8006982: subi  r14, 200           ; large stack frame
 *   8006984: mov   r4, r0              ; output
 *   8006986: mov   r5, r1              ; flags
 *   8006988: lrw   r6, 0x2001007c      ; param_block_ptr_addr
 *
 *   ; Read primary header
 *   800698c: lrw   r0, 0x080D0000
 *   8006990: bseti r0, 27
 *   8006992: addi  r1, r14, 64         ; primary_hdr
 *   8006994: movi  r2, 64
 *   8006996: bsr   flash_read
 *
 *   ; Validate
 *   800699a: addi  r0, r14, 64
 *   800699c: bsr   validate_image
 *   80069a0: mov   r7, r0              ; primary_status
 *   80069a2: cmpnei r0, 67
 *   80069a4: bt    check_upgrade_only
 *
 *   ; Primary is valid - extract params
 *   80069a8: addi  r0, r4, 0           ; output
 *   80069aa: ld.w  r3, (r6, 0x0)       ; param_block
 *   80069ac: mov   r1, r3
 *   80069ae: bsr   boot_param_setup    ; 0x08006880
 *   80069b2: blz   r0, primary_bad
 *
 *   ; If flags == 0, we're done
 *   80069b6: bez   r5, param_done
 *
 *   ; Check upgrade partition too
 *   80069b8: lrw   r0, 0x08010000
 *   80069bc: bseti r0, 27
 *   80069be: addi  r1, r14, 128        ; upgrade_hdr
 *   80069c0: movi  r2, 64
 *   80069c2: bsr   flash_read
 *
 *   ; Validate upgrade header
 *   80069c6: addi  r0, r14, 128
 *   80069c8: bsr   validate_image
 *   80069cc: cmpnei r0, 67
 *   80069ce: bt    param_done          ; upgrade not valid -> use primary
 *
 *   ; Compare upd_no: choose newer
 *   80069d2: ld.w  r8, (r14+64, 0x1c)  ; primary upd_no
 *   80069d6: ld.w  r9, (r14+128, 0x1c) ; upgrade upd_no
 *   80069da: cmphs r8, r9             ; primary >= upgrade?
 *   80069dc: bt    param_done          ; primary is same or newer
 *
 *   ; Upgrade is newer: extract its params instead
 *   80069e0: ld.w  r3, (r6, 0x0)
 *   80069e2: ld.w  r3, (r3, 0xc)       ; flash_total_size
 *   80069e4: ld.w  r2, (r14+128, 0x14) ; upgrade_img_addr
 *   80069e8: bseti r2, 27
 *   80069ea: st.w  r3, (r4, 0x0)       ; params[0] = total_size
 *   80069ec: ld.w  r3, (r14+128, 0x8)
 *   80069f0: bseti r3, 27
 *   80069f2: st.w  r3, (r4, 0x4)       ; params[1] = img_addr
 *   80069f4: ld.w  r3, (r14+128, 0xc)
 *   80069f8: st.w  r3, (r4, 0x8)       ; params[2] = img_len
 *   80069fa: st.w  r9, (r4, 0xc)       ; params[3] = upd_no
 *   ; ... (fill remaining params from upgrade header)
 *
 * param_done:
 *   8006a40: movi  r0, 67              ; return 'C'
 *   8006a42: addi  r14, 200
 *   8006a44: pop   r4-r11, r15
 *
 * check_upgrade_only:
 *   ; Primary is invalid: check if upgrade partition has valid image
 *   8006a48: lrw   r0, 0x08010000
 *   8006a4c: bseti r0, 27
 *   8006a4e: addi  r1, r14, 128
 *   8006a50: movi  r2, 64
 *   8006a52: bsr   flash_read
 *   8006a56: addi  r0, r14, 128
 *   8006a58: bsr   validate_image
 *   8006a5c: cmpnei r0, 67
 *   8006a5e: bt    no_valid_image
 *   ; Upgrade is valid, use it
 *   8006a62: br    use_upgrade
 *
 * primary_bad:
 *   8006a66: movi  r0, 76              ; return 'L'
 *   8006a68: addi  r14, 200
 *   8006a6a: pop   r4-r11, r15
 *
 * no_valid_image:
 *   8006a6e: movi  r0, 74              ; return 'J' (not found)
 *   8006a70: addi  r14, 200
 *   8006a72: pop   r4-r11, r15
 * ============================================================ */
int boot_param_read(uint32_t *boot_params, int flags)
{
    uint8_t primary_hdr[IMAGE_HEADER_SIZE];
    uint8_t upgrade_hdr[IMAGE_HEADER_SIZE];
    uint32_t *param_block = *(uint32_t **)PARAM_BLOCK_PTR_ADDR;

    /* Read primary image header from CODE_RUN_START_ADDR */
    uint32_t primary_addr = CODE_RUN_START_ADDR | (1 << 27);
    flash_read(primary_addr, primary_hdr, IMAGE_HEADER_SIZE);

    int primary_valid = (validate_image(primary_hdr) == 'C');

    if (primary_valid) {
        /* Extract boot params from primary */
        if (boot_param_setup(boot_params, param_block) < 0)
            return 'L';     /* Setup failed */

        if (flags == 0)
            return 'C';     /* Primary only mode - done */

        /* Check upgrade partition for newer image */
        uint32_t upg_addr = CODE_UPD_START_ADDR | (1 << 27);
        flash_read(upg_addr, upgrade_hdr, IMAGE_HEADER_SIZE);

        if (validate_image(upgrade_hdr) != 'C')
            return 'C';     /* Upgrade not valid - use primary */

        /* Compare update numbers */
        uint32_t primary_upd = *(uint32_t *)(primary_hdr + 0x1C);
        uint32_t upgrade_upd = *(uint32_t *)(upgrade_hdr + 0x1C);

        if (primary_upd >= upgrade_upd)
            return 'C';     /* Primary is same or newer */

        /* Upgrade is newer - fill params from upgrade header */
        boot_params[0] = param_block[3];    /* flash total_size */
        boot_params[1] = *(uint32_t *)(upgrade_hdr + 0x08) | (1 << 27);
        boot_params[2] = *(uint32_t *)(upgrade_hdr + 0x0C);
        boot_params[3] = upgrade_upd;
        boot_params[4] = *(uint32_t *)(upgrade_hdr + 0x04);
        boot_params[5] = *(uint32_t *)(upgrade_hdr + 0x10) | (1 << 27);
        boot_params[6] = *(uint32_t *)(upgrade_hdr + 0x14) | (1 << 27);
        boot_params[7] = *(uint32_t *)(upgrade_hdr + 0x38);

        return 'C';
    }

    /* Primary is invalid - try upgrade partition */
    uint32_t upg_addr = CODE_UPD_START_ADDR | (1 << 27);
    flash_read(upg_addr, upgrade_hdr, IMAGE_HEADER_SIZE);

    if (validate_image(upgrade_hdr) != 'C')
        return 'J';     /* No valid image found */

    /* Use upgrade image */
    boot_params[0] = param_block[3];
    boot_params[1] = *(uint32_t *)(upgrade_hdr + 0x08) | (1 << 27);
    boot_params[2] = *(uint32_t *)(upgrade_hdr + 0x0C);
    boot_params[3] = *(uint32_t *)(upgrade_hdr + 0x1C);
    boot_params[4] = *(uint32_t *)(upgrade_hdr + 0x04);
    boot_params[5] = *(uint32_t *)(upgrade_hdr + 0x10) | (1 << 27);
    boot_params[6] = *(uint32_t *)(upgrade_hdr + 0x14) | (1 << 27);
    boot_params[7] = *(uint32_t *)(upgrade_hdr + 0x38);

    return 'C';
}


/* ============================================================
 * boot_prepare (0x08006DEC)
 *
 * Prepare the system for booting the application image.
 * Handles flash protection, encryption setup, and VBR
 * relocation for the application.
 *
 * r0 = boot_params (from boot_param_setup/boot_param_read)
 * r1 = image header (64 bytes in RAM)
 *
 * Returns: 0 on success, -1 on failure
 *
 * Algorithm:
 *   1. Feed watchdog
 *   2. If code_encrypt bit set:
 *      a. Configure flash encryption controller
 *      b. Set RSA key registers if needed
 *   3. Configure flash read protection
 *   4. Set up VBR to point to application vector table
 *   5. Copy application header to fixed SRAM location
 *   6. Disable secboot-only interrupts
 *   7. Set VBR to application's vector base (img_addr)
 *
 * Disassembly:
 *   8006dec: push  r4-r10, r15
 *   8006dee: subi  r14, 48
 *   8006df0: mov   r4, r0              ; boot_params
 *   8006df2: mov   r5, r1              ; hdr buf
 *
 *   ; Feed watchdog
 *   8006df4: lrw   r3, 0x40000E00
 *   8006df8: ld.w  r2, (r3, 0x8)
 *   8006dfa: st.w  r2, (r3, 0x8)
 *
 *   ; Check code_encrypt flag
 *   8006dfe: ld.w  r6, (r5, 0x4)       ; img_attr
 *   8006e00: lsri  r3, r6, 4
 *   8006e02: andi  r3, r3, 1           ; code_encrypt
 *   8006e04: bez   r3, no_encrypt
 *
 *   ; Setup flash encryption
 *   8006e08: lrw   r7, 0x40000000      ; flash controller base
 *   8006e0c: ld.w  r3, (r7, 0x0)       ; flash ctrl reg
 *   8006e0e: bseti r3, 1              ; enable decrypt
 *   8006e10: st.w  r3, (r7, 0x0)
 *
 *   ; Check prikey_sel
 *   8006e14: lsri  r3, r6, 5
 *   8006e16: andi  r3, r3, 7           ; prikey_sel
 *   8006e18: bez   r3, no_key_sel
 *   ; Setup key selection in encryption controller
 *   8006e1c: ld.w  r2, (r7, 0x0)
 *   8006e1e: bseti r2, 4              ; select key bank
 *   8006e20: st.w  r2, (r7, 0x0)
 * no_key_sel:
 *
 * no_encrypt:
 *   ; Configure flash protection based on image area
 *   8006e26: ld.w  r0, (r4, 0x4)       ; img_addr
 *   8006e28: ld.w  r1, (r4, 0x8)       ; img_len
 *   8006e2a: bsr   flash_protect_config ; 0x080056B0
 *
 *   ; Copy image header to SRAM location for app use
 *   ; The application may reference the header at a known SRAM address
 *   8006e30: lrw   r0, 0x20010000      ; dest in SRAM .data
 *   8006e34: mov   r1, r5              ; header source
 *   8006e36: movi  r2, 64
 *   8006e38: bsr   memcpy              ; 0x08002B54
 *
 *   ; Store boot_params pointer in global location
 *   8006e3c: lrw   r3, 0x2001007C
 *   8006e3e: st.w  r4, (r3, 0x0)       ; update param_block_ptr
 *
 *   ; Setup VBR for application
 *   8006e42: ld.w  r3, (r4, 0x4)       ; img_addr = app vector base
 *   ; The VBR register is set using mtcr instruction
 *   ; 8006e44: mtcr r3, cr<1,0>        ; VBR = img_addr
 *
 *   ; Disable all interrupts in VIC before handoff
 *   8006e48: lrw   r2, 0xe000e100
 *   8006e4c: movi  r3, 0
 *   8006e4e: st.w  r3, (r2, 0x0)       ; disable all IRQs
 *   8006e50: st.w  r3, (r2, 0x4)       ; second bank too
 *
 *   ; Feed watchdog one more time
 *   8006e54: lrw   r3, 0x40000E00
 *   8006e58: ld.w  r2, (r3, 0x8)
 *   8006e5a: st.w  r2, (r3, 0x8)
 *
 *   8006e5e: movi  r0, 0
 *   8006e60: addi  r14, 48
 *   8006e62: pop   r4-r10, r15
 * ============================================================ */
int boot_prepare(uint32_t *boot_params, const uint8_t *header)
{
    /* Feed watchdog */
    WDG_FEED_REG = WDG_FEED_REG;

    /* Check code_encrypt flag */
    uint32_t img_attr = *(uint32_t *)(header + 0x04);
    int code_encrypt = (img_attr >> 4) & 1;

    if (code_encrypt) {
        /* Setup flash decryption */
        volatile uint32_t *flash_ctrl = (volatile uint32_t *)0x40000000;
        uint32_t ctrl_val = flash_ctrl[0];
        ctrl_val |= (1 << 1);      /* Enable code decrypt */
        flash_ctrl[0] = ctrl_val;

        /* Check prikey_sel */
        int prikey_sel = (img_attr >> 5) & 7;
        if (prikey_sel != 0) {
            ctrl_val = flash_ctrl[0];
            ctrl_val |= (1 << 4);   /* Select key bank */
            flash_ctrl[0] = ctrl_val;
        }
    }

    /* Configure flash protection for image area */
    flash_protect_config(boot_params[1]);    /* img_addr */

    /* Copy image header to known SRAM location for application use */
    memcpy((void *)0x20010000, header, IMAGE_HEADER_SIZE);

    /* Update global param block pointer */
    *(volatile uint32_t *)PARAM_BLOCK_PTR_ADDR = (uint32_t)boot_params;

    /* Setup VBR for application vector table */
    uint32_t app_vbr = boot_params[1];    /* img_addr = vector base */
    /* __set_VBR(app_vbr);  -- mtcr r3, cr<1,0> */
    /* The actual VBR change happens in assembly via mtcr */

    /* Disable all interrupts before handoff */
    volatile uint32_t *vic_en = (volatile uint32_t *)VIC_INT_EN_BASE;
    vic_en[0] = 0;
    vic_en[1] = 0;

    /* Feed watchdog */
    WDG_FEED_REG = WDG_FEED_REG;

    return 0;
}


/* ============================================================
 * boot_execute_prep (0x08006F94)
 *
 * Final preparation before executing the application.
 * Validates the image one more time, then sets up the
 * execution environment (SP, VBR, entry point).
 *
 * r0 = boot_params
 * r1 = image header (in RAM)
 *
 * Returns: function pointer to app entry (cast as int)
 *          0 if validation fails
 *
 * Algorithm:
 *   1. Read image header from flash (final check)
 *   2. Validate header CRC
 *   3. If signature bit set, verify signature
 *   4. Compute VBR = img_addr (vector table base)
 *   5. Read app entry point from vector table[0]
 *   6. Read initial SP from vector table (implicit in Reset_Handler)
 *   7. Return entry point address
 *
 * Disassembly:
 *   8006f94: push  r4-r8, r15
 *   8006f96: subi  r14, 80
 *   8006f98: mov   r4, r0              ; boot_params
 *   8006f9a: mov   r5, r1              ; header
 *
 *   ; Final header validation
 *   8006f9c: ld.w  r6, (r4, 0x4)       ; img_addr
 *   8006fa0: mov   r0, r6
 *   8006fa2: addi  r1, r14, 16         ; local_hdr
 *   8006fa4: movi  r2, 64
 *   8006fa6: bsr   flash_read
 *   8006faa: addi  r0, r14, 16
 *   8006fac: bsr   validate_image
 *   8006fb0: cmpnei r0, 67
 *   8006fb2: bt    exec_fail
 *
 *   ; Check signature
 *   8006fb6: ld.b  r3, (r14+16, 0x5)   ; img_attr byte[1]
 *   8006fb8: andi  r3, r3, 1           ; signature flag
 *   8006fba: bez   r3, no_sig_check
 *
 *   ; Verify RSA signature
 *   8006fbe: ld.w  r0, (r14+16, 0x8)   ; img_addr (data start)
 *   8006fc0: bseti r0, 27
 *   8006fc4: addi  r0, 64              ; skip header
 *   8006fc6: ld.w  r1, (r14+16, 0xc)   ; img_len
 *   8006fca: ld.w  r2, (r14+16, 0x8)   ; sig_addr = img_addr + 64 + img_len
 *   8006fce: bseti r2, 27
 *   8006fd0: addi  r2, 64
 *   8006fd2: ld.w  r3, (r14+16, 0xc)
 *   8006fd6: addu  r2, r3              ; sig_addr = base + hdr + img_len
 *   8006fd8: bsr   signature_verify    ; 0x080070C4
 *   8006fdc: blz   r0, exec_fail       ; signature invalid
 *
 * no_sig_check:
 *   ; Read application entry from vector table
 *   8006fe0: ld.w  r7, (r14+16, 0x8)   ; img_addr
 *   8006fe4: bseti r7, 27
 *   8006fe8: addi  r0, r7, 0x100       ; vector[0] = Reset_Handler
 *   ; (Vector table starts at img_addr, entry at offset 0x100)
 *   ; Actually, the C-SKY vector table entry [0] is at the base addr
 *   8006fec: ld.w  r0, (r0, 0x0)       ; Read entry point address
 *   8006ff0: pop   r4-r8, r15          ; Return entry point
 *
 * exec_fail:
 *   8006ff4: movi  r0, 0               ; return NULL
 *   8006ff6: addi  r14, 80
 *   8006ff8: pop   r4-r8, r15
 * ============================================================ */
uint32_t boot_execute_prep(uint32_t *boot_params, const uint8_t *header)
{
    uint8_t local_hdr[IMAGE_HEADER_SIZE];

    /* Read image header from flash for final check */
    uint32_t img_addr = boot_params[1];
    flash_read(img_addr, local_hdr, IMAGE_HEADER_SIZE);

    /* Validate */
    if (validate_image(local_hdr) != 'C')
        return 0;

    /* Check signature bit */
    if (local_hdr[0x05] & 1) {
        /* Verify RSA signature */
        uint32_t data_addr = (*(uint32_t *)(local_hdr + 0x08) | (1 << 27))
                             + IMAGE_HEADER_SIZE;
        uint32_t data_len  = *(uint32_t *)(local_hdr + 0x0C);
        uint32_t sig_addr  = data_addr + data_len;

        if (signature_verify(data_addr, data_len, sig_addr) < 0)
            return 0;   /* Signature invalid */
    }

    /* Read entry point from vector table */
    uint32_t vbr = *(uint32_t *)(local_hdr + 0x08) | (1 << 27);

    /* C-SKY vector table: entry [0] at base + 0x100 is Reset_Handler
     * (matching platform/arch/xt804/bsp/startup.S layout) */
    uint32_t entry_addr;
    flash_read(vbr, &entry_addr, 4);    /* vector[0] = Reset_Handler */

    return entry_addr;
}


/* ============================================================
 * app_boot_sequence (0x08007058)
 *
 * Complete application boot sequence. This is the last function
 * called by the secboot before handing control to the application.
 *
 * Orchestrates:
 *   1. boot_param_setup() - collect boot parameters
 *   2. boot_prepare() - configure hardware for app
 *   3. boot_execute_prep() - validate and get entry point
 *   4. Jump to application
 *
 * r0 = 0 (normal boot) or 1 (post-OTA boot)
 *
 * Returns: 0 if application returns normally,
 *          does not return on success (jumps to app),
 *          -1 on any failure
 *
 * Disassembly:
 *   8007058: push  r4-r7, r15
 *   800705a: subi  r14, 96
 *   800705c: mov   r4, r0              ; mode
 *
 *   ; Setup boot parameters
 *   800705e: addi  r0, r14, 0          ; boot_params on stack
 *   8007060: mov   r1, r4              ; flags
 *   8007062: bsr   boot_param_read     ; 0x08006980
 *   8007066: cmpnei r0, 67            ; 'C'?
 *   8007068: bt    boot_fail
 *
 *   ; Read image header for the chosen image
 *   800706c: ld.w  r5, (r14, 0x4)      ; params[1] = img_addr
 *   8007070: mov   r0, r5
 *   8007072: addi  r1, r14, 32         ; local_hdr buf
 *   8007074: movi  r2, 64
 *   8007076: bsr   flash_read
 *
 *   ; Prepare hardware for app
 *   800707a: addi  r0, r14, 0          ; boot_params
 *   800707c: addi  r1, r14, 32         ; header
 *   800707e: bsr   boot_prepare        ; 0x08006DEC
 *   8007082: blz   r0, boot_fail
 *
 *   ; Get entry point
 *   8007086: addi  r0, r14, 0          ; boot_params
 *   8007088: addi  r1, r14, 32         ; header
 *   800708a: bsr   boot_execute_prep   ; 0x08006F94
 *   800708e: mov   r6, r0              ; entry_point
 *   8007090: bez   r0, boot_fail       ; NULL = failure
 *
 *   ; Feed watchdog one last time
 *   8007094: lrw   r3, 0x40000E00
 *   8007098: ld.w  r2, (r3, 0x8)
 *   800709a: st.w  r2, (r3, 0x8)
 *
 *   ; Set VBR to application vector table
 *   800709e: ld.w  r3, (r14, 0x4)      ; img_addr
 *   80070a0: mtcr  r3, cr<1,0>         ; VBR = img_addr
 *
 *   ; Store boot_params address for app to find
 *   80070a4: lrw   r3, 0x200101D0      ; APP_ENTRY_PTR location
 *   80070a8: addi  r2, r14, 0          ; &boot_params
 *   80070aa: st.w  r2, (r3, 0x0)
 *
 *   ; Jump to application entry point
 *   80070ae: mov   r0, r14             ; r0 = &boot_params (arg)
 *   80070b0: jsr   r6                  ; call entry_point(boot_params)
 *   80070b4: mov   r0, r0              ; return value from app
 *   80070b6: addi  r14, 96
 *   80070b8: pop   r4-r7, r15
 *
 * boot_fail:
 *   80070bc: movi  r0, -1
 *   80070be: addi  r14, 96
 *   80070c0: pop   r4-r7, r15
 * ============================================================ */
int app_boot_sequence(int mode)
{
    uint32_t boot_params[8];
    uint8_t  local_hdr[IMAGE_HEADER_SIZE];

    /* Step 1: Discover and read boot parameters */
    int result = boot_param_read(boot_params, mode);
    if (result != 'C')
        return -1;

    /* Step 2: Read image header for chosen image */
    uint32_t img_addr = boot_params[1];
    flash_read(img_addr, local_hdr, IMAGE_HEADER_SIZE);

    /* Step 3: Prepare hardware for application */
    if (boot_prepare(boot_params, local_hdr) < 0)
        return -1;

    /* Step 4: Validate and get entry point */
    uint32_t entry_point = boot_execute_prep(boot_params, local_hdr);
    if (entry_point == 0)
        return -1;

    /* Feed watchdog before handoff */
    WDG_FEED_REG = WDG_FEED_REG;

    /* Set VBR to application vector table */
    /* __set_VBR(boot_params[1]);  -- mtcr r3, cr<1,0> */

    /* Store boot_params address at known SRAM location
     * so application can find its boot parameters */
    *(volatile uint32_t *)APP_ENTRY_PTR = (uint32_t)boot_params;

    /* Jump to application entry point */
    typedef int (*app_entry_fn_t)(uint32_t *params);
    app_entry_fn_t app_entry = (app_entry_fn_t)entry_point;
    return app_entry(boot_params);
}
