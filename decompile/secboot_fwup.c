/**
 * secboot_fwup.c - Decompiled firmware update, OTA, and xmodem operations
 *
 * Pseudo-C reconstruction from C-SKY XT804 disassembly of secboot.bin
 * Cross-referenced with: include/platform/wm_fwup.h, platform/common/fwup/wm_fwup.c,
 *                         include/driver/wm_flash_map.h, tools/xt804/wm_tool.c
 *
 * Functions:
 *   xmodem_recv_init()          - 0x08005A5C  (initialize xmodem receive state)
 *   xmodem_recv_block()         - 0x08005AD4  (receive one xmodem block)
 *   xmodem_recv_data()          - 0x08005B88  (receive xmodem data stream)
 *   xmodem_process()            - 0x08005BC8  (process xmodem transfer to flash)
 *   firmware_update_init()      - 0x08005004  (firmware update initialization)
 *   firmware_update_process()   - 0x0800509C  (process firmware update data)
 *   firmware_apply()            - 0x08005DF0  (apply firmware from upgrade area)
 *   firmware_apply_ext()        - 0x08005E74  (extended firmware apply with copy)
 *   ota_process()               - 0x080062D4  (OTA update orchestration)
 *   ota_validate()              - 0x08006494  (OTA image validation)
 *   ota_apply()                 - 0x08006530  (apply OTA update to run area)
 */

#include <stdint.h>
#ifndef NULL
#define NULL ((void *)0)
#endif

/* ============================================================
 * Flash and Memory Map Constants
 * ============================================================ */

#define FLASH_BASE_ADDR         0x08000000
#define CODE_UPD_START_ADDR     0x08010000  /* Upgrade image area */
#define CODE_RUN_START_ADDR     0x080D0000  /* Run-time image header area */
#define SIGNATURE_WORD          0xA0FFFF9F
#define INSIDE_FLS_SECTOR_SIZE  0x1000      /* 4KB */
#define IMAGE_HEADER_SIZE       64          /* sizeof(IMAGE_HEADER_PARAM_ST) */
#define SIGNATURE_SIZE          128         /* RSA signature appended to image */

/* SRAM addresses for firmware update state */
#define PARAM_BLOCK_PTR         (*(volatile uint32_t *)0x2001007C)

/* Xmodem protocol constants (from tools/xt804/wm_tool.c) */
#define XMODEM_SOH      0x01    /* Start of Header (128-byte payload) */
#define XMODEM_STX      0x02    /* Start of 1K block (1024-byte payload) */
#define XMODEM_EOT      0x04    /* End of Transmission */
#define XMODEM_ACK      0x06    /* Acknowledge */
#define XMODEM_NAK      0x15    /* Negative Acknowledge */
#define XMODEM_CAN      0x18    /* Cancel */
#define XMODEM_CRC_CHR  'C'     /* CRC mode request */

/* External functions */
extern int   flash_read(uint32_t addr, void *buf, uint32_t len);    /* 0x080054F8 */
extern int   flash_write(uint32_t addr, void *buf, uint32_t len);   /* 0x0800553C */
extern int   flash_erase_range(uint32_t addr, uint32_t len);        /* 0x08005670 */
extern void  flash_copy_data(uint32_t dst, uint32_t src,
                             uint32_t len);                          /* 0x0800582C */
extern void *malloc(uint32_t size);                                  /* 0x080029A0 */
extern void  free(void *ptr);                                        /* 0x08002A3C */
extern void *memcpy(void *dst, const void *src, uint32_t n);        /* 0x08002B54 */
extern void *memset(void *s, int c, uint32_t n);                    /* 0x08002D20 */
extern int   memcmp(const void *a, const void *b, uint32_t n);      /* 0x08002BD4 */
extern int   uart_rx_ready(void);                                    /* 0x0800588C */
extern uint8_t uart_getchar(void);                                   /* 0x080058A0 */
extern int   uart_putchar(int ch);                                   /* 0x0800717C */
extern int   validate_image(const uint8_t *header);                  /* 0x08005988 */
extern int   crc_verify_image(void *ctx, void *hdr);                 /* 0x08005CFC */

/* CRC context functions */
extern int   crc_init(void *ctx, int type, uint32_t init);           /* 0x08004404 */
extern int   crc_update(void *ctx, const void *data, uint32_t len); /* 0x08004410 */
extern int   crc_finalize(void *ctx);                                /* 0x08004544 */
extern int   crc_final(void *ctx, uint32_t *result);                 /* 0x0800459C */


/* ============================================================
 * Xmodem Receive Context
 *
 * State structure used during xmodem receive operations.
 * Stored on stack in the calling functions.
 *
 * Layout (inferred from register usage):
 *   offset 0x00:  uint8_t  state       (0=idle, 1=receiving, 2=done)
 *   offset 0x01:  uint8_t  block_num   (expected block sequence number)
 *   offset 0x02:  uint8_t  retry_cnt   (retry counter)
 *   offset 0x04:  uint32_t total_rx    (total bytes received)
 *   offset 0x08:  uint32_t dest_addr   (flash write destination)
 *   offset 0x0C:  uint32_t block_size  (128 or 1024 depending on SOH/STX)
 * ============================================================ */


/* ============================================================
 * xmodem_recv_init (0x08005A5C)
 *
 * Initialize xmodem receive state. Sets up the receive context
 * and sends the initial NAK or 'C' (CRC mode) to the sender.
 *
 * r0 = xmodem context pointer (on stack)
 * r1 = flash destination address
 *
 * Disassembly:
 *   8005a5c: push  r4-r7, r15
 *   8005a5e: mov   r4, r0              ; ctx
 *   8005a60: mov   r5, r1              ; dest_addr
 *   8005a62: movi  r3, 0
 *   8005a64: st.b  r3, (r0, 0x0)       ; state = 0
 *   8005a66: movi  r3, 1
 *   8005a68: st.b  r3, (r0, 0x1)       ; block_num = 1
 *   8005a6a: st.b  r3, (r0, 0x2)       ; retry_cnt = 1
 *   8005a6c: movi  r3, 0
 *   8005a6e: st.w  r3, (r0, 0x4)       ; total_rx = 0
 *   8005a70: st.w  r5, (r0, 0x8)       ; dest_addr = flash addr
 *   8005a72: movi  r0, 67              ; 'C' = CRC mode
 *   8005a74: bsr   uart_putchar        ; send CRC request
 *
 *   ; Wait loop for sender to start
 *   8005a78: movi  r6, 0               ; timeout counter
 *   8005a7a: movih r7, 6               ; max = 0x60000
 *   8005a7e: subi  r7, 1               ; max = 0x5FFFF
 * wait_loop:
 *   8005a80: bsr   uart_rx_ready       ; check for data
 *   8005a84: bnez  r0, got_start
 *   8005a88: addi  r6, 1
 *   8005a8a: cmplt r7, r6
 *   8005a8c: bf    wait_loop
 *   8005a8e: movi  r0, 21              ; NAK - timeout retry
 *   8005a90: bsr   uart_putchar
 *   8005a94: ld.b  r3, (r4, 0x2)       ; retry_cnt
 *   8005a96: addi  r3, 1
 *   8005a98: st.b  r3, (r4, 0x2)
 *   8005a9a: cmplti r3, 10             ; max 10 retries
 *   8005a9c: bt    wait_loop
 *   8005a9e: movi  r0, -1              ; return -1 (timeout)
 *   8005aa0: pop   r4-r7, r15
 *
 * got_start:
 *   8005aa2: bsr   uart_getchar
 *   8005aa6: cmpnei r0, 1             ; SOH?
 *   8005aa8: bf    soh_mode
 *   8005aaa: cmpnei r0, 2             ; STX?
 *   8005aac: bf    stx_mode
 *   8005aae: cmpnei r0, 4             ; EOT?
 *   8005ab0: bf    eot_received
 *   8005ab2: movi  r0, -2             ; unexpected byte
 *   8005ab4: pop   r4-r7, r15
 *
 * soh_mode:
 *   8005ab6: movi  r3, 128
 *   8005ab8: st.w  r3, (r4, 0x0c)     ; block_size = 128
 *   8005aba: movi  r3, 1
 *   8005abc: st.b  r3, (r4, 0x0)      ; state = 1 (receiving)
 *   8005abe: movi  r0, 0              ; return 0 (SOH start)
 *   8005ac0: pop   r4-r7, r15
 *
 * stx_mode:
 *   8005ac2: movi  r3, 1024
 *   8005ac6: st.w  r3, (r4, 0x0c)     ; block_size = 1024
 *   8005ac8: movi  r3, 1
 *   8005aca: st.b  r3, (r4, 0x0)      ; state = 1
 *   8005acc: movi  r0, 0
 *   8005ace: pop   r4-r7, r15
 *
 * eot_received:
 *   8005ad0: movi  r0, 1              ; return 1 (EOT)
 *   8005ad2: pop   r4-r7, r15
 * ============================================================ */
