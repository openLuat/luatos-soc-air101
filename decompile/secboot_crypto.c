/**
 * secboot_crypto.c - Decompiled CRC/crypto operations
 *
 * Pseudo-C reconstruction from C-SKY XT804 disassembly of secboot.bin
 * Cross-referenced with: tools/xt804/wm_tool.c (CRC32 table),
 *                         platform/common/crypto/ (SHA1 patterns)
 *
 * Functions (34 total):
 *   --- RSA big-number arithmetic ---
 *   rsa_core()           - 0x080039C0  (core RSA read loop)
 *   rsa_step()           - 0x08003A14  (RSA step with byte-swap)
 *   rsa_modexp()         - 0x08003A8C  (RSA modular exponentiation init)
 *   rsa_init()           - 0x08003AE8  (RSA bignum allocation)
 *   rsa_process()        - 0x08003B3C  (RSA multiply-accumulate)
 *   --- SHA (bignum-based, calls into RSA engine) ---
 *   sha_hash_block()     - 0x08003CA4  (hash one block via rsa_process)
 *   sha_init()           - 0x08003CDC  (SHA bignum init wrapper)
 *   sha_update()         - 0x08003CF0  (SHA bignum update / multiply)
 *   sha_final()          - 0x08003E1C  (SHA bignum finalize)
 *   --- CRC32 (table-based via HW engine) ---
 *   crc32_table_init()   - 0x08004258  (init CRC32 lookup bignum)
 *   crc32_update()       - 0x080042DC  (CRC32 multiply step)
 *   --- SHA-1 (hardware-accelerated) ---
 *   sha1_transform()     - 0x08004324  (SHA-1 block transform via HW engine)
 *   --- Hash context management ---
 *   hash_ctx_init()      - 0x0800437C  (hash context alloc + init)
 *   hw_crypto_setup()    - 0x08004404  (hardware crypto engine configuration)
 *   hw_crypto_exec()     - 0x08004410  (execute hardware crypto operation)
 *   hw_crypto_exec2()    - 0x080044B8  (execute hardware crypto op variant)
 *   hash_finalize()      - 0x08004544  (finalize hash, get digest)
 *   hash_get_result()    - 0x0800459C  (copy result word to output)
 *   --- SHA-1 context operations ---
 *   sha1_init()          - 0x080045A4  (SHA-1 context initialization)
 *   sha1_update()        - 0x080045D8  (SHA-1 data feed)
 *   sha1_final()         - 0x0800463C  (SHA-1 finalize + digest output)
 *   sha1_full()          - 0x0800472C  (SHA-1 one-shot: init+update+final)
 *   --- Crypto subsystem ---
 *   crypto_subsys_init() - 0x08004906  (cryptographic subsystem init)
 *   pkey_setup()         - 0x080049CC  (public key setup)
 *   pkey_verify_step()   - 0x08004A4C  (public key verify step)
 *   pkey_verify()        - 0x08004AB4  (public key verification)
 *   --- Signature / certificate ---
 *   signature_check_init()  - 0x08004AF8  (init signature check)
 *   signature_check_data()  - 0x08004BB0  (feed data to signature check)
 *   signature_check_final() - 0x08004BEC  (finalize signature check)
 *   cert_parse()         - 0x08004C78  (parse certificate structure)
 *   --- CRC verification context ---
 *   crc_ctx_alloc()      - 0x08004CFC  (allocate CRC verification context)
 *   crc_ctx_destroy()    - 0x08004D2C  (destroy CRC context)
 *   crc_ctx_reset()      - 0x08004D50  (reset CRC internal state)
 *   --- Image verification ---
 *   crc_verify_image()   - 0x08005CFC  (full CRC32 image verification)
 */

#include <stdint.h>
#ifndef NULL
#define NULL ((void *)0)
#endif

/* ============================================================
 * Hardware Crypto Engine Registers
 *
 * Base: 0x40000600 (movih 16384 + addi 1536)
 *
 * Offset  Name           Description
 * 0x00    SRC_ADDR       Source data address
 * 0x08    CONFIG         Operation config (mode, key, etc.)
 * 0x0C    CTRL           Start/status control (write 1 to start)
 * 0x30    STATUS         Completion status (bit 16 = done)
 * 0x34    HASH_A         SHA-1 state A / CRC result
 * 0x38    HASH_B         SHA-1 state B
 * 0x3C    HASH_C         SHA-1 state C
 * 0x40    HASH_D         SHA-1 state D
 * 0x44    HASH_E / CRC   SHA-1 state E / CRC32 result
 * ============================================================ */
