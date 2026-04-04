/**
 * secboot_common.h - Shared definitions for secboot.bin decompilation
 *
 * Common header file for all decompiled secboot source files.
 * Consolidates register addresses, structure definitions, constants,
 * and cross-file function declarations that were previously duplicated
 * across individual .c files.
 *
 * References:
 *   - include/wm_regs.h                    (peripheral register addresses)
 *   - include/driver/wm_flash_map.h        (flash memory map constants)
 *   - include/driver/wm_internal_flash.h   (flash sector size)
 *   - include/platform/wm_fwup.h           (IMAGE_HEADER_PARAM_ST)
 *   - include/platform/wm_crypto_hard.h    (crypto engine, hstm_int)
 *   - platform/inc/libtommath.h            (mp_int, DIGIT_BIT)
 */

#ifndef SECBOOT_COMMON_H
#define SECBOOT_COMMON_H

/* ============================================================
 * Standard Library Headers
 * ============================================================ */
#include <stdint.h>
#include <stddef.h>     /* size_t, NULL */
#include <stdarg.h>     /* va_list, va_start, va_end */

#ifndef NULL
#define NULL ((void *)0)
#endif

/* ============================================================
 * Peripheral Register Base Addresses
 * (Reference: include/wm_regs.h)
 * ============================================================ */

/* APB peripherals */
#define DEVICE_BASE_ADDR        0x40000000
#define RSA_BASE                0x40000000  /* RSA accelerator engine */
#define RSA_CTRL_BASE           0x40000400  /* RSA control registers */
#define CRYPTO_BASE             0x40000600  /* Hardware crypto/hash/CRC engine */
#define PMU_BASE                0x40000D00  /* Power management unit */
#define WDG_BASE                0x40000E00  /* Watchdog timer */
#define CLK_BASE                0x40011400  /* Clock/PMU control */
#define UART0_BASE              0x40010600  /* UART0 */

/* Internal flash SPI controller */
#define FLASH_SPI_BASE          0xC0002000

/* ARM-style interrupt controller */
#define VIC_BASE                0xE000E000  /* Vector interrupt controller */
#define VIC_INT_EN_BASE         0xE000E100  /* Interrupt enable registers */

/* ============================================================
 * UART0 Register Offsets
 * (Reference: include/wm_regs.h HR_UART0_*)
 * ============================================================ */
#define UART_LINE_CTRL          0x00  /* Line control register */
#define UART_DMA_CTRL           0x04  /* DMA control */
#define UART_FLOW_CTRL          0x08  /* Flow control */
#define UART_INT_MASK           0x0C  /* Interrupt mask */
#define UART_BAUD_RATE          0x10  /* Baud rate divider */
#define UART_INT_SRC            0x18  /* Interrupt source */
#define UART_FIFO_STATUS        0x1C  /* FIFO status */
#define UART_TX_DATA            0x20  /* TX data window */
#define UART_RX_DATA            0x30  /* RX data window */

/* FIFO status masks */
#define UART_TX_FIFO_CNT_MASK   0x3F    /* TX FIFO count (bits 5:0) */
#define UART_RX_FIFO_CNT_MASK   0xFC0   /* RX FIFO count (bits 11:6) */

/* APB clock for baud rate calculation */
#define APB_CLK                 40000000

/* ============================================================
 * Watchdog Registers
 * ============================================================ */
#define WDG_FEED_REG            (*(volatile uint32_t *)(WDG_BASE + 0x08))
#define WDG_CTRL_REG            (*(volatile uint32_t *)(WDG_BASE + 0x14))

/* ============================================================
 * Clock Control
 * ============================================================ */
#define CLK_CTRL_OFFSET         0x0C

/* ============================================================
 * VIC Interrupt Controller Offsets
 * ============================================================ */
#define VIC_INT_CLR_OFFSET      0x180
#define VIC_INT_PEND_OFFSET     0x200

/* ============================================================
 * Hardware Crypto Engine Registers
 * (Reference: include/platform/wm_crypto_hard.h)
 *
 * Base: 0x40000600
 * ============================================================ */
#define CRYPTO_SRC_ADDR         (*(volatile uint32_t *)(CRYPTO_BASE + 0x00))
#define CRYPTO_DEST_ADDR        (*(volatile uint32_t *)(CRYPTO_BASE + 0x04))
#define CRYPTO_CONFIG           (*(volatile uint32_t *)(CRYPTO_BASE + 0x08))  /* SEC_CFG */
#define CRYPTO_CTRL             (*(volatile uint32_t *)(CRYPTO_BASE + 0x0C))  /* SEC_CTRL */
#define CRYPTO_STATUS           (*(volatile uint32_t *)(CRYPTO_BASE + 0x30))  /* SEC_STS */