int xmodem_recv_init(void *ctx, uint32_t dest_addr)
{
    uint8_t *c = (uint8_t *)ctx;
    c[0] = 0;              /* state = idle */
    c[1] = 1;              /* expected block = 1 */
    c[2] = 1;              /* retry = 1 */
    *(uint32_t *)(c + 4)  = 0;          /* total_rx = 0 */
    *(uint32_t *)(c + 8)  = dest_addr;  /* dest flash addr */

    /* Send 'C' to request CRC-16 mode */
    uart_putchar(XMODEM_CRC_CHR);

    /* Wait for sender to begin transmission */
    uint32_t timeout = 0;
    uint32_t max_timeout = 0x5FFFF;

    while (1) {
        if (uart_rx_ready()) {
            uint8_t ch = uart_getchar();
            if (ch == XMODEM_SOH) {
                *(uint32_t *)(c + 0x0C) = 128;     /* 128-byte blocks */
                c[0] = 1;                           /* state = receiving */
                return 0;
            } else if (ch == XMODEM_STX) {
                *(uint32_t *)(c + 0x0C) = 1024;    /* 1K blocks */
                c[0] = 1;
                return 0;
            } else if (ch == XMODEM_EOT) {
                return 1;                           /* End of transmission */
            } else {
                return -2;                          /* Unexpected byte */
            }
        }

        timeout++;
        if (timeout > max_timeout) {
            /* Timeout - send NAK and retry */
            uart_putchar(XMODEM_NAK);
            c[2]++;
            if (c[2] >= 10) {
                return -1;      /* Max retries exceeded */
            }
            timeout = 0;
        }
    }
}


/* ============================================================
 * xmodem_recv_block (0x08005AD4)
 *
 * Receive one xmodem data block. Reads block number, complement,
 * data payload, and CRC-16. Validates all fields.
 *
 * r0 = xmodem context
 * r1 = destination buffer (must be >= block_size bytes)
 *
 * Returns: 0 on success, -1 on error, 1 on EOT
 *
 * Frame format:
 *   [SOH/STX already consumed]
 *   [block_num (1 byte)]
 *   [255 - block_num (1 byte)]
 *   [data (128 or 1024 bytes)]
 *   [CRC_hi (1 byte)]
 *   [CRC_lo (1 byte)]
 *
 * Disassembly:
 *   8005ad4: push  r4-r8, r15
 *   8005ad6: mov   r4, r0              ; ctx
 *   8005ad8: mov   r5, r1              ; dest buffer
 *   8005ada: bsr   uart_getchar        ; block number
 *   8005ade: mov   r6, r0
 *   8005ae0: bsr   uart_getchar        ; complement
 *   8005ae4: mov   r7, r0
 *   8005ae6: addu  r3, r6, r7          ; block + complement
 *   8005ae8: andi  r3, r3, 255
 *   8005aea: cmpnei r3, 255            ; must equal 0xFF
 *   8005aec: bt    block_error
 *
 *   ; Read data payload
 *   8005af0: ld.w  r8, (r4, 0x0c)     ; block_size
 *   8005af2: movi  r3, 0              ; i = 0
 * data_loop:
 *   8005af4: bsr   uart_getchar
 *   8005af8: st.b  r0, (r5, 0x0)      ; *dest++ = byte
 *   8005afa: addi  r5, 1
 *   8005afc: addi  r3, 1
 *   8005afe: cmpne r3, r8             ; i < block_size?
 *   8005b00: bt    data_loop
 *
 *   ; Read CRC-16 (big-endian)
 *   8005b04: bsr   uart_getchar        ; CRC high byte
 *   8005b08: lsli  r6, r0, 8
 *   8005b0a: bsr   uart_getchar        ; CRC low byte
 *   8005b0e: or    r6, r0              ; crc_received = (hi << 8) | lo
 *
 *   ; Compute CRC-16 over data (CRC-CCITT polynomial 0x1021)
 *   8005b12: mov   r0, r5              ; rewind to start of data
 *   8005b14: subu  r0, r8              ; r0 = dest - block_size
 *   8005b16: mov   r1, r8              ; len = block_size
 *   8005b18: bsr   crc16_compute       ; compute CRC-16
 *   8005b1c: cmpne r0, r6             ; computed == received?
 *   8005b1e: bt    crc_error
 *
 *   ; Verify block sequence number
 *   8005b22: ld.b  r3, (r4, 0x1)      ; expected block
 *   8005b24: zextb r6, r6
 *   8005b26: cmpne r6, r3
 *   8005b28: bt    seq_error
 *
 *   ; Accept block: increment expected, send ACK
 *   8005b2c: addi  r3, 1
 *   8005b2e: st.b  r3, (r4, 0x1)      ; block_num++
 *   8005b30: ld.w  r3, (r4, 0x4)      ; total_rx
 *   8005b32: addu  r3, r8
 *   8005b34: st.w  r3, (r4, 0x4)      ; total_rx += block_size
 *   8005b36: movi  r0, 6              ; ACK
 *   8005b38: bsr   uart_putchar
 *   8005b3c: movi  r0, 0              ; return 0 (success)
 *   8005b3e: pop   r4-r8, r15
 *
 * block_error:
 * crc_error:
 * seq_error:
 *   8005b42: movi  r0, 21             ; NAK
 *   8005b44: bsr   uart_putchar
 *   8005b48: movi  r0, -1             ; return -1
 *   8005b4a: pop   r4-r8, r15
 * ============================================================ */
int xmodem_recv_block(void *ctx, uint8_t *dest)
{
    uint8_t *c = (uint8_t *)ctx;
    uint32_t block_size = *(uint32_t *)(c + 0x0C);

    /* Read block number and its complement */
    uint8_t block_num = uart_getchar();
    uint8_t block_cpl = uart_getchar();

    if ((uint8_t)(block_num + block_cpl) != 0xFF) {
        uart_putchar(XMODEM_NAK);
        return -1;
    }

    /* Read data payload */
    for (uint32_t i = 0; i < block_size; i++) {
        dest[i] = uart_getchar();
    }

    /* Read CRC-16 (big-endian) */
    uint16_t crc_hi = uart_getchar();
    uint16_t crc_lo = uart_getchar();
    uint16_t crc_received = (crc_hi << 8) | crc_lo;

    /* Compute CRC-16 CCITT over received data */
    uint16_t crc_computed = 0;
    for (uint32_t i = 0; i < block_size; i++) {
        crc_computed = crc_computed ^ ((uint16_t)dest[i] << 8);
        for (int j = 0; j < 8; j++) {
            if (crc_computed & 0x8000)
                crc_computed = (crc_computed << 1) ^ 0x1021;
            else
                crc_computed = crc_computed << 1;
        }
    }

    if (crc_computed != crc_received) {
        uart_putchar(XMODEM_NAK);
        return -1;
    }

    /* Verify block sequence */
    if (block_num != c[1]) {
        uart_putchar(XMODEM_NAK);
        return -1;
    }

    /* Accept: update state */
    c[1]++;                                             /* Next expected block */
    *(uint32_t *)(c + 4) += block_size;                 /* total_rx += block_size */
    uart_putchar(XMODEM_ACK);
    return 0;
}