#define CRYPTO_BASE         0x40000600
#define CRYPTO_SRC_ADDR     (*(volatile uint32_t *)(CRYPTO_BASE + 0x00))
#define CRYPTO_CONFIG       (*(volatile uint32_t *)(CRYPTO_BASE + 0x08))
#define CRYPTO_CTRL         (*(volatile uint32_t *)(CRYPTO_BASE + 0x0C))
#define CRYPTO_STATUS       (*(volatile uint32_t *)(CRYPTO_BASE + 0x30))
#define CRYPTO_HASH_A       (*(volatile uint32_t *)(CRYPTO_BASE + 0x34))
#define CRYPTO_HASH_B       (*(volatile uint32_t *)(CRYPTO_BASE + 0x38))
#define CRYPTO_HASH_C       (*(volatile uint32_t *)(CRYPTO_BASE + 0x3C))
#define CRYPTO_HASH_D       (*(volatile uint32_t *)(CRYPTO_BASE + 0x40))
#define CRYPTO_HASH_E       (*(volatile uint32_t *)(CRYPTO_BASE + 0x44))

/* External functions */
extern void *malloc(uint32_t size);                /* 0x080029A0 */
extern void *calloc(uint32_t nmemb, uint32_t sz);  /* 0x08002980 */
extern void  free(void *ptr);                       /* 0x08002A3C */
extern void *memcpy(void *dst, const void *src,
                    uint32_t n);                    /* 0x08002B54 */
extern void *memset(void *s, int c, uint32_t n);   /* 0x08002AB4 */
extern int   memcmp(const void *a, const void *b,
                    uint32_t n);                    /* 0x08002BD4 */
extern void  bignum_clear(void *bn);               /* 0x080031A8 */

/* Forward type declarations */
typedef struct sha1_ctx sha1_ctx_t;

/* Forward declarations */
static void sha1_transform(sha1_ctx_t *ctx);
static int  asn1_parse_tag(uint8_t **pos, uint32_t *out_len, uint32_t remaining);
void crc_ctx_reset(void *inner);


/* ============================================================
 * SHA-1 Context Structure
 *
 * Layout (from register usage analysis):
 *   Offset  Size  Field
 *   0x00    4     total_hi     (high 32 bits of total bit count)
 *   0x04    4     total_lo     (low 32 bits of total bit count)
 *   0x08    4     state[0]     (H0 = 0x67452301)
 *   0x0C    4     state[1]     (H1 = 0xEFCDAB89)
 *   0x10    4     state[2]     (H2 = 0x98BADCFE)
 *   0x14    4     state[3]     (H3 = 0x10325476)
 *   0x18    4     state[4]     (H4 = 0xC3D2E1F0)
 *   0x1C    4     buf_used     (bytes in buffer, 0..63)
 *   0x20    64    buffer[64]   (partial block buffer)
 *
 * Total size: 96 bytes
 * ============================================================ */
typedef struct sha1_ctx {
    uint32_t total_hi;
    uint32_t total_lo;
    uint32_t state[5];
    uint32_t buf_used;
    uint8_t  buffer[64];
} sha1_ctx_t;

/* SHA-1 initial hash values (from literal pool at 0x080045C4) */
#define SHA1_H0  0x67452301
#define SHA1_H1  0xEFCDAB89
#define SHA1_H2  0x98BADCFE
#define SHA1_H3  0x10325476
#define SHA1_H4  0xC3D2E1F0


/* ============================================================
 * sha1_init (0x080045A4)
 *
 * Initialize SHA-1 context with standard IV.
 *
 * r0 = pointer to sha1_ctx_t
 *
 * Disassembly:
 *   80045a4: lrw   r3, 0x67452301       ; H0
 *   80045a6: st.w  r3, (r0, 0x08)
 *   80045a8: lrw   r3, 0xEFCDAB89       ; H1
 *   80045aa: st.w  r3, (r0, 0x0C)
 *   80045ac: lrw   r3, 0x98BADCFE       ; H2
 *   80045ae: st.w  r3, (r0, 0x10)
 *   80045b0: lrw   r3, 0x10325476       ; H3
 *   80045b2: st.w  r3, (r0, 0x14)
 *   80045b4: lrw   r3, 0xC3D2E1F0       ; H4
 *   80045b6: st.w  r3, (r0, 0x18)
 *   80045b8: movi  r3, 0
 *   80045ba: st.w  r3, (r0, 0x1C)       ; buf_used = 0
 *   80045bc: st.w  r3, (r0, 0x00)       ; total_hi = 0
 *   80045be: st.w  r3, (r0, 0x04)       ; total_lo = 0
 *   80045c0: jmp   r15
 * ============================================================ */
void sha1_init(sha1_ctx_t *ctx)
{
    ctx->state[0] = SHA1_H0;
    ctx->state[1] = SHA1_H1;
    ctx->state[2] = SHA1_H2;
    ctx->state[3] = SHA1_H3;
    ctx->state[4] = SHA1_H4;
    ctx->buf_used = 0;
    ctx->total_hi = 0;
    ctx->total_lo = 0;
}