/* SHA-1 digest registers */
#define CRYPTO_HASH_A           (*(volatile uint32_t *)(CRYPTO_BASE + 0x34))  /* SHA1_DIGEST0 */
#define CRYPTO_HASH_B           (*(volatile uint32_t *)(CRYPTO_BASE + 0x38))  /* SHA1_DIGEST1 */
#define CRYPTO_HASH_C           (*(volatile uint32_t *)(CRYPTO_BASE + 0x3C))  /* SHA1_DIGEST2 */
#define CRYPTO_HASH_D           (*(volatile uint32_t *)(CRYPTO_BASE + 0x40))  /* SHA1_DIGEST3 */
#define CRYPTO_HASH_E           (*(volatile uint32_t *)(CRYPTO_BASE + 0x44))  /* SHA1_DIGEST4 / CRC result */

/* ============================================================
 * RSA Accelerator Engine
 * (Reference: include/platform/wm_crypto_hard.h)
 *
 * Base: 0x40000000
 * Buffers at 0x40000000..0x400003FF, control at 0x40000400
 * ============================================================ */
#define RSA_BUF_A               ((volatile uint32_t *)0x40000000)  /* X buffer */
#define RSA_BUF_B               ((volatile uint32_t *)0x40000100)  /* Y buffer */
#define RSA_BUF_M               ((volatile uint32_t *)0x40000200)  /* M buffer */
#define RSA_BUF_D               ((volatile uint32_t *)0x40000300)  /* D buffer */
#define RSACON                  (*(volatile uint32_t *)(RSA_CTRL_BASE + 0x00))
#define RSAMC                   (*(volatile uint32_t *)(RSA_CTRL_BASE + 0x04))
#define RSASIZE                 (*(volatile uint32_t *)(RSA_CTRL_BASE + 0x08))

/* ============================================================
 * Flash Memory Map Constants
 * (Reference: include/driver/wm_flash_map.h,
 *             include/driver/wm_internal_flash.h)
 * ============================================================ */
#define FLASH_BASE_ADDR         0x08000000
#define CODE_UPD_START_ADDR     0x08010000  /* Upgrade image area */
#define CODE_RUN_START_ADDR     0x080D0000  /* Runtime image header area */
#define INSIDE_FLS_SECTOR_SIZE  0x1000      /* 4KB sector size */
#define SIGNATURE_WORD          0xA0FFFF9F  /* Image header magic number */
#define IMAGE_HEADER_SIZE       64          /* Bytes per image header */
#define SIGNATURE_SIZE          128         /* RSA signature size */

/* Flash address masks */
#define FLASH_MAX_ADDR_MASK     0x07FFFFFF  /* bmaski(27) = 128MB */
#define FLASH_2MB_OFFSET        0x00800000  /* 2MB boundary */

/* ============================================================
 * SRAM Address Map
 * ============================================================ */

/* Boot parameters */
#define PARAM_BLOCK_PTR_ADDR    0x2001007C  /* Pointer to param block */
#define BOOT_PARAMS_BASE        0x200101A0  /* Boot parameter structure */
#define APP_ENTRY_PTR_ADDR      0x200101D0  /* App entry function pointer */
#define ERROR_HANDLER_PTR_ADDR  0x200101D4  /* Error handler function pointer */
#define BOOT_MODE_FLAG_ADDR     0x200113EC  /* Boot mode flag byte */

/* Interrupt/trap state */
#define TSPEND_COUNTER_ADDR     0x200113E4  /* Timer/software pending counter */
#define TRAP_SAVE_SP_ADDR       0x200113D8  /* Trap handler save area */
#define INITIAL_SP              0x200111D8  /* Initial stack pointer */

/* Heap */
#define HEAP_START              0x20011430
#define HEAP_END                0x20028000
#define HEAP_STATE_ADDR         0x20010060
#define HEAP_LOCK_ADDR          0x2001142C

/* ============================================================
 * SRAM Access Helpers
 * ============================================================ */
#define PARAM_BLOCK_PTR         (*(volatile uint32_t *)PARAM_BLOCK_PTR_ADDR)
#define APP_ENTRY_PTR           (*(volatile uint32_t *)APP_ENTRY_PTR_ADDR)
#define ERROR_HANDLER_PTR       (*(volatile uint32_t *)ERROR_HANDLER_PTR_ADDR)
#define BOOT_MODE_FLAG          (*(volatile uint8_t *)BOOT_MODE_FLAG_ADDR)