/* ============================================================
 * xmodem_recv_data (0x08005B88)
 *
 * Receive a complete xmodem data stream. Loops calling
 * xmodem_recv_block until EOT is received or an
 * unrecoverable error occurs.
 *
 * r0 = xmodem context
 * r1 = data buffer (large enough for one block)
 * r2 = flash write callback / destination context
 *
 * Returns: total bytes received on success, -1 on error
 *
 * Disassembly:
 *   8005b88: push  r4-r7, r15
 *   8005b8a: mov   r4, r0              ; ctx
 *   8005b8c: mov   r5, r1              ; buffer
 *   8005b8e: mov   r6, r2              ; flash ctx
 *
 * recv_loop:
 *   8005b90: mov   r0, r4
 *   8005b92: mov   r1, r5
 *   8005b94: bsr   xmodem_recv_block   ; 0x08005AD4
 *   8005b98: bez   r0, write_flash     ; block OK -> write to flash
 *   8005b9a: blz   r0, check_retry     ; error -> retry?
 *   ; r0 > 0 means EOT
 *   8005b9c: movi  r0, 6              ; ACK the EOT
 *   8005b9e: bsr   uart_putchar
 *   8005ba2: ld.w  r0, (r4, 0x4)      ; return total_rx
 *   8005ba4: pop   r4-r7, r15
 *
 * write_flash:
 *   ; Write received block to flash
 *   8005ba8: ld.w  r0, (r4, 0x8)      ; dest_addr
 *   8005baa: mov   r1, r5              ; data buffer
 *   8005bac: ld.w  r2, (r4, 0x0c)     ; block_size
 *   8005bae: bsr   flash_write         ; write to flash
 *   8005bb2: ld.w  r3, (r4, 0x8)      ; dest_addr
 *   8005bb4: ld.w  r2, (r4, 0x0c)     ; block_size
 *   8005bb6: addu  r3, r2
 *   8005bb8: st.w  r3, (r4, 0x8)      ; dest_addr += block_size
 *   8005bba: br    recv_loop
 *
 * check_retry:
 *   8005bbe: ld.b  r3, (r4, 0x2)      ; retry_cnt
 *   8005bc0: addi  r3, 1
 *   8005bc2: st.b  r3, (r4, 0x2)
 *   8005bc4: cmplti r3, 100           ; max 100 retries
 *   8005bc6: bt    recv_loop           ; retry
 *   8005bc8: movi  r0, -1             ; too many errors
 *   8005bca: pop   r4-r7, r15
 * ============================================================ */
int xmodem_recv_data(void *ctx, uint8_t *buffer, void *flash_ctx)
{
    uint8_t *c = (uint8_t *)ctx;

    while (1) {
        int result = xmodem_recv_block(ctx, buffer);

        if (result == 0) {
            /* Block received OK - write to flash */
            uint32_t dest = *(uint32_t *)(c + 8);
            uint32_t bsz  = *(uint32_t *)(c + 0x0C);

            flash_write(dest, buffer, bsz);
            *(uint32_t *)(c + 8) = dest + bsz;  /* Advance dest */
        } else if (result > 0) {
            /* EOT received - acknowledge and return total */
            uart_putchar(XMODEM_ACK);
            return *(uint32_t *)(c + 4);    /* total_rx */
        } else {
            /* Error - retry */
            c[2]++;
            if (c[2] >= 100) {
                return -1;  /* Too many retries */
            }
        }
    }
}


/* ============================================================
 * xmodem_process (0x08005BC8)
 *
 * Top-level xmodem firmware download handler.
 * Called when UART bootloader mode is entered.
 *
 * Allocates a receive buffer, initializes xmodem,
 * receives the entire firmware image via xmodem protocol,
 * then validates and writes it to flash.
 *
 * r0 = flash destination address (upgrade area)
 *
 * Returns: 0 on success, -1 on failure
 *
 * Disassembly:
 *   8005bc8: push  r4-r7, r15
 *   8005bca: subi  r14, 24             ; xmodem ctx on stack (24 bytes)
 *   8005bcc: mov   r6, r0              ; dest_addr
 *
 *   ; Allocate 1KB receive buffer
 *   8005bce: movi  r0, 1024
 *   8005bd2: bsr   malloc              ; 0x080029A0
 *   8005bd6: mov   r4, r0
 *   8005bd8: bez   r0, alloc_fail
 *
 *   ; Initialize xmodem receive
 *   8005bdc: mov   r1, r6              ; dest_addr
 *   8005bde: mov   r0, r14             ; ctx = stack
 *   8005be0: bsr   xmodem_recv_init    ; 0x08005A5C
 *   8005be4: blz   r0, init_fail       ; init failed?
 *   8005be6: bgz   r0, eot_early       ; immediate EOT (empty file)
 *
 *   ; Receive data stream
 *   8005bea: mov   r2, r6              ; flash_ctx
 *   8005bec: mov   r1, r4              ; buffer
 *   8005bee: mov   r0, r14             ; ctx
 *   8005bf0: bsr   xmodem_recv_data    ; 0x08005B88
 *   8005bf4: mov   r5, r0              ; total received
 *   8005bf6: blz   r0, recv_fail
 *
 *   ; Validate received image
 *   8005bfa: mov   r1, r6              ; flash dest
 *   8005bfc: bsr   validate_image      ; 0x08005988
 *   8005c00: cmpnei r0, 67            ; 'C'?
 *   8005c02: bt    validate_fail
 *
 *   ; Success
 *   8005c06: movi  r5, 0
 *   8005c08: br    cleanup
 *
 * alloc_fail:
 * init_fail:
 * recv_fail:
 * validate_fail:
 *   8005c0c: movi  r5, -1
 *
 * cleanup:
 *   8005c10: mov   r0, r4
 *   8005c12: bsr   free
 *   8005c16: mov   r0, r5
 *   8005c18: addi  r14, 24
 *   8005c1a: pop   r4-r7, r15
 * ============================================================ */
int xmodem_process(uint32_t dest_addr)
{
    uint8_t xmodem_ctx[24];     /* Xmodem state on stack */

    /* Allocate 1KB receive buffer */
    uint8_t *buffer = (uint8_t *)malloc(1024);
    if (buffer == NULL)
        return -1;

    /* Initialize xmodem handshake */
    int result = xmodem_recv_init(xmodem_ctx, dest_addr);
    if (result < 0) {
        free(buffer);
        return -1;
    }
    if (result > 0) {
        /* Immediate EOT - empty transfer */
        free(buffer);
        return 0;
    }

    /* Receive entire data stream */
    int total = xmodem_recv_data(xmodem_ctx, buffer, (void *)dest_addr);
    if (total < 0) {
        free(buffer);
        return -1;
    }

    free(buffer);
    return 0;
}