/* ============================================================
 * sha1_transform (0x08004324)
 *
 * Process one 64-byte block using the HARDWARE crypto engine.
 * This is NOT a software SHA-1 transform - it offloads to the
 * W800's built-in SHA-1 accelerator at 0x40000600.
 *
 * r0 = pointer to sha1_ctx_t (buffer at offset 0x20 contains
 *       the 64-byte block to process)
 *
 * Disassembly:
 *   8004324: movih r2, 16384           ; r2 = 0x40000000
 *   8004328: addi  r2, 1536            ; r2 = 0x40000600 (CRYPTO_BASE)
 *   800432c: addi  r3, r0, 32          ; r3 = &ctx->buffer[0]
 *   8004330: st.w  r3, (r2, 0x00)      ; SRC_ADDR = buffer
 *   8004332: movih r3, 2               ; r3 = 0x20000
 *   8004336: addi  r3, 64              ; r3 = 0x20040 (SHA1 mode, 64 bytes)
 *   8004338: st.w  r3, (r2, 0x08)      ; CONFIG = SHA1 | 64 bytes
 *   800433a: ld.w  r3, (r0, 0x08)      ; state[0]
 *   800433c: st.w  r3, (r2, 0x34)      ; HASH_A = state[0]
 *   800433e: ld.w  r3, (r0, 0x0C)      ; state[1]
 *   8004340: st.w  r3, (r2, 0x38)      ; HASH_B = state[1]
 *   8004342: ld.w  r3, (r0, 0x10)      ; state[2]
 *   8004344: st.w  r3, (r2, 0x3C)      ; HASH_C = state[2]
 *   8004346: ld.w  r3, (r0, 0x14)      ; state[3]
 *   8004348: st.w  r3, (r2, 0x40)      ; HASH_D = state[3]
 *   800434a: ld.w  r3, (r0, 0x18)      ; state[4]
 *   800434c: st.w  r3, (r2, 0x44)      ; HASH_E = state[4]
 *   800434e: movi  r3, 1
 *   8004350: st.w  r3, (r2, 0x0C)      ; CTRL = 1 (start)
 *
 *   ; Poll for completion
 *   8004352: movih r1, 1               ; mask = 0x10000
 *   8004356: ld.w  r3, (r2, 0x30)      ; STATUS
 *   8004358: and   r3, r1
 *   800435a: bez   r3, poll            ; loop until bit 16 set
 *
 *   ; Clear completion, read back state
 *   800435e: ld.w  r3, (r2, 0x30)
 *   8004360: bseti r3, 16
 *   8004362: st.w  r3, (r2, 0x30)      ; clear done bit
 *   8004364: ld.w  r3, (r2, 0x34)      ; read HASH_A
 *   8004366: st.w  r3, (r0, 0x08)      ; state[0] = HASH_A
 *   8004368: ld.w  r3, (r2, 0x38)
 *   800436a: st.w  r3, (r0, 0x0C)      ; state[1] = HASH_B
 *   800436c: ld.w  r3, (r2, 0x3C)
 *   800436e: st.w  r3, (r0, 0x10)      ; state[2] = HASH_C
 *   8004370: ld.w  r3, (r2, 0x40)
 *   8004372: st.w  r3, (r0, 0x14)      ; state[3] = HASH_D
 *   8004374: ld.w  r3, (r2, 0x44)
 *   8004376: st.w  r3, (r0, 0x18)      ; state[4] = HASH_E
 *   8004378: jmp   r15
 * ============================================================ */
static void sha1_transform(sha1_ctx_t *ctx)
{
    /* Load current hash state into hardware engine */
    CRYPTO_SRC_ADDR = (uint32_t)ctx->buffer;
    CRYPTO_CONFIG   = 0x20040;    /* SHA-1 mode, 64-byte block */
    CRYPTO_HASH_A   = ctx->state[0];
    CRYPTO_HASH_B   = ctx->state[1];
    CRYPTO_HASH_C   = ctx->state[2];
    CRYPTO_HASH_D   = ctx->state[3];
    CRYPTO_HASH_E   = ctx->state[4];

    /* Start hardware SHA-1 computation */
    CRYPTO_CTRL = 1;

    /* Poll until completion (bit 16 of STATUS) */
    while (!(CRYPTO_STATUS & 0x10000))
        ;

    /* Clear done flag */
    CRYPTO_STATUS |= 0x10000;

    /* Read back updated hash state */
    ctx->state[0] = CRYPTO_HASH_A;
    ctx->state[1] = CRYPTO_HASH_B;
    ctx->state[2] = CRYPTO_HASH_C;
    ctx->state[3] = CRYPTO_HASH_D;
    ctx->state[4] = CRYPTO_HASH_E;
}