/* ============================================================
 * Image Header Structure
 * (Reference: include/platform/wm_fwup.h IMAGE_HEADER_PARAM_ST)
 *
 * The binary uses this as a 64-byte structure at flash addresses.
 * Field offsets match the SDK definition exactly.
 * ============================================================ */
typedef union {
    struct {
        uint32_t img_type       : 4;   /* bits  0.. 3: IMAGE_TYPE_ENUM */
        uint32_t code_encrypt   : 1;   /* bit   4:     code encrypted in flash */
        uint32_t prikey_sel     : 3;   /* bits  5.. 7: private key selection */
        uint32_t signature      : 1;   /* bit   8:     signature present */
        uint32_t _reserved1     : 7;   /* bits  9..15 */
        uint32_t zip_type       : 1;   /* bit  16:     compressed */
        uint32_t psram_io       : 1;   /* bit  17 */
        uint32_t erase_block_en : 1;   /* bit  18 */
        uint32_t erase_always   : 1;   /* bit  19 */
        uint32_t _reserved2     : 12;  /* bits 20..31 */
    } b;
    uint32_t w;
} img_attr_t;

typedef struct image_header_param {
    uint32_t    magic_no;           /* 0x00: must be SIGNATURE_WORD */
    img_attr_t  img_attr;           /* 0x04: image attributes bitfield */
    uint32_t    img_addr;           /* 0x08: image start address in flash */
    uint32_t    img_len;            /* 0x0C: image data length */
    uint32_t    img_header_addr;    /* 0x10: header location in flash */
    uint32_t    upgrade_img_addr;   /* 0x14: upgrade image address */
    uint32_t    org_checksum;       /* 0x18: original hash/checksum */
    uint32_t    upd_no;             /* 0x1C: update sequence number */
    uint8_t     ver[16];            /* 0x20: version string */
    uint32_t    _reserved0;         /* 0x30 */
    uint32_t    _reserved1;         /* 0x34 */
    struct image_header_param *next; /* 0x38: pointer to next header */
    uint32_t    hd_checksum;        /* 0x3C: header CRC32 */
} image_header_t;

/* Image type codes */
#define IMG_TYPE_SECBOOT        0x0
#define IMG_TYPE_FLASHBIN0      0x1
#define IMG_TYPE_CPFT           0xE

/* ============================================================
 * Boot Status Codes
 * (ASCII characters returned by various validation functions)
 * ============================================================ */
#define STATUS_OK               'C'  /* 0x43 - Check passed / CRC OK */
#define STATUS_BAD_MAGIC        'L'  /* 0x4C - Bad magic number */
#define STATUS_BAD_ALIGN        'K'  /* 0x4B - Bad alignment */
#define STATUS_BAD_RANGE        'J'  /* 0x4A - Address out of range / not found */
#define STATUS_BAD_LEN          'I'  /* 0x49 - Invalid length */
#define STATUS_CRC_BAD          'Z'  /* 0x5A - CRC mismatch */
#define STATUS_COPY_FAIL        'M'  /* 0x4D - Copy/verify failed */
#define STATUS_FLASH_FAIL       'N'  /* 0x4E - Flash init failed */
#define STATUS_APP_FAIL         'Y'  /* 0x59 - App boot handler returned error */
#define STATUS_SIG_FAIL         'M'  /* 0x4D - Signature verification failed */

/* ============================================================
 * Bignum / mp_int Structure
 * (Reference: include/platform/wm_crypto_hard.h hstm_int,
 *             platform/inc/libtommath.h mp_int)
 *
 * The firmware uses libtommath-compatible bignums with DIGIT_BIT=28.
 * The SDK names this "hstm_int" but the structure is identical to mp_int.
 *
 * NOTE: In this firmware, mp_copy(dst, src) copies src → dst,
 * which is REVERSED from standard libtommath mp_copy(src, dst).
 * ============================================================ */
#define DIGIT_BIT               28
#define MP_MASK                 ((1u << DIGIT_BIT) - 1)

typedef struct {
    int16_t     used;
    int16_t     alloc;
    int16_t     sign;
    /* 2 bytes padding to align dp pointer */
    uint32_t    *dp;
} mp_int;