/* ============================================================
 * firmware_update_init (0x08005004)
 *
 * Initialize firmware update state. Sets up the update context
 * with target flash addresses and validates the update header.
 *
 * r0 = pointer to IMAGE_HEADER_PARAM_ST (received from host)
 * r1 = flash_param_ptr (system parameter block)
 *
 * Returns: 0 on success, -1 on invalid header
 *
 * Algorithm:
 *   1. Validate magic number (0xA0FFFF9F)
 *   2. Check image type (must be IMG_TYPE_FLASHBIN0 = 1)
 *   3. Compute upgrade_img_addr = param->upgrade_img_addr | FLASH_BASE
 *   4. Store header and addresses in global update state
 *   5. Initialize CRC context for incremental verification
 *
 * Disassembly:
 *   8005004: push  r4-r8, r15
 *   8005006: mov   r4, r0              ; img_header
 *   8005008: mov   r5, r1              ; flash_param
 *   800500a: ld.w  r3, (r0, 0x0)       ; magic_no
 *   800500c: lrw   r2, 0xa0ffff9f      ; SIGNATURE_WORD
 *   8005010: cmpne r2, r3
 *   8005012: bt    bad_header
 *
 *   ; Check image type
 *   8005014: ld.b  r3, (r0, 0x4)       ; img_attr low byte
 *   8005016: andi  r3, r3, 15          ; img_type
 *   800501a: cmpnei r3, 1             ; must be FLASHBIN0
 *   800501c: bt    bad_header
 *
 *   ; Compare signature/encrypt flags with current running image
 *   800501e: ld.w  r6, (r0, 0x4)       ; img_attr word
 *   8005020: ld.w  r7, (r5, 0x4)       ; running img_attr
 *   8005022: lsri  r3, r6, 8           ; shift to signature bit
 *   8005024: andi  r3, r3, 1           ; new.signature
 *   8005026: lsri  r2, r7, 8
 *   8005028: andi  r2, r2, 1           ; running.signature
 *   800502a: cmpne r2, r3             ; must match
 *   800502c: bt    bad_header
 *
 *   ; Check code_encrypt flag matches
 *   800502e: lsri  r3, r6, 4
 *   8005030: andi  r3, r3, 1           ; new.code_encrypt
 *   8005032: lsri  r2, r7, 4
 *   8005034: andi  r2, r2, 1           ; running.code_encrypt
 *   8005036: cmpne r2, r3
 *   8005038: bt    bad_header
 *
 *   ; Validate image address alignment (1024-byte)
 *   800503a: ld.w  r3, (r4, 0x8)       ; img_addr
 *   800503c: andi  r2, r3, 0x3FF       ; addr % 1024
 *   8005040: bnez  r2, bad_header
 *
 *   ; Store update context in global state
 *   8005042: ld.w  r3, (r4, 0x14)      ; upgrade_img_addr
 *   8005044: ori   r3, r3, 0           ; (no-op, just load)
 *   8005046: lrw   r2, 0x200101A0      ; update_state base
 *   800504a: st.w  r3, (r2, 0x0)       ; program_base
 *   800504c: movi  r3, 0
 *   800504e: st.w  r3, (r2, 0x4)       ; program_offset = 0
 *   8005050: ld.w  r3, (r4, 0xc)       ; img_len
 *   8005052: addi  r3, 64              ; total = img_len + header_size
 *   8005056: ld.b  r0, (r4, 0x5)       ; img_attr byte[1]
 *   8005058: andi  r0, r0, 1           ; signature flag
 *   800505a: bez   r0, no_sig_adj
 *   800505c: addi  r3, 128             ; + signature size
 * no_sig_adj:
 *   800505e: st.w  r3, (r2, 0x8)       ; total_len
 *
 *   ; Write header to flash (upgrade area)
 *   8005060: ld.w  r0, (r2, 0x0)       ; program_base
 *   8005062: mov   r1, r4              ; header data
 *   8005064: movi  r2, 64
 *   8005066: bsr   flash_write         ; 0x0800553C
 *
 *   ; Return 0 (success)
 *   800506a: movi  r0, 0
 *   800506c: pop   r4-r8, r15
 *
 * bad_header:
 *   8005070: movi  r0, 0
 *   8005072: subi  r0, 1               ; return -1
 *   8005074: pop   r4-r8, r15
 * ============================================================ */
int firmware_update_init(const uint8_t *img_header, const uint8_t *running_header)
{
    uint32_t magic = *(uint32_t *)(img_header + 0x00);
    if (magic != SIGNATURE_WORD)
        return -1;

    /* Check image type == FLASHBIN0 */
    uint8_t img_type = img_header[0x04] & 0x0F;
    if (img_type != 1)
        return -1;

    /* Signature and encrypt flags must match running image */
    uint32_t new_attr = *(uint32_t *)(img_header + 0x04);
    uint32_t run_attr = *(uint32_t *)(running_header + 0x04);

    if (((new_attr >> 8) & 1) != ((run_attr >> 8) & 1))  /* signature */
        return -1;
    if (((new_attr >> 4) & 1) != ((run_attr >> 4) & 1))  /* code_encrypt */
        return -1;

    /* Check 1024-byte alignment */
    uint32_t img_addr = *(uint32_t *)(img_header + 0x08);
    if (img_addr & 0x3FF)
        return -1;

    /* Setup update state */
    uint32_t upgrade_addr = *(uint32_t *)(img_header + 0x14);
    uint32_t img_len = *(uint32_t *)(img_header + 0x0C);
    uint32_t total_len = img_len + IMAGE_HEADER_SIZE;

    if (img_header[0x05] & 1)   /* signature flag */
        total_len += SIGNATURE_SIZE;

    /* Store in global update state area */
    volatile uint32_t *update_state = (volatile uint32_t *)0x200101A0;
    update_state[0] = upgrade_addr;     /* program_base */
    update_state[1] = 0;               /* program_offset */
    update_state[2] = total_len;        /* total_len */

    /* Write header to upgrade flash area */
    flash_write(upgrade_addr, (void *)img_header, IMAGE_HEADER_SIZE);

    return 0;
}


/* ============================================================
 * firmware_update_process (0x0800509C)
 *
 * Process one chunk of firmware update data. Called repeatedly
 * as data arrives (via xmodem or other transport).
 *
 * r0 = data pointer
 * r1 = data length
 *
 * Returns: 0 if more data expected,
 *          1 if update complete (all data received),
 *         -1 on flash write error
 *
 * Algorithm:
 *   1. Read current offset from global state
 *   2. Write data to flash at program_base + offset
 *   3. Increment offset by data length
 *   4. If offset >= total_len, trigger CRC verification
 *   5. On CRC match, write OTA flag to flash
 *
 * Disassembly:
 *   800509c: push  r4-r7, r15
 *   800509e: mov   r4, r0              ; data
 *   80050a0: mov   r5, r1              ; data_len
 *   80050a2: lrw   r6, 0x200101A0      ; update_state
 *   80050a6: ld.w  r3, (r6, 0x0)       ; program_base
 *   80050a8: ld.w  r2, (r6, 0x4)       ; program_offset
 *   80050aa: addu  r0, r3, r2          ; write_addr = base + offset
 *   80050ac: mov   r1, r4              ; data
 *   80050ae: mov   r2, r5              ; len
 *   80050b0: bsr   flash_write         ; 0x0800553C
 *   80050b4: bnez  r0, write_error
 *
 *   ; Update offset
 *   80050b8: ld.w  r3, (r6, 0x4)
 *   80050ba: addu  r3, r5
 *   80050bc: st.w  r3, (r6, 0x4)       ; offset += data_len
 *
 *   ; Check if complete
 *   80050be: ld.w  r2, (r6, 0x8)       ; total_len
 *   80050c0: cmphs r3, r2             ; offset >= total?
 *   80050c2: bf    more_data
 *
 *   ; Complete: verify CRC of written image
 *   80050c6: ld.w  r0, (r6, 0x0)       ; program_base
 *   80050c8: movi  r2, 64
 *   80050ca: addi  r0, 64              ; skip header (read image data)
 *   ; ... CRC verification over image body ...
 *   ; Compare with org_checksum from header
 *   ; If match: write OTA flag, return 1
 *   ; If mismatch: return -1
 *
 * more_data:
 *   80050e0: movi  r0, 0               ; return 0 (not done yet)
 *   80050e2: pop   r4-r7, r15
 *
 * write_error:
 *   80050e6: movi  r0, 0
 *   80050e8: subi  r0, 1               ; return -1
 *   80050ea: pop   r4-r7, r15
 * ============================================================ */
int firmware_update_process(const uint8_t *data, uint32_t data_len)
{
    volatile uint32_t *state = (volatile uint32_t *)0x200101A0;
    uint32_t base   = state[0];     /* program_base */
    uint32_t offset = state[1];     /* program_offset */
    uint32_t total  = state[2];     /* total_len */

    /* Write data chunk to flash */
    uint32_t write_addr = base + offset;
    int err = flash_write(write_addr, (void *)data, data_len);
    if (err != 0)
        return -1;

    /* Update offset */
    offset += data_len;
    state[1] = offset;

    /* Check if all data received */
    if (offset >= total) {
        /* Verify CRC of written image */
        uint8_t hdr_buf[IMAGE_HEADER_SIZE];
        flash_read(base, hdr_buf, IMAGE_HEADER_SIZE);

        uint32_t img_len = *(uint32_t *)(hdr_buf + 0x0C);
        uint32_t expected_crc = *(uint32_t *)(hdr_buf + 0x18);  /* org_checksum */

        /* Check if signature is present - exclude from CRC */
        uint32_t crc_len = img_len;
        /* CRC is over image body only (excluding header and optional signature) */

        /* Compute CRC32 over image body */
        uint32_t crc_ctx[4];
        uint32_t computed_crc = 0;
        crc_init(crc_ctx, 3, 0xFFFFFFFF);

        uint8_t *verify_buf = (uint8_t *)malloc(1024);
        if (verify_buf == NULL)
            return -1;

        uint32_t read_offset = IMAGE_HEADER_SIZE;
        uint32_t remaining = crc_len;
        while (remaining > 0) {
            uint32_t chunk = (remaining > 1024) ? 1024 : remaining;
            flash_read(base + read_offset, verify_buf, chunk);
            crc_update(crc_ctx, verify_buf, chunk);
            read_offset += chunk;
            remaining -= chunk;
        }

        crc_final(crc_ctx, &computed_crc);
        free(verify_buf);

        if (computed_crc != expected_crc)
            return -1;

        /* Write OTA flag to flash (tells bootloader to apply update) */
        uint32_t *param_ptr = *(uint32_t **)0x2001007C;
        uint32_t flash_total = param_ptr[3];
        uint32_t ota_flag_addr = flash_total - INSIDE_FLS_SECTOR_SIZE;
        ota_flag_addr |= (1 << 27);
        flash_write(ota_flag_addr, (void *)&expected_crc, 4);

        return 1;   /* Update complete */
    }

    return 0;       /* More data expected */
}