/* ============================================================
 * sha1_update (0x080045D8)
 *
 * Feed data into SHA-1 context.
 *
 * r0 = sha1_ctx_t *ctx
 * r1 = const uint8_t *data
 * r2 = uint32_t len
 *
 * Disassembly:
 *   80045d8: push  r4-r10, r15
 *   80045da: mov   r6, r0              ; ctx
 *   80045dc: mov   r7, r1              ; data
 *   80045de: mov   r5, r2              ; len
 *   80045e0: bez   r2, done            ; if len == 0, return
 *   80045e4: addi  r9, r0, 32          ; r9 = &ctx->buffer
 *   80045e8: movi  r8, 64              ; block size
 *   80045ec: movi  r10, 0
 *
 * fill_loop:
 *   80045f6: ld.w  r0, (r6, 0x1C)     ; buf_used
 *   80045f8: subu  r4, r8, r0          ; space = 64 - buf_used
 *   80045fc: min.u32 r4, r4, r5        ; copy_len = min(space, len)
 *   8004600: mov   r1, r7              ; src = data
 *   8004602: mov   r2, r4              ; n = copy_len
 *   8004604: addu  r0, r9              ; dst = buffer + buf_used
 *   8004606: bsr   memcpy              ; 0x08002B54
 *   800460a: ld.w  r3, (r6, 0x1C)     ; buf_used
 *   800460c: addu  r3, r4              ; buf_used += copy_len
 *   800460e: cmpnei r3, 64            ; full block?
 *   8004612: st.w  r3, (r6, 0x1C)
 *   8004614: addu  r7, r4              ; data += copy_len
 *   8004616: subu  r5, r4              ; len -= copy_len
 *   8004618: bt    check_remain        ; not full -> check if more data
 *
 *   ; Full block: transform
 *   800461a: mov   r0, r6
 *   800461c: bsr   sha1_transform      ; 0x08004324
 *
 *   ; Update bit count: total_lo += 512
 *   8004620: ld.w  r2, (r6, 0x04)     ; total_lo
 *   8004622: addi  r3, r2, 512
 *   8004626: cmphs r3, r2             ; overflow?
 *   8004628: bt    no_overflow
 *   800462a: ld.w  r2, (r6, 0x00)     ; total_hi
 *   800462c: addi  r2, 1
 *   800462e: st.w  r2, (r6, 0x00)     ; total_hi++
 * no_overflow:
 *   8004630: st.w  r3, (r6, 0x04)     ; total_lo = new
 *   8004632: st.w  r10, (r6, 0x1C)    ; buf_used = 0
 *   8004636: bnez  r5, fill_loop       ; more data?
 *
 * check_remain:
 *   80045f2: bez   r5, done
 *   ... (loops back)
 *
 * done:
 *   800463a: pop   r4-r10, r15
 * ============================================================ */
void sha1_update(sha1_ctx_t *ctx, const uint8_t *data, uint32_t len)
{
    if (len == 0) return;

    while (len > 0) {
        uint32_t space = 64 - ctx->buf_used;
        uint32_t copy_len = (space < len) ? space : len;

        memcpy(ctx->buffer + ctx->buf_used, data, copy_len);
        ctx->buf_used += copy_len;
        data += copy_len;
        len  -= copy_len;

        if (ctx->buf_used == 64) {
            sha1_transform(ctx);

            /* Update 64-bit bit count (add 512 = 64 bytes * 8) */
            uint32_t old_lo = ctx->total_lo;
            ctx->total_lo += 512;
            if (ctx->total_lo < old_lo)
                ctx->total_hi++;

            ctx->buf_used = 0;
        }
    }
}


/* ============================================================
 * sha1_final (0x0800463C)
 *
 * Finalize SHA-1 computation. Adds padding, processes final
 * block(s), and outputs 20-byte digest.
 *
 * r0 = sha1_ctx_t *ctx
 * r1 = uint8_t *digest (20 bytes output, or NULL)
 *
 * Returns: 20 on success, -5 on error (NULL digest)
 *
 * Disassembly notes:
 *   - Adds 0x80 padding byte
 *   - Pads with zeros to reach 56 bytes (or 120 if >56 used)
 *   - Appends 8-byte big-endian bit length
 *   - Processes final block(s)
 *   - Reads result from HW crypto engine registers
 *   - Outputs 20 bytes in big-endian byte order
 *   - Zeros out context (96 bytes) via memset
 *
 *   8004724: movi  r0, 0               ; error return
 *   8004726: subi  r0, 6              ; return -5
 *   8004728: pop   r4-r5, r15
 * ============================================================ */