/* ============================================================
 * SHA-1 Context Structure
 *
 * Used by sha_init/sha_update/sha_final and sha1_init/sha1_update/sha1_final.
 * Compatible with SDK psSha1_t (include/platform/wm_crypto_hard.h).
 * ============================================================ */
typedef struct sha1_ctx {
    uint32_t    total_hi;       /* Total bytes processed (high 32 bits) */
    uint32_t    total_lo;       /* Total bytes processed (low 32 bits) */
    uint32_t    state[5];       /* SHA-1 hash state (H0..H4) */
    uint32_t    buf_used;       /* Bytes currently in buffer */
    uint8_t     buffer[64];     /* Pending data buffer (512-bit block) */
} sha1_ctx_t;

/* SHA-1 initial hash values */
#define SHA1_H0                 0x67452301
#define SHA1_H1                 0xEFCDAB89
#define SHA1_H2                 0x98BADCFE
#define SHA1_H3                 0x10325476
#define SHA1_H4                 0xC3D2E1F0

/* ============================================================
 * Hardware Crypto Operation Descriptor
 * ============================================================ */
typedef struct {
    uint32_t    data;
    uint8_t     mode;
    uint8_t     flags;
} hw_crypto_op_t;

/* ============================================================
 * CRC Verification Context
 * ============================================================ */
typedef struct {
    void        *inner_ctx;
    uint32_t    digest_len;
    uint32_t    flags;
} crc_ctx_t;

/* ============================================================
 * RSA Key Structure
 *
 * 104 bytes total. Contains two mp_int bignums for e and n,
 * plus CRT parameters. key_size at offset 0x60 holds the RSA
 * modulus size in bytes (e.g., 128 for RSA-1024).
 *
 * Parsed by signature_check_final() (0x08004BEC).
 * ============================================================ */
typedef struct {
    uint8_t     data[0x60];     /* e(12) + d(12) + n(12) + CRT params */
    uint32_t    key_size;       /* Offset 0x60: RSA key size in bytes */
    uint32_t    _pad;
} rsa_key_t;

/* ============================================================
 * Heap Allocator Structures
 * (Used by malloc/free/realloc in secboot_memory.c)
 * ============================================================ */
#define BLOCK_HEADER_SIZE       8
#define MIN_SPLIT_SIZE          15
#define HEAP_ALIGNMENT          8

typedef struct free_block {
    struct free_block *next;
    uint32_t    size;
} free_block_t;

typedef struct {
    uint32_t    initialized;
    free_block_t *free_list;
} heap_state_t;

/* Printf buffer descriptor (used by vsnprintf/sprintf) */
typedef struct {
    uint16_t    _field0;
    int16_t     capacity;
    uint32_t    _field4;
    void        *buffer;
} buf_desc_t;

/* ============================================================
 * C-SKY Control Register IDs
 * (Used by mtcr/mfcr instructions in secboot_hw_init.c)
 * ============================================================ */
#define CR_PSR                  0   /* cr<0,0> - Processor Status Register */
#define CR_VBR                  1   /* cr<1,0> - Vector Base Register */
#define CR_EPC                  2   /* cr<2,0> - Exception PC */
#define CR_EPSR                 4   /* cr<4,0> - Exception PSR */
#define CR_HINT                 31  /* cr<31,0> - Cache hint/control */

/* ============================================================
 * XMODEM Protocol Constants
 * (Used by secboot_fwup.c)
 * ============================================================ */
#define XMODEM_SOH              0x01
#define XMODEM_STX              0x02
#define XMODEM_EOT              0x04
#define XMODEM_ACK              0x06
#define XMODEM_NAK              0x15
#define XMODEM_CAN              0x18
#define XMODEM_CRC_CHR          'C'

/* ============================================================
 * Cross-File Function Declarations
 *
 * These extern declarations allow each .c file to call functions
 * defined in other decompiled source files. Addresses are listed
 * in comments for cross-reference with the binary.
 * ============================================================ */

/* --- secboot_stdlib.c --- */
extern void  puts(const char *s);                                       /* 0x080028F4 */
extern void  printf_simple(const char *fmt, ...);                       /* 0x0800290C */
extern void *calloc(uint32_t nmemb, uint32_t size);                    /* 0x08002980 */

/* --- secboot_memory.c --- */
extern void *malloc(uint32_t size);                                     /* 0x080029A0 */
extern void  free(void *ptr);                                           /* 0x08002A3C */
extern void *realloc_internal(void *ptr, uint32_t size);                /* 0x08002958 */