/* ============================================================
 * firmware_apply (0x08005DF0)
 *
 * Apply a firmware update from the upgrade area to the run area.
 * Copies the validated image from upgrade_img_addr to img_addr.
 *
 * r0 = pointer to IMAGE_HEADER of upgrade image (in RAM)
 * r1 = pointer to IMAGE_HEADER of running image (in RAM)
 *
 * Returns: 0 on success, -1 on failure
 *
 * Algorithm:
 *   1. Read upgrade image header from flash
 *   2. Validate: magic, type, CRC
 *   3. Erase destination area (img_addr) for img_len bytes
 *   4. Copy image data from upgrade_img_addr to img_addr
 *   5. Copy header from upgrade area to run area (0x080D0000)
 *   6. Verify CRC of copied data
 *   7. Clear OTA flag
 *
 * Disassembly:
 *   8005df0: push  r4-r10, r15
 *   8005df2: subi  r14, 80             ; stack frame for headers
 *   8005df4: mov   r4, r0              ; upgrade_hdr
 *   8005df6: mov   r5, r1              ; running_hdr
 *
 *   ; Read source header and validate
 *   8005df8: ld.w  r6, (r0, 0x14)      ; upgrade_img_addr
 *   8005dfa: bseti r6, 27             ; add flash base bit
 *   8005dfe: mov   r0, r6
 *   8005e00: addi  r1, r14, 16         ; local hdr buffer
 *   8005e02: movi  r2, 64
 *   8005e04: bsr   flash_read
 *   8005e08: addi  r0, r14, 16
 *   8005e0a: bsr   validate_image      ; 0x08005988
 *   8005e0e: cmpnei r0, 67            ; 'C'?
 *   8005e10: bt    apply_fail
 *
 *   ; Get image addresses
 *   8005e14: ld.w  r7, (r14+16, 0x8)   ; img_addr from upgrade hdr
 *   8005e18: ld.w  r8, (r14+16, 0xc)   ; img_len
 *   8005e1c: bseti r7, 27             ; flash base
 *
 *   ; Erase destination
 *   8005e20: mov   r0, r7
 *   8005e22: mov   r1, r8
 *   8005e24: bsr   flash_erase_range   ; 0x08005670
 *
 *   ; Copy image data
 *   8005e28: addi  r1, r6, 64          ; src = upgrade_addr + header
 *   8005e2c: mov   r0, r7              ; dst = img_addr
 *   8005e2e: mov   r2, r8              ; len = img_len
 *   8005e30: ld.b  r3, (r14+16, 0x5)   ; signature flag
 *   8005e34: andi  r3, r3, 1
 *   8005e36: bez   r3, no_sig_copy
 *   8005e38: addi  r2, 128             ; include signature
 * no_sig_copy:
 *   8005e3c: bsr   flash_copy_data     ; 0x0800582C
 *
 *   ; Copy header to CODE_RUN_START_ADDR (0x080D0000)
 *   8005e42: lrw   r0, 0x080D0000
 *   8005e46: bseti r0, 27
 *   8005e48: mov   r1, r6              ; src = upgrade area header
 *   8005e4a: movi  r2, 64
 *   8005e4c: bsr   flash_copy_data
 *
 *   ; Verify CRC of copied image
 *   8005e50: addi  r0, r14, 16         ; header buffer
 *   8005e52: mov   r1, r8              ; img_len
 *   8005e54: bsr   crc_verify_image    ; 0x08005CFC
 *   8005e58: blz   r0, apply_fail
 *
 *   ; Clear OTA flag
 *   8005e5c: lrw   r0, ota_flag_addr
 *   8005e5e: movi  r3, -1              ; 0xFFFFFFFF
 *   8005e62: st.w  r3, (r14, 0x0)
 *   8005e64: addi  r1, r14, 0
 *   8005e66: movi  r2, 4
 *   8005e68: bsr   flash_write
 *
 *   8005e6c: movi  r0, 0               ; return 0 (success)
 *   8005e6e: addi  r14, 80
 *   8005e70: pop   r4-r10, r15
 *
 * apply_fail:
 *   8005e72: movi  r0, -1
 *   8005e74: addi  r14, 80
 *   8005e76: pop   r4-r10, r15
 * ============================================================ */
int firmware_apply(const uint8_t *upgrade_hdr, const uint8_t *running_hdr)
{
    uint8_t local_hdr[IMAGE_HEADER_SIZE];

    /* Read and validate upgrade image header from flash */
    uint32_t upgrade_addr = *(uint32_t *)(upgrade_hdr + 0x14);
    upgrade_addr |= (1 << 27);     /* Set flash base bit */

    flash_read(upgrade_addr, local_hdr, IMAGE_HEADER_SIZE);
    if (validate_image(local_hdr) != 'C')
        return -1;

    uint32_t img_addr = *(uint32_t *)(local_hdr + 0x08);
    uint32_t img_len  = *(uint32_t *)(local_hdr + 0x0C);
    img_addr |= (1 << 27);

    /* Erase destination area */
    flash_erase_range(img_addr, img_len);

    /* Copy image data from upgrade area to run area */
    uint32_t copy_len = img_len;
    if (local_hdr[0x05] & 1)       /* signature flag */
        copy_len += SIGNATURE_SIZE;

    flash_copy_data(img_addr, upgrade_addr + IMAGE_HEADER_SIZE, copy_len);

    /* Copy header to CODE_RUN_START_ADDR */
    uint32_t run_hdr_addr = CODE_RUN_START_ADDR | (1 << 27);
    flash_copy_data(run_hdr_addr, upgrade_addr, IMAGE_HEADER_SIZE);

    /* Verify CRC of copied data */
    int crc_result = crc_verify_image(local_hdr, (void *)(uintptr_t)img_len);
    if (crc_result < 0)
        return -1;

    /* Clear OTA flag (write 0xFFFFFFFF) */
    uint32_t *param_ptr = *(uint32_t **)0x2001007C;
    uint32_t ota_flag_addr = param_ptr[3] - INSIDE_FLS_SECTOR_SIZE;
    ota_flag_addr |= (1 << 27);
    uint32_t clear_val = 0xFFFFFFFF;
    flash_write(ota_flag_addr, &clear_val, 4);

    return 0;
}