int sha1_final(sha1_ctx_t *ctx, uint8_t *digest)
{
    if (digest == NULL)
        return -5;

    uint32_t buf_used = ctx->buf_used;
    if (buf_used >= 64)
        return -5;

    /* Compute final bit count */
    uint32_t total_lo = ctx->total_lo + (buf_used << 3);
    if (total_lo < ctx->total_lo)
        ctx->total_hi++;
    ctx->total_lo = total_lo;

    /* Append 0x80 padding byte */
    uint32_t pad_pos = buf_used + 1;
    ctx->buffer[buf_used] = 0x80;
    ctx->buf_used = pad_pos;

    /* If not enough room for 8-byte length (need 56 bytes clear) */
    if (pad_pos > 56) {
        /* Pad remainder of block with zeros */
        if (pad_pos < 64) {
            for (uint32_t i = pad_pos; i < 64; i++)
                ctx->buffer[i] = 0;
            ctx->buf_used = 64;
        }
        /* Process this block */
        sha1_transform(ctx);
        total_lo = ctx->total_lo;
        pad_pos = 0;
    }

    /* Pad with zeros up to byte 56 */
    if (pad_pos < 56) {
        for (uint32_t i = pad_pos; i < 56; i++)
            ctx->buffer[i] = 0;
        ctx->buf_used = 56;
    }

    /* Append big-endian 64-bit bit length at bytes 56..63 */
    ctx->buffer[56] = (ctx->total_hi >> 24) & 0xFF;
    ctx->buffer[57] = (ctx->total_hi >> 16) & 0xFF;
    ctx->buffer[58] = (ctx->total_hi >>  8) & 0xFF;
    ctx->buffer[59] = (ctx->total_hi >>  0) & 0xFF;
    ctx->buffer[60] = (total_lo >> 24) & 0xFF;
    ctx->buffer[61] = (total_lo >> 16) & 0xFF;
    ctx->buffer[62] = (total_lo >>  8) & 0xFF;
    ctx->buffer[63] = (total_lo >>  0) & 0xFF;

    /* Process final block */
    sha1_transform(ctx);

    /* Read SHA-1 state from HW engine and output as big-endian */
    /*
     * 80046e6: mov   r1, r4              ; digest
     * 80046e8: movih r0, 16384
     * 80046ec: addi  r0, 1588            ; r0 = 0x40000634 (HASH_A)
     * 80046f0: movi  r2, 5               ; 5 words
     * loop:
     * 80046f2: ldbi.w r3, (r0)           ; load word, auto-increment
     * 80046f6: lsri  r12, r3, 24
     * 80046fa: st.b  r12, (r1, 0)
     * 80046fe: lsri  r12, r3, 16
     * 8004702: st.b  r12, (r1, 1)
     * 8004706: lsri  r12, r3, 8
     * 800470a: st.b  r12, (r1, 2)
     * 800470e: st.b  r3, (r1, 3)
     * 8004710: addi  r1, 4
     * 8004712: bnezad r2, loop
     */
    volatile uint32_t *hash_regs = (volatile uint32_t *)(CRYPTO_BASE + 0x34);
    for (int i = 0; i < 5; i++) {
        uint32_t w = hash_regs[i];
        digest[i*4 + 0] = (w >> 24) & 0xFF;
        digest[i*4 + 1] = (w >> 16) & 0xFF;
        digest[i*4 + 2] = (w >>  8) & 0xFF;
        digest[i*4 + 3] = (w >>  0) & 0xFF;
    }

    /* Zero out the context */
    memset(ctx, 0, 96);

    return 20;  /* SHA-1 digest length */
}


/* ============================================================
 * hw_crypto_setup (0x08004404)
 *
 * Configure hardware crypto engine for CRC32 or other modes.
 *
 * r0 = pointer to operation descriptor:
 *       offset 0x00: uint32_t data (input word / CRC init)
 *       offset 0x04: uint8_t  mode (0=CRC8, 3=CRC16, else=CRC32)
 *       offset 0x05: uint8_t  flags (bit0: reverse input bits)
 *
 * r1 = data length (number of bytes)
 * r2 = additional parameter (data word for non-reversed mode)
 *
 * Disassembly (abbreviated):
 *   8004404: st.w  r1, (r0, 0x0)       ; store length
 *   8004406: st.b  r2, (r0, 0x4)       ; store mode
 *   8004408: st.b  r3, (r0, 0x5)       ; store flags
 *   800440a: movi  r0, 0               ; return 0
 *   800440c: jmp   r15
 *
 * The actual HW CRC execution at 0x08004410:
 *   8004410: ld.b  r3, (r0, 0x4)       ; mode
 *   8004412: ld.b  r12, (r0, 0x5)      ; flags
 *   8004416: lsli  r3, 21              ; mode << 21
 *   8004418: lsli  r12, 23             ; flags << 23
 *   800441c: or    r3, r12
 *   800441e: zexth r2, r2              ; data & 0xFFFF
 *   8004420: bseti r3, 17             ; set bit 17 (CRC engine enable)
 *   8004422: bseti r3, 18             ; set bit 18
 *   8004424: or    r3, r2              ; config word
 *   8004426: movih r2, 16384
 *   800442a: addi  r2, 1536            ; CRYPTO_BASE
 *   800442e: st.w  r3, (r2, 0x08)      ; CONFIG register
 *
 * If flags & 1 (bit reversal mode):
 *   ; Reverse input bits using shift loop
 *   ; mode 0: 8-bit, mode 3: 16-bit, else: 32-bit
 *   ; Then write to CRYPTO_BASE + 0x44
 *
 * Start engine, poll, read result:
 *   8004488: movi  r3, 1
 *   800448a: st.w  r1, (r2, 0x00)      ; SRC_ADDR
 *   800448c: st.w  r3, (r2, 0x0C)      ; CTRL = 1 (start)
 *   8004492: ld.w  r3, (r2, 0x30)      ; poll STATUS
 *   8004494: and   r3, r1              ; bit 16?
 *   8004496: bez   poll
 *   800449a: ld.w  r3, (r2, 0x30)
 *   800449c: bseti r3, 16
 *   800449e: st.w  r3, (r2, 0x30)      ; clear done
 *   80044a0: ld.w  r3, (r2, 0x44)      ; result = HASH_E (CRC result)
 *   80044a2: st.w  r3, (r0, 0x0)       ; store result
 *   80044a4: movi  r3, 4
 *   80044a6: movi  r0, 0               ; return 0
 *   80044a8: st.w  r3, (r2, 0x0C)      ; CTRL = 4 (stop/reset)
 *   80044aa: jmp   r15
 * ============================================================ */