/* --- secboot_stdlib.c (C library functions) ---
 * These are custom C-SKY minilibc implementations, NOT GCC builtins.
 * Must compile with -fno-builtin to avoid replacement by GCC intrinsics.
 * Binary sizes are much larger than GCC builtins due to word-aligned
 * optimization loops (4-byte batch + tail byte processing). */
extern void *memcpy(void *dst, const void *src, uint32_t n);           /* 0x08002B54 (128 bytes) */
extern void *memset(void *s, int c, uint32_t n);                       /* 0x08002D20 (92 bytes) */
extern int   memcmp(const void *a, const void *b, uint32_t n);         /* 0x08002BD4 (332 bytes) */
extern uint32_t strlen(const char *s);                                  /* 0x08002D7C (304 bytes) */

/* --- secboot_uart.c --- */
extern int   uart_rx_ready(void);                                       /* 0x0800588C */
extern uint8_t uart_getchar(void);                                      /* 0x080058A0 */
extern void  uart_init(uint32_t baud);                                  /* 0x080058B8 */
extern int   uart_putchar(int ch);                                      /* 0x0800717C */

/* --- secboot_flash.c --- */
extern uint8_t flash_init(void);                                        /* 0x08005338 */
extern int   flash_read(uint32_t addr, uint8_t *buf, uint32_t len);    /* 0x080054F8 */
extern int   flash_write(uint32_t addr, const uint8_t *src, uint32_t len);  /* 0x0800553C */
extern int   flash_read_raw(uint32_t addr, uint8_t *buf, uint32_t len);   /* 0x08005358 */
extern int   flash_erase_range(uint32_t addr, uint32_t len);           /* 0x08005670 */
extern void  flash_protect_config(uint32_t mode);                       /* 0x080056B0 */
extern int   flash_copy_data(uint32_t dst, uint32_t src, uint32_t len);  /* 0x0800582C */
extern int   flash_param_read(uint8_t *dest, uint32_t len);               /* 0x080057EC */
extern int   flash_param_write(const uint8_t *src, uint32_t len);         /* 0x08005804 */
extern void  flash_param_init(void);                                    /* 0x08005820 */

/* --- secboot_image.c --- */
extern int   image_copy_update(const uint8_t *header, uint32_t data_length); /* 0x080058EC */
extern int   validate_image(const uint8_t *header);                         /* 0x08005988 */
extern int   find_valid_image(uint32_t start_addr, uint8_t *hdr_buf, int mode); /* 0x08007278 */
extern int   signature_verify(uint32_t data_addr, uint32_t data_len,
                               uint32_t sig_addr);                          /* 0x080070C4 */

/* --- secboot_crypto.c --- */
extern crc_ctx_t *crc_ctx_alloc(void);                                      /* 0x08004CFC */
extern void  crc_ctx_destroy(crc_ctx_t *ctx);                              /* 0x08004D2C */
extern void  crc_ctx_reset(void *inner);                                    /* 0x08004D50 */
extern int   crc_verify_image(void *hdr, void *param);                     /* 0x08005CFC */
extern int   hw_crypto_setup(hw_crypto_op_t *op, uint32_t length,
                              uint16_t param);                              /* 0x08004404 */
extern int   hw_crypto_exec(hw_crypto_op_t *op, uint32_t src_addr,
                              uint16_t data_word);                          /* 0x08004410 */
extern int   hw_crypto_exec2(hw_crypto_op_t *op, uint32_t src_addr,
                               uint16_t data_word);                         /* 0x080044B8 */
extern int   hash_finalize(uint32_t *result);                               /* 0x08004544 */
extern int   hash_get_result(uint32_t *in, uint32_t *out);                  /* 0x0800459C */
extern void  sha1_init(sha1_ctx_t *ctx);                                    /* 0x080045A4 */
extern void  sha1_update(sha1_ctx_t *ctx, const uint8_t *data, uint32_t len); /* 0x080045D8 */
extern int   sha1_final(sha1_ctx_t *ctx, uint8_t *digest);                 /* 0x0800463C */
extern int   cert_parse(uint8_t *data, uint32_t total_len,
                         uint8_t *out, uint32_t len, uint32_t marker);     /* 0x08004C78 */
extern int   pkey_setup(uint8_t **pos, uint32_t remaining,
                         uint32_t *out_len);                                /* 0x080049CC */
extern int   pkey_verify(uint8_t **pos, uint32_t remaining,
                          mp_int *out);                                     /* 0x08004AB4 */