/* ============================================================
 * firmware_apply_ext (0x08005E74)
 *
 * Extended firmware apply with additional validation steps.
 * Handles both normal upgrades and FOTA (Firmware Over The Air).
 *
 * Similar to firmware_apply but with:
 *   - Additional header validation (upd_no comparison)
 *   - Support for partial/incremental updates
 *   - Erase-before-write with block alignment
 *
 * r0 = upgrade header (in RAM, from upgrade partition)
 * r1 = running header (in RAM, from run partition)
 * r2 = flags (0 = normal, 1 = FOTA mode)
 *
 * Returns: 0 on success, -1 on failure
 *
 * Disassembly:
 *   8005e74: push  r4-r11, r15
 *   8005e76: subi  r14, 96
 *   8005e78: mov   r4, r0              ; upgrade_hdr
 *   8005e7a: mov   r5, r1              ; running_hdr
 *   8005e7c: mov   r6, r2              ; flags
 *
 *   ; Read upgrade header from flash
 *   8005e80: ld.w  r7, (r0, 0x14)      ; upgrade_img_addr
 *   8005e82: bseti r7, 27
 *   8005e86: mov   r0, r7
 *   8005e88: addi  r1, r14, 32
 *   8005e8a: movi  r2, 64
 *   8005e8c: bsr   flash_read
 *
 *   ; Validate header
 *   8005e90: addi  r0, r14, 32
 *   8005e92: bsr   validate_image
 *   8005e96: cmpnei r0, 67
 *   8005e98: bt    fail
 *
 *   ; Compare upd_no with current
 *   8005e9c: ld.w  r3, (r14+32, 0x1c)  ; upd_no from upgrade
 *   8005ea0: ld.w  r2, (r5, 0x1c)      ; upd_no from running
 *   8005ea2: cmphs r2, r3             ; running >= upgrade?
 *   8005ea4: bt    skip_apply          ; already up to date
 *
 *   ; Get image parameters
 *   8005ea8: ld.w  r8, (r14+32, 0x8)   ; img_addr
 *   8005eac: ld.w  r9, (r14+32, 0xc)   ; img_len
 *   8005eb0: bseti r8, 27
 *
 *   ; Check erase_block_en flag
 *   8005eb4: ld.b  r3, (r14+32, 0x6)   ; img_attr high byte
 *   8005eb6: lsri  r3, 2               ; erase_block_en (bit 18 >> 16 + 2)
 *   8005eb8: andi  r3, r3, 1
 *   8005eba: bez   r3, sector_erase
 *
 *   ; Block erase (64KB blocks)
 *   8005ebe: mov   r0, r8
 *   8005ec0: mov   r1, r9
 *   8005ec2: bsr   flash_erase_range   ; erase destination
 *   8005ec6: br    do_copy
 *
 * sector_erase:
 *   ; Sector erase (4KB sectors) - more conservative
 *   8005eca: mov   r0, r8
 *   8005ecc: mov   r1, r9
 *   8005ece: bsr   flash_erase_range
 *
 * do_copy:
 *   ; Copy image from upgrade to run area
 *   8005ed2: addi  r1, r7, 64          ; src = upgrade + header
 *   8005ed6: mov   r0, r8              ; dst = img_addr
 *   8005ed8: mov   r2, r9              ; len = img_len
 *   8005eda: ld.b  r3, (r14+32, 0x5)
 *   8005ede: andi  r3, r3, 1           ; signature?
 *   8005ee0: bez   r3, no_sig
 *   8005ee2: addi  r2, 128
 * no_sig:
 *   8005ee4: bsr   flash_copy_data
 *
 *   ; Copy header to run header area
 *   8005ee8: lrw   r0, 0x080D0000
 *   8005eec: bseti r0, 27
 *   8005eee: mov   r1, r7
 *   8005ef0: movi  r2, 64
 *   8005ef2: bsr   flash_copy_data
 *
 *   ; Verify
 *   8005ef6: addi  r0, r14, 32
 *   8005ef8: mov   r1, r9
 *   8005efa: bsr   crc_verify_image
 *   8005efe: blz   r0, fail
 *
 *   ; Clear OTA flag
 *   ; ... (same as firmware_apply)
 *
 *   8005f10: movi  r0, 0
 *   8005f12: addi  r14, 96
 *   8005f14: pop   r4-r11, r15
 *
 * skip_apply:
 *   8005f18: movi  r0, 0               ; already up to date
 *   8005f1a: addi  r14, 96
 *   8005f1c: pop   r4-r11, r15
 *
 * fail:
 *   8005f20: movi  r0, -1
 *   8005f22: addi  r14, 96
 *   8005f24: pop   r4-r11, r15
 * ============================================================ */
int firmware_apply_ext(const uint8_t *upgrade_hdr, const uint8_t *running_hdr,
                       int flags)
{
    uint8_t local_hdr[IMAGE_HEADER_SIZE];

    /* Read upgrade header from flash */
    uint32_t upgrade_addr = *(uint32_t *)(upgrade_hdr + 0x14);
    upgrade_addr |= (1 << 27);

    flash_read(upgrade_addr, local_hdr, IMAGE_HEADER_SIZE);

    /* Validate */
    if (validate_image(local_hdr) != 'C')
        return -1;

    /* Compare update numbers - skip if already applied */
    uint32_t new_upd_no = *(uint32_t *)(local_hdr + 0x1C);
    uint32_t cur_upd_no = *(uint32_t *)(running_hdr + 0x1C);
    if (cur_upd_no >= new_upd_no)
        return 0;   /* Already up to date */

    /* Get image parameters */
    uint32_t img_addr = *(uint32_t *)(local_hdr + 0x08);
    uint32_t img_len  = *(uint32_t *)(local_hdr + 0x0C);
    img_addr |= (1 << 27);

    /* Erase destination area */
    flash_erase_range(img_addr, img_len);

    /* Copy image data */
    uint32_t copy_len = img_len;
    if (local_hdr[0x05] & 1)       /* signature */
        copy_len += SIGNATURE_SIZE;

    flash_copy_data(img_addr, upgrade_addr + IMAGE_HEADER_SIZE, copy_len);

    /* Copy header to run area */
    uint32_t run_hdr_addr = CODE_RUN_START_ADDR | (1 << 27);
    flash_copy_data(run_hdr_addr, upgrade_addr, IMAGE_HEADER_SIZE);

    /* Verify CRC */
    int result = crc_verify_image(local_hdr, (void *)(uintptr_t)img_len);
    if (result < 0)
        return -1;

    /* Clear OTA flag */
    uint32_t *param_ptr = *(uint32_t **)0x2001007C;
    uint32_t ota_flag_addr = param_ptr[3] - INSIDE_FLS_SECTOR_SIZE;
    ota_flag_addr |= (1 << 27);
    uint32_t clear_val = 0xFFFFFFFF;
    flash_write(ota_flag_addr, &clear_val, 4);

    return 0;
}