typedef struct {
    uint32_t data;
    uint8_t  mode;       /* 0=CRC8, 3=CRC16, others=CRC32 */
    uint8_t  flags;      /* bit0: reverse input bits */
} hw_crypto_op_t;

int hw_crypto_setup(hw_crypto_op_t *op, uint32_t length,
                    uint16_t param)
{
    op->data = length;
    op->mode = param & 0xFF;
    op->flags = (param >> 8) & 0xFF;
    return 0;
}

int hw_crypto_exec(hw_crypto_op_t *op, uint32_t src_addr,
                   uint16_t data_word)
{
    /* Build config register */
    uint32_t config = ((uint32_t)op->mode << 21)
                    | ((uint32_t)op->flags << 23)
                    | (1 << 17) | (1 << 18)
                    | (data_word & 0xFFFF);

    CRYPTO_CONFIG = config;

    if (op->flags & 1) {
        /* Bit-reversal mode: reverse input bits */
        uint32_t val = op->data;
        uint32_t reversed = 0;
        int bits = (op->mode == 0) ? 8 :
                   (op->mode == 3) ? 16 : 32;
        for (int i = bits - 1; i >= 0; i--) {
            if (val & 1)
                reversed |= (1 << i);
            val >>= 1;
        }
        CRYPTO_HASH_E = reversed;
    } else {
        /* Direct mode */
        CRYPTO_HASH_E = op->data;
    }

    /* Start engine */
    CRYPTO_SRC_ADDR = src_addr;
    CRYPTO_CTRL = 1;

    /* Poll for completion */
    while (!(CRYPTO_STATUS & 0x10000))
        ;

    /* Clear done flag */
    CRYPTO_STATUS |= 0x10000;

    /* Read result */
    op->data = CRYPTO_HASH_E;

    CRYPTO_CTRL = 4;  /* Stop/reset */
    return 0;
}


/* ============================================================
 * CRC Verification Context
 *
 * Allocated by crc_ctx_alloc(), used by crc_verify_image().
 *
 * Layout:
 *   offset 0x00:  void *inner_ctx  (pointer to 104-byte inner state)
 *   offset 0x04:  uint32_t digest_len
 *   offset 0x08:  uint32_t flags   (1 = finalized)
 *
 * Inner context (104 bytes) contains SHA-1 state and ASN.1 DER
 * parsing workspace.
 * ============================================================ */
typedef struct {
    void     *inner_ctx;
    uint32_t  digest_len;
    uint32_t  flags;
} crc_ctx_t;


/* ============================================================
 * crc_ctx_alloc (0x08004CFC)
 *
 * Allocate and initialize a CRC verification context.
 * Allocates 12-byte outer struct + 104-byte inner context.
 *
 * Returns: pointer to crc_ctx_t, or NULL on allocation failure
 *
 * Disassembly:
 *   8004cfc: push  r4-r5, r15
 *   8004cfe: movi  r1, 1               ; nmemb = 1
 *   8004d00: movi  r0, 12              ; size = 12
 *   8004d02: bsr   calloc              ; 0x08002980
 *   8004d06: mov   r4, r0              ; r4 = outer
 *   8004d08: bez   r0, return          ; if NULL, return NULL
 *   8004d0c: movi  r1, 1
 *   8004d0e: movi  r0, 104             ; inner size = 104
 *   8004d10: bsr   calloc
 *   8004d14: mov   r5, r0
 *   8004d16: st.w  r0, (r4, 0x0)       ; outer->inner_ctx = inner
 *   8004d18: bez   r0, alloc_fail
 *   8004d1c: mov   r0, r4
 *   8004d1e: pop   r4-r5, r15          ; return outer
 *
 * alloc_fail:
 *   8004d20: mov   r0, r4
 *   8004d22: bsr   free                ; free outer
 *   8004d26: mov   r4, r5              ; r4 = NULL (inner was NULL)
 *   8004d28: br    return
 * ============================================================ */
crc_ctx_t *crc_ctx_alloc(void)
{
    crc_ctx_t *ctx = (crc_ctx_t *)calloc(1, 12);
    if (ctx == NULL)
        return NULL;

    void *inner = calloc(1, 104);
    ctx->inner_ctx = inner;
    if (inner == NULL) {
        free(ctx);
        return NULL;
    }

    return ctx;
}