extern int   signature_check_data(uint8_t **pos, uint32_t remaining,
                                   uint32_t *out1, uint32_t *out2);        /* 0x08004BB0 */

/* CRC convenience aliases used by secboot_fwup.c and secboot_image.c.
 * These call the same addresses as hw_crypto_setup/exec/hash_get_result
 * with simplified void-pointer interfaces for the CRC use case. */
extern int   crc_init(void *ctx, int type, uint32_t init);                  /* 0x08004404 */
extern int   crc_update(void *ctx, const void *data, uint32_t len);        /* 0x08004410 */
extern int   crc_final(void *ctx, uint32_t *result);                        /* 0x0800459C */

/* --- secboot_crypto.c (bignum / libtommath) ---
 * NOTE: In this firmware, mp_copy(dst, src) is REVERSED from standard
 * libtommath. It copies src → dst (first arg = destination). */
extern int   mp_init(mp_int *a);                                           /* 0x08003178 */
extern void  mp_clear(mp_int *a);                                          /* 0x080031A8 */
extern int   mp_copy(mp_int *dst, mp_int *src);                            /* 0x08003390 */
extern int   mp_init_copy(mp_int *dst, mp_int *src);                       /* 0x08003408 */
extern int   mp_cmp_mag(mp_int *a, mp_int *b);                             /* 0x08003238 */
extern int   mp_cmp(mp_int *a, mp_int *b);                                 /* 0x08003298 */
extern int   mp_sub(mp_int *a, mp_int *b, mp_int *c);                      /* 0x080032CC */
extern int   mp_add(mp_int *a, mp_int *b, mp_int *c);                      /* 0x0800332C */
extern int   mp_grow(mp_int *a, int16_t size);                              /* 0x080034E4 */
extern int   mp_lshd(mp_int *a, int16_t count);                             /* 0x08003514 */
extern void  mp_rshd(mp_int *a, int16_t count);                             /* 0x08003588 */
extern void  mp_clamp(mp_int *a);                                           /* 0x080034A0 */
extern int   mp_read_unsigned_bin(mp_int *a, const uint8_t *b, int len);    /* 0x08003690 */
extern int   mp_count_bits(mp_int *a);                                      /* 0x08003744 */
extern int   mp_unsigned_bin_size(mp_int *a);                               /* 0x080031DC */
extern int   mp_to_unsigned_bin_nr(mp_int *a, uint8_t *b);                  /* 0x08003774 */
extern void  mp_reverse(uint8_t *s, int len);                               /* 0x08003150 */
extern int   mp_div_2d(mp_int *a, int b, mp_int *c, mp_int *d);            /* 0x08003860 */
extern int   s_mp_mul_digs(mp_int *a, mp_int *b, mp_int *c, int digs);     /* 0x08002FC8 */

/* --- secboot_hw_init.c --- */
extern void  SystemInit(void);                                              /* 0x080071B4 */
extern void  board_init(void);                                              /* 0x0800712C */
extern int   lock_acquire(uint32_t *lock);                                  /* 0x080071AC */
extern int   lock_release(uint32_t *lock);                                  /* 0x080071B0 */
extern void  trap_c(void *context);                                         /* 0x080071F0 */

/* --- secboot_main.c --- */
extern int   image_header_verify(const uint8_t *header, uint32_t data_param); /* 0x080071F4 */
extern int   boot_uart_check(void);                                         /* 0x08007220 */

/* --- secboot_fwup.c (xmodem/firmware update) --- */
extern int   flash_read_cert(void *ctx, void *buf, int len);               /* 0x08005B88 */
extern int   flash_cert_parse(void *ctx, void *hdr, void *out);            /* 0x08005BC8 */

/* --- secboot_crypto.c (RSA operations) --- */
extern int   rsa_core(mp_int *bn, uint8_t *out);                           /* 0x080039C0 */
extern int   rsa_step(mp_int *bn, uint8_t *out);                           /* 0x08003A14 */
extern int   rsa_modexp(mp_int *a, int bits);                               /* 0x08003A8C */
extern int   rsa_init(mp_int *a, int16_t size);                             /* 0x08003AE8 */
extern int   rsa_process(mp_int *a, mp_int *b, mp_int *c, int16_t digs);   /* 0x08003B3C */
extern int   sha_init(mp_int *a, uint32_t size);                           /* 0x08003CDC */
extern int   sha1_full(mp_int *a, mp_int *e, mp_int *n, mp_int *res);     /* 0x0800472C */

#endif /* SECBOOT_COMMON_H */