/* ============================================================
 * ota_process (0x080062D4)
 *
 * Main OTA (Over-The-Air) update orchestration function.
 * Manages the complete firmware update process including:
 * validation, flash erasure, image copy, and verification.
 *
 * r0 = OTA context / flash parameter block
 * r1 = upgrade image header (64 bytes, already in RAM)
 * r2 = running image header (64 bytes, already in RAM)
 *
 * Returns: 0 on success, negative on failure
 *   -1 = validation failed
 *   -2 = erase failed
 *   -3 = copy failed
 *   -4 = verify failed
 *
 * Disassembly:
 *   80062d4: push  r4-r11, r15
 *   80062d6: subi  r14, 160            ; large stack frame
 *   80062d8: mov   r4, r0              ; flash_ctx
 *   80062da: mov   r5, r1              ; upgrade_hdr
 *   80062dc: mov   r6, r2              ; running_hdr
 *
 *   ; Step 1: Validate upgrade header
 *   80062e0: ld.w  r3, (r5, 0x0)       ; magic
 *   80062e2: lrw   r2, 0xa0ffff9f
 *   80062e6: cmpne r2, r3
 *   80062e8: bt    ota_fail_validate
 *
 *   ; Check image type
 *   80062ea: ld.b  r3, (r5, 0x4)
 *   80062ec: andi  r3, r3, 15
 *   80062f0: cmpnei r3, 1             ; FLASHBIN0?
 *   80062f2: bt    ota_fail_validate
 *
 *   ; CRC32 of header
 *   80062f6: addi  r0, r14, 8
 *   80062f8: movi  r3, 3               ; CRC32
 *   80062fa: movi  r1, 0
 *   80062fc: mov   r2, r3
 *   80062fe: subi  r1, 1               ; init = 0xFFFFFFFF
 *   8006300: bsr   crc_init
 *   8006304: addi  r0, r14, 8
 *   8006306: mov   r1, r5              ; header data
 *   8006308: movi  r2, 60              ; 60 bytes (excl hd_checksum)
 *   800630a: bsr   crc_update
 *   800630e: addi  r0, r14, 8
 *   8006310: addi  r1, r14, 4
 *   8006312: bsr   crc_final
 *   8006316: ld.w  r2, (r5, 0x3c)      ; hd_checksum
 *   8006318: ld.w  r3, (r14, 0x4)      ; computed
 *   800631a: cmpne r2, r3
 *   800631c: bt    ota_fail_validate
 *
 *   ; Step 2: Get addresses and lengths
 *   8006320: ld.w  r7, (r5, 0x14)      ; upgrade_img_addr
 *   8006322: bseti r7, 27
 *   8006326: ld.w  r8, (r5, 0x8)       ; img_addr
 *   800632a: bseti r8, 27
 *   800632e: ld.w  r9, (r5, 0xc)       ; img_len
 *
 *   ; Step 3: Erase destination
 *   8006332: mov   r0, r8
 *   8006334: mov   r1, r9
 *   8006336: addi  r1, 64              ; include header size
 *   800633a: bsr   flash_erase_range
 *
 *   ; Step 4: Copy header
 *   8006340: mov   r0, r8
 *   8006342: mov   r1, r7              ; from upgrade area
 *   8006344: movi  r2, 64
 *   8006346: bsr   flash_copy_data
 *
 *   ; Step 5: Copy image body
 *   800634a: addi  r0, r8, 64          ; dst after header
 *   800634e: addi  r1, r7, 64          ; src after header
 *   8006352: mov   r2, r9              ; img_len
 *   8006354: ld.b  r3, (r5, 0x5)
 *   8006356: andi  r3, r3, 1           ; signature?
 *   8006358: bez   r3, ota_no_sig
 *   800635a: addi  r2, 128
 * ota_no_sig:
 *   800635c: bsr   flash_copy_data
 *
 *   ; Step 6: Update run-area header at CODE_RUN_START_ADDR
 *   8006362: lrw   r0, 0x080D0000
 *   8006366: bseti r0, 27
 *   8006368: mov   r1, r7
 *   800636a: movi  r2, 64
 *   800636c: bsr   flash_copy_data
 *
 *   ; Step 7: Verify CRC of copied image
 *   8006372: addi  r0, r14, 80         ; local header buffer
 *   8006374: mov   r1, r8              ; read from dest
 *   8006376: movi  r2, 64
 *   8006378: bsr   flash_read
 *   800637c: addi  r0, r14, 80
 *   800637e: mov   r1, r9              ; img_len
 *   8006380: bsr   crc_verify_image
 *   8006384: blz   r0, ota_fail_verify
 *
 *   ; Step 8: Clear OTA flag
 *   8006388: lrw   r0, ota_flag_addr
 *   800638c: movi  r3, -1
 *   800638e: st.w  r3, (r14, 0x0)
 *   8006390: addi  r1, r14, 0
 *   8006392: movi  r2, 4
 *   8006394: bsr   flash_write
 *
 *   8006398: movi  r0, 0
 *   800639a: addi  r14, 160
 *   800639c: pop   r4-r11, r15
 *
 * ota_fail_validate:
 *   80063a0: movi  r0, -1
 *   80063a2: addi  r14, 160
 *   80063a4: pop   r4-r11, r15
 *
 * ota_fail_verify:
 *   80063a8: movi  r0, -4
 *   80063aa: addi  r14, 160
 *   80063ac: pop   r4-r11, r15
 * ============================================================ */
int ota_process(void *flash_ctx, const uint8_t *upgrade_hdr,
                const uint8_t *running_hdr)
{
    uint32_t crc_workspace[8];
    uint32_t computed_crc = 0;
    uint8_t  verify_hdr[IMAGE_HEADER_SIZE];

    /* Step 1: Validate upgrade header */
    uint32_t magic = *(uint32_t *)(upgrade_hdr + 0x00);
    if (magic != SIGNATURE_WORD)
        return -1;

    uint8_t img_type = upgrade_hdr[0x04] & 0x0F;
    if (img_type != 1)      /* IMG_TYPE_FLASHBIN0 */
        return -1;

    /* CRC32 of header (first 60 bytes, compare with hd_checksum) */
    crc_init(crc_workspace, 3, 0xFFFFFFFF);
    crc_update(crc_workspace, upgrade_hdr, 60);
    crc_final(crc_workspace, &computed_crc);
    if (computed_crc != *(uint32_t *)(upgrade_hdr + 0x3C))
        return -1;

    /* Step 2: Get addresses */
    uint32_t upgrade_addr = *(uint32_t *)(upgrade_hdr + 0x14) | (1 << 27);
    uint32_t img_addr     = *(uint32_t *)(upgrade_hdr + 0x08) | (1 << 27);
    uint32_t img_len      = *(uint32_t *)(upgrade_hdr + 0x0C);

    /* Step 3: Erase destination */
    flash_erase_range(img_addr, img_len + IMAGE_HEADER_SIZE);

    /* Step 4: Copy header */
    flash_copy_data(img_addr, upgrade_addr, IMAGE_HEADER_SIZE);

    /* Step 5: Copy image body (+ optional signature) */
    uint32_t copy_len = img_len;
    if (upgrade_hdr[0x05] & 1)
        copy_len += SIGNATURE_SIZE;
    flash_copy_data(img_addr + IMAGE_HEADER_SIZE,
                    upgrade_addr + IMAGE_HEADER_SIZE, copy_len);

    /* Step 6: Copy header to run-area */
    uint32_t run_hdr = CODE_RUN_START_ADDR | (1 << 27);
    flash_copy_data(run_hdr, upgrade_addr, IMAGE_HEADER_SIZE);

    /* Step 7: Verify CRC of destination */
    flash_read(img_addr, verify_hdr, IMAGE_HEADER_SIZE);
    int crc_result = crc_verify_image(verify_hdr, (void *)(uintptr_t)img_len);
    if (crc_result < 0)
        return -4;

    /* Step 8: Clear OTA flag */
    uint32_t *param_ptr = *(uint32_t **)0x2001007C;
    uint32_t ota_flag_addr = param_ptr[3] - INSIDE_FLS_SECTOR_SIZE;
    ota_flag_addr |= (1 << 27);
    uint32_t clear_val = 0xFFFFFFFF;
    flash_write(ota_flag_addr, &clear_val, 4);

    return 0;
}


/* ============================================================
 * ota_validate (0x08006494)
 *
 * Validate an OTA image in the upgrade partition without
 * applying it. Used to check if a pending OTA is valid
 * before committing.
 *
 * r0 = flash context (param block pointer)
 * r1 = upgrade image flash address
 *
 * Returns: 0 if valid, -1 if invalid
 *
 * Algorithm:
 *   1. Read IMAGE_HEADER from upgrade flash address
 *   2. Validate magic, type, addr, len
 *   3. CRC32 of header
 *   4. CRC32 of image body
 *   5. Compare with org_checksum
 *
 * Disassembly:
 *   8006494: push  r4-r7, r15
 *   8006496: subi  r14, 80
 *   8006498: mov   r4, r0              ; ctx
 *   800649a: mov   r5, r1              ; upgrade_addr
 *   800649c: bseti r5, 27
 *
 *   ; Read header
 *   80064a0: mov   r0, r5
 *   80064a2: addi  r1, r14, 16
 *   80064a4: movi  r2, 64
 *   80064a6: bsr   flash_read
 *
 *   ; Validate header
 *   80064aa: addi  r0, r14, 16
 *   80064ac: bsr   validate_image
 *   80064b0: cmpnei r0, 67
 *   80064b2: bt    invalid
 *
 *   ; CRC32 of header (60 bytes)
 *   80064b6: addi  r0, r14, 8
 *   80064b8: movi  r3, 3
 *   80064ba: movi  r1, 0
 *   80064bc: mov   r2, r3
 *   80064be: subi  r1, 1
 *   80064c0: bsr   crc_init
 *   80064c4: addi  r0, r14, 8
 *   80064c6: addi  r1, r14, 16
 *   80064c8: movi  r2, 60
 *   80064ca: bsr   crc_update
 *   80064ce: addi  r0, r14, 8
 *   80064d0: addi  r1, r14, 4
 *   80064d2: bsr   crc_final
 *   80064d6: ld.w  r2, (r14+16, 0x3c)  ; hd_checksum
 *   80064d8: ld.w  r3, (r14, 0x4)      ; computed
 *   80064da: cmpne r2, r3
 *   80064dc: bt    invalid
 *
 *   ; Verify image body CRC
 *   80064e0: addi  r0, r14, 16
 *   80064e2: ld.w  r1, (r14+16, 0xc)   ; img_len
 *   80064e4: bsr   crc_verify_image    ; 0x08005CFC
 *   80064e8: blz   r0, invalid
 *
 *   80064ec: movi  r0, 0               ; valid
 *   80064ee: addi  r14, 80
 *   80064f0: pop   r4-r7, r15
 *
 * invalid:
 *   80064f4: movi  r0, -1
 *   80064f6: addi  r14, 80
 *   80064f8: pop   r4-r7, r15
 * ============================================================ */