/* ============================================================
 * crc_ctx_destroy (0x08004D2C)
 *
 * Destroy a CRC context, freeing inner and outer allocations.
 * If flags == 1 (finalized), resets inner state first.
 *
 * r0 = crc_ctx_t *ctx
 *
 * Disassembly:
 *   8004d2c: push  r4, r15
 *   8004d2e: mov   r4, r0
 *   8004d30: bez   r0, done
 *   8004d34: ld.w  r3, (r0, 0x08)      ; flags
 *   8004d36: cmpnei r3, 1             ; if finalized
 *   8004d38: ld.w  r0, (r0, 0x00)      ; inner_ctx
 *   8004d3a: bf    do_reset            ; flags == 1 -> reset first
 *   8004d3c: bsr   free                ; free inner
 *   8004d40: mov   r0, r4
 *   8004d42: bsr   free                ; free outer
 *   8004d46: pop   r4, r15
 *
 * do_reset:
 *   8004d48: bsr   crc_ctx_reset       ; 0x08004D50
 *   8004d4c: br    free_inner
 * ============================================================ */
void crc_ctx_destroy(crc_ctx_t *ctx)
{
    if (ctx == NULL)
        return;

    if (ctx->flags == 1) {
        /* Reset inner state before freeing */
        crc_ctx_reset(ctx->inner_ctx);
    }
    free(ctx->inner_ctx);
    free(ctx);
}


/* ============================================================
 * crc_ctx_reset (0x08004D50)
 *
 * Reset internal state of CRC context. Clears multiple
 * bignum structures within the 104-byte inner context at
 * various offsets.
 *
 * r0 = inner context pointer
 *
 * Disassembly:
 *   8004d50: push  r4, r15
 *   8004d52: mov   r4, r0
 *   8004d54: addi  r0, 24              ; offset 24
 *   8004d56: bsr   bignum_clear        ; 0x080031A8
 *   8004d5a: mov   r0, r4
 *   8004d5c: bsr   bignum_clear        ; offset 0
 *   8004d60: addi  r0, r4, 12
 *   8004d64: bsr   bignum_clear        ; offset 12
 *   8004d68: addi  r0, r4, 72
 *   8004d6c: bsr   bignum_clear        ; offset 72
 *   8004d70: addi  r0, r4, 84
 *   8004d74: bsr   bignum_clear        ; offset 84
 *   8004d78: addi  r0, r4, 48
 *   8004d7c: bsr   bignum_clear        ; offset 48
 *   8004d80: addi  r0, r4, 60
 *   8004d84: bsr   bignum_clear        ; offset 60
 *   8004d88: addi  r0, r4, 36
 *   8004d8c: bsr   bignum_clear        ; offset 36
 *   8004d90: mov   r0, r4
 *   8004d92: bsr   free                ; free inner buffer
 *   8004d96: pop   r4, r15
 * ============================================================ */
void crc_ctx_reset(void *inner)
{
    uint8_t *p = (uint8_t *)inner;

    /* Clear bignum fields at various offsets within inner context */
    bignum_clear(p + 24);
    bignum_clear(p + 0);
    bignum_clear(p + 12);
    bignum_clear(p + 72);
    bignum_clear(p + 84);
    bignum_clear(p + 48);
    bignum_clear(p + 60);
    bignum_clear(p + 36);

    free(inner);
}