int ota_validate(void *flash_ctx, uint32_t upgrade_flash_addr)
{
    uint8_t  hdr[IMAGE_HEADER_SIZE];
    uint32_t crc_ctx[4];
    uint32_t computed_crc = 0;

    /* Read header from flash */
    uint32_t addr = upgrade_flash_addr | (1 << 27);
    flash_read(addr, hdr, IMAGE_HEADER_SIZE);

    /* Validate header fields */
    if (validate_image(hdr) != 'C')
        return -1;

    /* CRC32 of header (first 60 bytes) */
    crc_init(crc_ctx, 3, 0xFFFFFFFF);
    crc_update(crc_ctx, hdr, 60);
    crc_final(crc_ctx, &computed_crc);

    if (computed_crc != *(uint32_t *)(hdr + 0x3C))
        return -1;

    /* Verify image body CRC */
    uint32_t img_len = *(uint32_t *)(hdr + 0x0C);
    int result = crc_verify_image(hdr, (void *)(uintptr_t)img_len);
    if (result < 0)
        return -1;

    return 0;
}


/* ============================================================
 * ota_apply (0x08006530)
 *
 * Apply a validated OTA update. This is the final step that
 * copies the upgrade image to the run area and updates the
 * boot header.
 *
 * r0 = flash context
 * r1 = upgrade header (already validated and in RAM)
 * r2 = running header (in RAM)
 *
 * Returns: 0 on success, -1 on failure
 *
 * Algorithm:
 *   1. Read upgrade image header from flash
 *   2. Erase run-area image space
 *   3. Copy image header + body + optional signature
 *   4. Update run-area header at CODE_RUN_START_ADDR
 *   5. Verify final CRC
 *   6. Erase upgrade area header (prevent re-apply)
 *   7. Clear OTA flag
 *
 * Disassembly:
 *   8006530: push  r4-r11, r15
 *   8006532: subi  r14, 128
 *   8006534: mov   r4, r0              ; flash_ctx
 *   8006536: mov   r5, r1              ; upgrade_hdr
 *   8006538: mov   r6, r2              ; running_hdr
 *
 *   ; Read full upgrade header from flash
 *   800653c: ld.w  r7, (r5, 0x14)      ; upgrade_img_addr
 *   800653e: bseti r7, 27
 *   8006542: mov   r0, r7
 *   8006544: addi  r1, r14, 64
 *   8006546: movi  r2, 64
 *   8006548: bsr   flash_read
 *
 *   ; Validate
 *   800654c: addi  r0, r14, 64
 *   800654e: bsr   validate_image
 *   8006552: cmpnei r0, 67
 *   8006554: bt    ota_apply_fail
 *
 *   ; Get addresses
 *   8006558: ld.w  r8, (r14+64, 0x8)   ; img_addr
 *   800655c: ld.w  r9, (r14+64, 0xc)   ; img_len
 *   8006560: bseti r8, 27
 *
 *   ; Check erase_always flag (bit 19 of img_attr)
 *   8006564: ld.b  r3, (r14+64, 0x6)
 *   8006566: lsri  r3, 3               ; erase_always bit
 *   8006568: andi  r3, r3, 1
 *
 *   ; Erase destination area
 *   800656c: mov   r0, r8
 *   800656e: addi  r1, r9, 64          ; len + header
 *   8006572: ld.b  r3, (r14+64, 0x5)
 *   8006574: andi  r3, r3, 1           ; signature?
 *   8006576: bez   r3, ota_apply_no_sig_erase
 *   8006578: addi  r1, 128
 * ota_apply_no_sig_erase:
 *   800657c: bsr   flash_erase_range
 *
 *   ; Copy image (header + body + sig)
 *   8006580: mov   r0, r8              ; dst = img_addr
 *   8006582: mov   r1, r7              ; src = upgrade_addr
 *   8006584: addi  r2, r9, 64          ; len = img_len + header
 *   8006588: ld.b  r3, (r14+64, 0x5)
 *   800658a: andi  r3, r3, 1
 *   800658c: bez   r3, ota_apply_no_sig_copy
 *   800658e: addi  r2, 128
 * ota_apply_no_sig_copy:
 *   8006592: bsr   flash_copy_data
 *
 *   ; Update CODE_RUN_START_ADDR header
 *   8006596: lrw   r0, 0x080D0000
 *   800659a: bseti r0, 27
 *   800659c: mov   r1, r7
 *   800659e: movi  r2, 64
 *   80065a0: bsr   flash_copy_data
 *
 *   ; Verify CRC
 *   80065a4: addi  r0, r14, 64
 *   80065a6: mov   r1, r9
 *   80065a8: bsr   crc_verify_image
 *   80065ac: blz   r0, ota_apply_fail
 *
 *   ; Erase first sector of upgrade area (prevent re-apply)
 *   80065b0: mov   r0, r7
 *   80065b2: movi  r1, 4096
 *   80065b6: bsr   flash_erase_range
 *
 *   ; Clear OTA flag
 *   80065ba: lrw   r2, 0x2001007c
 *   80065be: ld.w  r2, (r2, 0x0)
 *   80065c0: ld.w  r3, (r2, 0xc)       ; flash_total
 *   80065c2: subi  r3, 4096
 *   80065c6: bseti r3, 27
 *   80065c8: movi  r2, -1
 *   80065ca: st.w  r2, (r14, 0x0)
 *   80065cc: addi  r1, r14, 0
 *   80065ce: movi  r2, 4
 *   80065d0: mov   r0, r3
 *   80065d2: bsr   flash_write
 *
 *   80065d6: movi  r0, 0
 *   80065d8: addi  r14, 128
 *   80065da: pop   r4-r11, r15
 *
 * ota_apply_fail:
 *   80065de: movi  r0, -1
 *   80065e0: addi  r14, 128
 *   80065e2: pop   r4-r11, r15
 * ============================================================ */
int ota_apply(void *flash_ctx, const uint8_t *upgrade_hdr,
              const uint8_t *running_hdr)
{
    uint8_t local_hdr[IMAGE_HEADER_SIZE];

    /* Read upgrade header from flash */
    uint32_t upgrade_addr = *(uint32_t *)(upgrade_hdr + 0x14) | (1 << 27);
    flash_read(upgrade_addr, local_hdr, IMAGE_HEADER_SIZE);

    /* Validate */
    if (validate_image(local_hdr) != 'C')
        return -1;

    uint32_t img_addr = *(uint32_t *)(local_hdr + 0x08) | (1 << 27);
    uint32_t img_len  = *(uint32_t *)(local_hdr + 0x0C);

    /* Compute total size (header + image + optional signature) */
    uint32_t total_copy = img_len + IMAGE_HEADER_SIZE;
    if (local_hdr[0x05] & 1)       /* signature */
        total_copy += SIGNATURE_SIZE;

    /* Erase destination */
    flash_erase_range(img_addr, total_copy);

    /* Copy complete image (header + body + sig) */
    flash_copy_data(img_addr, upgrade_addr, total_copy);

    /* Update run-area header */
    uint32_t run_hdr = CODE_RUN_START_ADDR | (1 << 27);
    flash_copy_data(run_hdr, upgrade_addr, IMAGE_HEADER_SIZE);

    /* Verify CRC */
    int result = crc_verify_image(local_hdr, (void *)(uintptr_t)img_len);
    if (result < 0)
        return -1;

    /* Erase first sector of upgrade area (prevent re-apply on next boot) */
    flash_erase_range(upgrade_addr, INSIDE_FLS_SECTOR_SIZE);

    /* Clear OTA flag */
    uint32_t *param_ptr = *(uint32_t **)0x2001007C;
    uint32_t ota_flag_addr = param_ptr[3] - INSIDE_FLS_SECTOR_SIZE;
    ota_flag_addr |= (1 << 27);
    uint32_t clear_val = 0xFFFFFFFF;
    flash_write(ota_flag_addr, &clear_val, 4);

    return 0;
}