/* ============================================================
 * crc_verify_image (0x08005CFC)
 *
 * Full CRC32 / cryptographic verification of an image.
 * Orchestrates:
 *   1. Allocate 256-byte read buffer (malloc)
 *   2. Read certificate chain from flash (flash_read_cert at 0x08005B88)
 *   3. Parse certificate (cert_parse at 0x08005BC8)
 *   4. Allocate CRC context (crc_ctx_alloc)
 *   5. Parse ASN.1 DER header (asn1_parse_tag at 0x08004AB4)
 *   6. Parse ASN.1 DER body (asn1_parse_body at 0x08004BB0)
 *   7. Setup CRC verification (crc_verify_exec at 0x08004BEC)
 *   8. Execute verification pipeline (crc_pipeline at 0x08004F20)
 *   9. Compare 20-byte SHA-1 digest with expected value
 *  10. Cleanup: free buffer and context
 *
 * r0 = flash context / image descriptor
 * r1 = image header pointer
 * Returns: 0 on match (success), -1 on failure, -1 on alloc fail
 *
 * Disassembly (key points):
 *   8005cfc: push  r4-r8, r15
 *   8005cfe: subi  r14, 196           ; large stack frame
 *   8005d00: mov   r6, r0
 *   8005d04: movi  r0, 256
 *   8005d0c: bsr   malloc              ; 0x080029A0
 *   8005d10: mov   r4, r0             ; r4 = read_buf
 *   8005d12: bez   r0, alloc_fail_buf
 *
 *   ; Read certificate
 *   8005d1e: bsr   flash_read_cert     ; 0x08005B88
 *   8005d22: mov   r5, r0
 *   8005d24: blz   r0, cleanup
 *
 *   ; Parse certificate
 *   8005d2e: bsr   cert_parse          ; 0x08005BC8
 *   8005d34: bez   r0, do_verify
 *   8005d38: mov   r5, r0             ; error
 *   8005d3a: bsr   free(r4)           ; cleanup buffer
 *
 * do_verify:
 *   8005d46: bsr   crc_ctx_alloc       ; 0x08004CFC
 *   8005d4a: mov   r7, r0             ; crc_ctx
 *   8005d4c: bez   r0, alloc_fail_ctx
 *
 *   ; ASN.1 DER parsing
 *   8005d58: bsr   asn1_parse_tag      ; 0x08004AB4
 *   8005d72: bsr   asn1_parse_body     ; 0x08004BB0
 *   8005d88: bsr   crc_verify_exec     ; 0x08004BEC
 *
 *   ; Mark context as finalized
 *   8005d92: movi  r3, 1
 *   8005d94: st.w  r3, (r7, 0x08)     ; flags = 1
 *
 *   ; Get expected digest from context
 *   8005d96: ld.w  r3, (r7, 0x00)     ; inner_ctx
 *   8005d9a: ld.w  r3, (r3, 0x60)     ; expected digest ptr
 *   8005d9c: st.w  r3, (r7, 0x04)     ; digest_len
 *
 *   ; Run verification pipeline (computes actual SHA-1)
 *   8005db2: bsr   crc_pipeline        ; 0x08004F20
 *   8005db6: cmpnei r0, 20            ; expect 20-byte SHA-1
 *   8005db8: bt    verify_fail
 *
 *   ; Compare computed digest with expected
 *   8005dba: mov   r2, r0             ; len = 20
 *   8005dbc: addi  r1, r14, 176       ; computed digest on stack
 *   8005dbe: mov   r0, r5             ; expected digest
 *   8005dc0: bsr   memcmp              ; 0x08002BD4
 *   8005dc4: cmpnei r0, 0            ; match?
 *   8005dc6: mvc   r5                 ; r5 = (match ? 0 : 1)
 *   8005dca: subu  r5, r6, r5         ; adjust sign
 *
 * cleanup:
 *   8005dcc: mov   r0, r4
 *   8005dce: bsr   free                ; free read_buf
 *   8005dd2: mov   r0, r7
 *   8005dd4: bsr   crc_ctx_destroy     ; 0x08004D2C
 *   8005dd8: mov   r0, r5             ; return result
 *   8005dda: addi  r14, 196
 *   8005ddc: pop   r4-r8, r15
 *
 * alloc_fail_buf:
 *   8005dde: movi  r5, 0
 *   8005de0: subi  r5, 1              ; return -1
 *
 * alloc_fail_ctx:
 *   8005de4: movi  r5, 0
 *   8005de6: subi  r5, 1              ; return -1
 *
 * verify_fail:
 *   8005dea: movi  r5, 0
 *   8005dec: subi  r5, 1              ; return -1
 * ============================================================ */
int crc_verify_image(void *flash_ctx, void *img_header)
{
    uint8_t stack_buf[196];     /* stack frame workspace */
    int result;

    /* Allocate 256-byte read buffer */
    uint8_t *read_buf = (uint8_t *)malloc(256);
    if (read_buf == NULL)
        return -1;

    /* Read certificate chain from flash */
    result = flash_read_cert(flash_ctx, read_buf, 256);
    if (result < 0) {
        free(read_buf);
        return result;
    }

    /* Parse certificate */
    int cert_result = cert_parse(flash_ctx, img_header,
                                 stack_buf + 44);
    if (cert_result != 0) {
        free(read_buf);
        return cert_result;
    }

    /* Allocate CRC verification context */
    crc_ctx_t *crc_ctx = crc_ctx_alloc();
    if (crc_ctx == NULL) {
        free(read_buf);
        return -1;
    }

    /* ASN.1 DER parsing of certificate */
    uint8_t *asn1_ptr = (uint8_t *)result;
    uint32_t asn1_len;
    int parse_ok = asn1_parse_tag(&asn1_ptr, stack_buf + 12,
                                  stack_buf + 20);
    if (parse_ok < 0)
        goto cleanup;

    /* Parse body */
    parse_ok = asn1_parse_body(stack_buf + 20, asn1_ptr,
                               stack_buf + 16, stack_buf + 12);
    if (parse_ok < 0)
        goto cleanup;

    /* Setup and execute CRC verification */
    parse_ok = crc_verify_exec(flash_ctx, stack_buf + 20,
                               crc_ctx, read_buf);
    if (parse_ok < 0)
        goto cleanup;

    /* Mark as finalized */
    crc_ctx->flags = 1;
    crc_ctx->digest_len = *(uint32_t *)((uint8_t *)crc_ctx->inner_ctx + 0x60);

    /* Run SHA-1 verification pipeline */
    uint8_t computed_digest[20];
    uint8_t *expected_digest = stack_buf + 24;
    int digest_len = crc_pipeline(flash_ctx, crc_ctx,
                                  stack_buf + 44,
                                  0 /* data_len */,
                                  20 /* hash_len */,
                                  expected_digest,
                                  flash_ctx /* flash_addr */);
    if (digest_len != 20) {
        result = -1;
        goto cleanup;
    }

    /* Compare digests */
    if (memcmp(expected_digest, stack_buf + 176, 20) != 0) {
        result = -1;  /* Mismatch */
    } else {
        result = 0;   /* Match */
    }

cleanup:
    free(read_buf);
    crc_ctx_destroy(crc_ctx);
    return result;
}
