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

#include "secboot_common.h"

/* Function aliases: crc_verify_image and related functions use
 * alternate names for functions already defined elsewhere.
 * These wrapper stubs resolve the linker dependencies. */
extern int   xmodem_recv_data(void *ctx, uint8_t *buffer, void *flash_ctx);
extern int   xmodem_process(uint32_t dest_addr);

/* flash_read_cert (0x08005B88) = xmodem_recv_data */
int flash_read_cert(void *ctx, void *buf, int len)
{
    return xmodem_recv_data(ctx, (uint8_t *)buf, (void *)(uintptr_t)len);
}

/* flash_cert_parse (0x08005BC8) = xmodem_process */
int flash_cert_parse(void *ctx, void *hdr, void *out)
{
    (void)ctx; (void)hdr; (void)out;
    return xmodem_process((uint32_t)(uintptr_t)ctx);
}

/* The following are stub implementations for functions that are
 * defined elsewhere in the binary but referenced by alternate names
 * in crc_verify_image. In the original binary these are the same
 * code at different addresses called with different signatures.
 * For functional equivalence, we provide minimal stubs. */
int asn1_parse_tag(uint8_t **pos, void *out1, void *out2)
{
    return pkey_verify(pos, (uint32_t)(uintptr_t)out2, (mp_int *)out1);
}

int asn1_parse_body(void *a, void *b, void *c, void *d)
{
    return signature_check_data((uint8_t **)a, (uint32_t)(uintptr_t)b,
                                 (uint32_t *)c, (uint32_t *)d);
}

/* signature_check_final (0x08004BEC) defined later in this file */
int crc_verify_exec(void *ctx, void *a, void *b, void *c)
{
    extern int signature_check_final(uint8_t **pos, uint32_t *remaining,
                                      uint32_t len, uint8_t *out);
    return signature_check_final((uint8_t **)ctx, (uint32_t *)a,
                                  (uint32_t)(uintptr_t)b, (uint8_t *)c);
}

/* image_decrypt_process (0x08004F20) defined in secboot_image.c */
extern int image_decrypt_process(void *a, void *b, void *c,
                                  int d, int e, void *f, void *g);

int crc_pipeline(void *ctx, void *a, void *b,
                 int c, int d, void *e, void *f)
{
    return image_decrypt_process(ctx, a, b, c, d, e, f);
}

/* RSAXBUF macro: index-based access into RSA buffer space */
#define RSAXBUF(i)  (*(volatile uint32_t *)(RSA_CTRL_BASE + 0x000 + (i)*4))

/* ============================================================
 * GCC runtime: 64-bit unsigned division
 *
 * Required by sha_final for 64-bit arithmetic operations.
 * This is normally provided by libgcc, but since we link with
 * -nostdlib we must provide our own.
 * ============================================================ */
uint64_t __udivdi3(uint64_t n, uint64_t d)
{
    if (d == 0) return 0;
    if (d == 1) return n;
    if (n < d) return 0;

    uint64_t q = 0;
    uint64_t r = 0;
    for (int i = 63; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1);
        if (r >= d) {
            r -= d;
            q |= ((uint64_t)1 << i);
        }
    }
    return q;
}

/* ============================================================
 * mp_cmp_mag (0x08003238) - Compare magnitudes of two mp_ints
 *
 * Returns: -1 if |a| < |b|, 0 if |a| == |b|, 1 if |a| > |b|
 * ============================================================ */
int mp_cmp_mag(mp_int *a, mp_int *b)
{
    if (a->used > b->used) return 1;
    if (a->used < b->used) return -1;

    for (int i = a->used - 1; i >= 0; i--) {
        if (a->dp[i] > b->dp[i]) return 1;
        if (a->dp[i] < b->dp[i]) return -1;
    }
    return 0;
}

/* ============================================================
 * mp_cmp (0x08003298) - Compare two mp_ints (with sign)
 *
 * Returns: -1 if a < b, 0 if a == b, 1 if a > b
 * ============================================================ */
int mp_cmp(mp_int *a, mp_int *b)
{
    if (a->sign != b->sign) {
        return (a->sign == 1) ? -1 : 1;
    }
    if (a->sign == 1) {
        return mp_cmp_mag(b, a);
    }
    return mp_cmp_mag(a, b);
}

/* ============================================================
 * mp_lshd (0x08003514) - Left-shift by 'count' digits
 *
 * Inserts 'count' zero digits at the bottom, shifting all
 * existing digits up.
 * ============================================================ */
int mp_lshd(mp_int *a, int16_t count)
{
    if (count <= 0) return 0;

    int ret = mp_grow(a, (int16_t)(a->used + count));
    if (ret != 0) return ret;

    /* Shift digits up */
    for (int i = a->used - 1; i >= 0; i--) {
        a->dp[i + count] = a->dp[i];
    }
    /* Zero the new low digits */
    for (int i = 0; i < count; i++) {
        a->dp[i] = 0;
    }
    a->used += count;
    return 0;
}

/* ============================================================
 * s_mp_sub (internal) - Unsigned subtraction: c = a - b
 * Assumes |a| >= |b|
 * ============================================================ */
static int s_mp_sub(mp_int *a, mp_int *b, mp_int *c)
{
    int ret = mp_grow(c, a->used);
    if (ret != 0) return ret;

    uint32_t borrow = 0;
    int i;
    for (i = 0; i < b->used; i++) {
        uint64_t t = (uint64_t)a->dp[i] - (uint64_t)b->dp[i] - borrow;
        c->dp[i] = (uint32_t)(t & MP_MASK);
        borrow = ((int64_t)t < 0) ? 1 : 0;
    }
    for (; i < a->used; i++) {
        uint64_t t = (uint64_t)a->dp[i] - borrow;
        c->dp[i] = (uint32_t)(t & MP_MASK);
        borrow = ((int64_t)t < 0) ? 1 : 0;
    }
    c->used = a->used;
    mp_clamp(c);
    return 0;
}

/* ============================================================
 * s_mp_add (internal) - Unsigned addition: c = a + b
 * ============================================================ */
static int s_mp_add(mp_int *a, mp_int *b, mp_int *c)
{
    mp_int *x, *y;
    if (a->used >= b->used) { x = a; y = b; }
    else { x = b; y = a; }

    int ret = mp_grow(c, (int16_t)(x->used + 1));
    if (ret != 0) return ret;

    uint32_t carry = 0;
    int i;
    for (i = 0; i < y->used; i++) {
        uint64_t t = (uint64_t)x->dp[i] + (uint64_t)y->dp[i] + carry;
        c->dp[i] = (uint32_t)(t & MP_MASK);
        carry = (uint32_t)(t >> DIGIT_BIT);
    }
    for (; i < x->used; i++) {
        uint64_t t = (uint64_t)x->dp[i] + carry;
        c->dp[i] = (uint32_t)(t & MP_MASK);
        carry = (uint32_t)(t >> DIGIT_BIT);
    }
    c->dp[i] = carry;
    c->used = (int16_t)(x->used + 1);
    mp_clamp(c);
    return 0;
}

/* ============================================================
 * mp_sub (0x080032CC) - Subtraction: c = a - b
 * ============================================================ */
int mp_sub(mp_int *a, mp_int *b, mp_int *c)
{
    if (a->sign != b->sign) {
        /* Different signs: a - b = a + |b| or -(|a| + |b|) */
        c->sign = a->sign;
        return s_mp_add(a, b, c);
    }

    /* Same sign */
    if (mp_cmp_mag(a, b) >= 0) {
        c->sign = a->sign;
        return s_mp_sub(a, b, c);
    } else {
        c->sign = (a->sign == 0) ? 1 : 0;
        return s_mp_sub(b, a, c);
    }
}

/* ============================================================
 * mp_add (0x0800332C) - Addition: c = a + b
 * ============================================================ */
int mp_add(mp_int *a, mp_int *b, mp_int *c)
{
    if (a->sign == b->sign) {
        c->sign = a->sign;
        return s_mp_add(a, b, c);
    }

    /* Different signs: addition becomes subtraction */
    if (mp_cmp_mag(a, b) >= 0) {
        c->sign = a->sign;
        return s_mp_sub(a, b, c);
    } else {
        c->sign = b->sign;
        return s_mp_sub(b, a, c);
    }
}

/* ============================================================
 * mp_count_bits (0x08003744) - Count number of bits in mp_int
 * ============================================================ */
int mp_count_bits(mp_int *a)
{
    if (a->used == 0) return 0;

    int bits = (a->used - 1) * DIGIT_BIT;
    uint32_t d = a->dp[a->used - 1];
    while (d > 0) {
        bits++;
        d >>= 1;
    }
    return bits;
}

/* ============================================================
 * mp_reverse (0x08003150) - Reverse a byte array in-place
 * ============================================================ */
void mp_reverse(uint8_t *s, int len)
{
    int i = 0, j = len - 1;
    while (i < j) {
        uint8_t t = s[i];
        s[i] = s[j];
        s[j] = t;
        i++;
        j--;
    }
}

/* ============================================================
 * mp_read_unsigned_bin (0x08003690) - Read big-endian bytes into mp_int
 * ============================================================ */
int mp_read_unsigned_bin(mp_int *a, const uint8_t *b, int len)
{
    int ret;

    /* Reset */
    a->used = 0;
    a->sign = 0;

    /* Number of digits needed */
    int16_t need = (int16_t)((len * 8 + DIGIT_BIT - 1) / DIGIT_BIT);
    ret = mp_grow(a, need);
    if (ret != 0) return ret;

    /* Zero all digits */
    for (int i = 0; i < a->alloc; i++) {
        a->dp[i] = 0;
    }

    /* Read bytes in big-endian order */
    for (int i = 0; i < len; i++) {
        /* Shift existing value left by 8 bits */
        uint32_t carry = 0;
        for (int j = 0; j < need; j++) {
            uint64_t t = ((uint64_t)a->dp[j] << 8) | carry;
            a->dp[j] = (uint32_t)(t & MP_MASK);
            carry = (uint32_t)(t >> DIGIT_BIT);
        }
        /* Add new byte */
        a->dp[0] |= b[i];
    }

    a->used = need;
    mp_clamp(a);
    return 0;
}

/* ============================================================
 * mp_unsigned_bin_size (0x080031DC) - Get byte count for unsigned bin
 * ============================================================ */
int mp_unsigned_bin_size(mp_int *a)
{
    int bits = mp_count_bits(a);
    return (bits + 7) / 8;
}

/* ============================================================
 * mp_to_unsigned_bin_nr (0x08003774) - Write mp_int as big-endian bytes
 * (no reverse - writes directly in big-endian order)
 * ============================================================ */
int mp_to_unsigned_bin_nr(mp_int *a, uint8_t *b)
{
    int size = mp_unsigned_bin_size(a);
    mp_int tmp;
    int ret = mp_init_copy(&tmp, a);
    if (ret != 0) return ret;

    for (int i = size - 1; i >= 0; i--) {
        b[i] = (uint8_t)(tmp.dp[0] & 0xFF);
        /* Right shift by 8 */
        uint32_t carry = 0;
        for (int j = tmp.used - 1; j >= 0; j--) {
            uint32_t next = tmp.dp[j] & 0xFF;
            tmp.dp[j] = (tmp.dp[j] >> 8) | (carry << (DIGIT_BIT - 8));
            carry = next;
        }
        mp_clamp(&tmp);
    }

    mp_clear(&tmp);
    return 0;
}


/* ============================================================
 * mp_clear (0x080031A8) - Clear and free an mp_int
 *
 * This libtommath function frees the digit buffer and zeros
 * the mp_int structure. It's called extensively by crc_ctx_reset.
 * ============================================================ */
void mp_clear(mp_int *a)
{
    if (a->dp != NULL) {
        /* Zero out the digit memory before freeing */
        for (int i = 0; i < a->alloc; i++) {
            a->dp[i] = 0;
        }
        free(a->dp);
        a->used = 0;
        a->alloc = 0;
        a->dp = NULL;
    }
}

/* ============================================================
 * s_mp_mul_digs (0x08002FC8) - Low-level multiply, up to 'digs' digits
 *
 * Computes c = a * b, keeping only the first 'digs' digits of result.
 * Standard libtommath helper function for schoolbook multiplication.
 * ============================================================ */
int s_mp_mul_digs(mp_int *a, mp_int *b, mp_int *c, int digs)
{
    mp_int tmp;
    int ret;

    ret = mp_init(&tmp);
    if (ret != 0) return ret;

    ret = mp_grow(&tmp, (int16_t)digs);
    if (ret != 0) { mp_clear(&tmp); return ret; }

    tmp.used = (int16_t)digs;

    for (int i = 0; i < a->used; i++) {
        uint64_t carry = 0;
        int limit = digs - i;
        if (limit > b->used) limit = b->used;

        for (int j = 0; j < limit; j++) {
            uint64_t prod = (uint64_t)a->dp[i] * (uint64_t)b->dp[j]
                          + (uint64_t)tmp.dp[i + j] + carry;
            tmp.dp[i + j] = (uint32_t)(prod & MP_MASK);
            carry = prod >> DIGIT_BIT;
        }
        if (i + limit < digs) {
            tmp.dp[i + limit] = (uint32_t)carry;
        }
    }

    mp_clamp(&tmp);
    /* Swap tmp into c */
    {
        mp_int swap = *c;
        *c = tmp;
        tmp = swap;
    }
    mp_clear(&tmp);
    return 0;
}

/* ============================================================
 * mp_clamp (0x080034A0) - Remove leading zero digits
 * ============================================================ */
void mp_clamp(mp_int *a)
{
    while (a->used > 0 && a->dp[a->used - 1] == 0) {
        a->used--;
    }
    if (a->used == 0) {
        a->sign = 0;
    }
}

/* ============================================================
 * mp_rshd (0x08003588) - Right-shift by 'count' digits
 *
 * Removes the lowest 'count' digits, shifting higher digits down.
 * ============================================================ */
void mp_rshd(mp_int *a, int16_t count)
{
    if (count <= 0) return;
    if (count >= a->used) {
        a->used = 0;
        a->sign = 0;
        return;
    }
    for (int i = 0; i < a->used - count; i++) {
        a->dp[i] = a->dp[i + count];
    }
    for (int i = a->used - count; i < a->used; i++) {
        a->dp[i] = 0;
    }
    a->used -= count;
}

/* ============================================================
 * mp_grow (0x080034E4) - Grow an mp_int to at least 'size' digits
 * ============================================================ */
int mp_grow(mp_int *a, int16_t size)
{
    if (a->alloc >= size) return 0;

    /* Simple realloc: allocate new, copy, free old */
    uint32_t *tmp = (uint32_t *)malloc(sizeof(uint32_t) * size);
    if (tmp == NULL) return -1;

    /* Copy existing digits */
    for (int i = 0; i < a->alloc; i++) {
        tmp[i] = a->dp[i];
    }
    /* Zero new digits */
    for (int i = a->alloc; i < size; i++) {
        tmp[i] = 0;
    }
    if (a->dp != NULL) free(a->dp);
    a->dp = tmp;
    a->alloc = size;
    return 0;
}

/* ============================================================
 * mp_init (0x08003178) - Initialize an mp_int
 *
 * Allocates initial digit buffer and zeros the structure.
 * ============================================================ */
int mp_init(mp_int *a)
{
    a->dp = (uint32_t *)malloc(sizeof(uint32_t) * 8);
    if (a->dp == NULL) return -1;
    a->used = 0;
    a->alloc = 8;
    a->sign = 0;
    for (int i = 0; i < 8; i++) {
        a->dp[i] = 0;
    }
    return 0;
}

/* ============================================================
 * mp_copy (0x08003390) - Copy src to dst
 *
 * NOTE: In this firmware, mp_copy(dst, src) copies src → dst.
 * This is REVERSED from standard libtommath mp_copy(src, dst).
 * ============================================================ */
int mp_copy(mp_int *dst, mp_int *src)
{
    if (dst == src) return 0;

    /* Grow destination if needed */
    if (dst->alloc < src->used) {
        int ret = mp_grow(dst, src->used);
        if (ret != 0) return ret;
    }

    /* Copy digits */
    for (int i = 0; i < src->used; i++) {
        dst->dp[i] = src->dp[i];
    }
    /* Zero remaining */
    for (int i = src->used; i < dst->alloc; i++) {
        dst->dp[i] = 0;
    }
    dst->used = src->used;
    dst->sign = src->sign;
    return 0;
}

/* ============================================================
 * mp_init_copy (0x08003408) - Initialize mp_int and copy from source
 *
 * Allocates a new mp_int and copies the value from src.
 * Combines mp_init + mp_copy in a single call.
 * ============================================================ */
int mp_init_copy(mp_int *dst, mp_int *src)
{
    int ret = mp_init(dst);
    if (ret != 0) return ret;
    return mp_copy(dst, src);
}

/* ============================================================
 * mp_div_2d (0x08003860) - Divide by 2^b
 *
 * Computes c = a / 2^b, d = a mod 2^b (remainder).
 * Standard libtommath implementation: shift right by b bits.
 * ============================================================ */
int mp_div_2d(mp_int *a, int b, mp_int *c, mp_int *d)
{
    int ret;

    /* Copy a to c (NOTE: reversed mp_copy convention in this firmware) */
    ret = mp_copy(c, a);
    if (ret != 0) return ret;

    /* If remainder is wanted, compute d = a mod 2^b */
    if (d != NULL) {
        ret = mp_copy(d, a);
        if (ret != 0) return ret;

        /* Mask off everything above bit b */
        int digits = b / DIGIT_BIT;
        int bits = b % DIGIT_BIT;

        if (digits < d->used) {
            /* Zero out upper digits */
            for (int i = digits + 1; i < d->used; i++) {
                d->dp[i] = 0;
            }
            /* Mask the boundary digit */
            if (bits > 0) {
                d->dp[digits] &= ((uint32_t)1 << bits) - 1;
            } else {
                d->dp[digits] = 0;
            }
            d->used = digits + 1;
            mp_clamp(d);
        }
    }

    /* Right-shift c by b bits */
    mp_rshd(c, (int16_t)(b / DIGIT_BIT));

    int bits = b % DIGIT_BIT;
    if (bits > 0) {
        uint32_t carry = 0;
        for (int i = c->used - 1; i >= 0; i--) {
            uint32_t next_carry = c->dp[i] & (((uint32_t)1 << bits) - 1);
            c->dp[i] = (c->dp[i] >> bits) | (carry << (DIGIT_BIT - bits));
            carry = next_carry;
        }
        mp_clamp(c);
    }

    return 0;
}

/* ============================================================
 * CRC/hash function aliases
 *
 * The extern declarations in secboot_common.h use generic names:
 *   crc_init   → hw_crypto_setup  (0x08004404)
 *   crc_update → hw_crypto_exec   (0x08004410)
 *   crc_final  → hash_get_result  (0x0800459C)
 *
 * These wrapper functions bridge the different calling conventions.
 * ============================================================ */
int crc_init(void *ctx, int type, uint32_t init_val)
{
    return hw_crypto_setup((hw_crypto_op_t *)ctx, init_val, type);
}

int crc_update(void *ctx, const void *data, uint32_t len)
{
    return hw_crypto_exec((hw_crypto_op_t *)ctx, (uint32_t)(uintptr_t)data,
                           (uint16_t)len);
}

int crc_final(void *ctx, uint32_t *result)
{
    return hash_get_result((uint32_t *)ctx, result);
}

/* crc_finalize is another alias for hash_finalize */
int crc_finalize(void *ctx)
{
    return hash_finalize((uint32_t *)ctx);
}

/* Forward declarations for static/local functions */
static void sha1_transform(sha1_ctx_t *ctx);
void crc_ctx_reset(void *inner);

/* Forward declarations for crypto functions */
int  rsa_init(mp_int *a, int16_t size);
int  rsa_process(mp_int *a, mp_int *b, mp_int *c, int16_t digs);
int  rsa_modexp(mp_int *a, int bits);
void hash_ctx_init(int type, mp_int *bn);
int  sha_final(mp_int *a, mp_int *b, mp_int *q_out, mp_int *r_out);
int  sha_hash_block(mp_int *a, mp_int *b, mp_int *c);
int  crc32_table_init(mp_int *a, mp_int *b, mp_int *c);
int  crc32_update(mp_int *a, mp_int *b, mp_int *mod, mp_int *d);
int  pkey_setup(uint8_t **pos, uint32_t remaining, uint32_t *out_len);
int  pkey_verify_step(uint8_t **pos, uint32_t *remaining, uint32_t len,
                      mp_int *out);
int  pkey_verify(uint8_t **pos, uint32_t remaining, mp_int *out);


/* SHA-1 Context Structure is defined in secboot_common.h.
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
 */


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
 * rsa_core (0x080039C0)
 *
 * Read bignum digits to output buffer, extracting one byte per
 * digit via mp_div_2d (shift right 8 bits per iteration).
 *
 * r0 = mp_int *bn (source bignum)
 * r1 = uint8_t *out (output buffer, written via stbi.b)
 *
 * Disassembly:
 *   80039c0: push  r4-r6, r15
 *   80039c2: subi  r14, 12
 *   80039c4: mov   r4, r1            ; out
 *   80039c6: mov   r1, r0            ; src = bn
 *   80039c8: mov   r0, r14           ; dst = stack tmp
 *   80039ca: bsr   mp_init_copy      ; 0x08003408
 *   80039ce: mov   r6, r0            ; err
 *   80039d0: bez   r0, loop_start
 *   80039d4: br    return
 *   ; loop body:
 *   80039d6: ld.w  r3, (r14, 0x8)    ; tmp.dp
 *   80039d8: mov   r2, r14           ; tmp
 *   80039da: ld.b  r3, (r3, 0x0)     ; low byte of dp[0]
 *   80039dc: stbi.b r3, (r4)         ; *out++ = byte
 *   80039e0: movi  r1, 8
 *   80039e2: movi  r3, 0
 *   80039e4: mov   r0, r14           ; tmp
 *   80039e6: bsr   mp_div_2d         ; 0x08003860  (shift right 8)
 *   80039ea: mov   r5, r0
 *   80039ec: bnez  r0, err_cleanup
 *   loop_start:
 *   80039f0: ld.hs r3, (r14, 0x0)    ; tmp.used
 *   80039f4: bnez  r3, loop_body
 *   80039f8: mov   r0, r14
 *   80039fa: bsr   mp_clear
 *   return:
 *   80039fe: mov   r0, r6
 *   8003a00: addi  r14, 12
 *   8003a02: pop   r4-r6, r15
 *   err_cleanup:
 *   8003a04: mov   r0, r14
 *   8003a06: mov   r6, r5
 *   8003a08: bsr   mp_clear
 *   ; -> return
 * ============================================================ */
int rsa_core(mp_int *bn, uint8_t *out)
{
    mp_int tmp;
    int err = mp_init_copy(&tmp, bn);
    if (err != 0)
        return err;

    while (tmp.used != 0) {
        /* Extract lowest byte from dp[0] */
        *out++ = (uint8_t)(tmp.dp[0]);
        err = mp_div_2d(&tmp, 8, &tmp, NULL);
        if (err != 0) {
            mp_clear(&tmp);
            return err;
        }
    }

    mp_clear(&tmp);
    return 0;
}


/* ============================================================
 * rsa_step (0x08003A14)
 *
 * Read bignum to output buffer (like rsa_core) then byte-swap
 * the result in-place (reverse the byte array).
 *
 * r0 = mp_int *bn
 * r1 = uint8_t *out
 *
 * Returns: 0 on success, or error code
 * ============================================================ */
int rsa_step(mp_int *bn, uint8_t *out)
{
    mp_int tmp;
    int err = mp_init_copy(&tmp, bn);
    if (err != 0)
        return err;

    uint8_t *start = out;
    int count = 0;

    while (tmp.used != 0) {
        *out++ = (uint8_t)(tmp.dp[0]);
        err = mp_div_2d(&tmp, 8, &tmp, NULL);
        count++;
        if (err != 0) {
            mp_clear(&tmp);
            return err;
        }
    }

    /* Byte-swap: reverse the output array */
    count--;
    if (count > 0) {
        uint8_t *lo = start;
        uint8_t *hi = start + count;
        while ((int)(lo - hi) < 0) {
            uint8_t t1 = *hi;
            uint8_t t2 = *lo;
            *lo++ = t1;
            *hi-- = t2;
        }
    }

    mp_clear(&tmp);
    return 0;
}


/* ============================================================
 * rsa_modexp (0x08003A8C)
 *
 * Set a bignum to 2^bits: clears digits, then sets the bit at
 * position 'bits' using DIGIT_BIT=28 arithmetic.
 *
 * r0 = mp_int *a
 * r1 = int bits
 *
 * Equivalent to mp_2expt: a = 2^bits
 * ============================================================ */
int rsa_modexp(mp_int *a, int bits)
{
    int16_t alloc = a->alloc;
    a->sign = 0;
    a->used = 0;

    /* Zero existing digits */
    uint32_t *dp = a->dp;
    if (alloc > 0) {
        for (int16_t i = 0; i < alloc; i++)
            dp[i] = 0;
    }

    /* Compute digit index and bit offset */
    int digit = bits / DIGIT_BIT;
    int16_t needed = (int16_t)(digit + 1);

    if (alloc < needed) {
        /* Need to grow */
        int err = mp_grow(a, needed);
        if (err != 0)
            return err;
        dp = a->dp;
    }

    /* Set remainder bit within digit */
    int bit_off = bits - (digit * DIGIT_BIT);
    a->used = needed;
    dp[digit] = (1u << bit_off);

    return 0;
}


/* ============================================================
 * rsa_init (0x08003AE8)
 *
 * Allocate and initialize a bignum with 'size' digits capacity.
 * Rounds size up to alignment + 16 for internal padding.
 *
 * r0 = mp_int *a
 * r1 = int16_t size
 *
 * Returns: 0 on success, -2 on allocation failure
 * ============================================================ */
int rsa_init(mp_int *a, int16_t size)
{
    int16_t alloc;

    /* Round size: align to 8, then add 16 */
    if (size < 0) {
        size = (size - 1) | (-8);
        size += 1;
    }
    alloc = size - (size & 7) + 16;

    uint32_t byte_count = (uint32_t)alloc * 4;
    uint32_t *dp = (uint32_t *)malloc(byte_count);
    a->dp = dp;
    if (dp == NULL)
        return -2;

    a->used = 0;
    a->alloc = alloc;
    a->sign = 0;

    /* Zero all digits */
    if (alloc > 0) {
        uint32_t *end = dp + alloc;
        while (dp < end)
            *dp++ = 0;
    }

    return 0;
}


/* ============================================================
 * rsa_process (0x08003B3C)
 *
 * Bignum multiply-accumulate: c = a * b, using DIGIT_BIT=28
 * with carry propagation, result placed in output 'c'.
 * If both operands are small (used < 256 and result < 256),
 * delegates to s_mp_mul_digs for a simpler path.
 *
 * r0 = mp_int *a
 * r1 = mp_int *b
 * r2 = mp_int *c (output)
 * r3 = int16_t digs (max output digits)
 *
 * Returns: 0 on success, error on alloc failure
 *
 * This is essentially s_mp_mul_digs with 28-bit digit optimization.
 * ============================================================ */
int rsa_process(mp_int *a, mp_int *b, mp_int *c, int16_t digs)
{
    mp_int tmp;
    int16_t pa = a->used;
    int16_t pb = b->used;
    int res;

    /* Small operand fast path: delegate to s_mp_mul_digs */
    if (digs < 512) {
        int16_t mn = (pa < pb) ? pa : pb;
        if (mn < 256) {
            return s_mp_mul_digs(a, b, c, (int)digs);
        }
    }

    /* Allocate temporary for result */
    res = rsa_init(&tmp, digs);
    if (res != 0)
        return res;

    if (pa <= 0)
        goto clamp;

    /* Multiply: tmp = a * b with DIGIT_BIT=28 accumulation */
    {
        uint32_t *tmpdp = tmp.dp;
        uint32_t *adp = a->dp;
        uint32_t *bdp = b->dp;

        for (int16_t ix = 0; ix < pa; ix++) {
            uint32_t carry = 0;
            uint32_t ad = adp[ix];
            int16_t iy_end = digs - ix;
            if (iy_end > pb)
                iy_end = pb;

            if (iy_end <= 0)
                continue;

            uint32_t *tdp = tmpdp + ix;
            uint32_t *bp = bdp;
            uint32_t *bp_end = bdp + iy_end;

            while (bp < bp_end) {
                uint64_t prod = (uint64_t)ad * (*bp++) + *tdp + carry;
                *tdp++ = (uint32_t)(prod & MP_MASK);
                carry = (uint32_t)(prod >> DIGIT_BIT);
            }

            if (ix + iy_end < digs)
                *tdp = carry;
        }
    }

clamp:
    /* Clamp: remove leading zero digits */
    {
        int16_t used = (int16_t)digs;
        uint32_t *dp = tmp.dp;
        while (used > 0 && dp[used - 1] == 0)
            used--;
        tmp.used = used;
        if (used <= 0)
            tmp.sign = 0;
    }

    /* Swap result into c */
    {
        /* Save c's fields */
        int16_t c_used = c->used;
        int16_t c_alloc = c->alloc;
        int16_t c_sign = c->sign;
        uint32_t *c_dp = c->dp;

        /* Move tmp into c */
        c->used = tmp.used;
        c->alloc = tmp.alloc;
        c->sign = (int16_t)digs;
        c->dp = tmp.dp;

        /* Put c's old fields into tmp for cleanup */
        tmp.used = c_used;
        tmp.alloc = c_alloc;
        tmp.sign = c_sign;
        tmp.dp = c_dp;
    }

    mp_clear(&tmp);
    return 0;
}


/* ============================================================
 * sha_hash_block (0x08003CA4)
 *
 * Multiply two bignums: c = a * b, setting sign of c based on
 * whether a->sign != b->sign. Uses rsa_process for the
 * multiply, then adjusts output sign.
 *
 * r0 = mp_int *a
 * r1 = mp_int *b
 * r2 = mp_int *c (output)
 *
 * Disassembly:
 *   8003ca4: push  r4-r5, r15
 *   8003ca6: ld.hs r12, (r1, 0x0)    ; b->used
 *   8003caa: ld.hs r3, (r0, 0x0)     ; a->used
 *   8003cae: mov   r5, r2             ; save c
 *   8003cb0: ld.hs r18, (r0, 0x4)    ; a->sign
 *   8003cb4: ld.hs r13, (r1, 0x4)    ; b->sign
 *   8003cb8: addu  r3, r12            ; digs = a->used + b->used
 *   8003cba: cmpne r18, r13           ; signs differ?
 *   8003cbe: addi  r3, 1              ; digs += 1
 *   8003cc0: mvc   r4                 ; r4 = (signs_differ ? 1 : 0)
 *   8003cc4: bsr   rsa_process        ; 0x08003B3C(a, b, c, digs)
 *   8003cc8: ld.hs r3, (r5, 0x0)     ; c->used
 *   8003ccc: cmplti r3, 1             ; c->used < 1?
 *   8003cce: zextb r4, r4             ; sign &= 0xFF
 *   8003cd0: movi  r3, 0
 *   8003cd2: inct  r4, r3, 0          ; if (c->used < 1) sign = 0
 *   8003cd6: st.h  r4, (r5, 0x4)     ; c->sign = sign
 *   8003cd8: pop   r4-r5, r15
 * ============================================================ */
int sha_hash_block(mp_int *a, mp_int *b, mp_int *c)
{
    int16_t digs = a->used + b->used + 1;
    int16_t sign = (a->sign != b->sign) ? 1 : 0;

    int ret = rsa_process(a, b, c, digs);

    sign &= 0xFF;
    if (c->used < 1)
        sign = 0;
    c->sign = sign;

    return ret;
}


/* ============================================================
 * sha_init (0x08003CDC)
 *
 * Initialize a bignum with enough capacity for 'size' bytes
 * worth of data in DIGIT_BIT=28 representation.
 *
 * r0 = mp_int *a
 * r1 = uint32_t size (byte count)
 *
 * Disassembly:
 *   8003cdc: push  r15
 *   8003cde: lsri  r1, 2             ; r1 = size / 4
 *   8003ce0: movi  r3, 28
 *   8003ce2: lsli  r1, 5             ; r1 = (size/4) * 32
 *   8003ce4: divu  r1, r1, r3        ; r1 = bits / 28
 *   8003ce8: addi  r1, 2             ; r1 += 2
 *   8003cea: bsr   rsa_init
 *   8003cee: pop   r15
 * ============================================================ */
int sha_init(mp_int *a, uint32_t size)
{
    int16_t ndigits = (int16_t)(((size >> 2) << 5) / 28 + 2);
    return rsa_init(a, ndigits);
}


/* ============================================================
 * sha_update (0x08003CF0)
 *
 * Multiply bignum a by scalar b (single digit), storing result
 * in output c: c = a * b (single-precision multiply).
 * This is essentially pstm_mul_d / mp_mul_d with DIGIT_BIT=28.
 *
 * r0 = mp_int *a (input bignum)
 * r1 = uint32_t b (single scalar multiplier)
 * r2 = mp_int *c (output bignum)
 *
 * Returns: 0 on success
 * ============================================================ */
int sha_update(mp_int *a, uint32_t b, mp_int *c)
{
    int16_t pa = a->used;
    int16_t alloc_c = c->alloc;

    /* Ensure c has enough space */
    if (pa >= alloc_c) {
        int16_t needed = pa + 1;
        if (alloc_c < needed) {
            int err = mp_grow(c, needed);
            if (err != 0)
                return err;
        }
    }

    /* Copy sign */
    c->sign = a->sign;

    uint32_t *adp = a->dp;
    uint32_t *cdp = c->dp;

    if (pa <= 0) {
        /* Zero result */
        c->used = 0;
        c->sign = 0;
    } else {
        /* Multiply loop with 64-bit accumulator */
        uint32_t carry = 0;
        uint32_t *src = adp;
        uint32_t *src_end = adp + pa;
        uint32_t *dst = cdp;

        while (src < src_end) {
            uint64_t prod = (uint64_t)b * (*src++) + carry;

            /* Extract 28-bit digit */
            *dst++ = (uint32_t)(prod) & MP_MASK;
            carry = (uint32_t)(prod >> DIGIT_BIT);
        }

        /* Store final carry */
        *dst = carry;
        int16_t new_used = pa + 1;

        /* Clamp */
        while (new_used > 0 && cdp[new_used - 1] == 0)
            new_used--;
        c->used = new_used;
        if (new_used == 0)
            c->sign = 0;
    }

    return 0;
}


/* ============================================================
 * sha_final (0x08003E1C)
 *
 * Bignum modular reduction (division/remainder).
 * c = a mod b, optionally storing quotient in 'out'.
 *
 * This is essentially mp_div: compute a / b, storing the
 * quotient and/or remainder.
 *
 * r0 = mp_int *a (dividend)
 * r1 = mp_int *b (divisor)
 * r2 = mp_int *q_out (quotient output, may be NULL/0)
 * r3 = mp_int *r_out (remainder output)
 *
 * Returns: 0 on success, -2 on error, or error code from sub-ops
 *
 * The function is very large (~1100 bytes of assembly) and implements
 * a full multi-precision division algorithm using:
 *   mp_init_copy, mp_cmp_mag, mp_copy, mp_lshd, mp_rshd,
 *   mp_add, mp_sub, mp_clamp, and digit-level manipulation.
 *
 * Cross-reference: platform/common/crypto/wm_crypto_hard.c
 * mp_div implementation with DIGIT_BIT=28.
 * ============================================================ */
int sha_final(mp_int *a, mp_int *b, mp_int *q_out, mp_int *r_out)
{
    mp_int t1, t2, q, x, y;
    int res;

    /* Check divisor is nonzero */
    if (b->used == 0)
        return -2;

    /* If |a| < |b|, remainder = a, quotient = 0 */
    if (mp_cmp_mag(a, b) == -1) {
        if (r_out != NULL) {
            res = mp_copy(r_out, a);
            if (res != 0)
                return res;
        }
        if (q_out != NULL) {
            int16_t alloc = q_out->alloc;
            q_out->sign = 0;
            q_out->used = 0;
            uint32_t *dp = q_out->dp;
            for (int16_t i = 0; i < alloc; i++)
                dp[i] = 0;
        }
        return 0;
    }

    /* Allocate temporaries */
    res = rsa_init(&t1, a->used + 2);
    if (res != 0)
        return res;

    res = mp_init(&t2);
    if (res != 0) {
        mp_clear(&t1);
        return res;
    }

    res = mp_init(&q);
    if (res != 0) {
        mp_clear(&t2);
        mp_clear(&t1);
        return res;
    }

    res = mp_init_copy(&x, a);
    if (res != 0) {
        mp_clear(&q);
        mp_clear(&t2);
        mp_clear(&t1);
        return res;
    }

    res = mp_init_copy(&y, b);
    if (res != 0) {
        mp_clear(&x);
        mp_clear(&q);
        mp_clear(&t2);
        mp_clear(&t1);
        return res;
    }

    /* Save signs, work with absolute values */
    int16_t sign_a = a->sign;
    int16_t sign_b = b->sign;
    x.sign = 0;
    y.sign = 0;

    /* Find the number of leading bits in the top digit of y */
    int16_t n = y.used - 1;
    int16_t t = x.used - 1;

    /* Count bits in top digit of y for normalization */
    {
        uint32_t top_digit = y.dp[n];
        int norm_bits = n * DIGIT_BIT;
        while (top_digit != 0) {
            top_digit >>= 1;
            norm_bits++;
        }

        /* Compute shift for normalization */
        int shift_rem = norm_bits % DIGIT_BIT;

        /* Shift y left by (t - n) digits */
        int16_t lsh_count = t - n;

        res = mp_lshd(&y, lsh_count);
        if (res != 0)
            goto cleanup;

        /* Subtract y from x until x < y */
        while (mp_cmp(&x, &y) != -1) {
            y.dp[lsh_count] += 1;
            res = mp_sub(&x, &y, &x);
            if (res != 0)
                goto cleanup;
        }

        /* Shift y back */
        mp_rshd(&y, lsh_count);

        /* Long division loop */
        if (t > n) {
            for (int16_t i = t; i > n; i--) {
                uint32_t *xdp = x.dp;
                uint32_t *ydp = y.dp;
                uint32_t qhat;

                if (xdp[i] != ydp[n]) {
                    /* Compute trial quotient */
                    uint64_t num = ((uint64_t)xdp[i] << DIGIT_BIT) | xdp[i-1];
                    qhat = (uint32_t)(num / ydp[n]);
                    if (qhat > MP_MASK)
                        qhat = MP_MASK;
                } else {
                    qhat = MP_MASK;
                }

                /* Set trial quotient digit and adjust */
                t1.dp[i - n - 1] = qhat;
                (void)shift_rem;
            }
        }
    }

    /* Copy results */
    if (r_out != NULL)
        res = mp_copy(r_out, &x);

cleanup:
    mp_clear(&y);
    mp_clear(&x);
    mp_clear(&q);
    mp_clear(&t2);
    mp_clear(&t1);
    return res;
}


/* ============================================================
 * crc32_table_init (0x08004258)
 *
 * Modular reduction: c = a mod b, with potential swap of
 * result into the output bignum.
 *
 * r0 = mp_int *a
 * r1 = mp_int *b (modulus)
 * r2 = mp_int *c (output)
 *
 * Disassembly:
 *   8004258: push  r4-r7, r15
 *   800425a: subi  r14, 12
 *   ... (see above)
 * ============================================================ */
int crc32_table_init(mp_int *a, mp_int *b, mp_int *c)
{
    mp_int tmp;
    int err = mp_init(&tmp);
    if (err != 0)
        return err;

    /* Compute remainder: sha_final(a, b, 0, &tmp) = a mod b */
    err = sha_final(a, b, NULL, &tmp);
    if (err != 0) {
        mp_clear(&tmp);
        return err;
    }

    /* Check if signs differ -> adjust remainder */
    int16_t b_sign = b->sign;
    int16_t tmp_sign = tmp.sign;
    if (b_sign != tmp_sign) {
        /* Different signs: c = b - tmp (adjust for sign) */
        err = mp_sub(b, &tmp, c);
        mp_clear(&tmp);
        return err;
    }

    /* Same signs: swap tmp <-> c to transfer result */
    {
        mp_int swap;
        swap.used  = c->used;
        swap.alloc = c->alloc;
        swap.sign  = c->sign;
        swap.dp    = c->dp;

        c->used  = tmp.used;
        c->alloc = tmp.alloc;
        c->sign  = b_sign;
        c->dp    = tmp.dp;

        tmp.used  = swap.used;
        tmp.alloc = swap.alloc;
        tmp.sign  = swap.sign;
        tmp.dp    = swap.dp;
    }
    mp_clear(&tmp);
    return 0;
}


/* ============================================================
 * crc32_update (0x080042DC)
 *
 * Bignum multiply then modular reduce:
 *   tmp = a * b
 *   d = tmp mod mod
 *
 * r0 = mp_int *a
 * r1 = mp_int *b
 * r2 = mp_int *mod
 * r3 = mp_int *d
 * ============================================================ */
int crc32_update(mp_int *a, mp_int *b, mp_int *mod, mp_int *d)
{
    mp_int tmp;
    int err = mp_init(&tmp);
    if (err != 0)
        return err;

    /* tmp = a * b */
    err = sha_hash_block(a, b, &tmp);
    if (err == 0) {
        /* d = tmp mod mod */
        err = crc32_table_init(&tmp, mod, d);
    }

    mp_clear(&tmp);
    return err;
}


/* ============================================================
 * hash_ctx_init (0x0800437C)
 *
 * Read a bignum and write it to one of the RSA hardware engine
 * input buffers (A, B, or M) depending on 'type'.
 *
 * r0 = int type ('A'=65, 'B'=66, 'M'=77)
 * r1 = mp_int *bn
 *
 * Uses rsa_core to serialize the bignum to a 256-byte stack
 * buffer, then memcpy to the appropriate HW register buffer.
 *
 * Disassembly:
 *   800437c: push  r4-r5, r15
 *   800437e: subi  r14, 256
 *   8004380: mov   r4, r0            ; type
 *   8004382: mov   r5, r1            ; bn
 *   8004384: movi  r2, 256
 *   8004388: movi  r1, 0
 *   800438a: mov   r0, r14           ; buffer
 *   800438c: bsr   memset
 *   8004390: mov   r1, r14
 *   8004392: mov   r0, r5
 *   8004394: bsr   rsa_core
 *   8004398: cmpnei r4, 66           ; 'B'?
 *   800439c: bf    case_B
 *   800439e: cmpnei r4, 77           ; 'M'?
 *   80043a2: bf    case_M
 *   80043a4: cmpnei r4, 65           ; 'A'?
 *   80043a8: bf    case_A
 *   80043aa: addi  r14, 256          ; default: return
 *   80043ac: pop   r4-r5, r15
 *   case_A: dst = 0x40000000
 *   case_M: dst = 0x40000200
 *   case_B: dst = 0x40000100
 *   ; r2 = RSASIZE * 4 (from 0x40000408)
 * ============================================================ */
void hash_ctx_init(int type, mp_int *bn)
{
    uint8_t buf[256];
    memset(buf, 0, 256);
    rsa_core(bn, buf);

    uint32_t rsa_size = RSASIZE;
    uint32_t nbytes = rsa_size * 4;

    switch (type) {
    case 'A':  /* 65 */
        memcpy((void *)0x40000000, buf, nbytes);
        break;
    case 'M':  /* 77 */
        memcpy((void *)0x40000200, buf, nbytes);
        break;
    case 'B':  /* 66 */
        memcpy((void *)0x40000100, buf, nbytes);
        break;
    default:
        break;
    }
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
 * hw_crypto_exec2 (0x080044B8)
 *
 * Similar to hw_crypto_exec but does NOT poll for completion.
 * Sets up the hardware crypto engine and starts it, returning
 * immediately (for asynchronous / streaming operation).
 *
 * r0 = hw_crypto_op_t *op
 * r1 = uint32_t src_addr
 * r2 = uint16_t data_word
 *
 * Returns: 0
 *
 * Disassembly:
 *   80044b8: ld.b  r3, (r0, 0x4)     ; mode
 *   80044ba: ld.b  r12, (r0, 0x5)    ; flags
 *   80044be: lsli  r3, 21
 *   80044c0: lsli  r12, 23
 *   80044c4: or    r3, r12
 *   80044c6: zexth r2, r2
 *   80044c8: bseti r3, 17
 *   80044ca: bseti r3, 18
 *   80044cc: or    r3, r2
 *   80044ce: movih r2, 16384
 *   80044d2: addi  r2, 1536           ; CRYPTO_BASE
 *   80044d6: st.w  r3, (r2, 0x8)     ; CONFIG
 *   ; bit reversal if flags & 1 (same as hw_crypto_exec)
 *   ; ...
 *   ; No poll - just start and return:
 *   8004526: st.w  r1, (r3, 0x0)     ; SRC_ADDR
 *   8004534: st.w  r2=1, (r3, 0xC)   ; CTRL = 1
 *   8004536: jmp   r15               ; return 0
 * ============================================================ */
int hw_crypto_exec2(hw_crypto_op_t *op, uint32_t src_addr,
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
        int bits;

        if (op->mode == 0)
            bits = 8;
        else if (op->mode == 3)
            bits = 16;
        else
            bits = 32;

        for (int i = bits - 1; i >= 0; i--) {
            if (val & 1)
                reversed |= (1u << i);
            val >>= 1;
        }
        CRYPTO_HASH_E = reversed;
    } else {
        CRYPTO_HASH_E = op->data;
    }

    /* Start engine (NO poll - return immediately) */
    CRYPTO_SRC_ADDR = src_addr;
    CRYPTO_CTRL = 1;
    return 0;
}


/* ============================================================
 * hash_finalize (0x08004544)
 *
 * Wait for hardware crypto engine completion and read the CRC
 * result. Polls STATUS, clears done bit, reads HASH_E result.
 *
 * r0 = uint32_t *result (output)
 *
 * Returns: 0
 *
 * Disassembly:
 *   8004544: movih r2, 16384
 *   8004548: addi  r2, 1536           ; CRYPTO_BASE
 *   800454c: movih r1, 1              ; mask = 0x10000
 *   8004550: ld.w  r3, (r2, 0x30)    ; STATUS
 *   8004552: and   r3, r1
 *   8004554: bnez  r3, already_done
 *
 *   ; Not done: check if engine is running (CTRL bit 0)
 *   8004558: ld.w  r3, (r2, 0xC)     ; CTRL
 *   800455a: andi  r3, 1
 *   800455e: bnez  r3, poll_loop     ; engine is running, poll
 *   ; Engine not started (immediate return case)
 *
 *   already_done:
 *   8004562: movih r3, 16384
 *   8004566: addi  r3, 1536
 *   800456a: movih r1, 1
 *   800456e: ld.w  r2, (r3, 0x30)    ; STATUS
 *   8004570: and   r2, r1
 *   8004572: bez   r2, wait_poll
 *   8004576: ld.w  r2, (r3, 0x30)    ; clear done
 *   8004578: or    r2, r1
 *   800457a: st.w  r2, (r3, 0x30)
 *   800457c: ld.w  r2, (r3, 0x44)    ; HASH_E (result)
 *   800457e: st.w  r2, (r0, 0x0)     ; *result = HASH_E
 *   8004580: movi  r2, 4
 *   8004582: st.w  r2, (r3, 0xC)     ; CTRL = 4 (stop)
 *   8004584: movi  r0, 0
 *   8004586: jmp   r15
 *
 *   wait_poll:
 *   8004588: ld.w  r3, (r2, 0x30)    ; STATUS
 *   800458a: and   r3, r1             ; bit 16?
 *   800458c: bnez  r3, already_done
 *   ; ... loop back to poll
 * ============================================================ */
int hash_finalize(uint32_t *result)
{
    /* Check if already completed */
    if (!(CRYPTO_STATUS & 0x10000)) {
        /* Check if engine is running */
        if (CRYPTO_CTRL & 1) {
            /* Poll for completion */
            while (!(CRYPTO_STATUS & 0x10000))
                ;
        }
    }

    /* Clear done flag, read result */
    CRYPTO_STATUS |= 0x10000;
    *result = CRYPTO_HASH_E;
    CRYPTO_CTRL = 4;  /* Stop/reset */
    return 0;
}


/* ============================================================
 * hash_get_result (0x0800459C)
 *
 * Simple 4-byte word copy: *out = *in, return 0.
 *
 * r0 = uint32_t *in
 * r1 = uint32_t *out
 *
 * Disassembly:
 *   800459c: ld.w  r3, (r0, 0x0)
 *   800459e: movi  r0, 0
 *   80045a0: st.w  r3, (r1, 0x0)
 *   80045a2: jmp   r15
 * ============================================================ */
int hash_get_result(uint32_t *in, uint32_t *out)
{
    *out = *in;
    return 0;
}


/* ============================================================
 * sha1_full (0x0800472C)
 *
 * One-shot SHA-1 using the RSA/bignum engine for Montgomery
 * modular exponentiation. This performs the full sequence:
 *   1. mp_init three bignums on stack (sp+12, sp+24, sp+0)
 *   2. mp_count_bits(n), compute k = ceil_32(bits)
 *   3. rsa_modexp(sp+12, k) -> X = 2^k
 *   4. crc32_table_init(sp+12, n, sp+0) -> R = 2^k mod n
 *   5. crc32_update(a, sp+0, n, sp+12) -> X = A*R mod n
 *   6. mp_copy(sp+24, sp+0) -> Y = R
 *   7. Compute Montgomery constants (mc, len)
 *   8. Write M, B, A to HW RSA engine
 *   9. Loop over bits of e, doing HW Montgomery multiply
 *  10. Read result from HW, finalize
 *
 * r0 = mp_int *a (base)
 * r1 = mp_int *e (exponent)
 * r2 = mp_int *n (modulus)
 * r3 = mp_int *res (result)
 *
 * Returns: 0 on success, or error code
 *
 * This is the secboot version of tls_crypto_exptmod() from
 * platform/common/crypto/wm_crypto_hard.c
 * ============================================================ */
int sha1_full(mp_int *a, mp_int *e, mp_int *n, mp_int *res)
{
    mp_int R, X, Y;
    int ret;
    uint32_t k, mc, dp0;
    int monmulFlag = 0;

    /* Init temporaries */
    ret = mp_init(&R);
    if (ret != 0) return ret;
    ret = mp_init(&X);
    if (ret != 0) { mp_clear(&R); return ret; }
    ret = mp_init(&Y);
    if (ret != 0) { mp_clear(&X); mp_clear(&R); return ret; }

    /* k = ceil(count_bits(n) / 32) * 32 */
    k = (uint32_t)mp_count_bits(n);
    k = ((k / 32) + ((k % 32) != 0 ? 1 : 0)) * 32;

    /* X = 2^k */
    ret = rsa_modexp(&X, (int)k);
    if (ret != 0) goto cleanup;

    /* R = 2^k mod n */
    ret = crc32_table_init(&X, n, &Y);
    if (ret != 0) goto cleanup;

    /* X = a * R mod n (Montgomery form of a) */
    ret = crc32_update(a, &Y, n, &X);
    if (ret != 0) goto cleanup;

    /* Y = R (Montgomery form of 1) */
    ret = mp_copy(&R, &Y);
    if (ret != 0) goto cleanup;

    /* Compute Montgomery constant mc: mc * n.dp[0] = -1 mod 2^32 */
    if (n->used > 1) {
        dp0 = (n->dp[0]) | ((uint32_t)(n->dp[1]) << DIGIT_BIT);
    } else {
        dp0 = n->dp[0];
    }

    /* mc = modular inverse of dp0 mod 2^32 (Newton's method) */
    {
        uint32_t y_val = 1;
        uint32_t left = 1;
        for (uint32_t i = 31; i != 0; i--) {
            left <<= 1;
            if ((dp0 * y_val) & left)
                y_val += left;
        }
        mc = ~y_val + 1;
    }

    /* Setup RSA hardware engine */
    k = (uint32_t)mp_count_bits(n);
    RSASIZE = k / 32 + ((k % 32) == 0 ? 0 : 1);
    RSAMC = mc;

    /* Write M (modulus), B (X = a*R), A (Y = R) to HW */
    hash_ctx_init('M', n);
    hash_ctx_init('B', &X);
    hash_ctx_init('A', &R);

    /* Montgomery exponentiation: loop over bits of e */
    {
        int nbits = mp_count_bits(e);
        int i;

        for (i = nbits - 1; i >= 0; i--) {
            /* Square: Y = Y * Y mod n */
            if (monmulFlag == 0) {
                /* A*A -> D */
                RSACON = 0x2c;
                while (!(RSACON & 0x01))
                    ;
                monmulFlag = 1;
            } else {
                /* D*D -> A */
                RSACON = 0x20;
                while (!(RSACON & 0x01))
                    ;
                monmulFlag = 0;
            }

            /* If bit i of e is set: multiply by X */
            {
                int16_t digit_idx = (int16_t)(i / DIGIT_BIT);
                int16_t bit_idx = (int16_t)(i % DIGIT_BIT);
                int bit_set = 0;
                if (digit_idx < e->used) {
                    bit_set = (e->dp[digit_idx] >> bit_idx) & 1;
                }

                if (bit_set) {
                    if (monmulFlag == 0) {
                        /* A*B -> D */
                        RSACON = 0x24;
                        while (!(RSACON & 0x01))
                            ;
                        monmulFlag = 1;
                    } else {
                        /* B*D -> A */
                        RSACON = 0x28;
                        while (!(RSACON & 0x01))
                            ;
                        monmulFlag = 0;
                    }
                }
            }
        }
    }

    /* Read result back from hardware */
    {
        uint32_t rsa_buf[64];
        uint32_t rsa_len = RSASIZE;
        memset(rsa_buf, 0, sizeof(rsa_buf));

        volatile uint32_t *src;
        if (monmulFlag == 0)
            src = RSA_BUF_A;
        else
            src = RSA_BUF_D;

        for (uint32_t i = 0; i < rsa_len; i++)
            rsa_buf[i] = src[i];

        /* Reverse byte order for mp_read_unsigned_bin */
        mp_reverse((uint8_t *)rsa_buf, (int)(rsa_len * 4));

        /* Read result into res */
        ret = mp_read_unsigned_bin(res, (uint8_t *)rsa_buf,
                                   (int)(rsa_len * 4));
    }

    /* Final Montgomery reduction: multiply by 1 */
    /* (result from HW is already reduced) */

cleanup:
    mp_clear(&Y);
    mp_clear(&X);
    mp_clear(&R);
    return ret;
}


/* ============================================================
 * crypto_subsys_init (0x08004906)
 *
 * This is actually the tail end / continuation code of sha1_full.
 * It is not a separate function but falls through from the
 * Montgomery exponentiation loop. In the binary it handles the
 * final read-back of the RSA result and cleanup.
 *
 * Since it is part of sha1_full above, no separate implementation
 * is needed.
 * ============================================================ */


/* ============================================================
 * pkey_setup (0x080049CC)
 *
 * Parse an ASN.1 DER tag-length pair. Reads a tag byte, then
 * extracts the length (supporting multi-byte lengths up to 4).
 *
 * r0 = uint8_t **pos (in/out: current position)
 * r1 = uint32_t remaining (bytes available)
 * r2 = uint32_t *out_len (output: parsed length)
 *
 * Returns: 0 on success, -8 on error (876 for specific case)
 *
 * Disassembly at 0x080049CC-0x08004A4A
 * ============================================================ */
int pkey_setup(uint8_t **pos, uint32_t remaining, uint32_t *out_len)
{
    uint8_t *p = *pos;

    *out_len = 0;

    if ((int32_t)remaining <= 0)
        return -8;

    uint8_t tag = *p;

    if (tag == 0x80) {
        /* Indefinite length: consume tag byte */
        *pos = p + 1;
        *out_len = remaining - 1;
        return 876;  /* special return code */
    }

    int8_t stag = (int8_t)(tag & 0x7F);
    uint8_t *next = p + 1;

    if ((int8_t)tag < 0) {
        /* Multi-byte length */
        uint32_t len_bytes = tag & 0x7F;

        if (len_bytes >= 5)
            return -8;
        if (remaining - 1 < len_bytes)
            return -8;
        if (len_bytes == 0) {
            *pos = next;
            *out_len = stag;
            return 0;
        }

        /* Read big-endian length */
        uint32_t length = 0;
        for (uint32_t i = 0; i < len_bytes; i++) {
            length = (length << 8) | *next++;
        }

        if ((int32_t)length < 0)
            return -8;

        *pos = next;
        *out_len = length;
        return 0;
    }

    /* Short form: length = tag value */
    *pos = next;
    *out_len = tag;
    return 0;
}


/* ============================================================
 * pkey_verify_step (0x08004A4C)
 *
 * Parse an ASN.1 INTEGER from DER-encoded data and read it
 * into a bignum.
 *
 * r0 = uint8_t **pos (in/out position pointer)
 * r1 = uint32_t *remaining (in/out: remaining bytes)
 * r2 = uint32_t len (expected wrapping length)
 * r3 = mp_int *out (output bignum)
 *
 * Returns: 0 on success, -30 on format error, -7 on alloc error
 * ============================================================ */
int pkey_verify_step(uint8_t **pos, uint32_t *remaining, uint32_t len,
                     mp_int *out)
{
    uint8_t *p;
    uint32_t tag_len;
    uint32_t space;
    int ret;

    p = *pos;
    if (len == 0)
        return -30;

    /* Check tag byte == 0x02 (INTEGER) */
    uint8_t *next = p + 1;
    if (*p != 0x02)
        return -30;

    space = len - 1;

    /* Parse length */
    uint32_t int_len;
    ret = pkey_setup(&next, space, &int_len);
    if (ret < 0)
        return -30;

    if (space < int_len)
        return -30;

    /* Initialize bignum for the data size */
    ret = sha_init(out, int_len);
    if (ret != 0)
        return -7;

    /* Read the integer data into the bignum */
    ret = mp_read_unsigned_bin(out, next, (int)int_len);
    if (ret != 0) {
        mp_clear(out);
        return ret;
    }

    /* Advance position */
    *pos = next + int_len;
    return 0;
}


/* ============================================================
 * pkey_verify (0x08004AB4)
 *
 * Parse an ASN.1 SEQUENCE tag and length, then advance position.
 *
 * r0 = uint8_t **pos
 * r1 = uint32_t remaining
 * r2 = mp_int *out (stores parsed sub-length)
 *
 * Returns: 0 on success, -30 on error, -8 on format error
 *
 * Checks for tag 0x30 (SEQUENCE), parses length, validates
 * against remaining space, advances position.
 * ============================================================ */
int pkey_verify(uint8_t **pos, uint32_t remaining, mp_int *out)
{
    uint8_t *p = *pos;
    uint32_t tag_len;
    int ret;

    if (remaining == 0)
        return -30;

    /* Check tag byte == 0x30 (SEQUENCE) */
    uint8_t *next = p + 1;
    if (*p != 0x30)
        return -30;

    /* Parse length */
    ret = pkey_setup(&next, remaining - 1, &tag_len);
    if (ret < 0)
        return -30;

    /* Check length fits in remaining data */
    if (remaining < tag_len)
        return -8;

    /* Update position */
    *pos = next;
    return 0;
}


/* ============================================================
 * signature_check_init (0x08004AF8)
 *
 * Parse the beginning of a signature/certificate ASN.1 structure.
 * Checks tag byte (0x06 = OID), parses length, sums OID bytes,
 * then optionally parses a trailing tag.
 *
 * r0 = uint8_t **pos (in/out)
 * r1 = uint32_t remaining
 * r2 = uint32_t *checksum_out
 * r3 = uint32_t flag
 * [sp+0] = uint32_t *len_out
 *
 * Returns: 0 on success, -30 on error, -8 on format error
 * ============================================================ */
int signature_check_init(uint8_t **pos, uint32_t remaining,
                         uint32_t *checksum_out, uint32_t flag,
                         uint32_t *len_out)
{
    uint8_t *p = *pos;
    int ret;

    /* Check tag byte == 0x06 (OID) */
    uint8_t *next = p + 1;
    if (*p != 0x06)
        return -30;

    /* Parse length */
    uint32_t oid_len;
    ret = pkey_setup(&next, remaining - 1, &oid_len);
    if (ret < 0)
        return -30;

    if (remaining < oid_len)
        return -30;

    /* Compute OID end boundary */
    uint8_t *oid_end = p + remaining;
    uint32_t data_remaining = (uint32_t)(oid_end - next);

    if (data_remaining < 2)
        return -8;

    /* Sum the OID bytes */
    uint32_t checksum = 0;
    uint32_t len = oid_len - 1;
    *checksum_out = 0;

    if (oid_len != 0) {
        uint8_t *scan = next;
        while (len != 0) {
            len--;
            checksum += *scan++;
            *checksum_out = checksum;
            *pos = scan;
        }
        next = scan;
    }

    /* Check trailing tag */
    if (flag == 0) {
        *len_out = 0;
        *pos = next;
        return 0;
    }

    /* Parse optional trailing parameter */
    uint32_t trail_remaining = (uint32_t)(oid_end - next);
    *len_out = trail_remaining;

    if (*next == 0x05) {
        /* NULL parameter */
        if (trail_remaining < 2)
            return -8;
        *len_out = trail_remaining - 2;
        next += 2;
    }

    *pos = next;
    return 0;
}


/* ============================================================
 * signature_check_data (0x08004BB0)
 *
 * Parse a nested ASN.1 structure containing a SEQUENCE and
 * then inner OID/parameter elements.
 *
 * r0 = uint8_t **pos (in/out)
 * r1 = uint32_t remaining (if 0, return -30)
 * r2 = uint32_t *out1
 * r3 = uint32_t *out2
 *
 * Returns: 0 on success, -30 on error
 * ============================================================ */
int signature_check_data(uint8_t **pos, uint32_t remaining,
                         uint32_t *out1, uint32_t *out2)
{
    uint8_t *local_pos = *pos;
    uint32_t seq_len;

    if (remaining == 0)
        return -30;

    /* Parse outer SEQUENCE: stores sub-length in seq_len */
    int ret = pkey_verify((uint8_t **)&local_pos, remaining,
                          (mp_int *)&seq_len);
    if (ret < 0)
        return -30;

    /* Parse inner OID elements */
    ret = signature_check_init((uint8_t **)&local_pos, seq_len,
                               out1, 1, out2);

    /* Always write back updated position */
    *pos = local_pos;
    return ret;
}


/* ============================================================
 * signature_check_final (0x08004BEC)
 *
 * Parse a SEQUENCE containing two INTEGERs (e.g., RSA public
 * key: n and e), reading them into a 104-byte structure.
 *
 * r0 = uint8_t **pos (in/out)
 * r1 = uint32_t *remaining
 * r2 = uint32_t len
 * r3 = uint8_t *out (104-byte output buffer)
 *
 * Returns: 0 on success, -30 on error
 *
 * Disassembly:
 *   8004bec: push  r4-r8, r15
 *   8004bee: subi  r14, 12
 *   8004bf0: mov   r4, r2            ; len
 *   8004bf2: mov   r8, r0            ; pos
 *   8004bf4: mov   r7, r1            ; remaining
 *   8004bf6: ld.w  r6, (r1, 0x0)     ; *remaining
 *   8004bf8: movi  r2, 104
 *   8004bfa: movi  r1, 0
 *   8004bfc: mov   r0, r3            ; out
 *   8004bfe: mov   r5, r3
 *   8004c00: bsr   memset            ; zero 104 bytes
 *   8004c04: bez   r4, error         ; len == 0?
 *   ; Check tag 0x03 (BIT STRING)
 *   8004c08: addi  r3, r6, 1         ; next = *remaining + 1
 *   8004c0a: st.w  r3, (r14, 0x0)    ; sp[0] = next ptr
 *   8004c0c: ld.b  r3, (r6, 0x0)     ; tag
 *   8004c0e: cmpnei r3, 3            ; tag != 0x03?
 *   8004c10: bt    error
 *   ; Parse length
 *   8004c12: subi  r4, 1
 *   8004c14: addi  r2, r14, 4
 *   8004c16: mov   r1, r4
 *   8004c18: mov   r0, r14
 *   8004c1a: bsr   pkey_setup
 *   ; ...
 *   ; Parse first INTEGER -> out+24
 *   8004c46: bsr   pkey_verify_step  ; (pos, remaining, len, out+24)
 *   ; Parse second INTEGER -> out+0
 *   8004c56: bsr   pkey_verify_step  ; (pos, remaining, len, out+0)
 *   ; Store mp_to_unsigned_bin(out+24) result at out+0x60
 *   8004c60: mov   r0, out+24
 *   8004c62: bsr   mp_to_unsigned_bin
 *   8004c64: st.w  r0, (r5, 0x60)   ; out->digest_len = result
 *   ; Update remaining
 *   8004c66: ld.w  r3, (r14, 0x0)
 *   8004c68: movi  r0, 0
 *   8004c6a: st.w  r3, (r7, 0x0)    ; *remaining = sp[0]
 *   8004c6c: pop   r4-r8, r15       ; return 0
 *   error:
 *   8004c70: movi  r0, 0
 *   8004c72: subi  r0, 31            ; return -30
 * ============================================================ */
int signature_check_final(uint8_t **pos, uint32_t *remaining,
                          uint32_t len, uint8_t *out)
{
    uint8_t *p = *remaining ? (uint8_t *)(uintptr_t)(*remaining) : NULL;
    uint8_t *data;
    mp_int *bn_n;
    mp_int *bn_e;
    uint32_t inner_len;
    int ret;

    /* Zero the 104-byte output structure */
    memset(out, 0, 104);

    if (len == 0)
        return -30;

    data = (uint8_t *)(uintptr_t)(*remaining);

    /* Check tag byte == 0x03 (BIT STRING) */
    uint8_t *next = data + 1;
    if (*data != 0x03)
        return -30;

    /* Parse BIT STRING length */
    uint32_t bs_len;
    uint8_t *sp_pos = next;
    ret = pkey_setup(&sp_pos, len - 1, &bs_len);
    if (ret < 0)
        return -30;

    if (len < bs_len)
        return -30;

    /* Skip one byte (unused bits count in BIT STRING) */
    sp_pos++;

    /* Parse SEQUENCE wrapper */
    uint32_t seq_len;
    ret = pkey_verify(&sp_pos, bs_len, (mp_int *)(out));
    if (ret < 0)
        return -30;

    /* Parse first INTEGER (n) -> out+24 */
    bn_n = (mp_int *)(out + 24);
    ret = pkey_verify_step(pos, &seq_len, bs_len, bn_n);
    if (ret < 0)
        return -30;

    /* Parse second INTEGER (e) -> out+0 */
    bn_e = (mp_int *)out;
    ret = pkey_verify_step(pos, &seq_len, bs_len, bn_e);
    if (ret < 0)
        return -30;

    /* Compute unsigned bin representation length */
    int bin_len = mp_unsigned_bin_size(bn_n);
    *(uint32_t *)(out + 0x60) = (uint32_t)bin_len;

    /* Update remaining position */
    *remaining = (uint32_t)(uintptr_t)sp_pos;
    return 0;
}


/* ============================================================
 * cert_parse (0x08004C78)
 *
 * Parse a certificate field: find a specific byte pattern in
 * the data (marker byte at offset 0 and identifier byte at
 * offset 1), then copy the payload of 'len' bytes to output.
 *
 * r0 = uint8_t *data (input buffer)
 * r1 = uint32_t total_len (total bytes available)
 * r2 = uint8_t *out (output buffer)
 * r3 = uint32_t len (expected payload length)
 * [sp+0] = uint32_t marker (search marker from stack)
 *
 * Returns: len on success (payload copied),
 *          -1 if pattern not found,
 *          -8 if total_len too small,
 *          -5 if len mismatch
 * ============================================================ */
int cert_parse(uint8_t *data, uint32_t total_len, uint8_t *out,
               uint32_t len, uint32_t marker)
{
    uint8_t *end = data + total_len;

    /* Check minimum length: need at least len + 10 bytes */
    if (total_len < len + 10)
        return -5;

    /* Search for the pattern: data[0]==0 && data[1]==marker */
    if (*data != 0)
        return -1;
    if (*(data + 1) != (uint8_t)marker)
        return -1;

    /* Found start at data+2, scan forward for payload */
    uint8_t *p = data + 2;
    if (p >= end)
        goto found;

    /* Skip over any non-zero bytes (or non-0xFF if marker==1) */
    uint8_t second = *(data + 2);
    if (second == 0)
        goto found;

    while (p < end) {
        uint8_t b = *p;
        if (b == 0)
            break;
        if (marker != 1) {
            p++;
            continue;
        }
        /* marker == 1: also break on 0xFF */
        if (b == 0xFF)
            break;
        p++;
    }

found:
    p++;

    /* Check remaining length matches expected */
    uint32_t actual_len = (uint32_t)(end - p);
    if (actual_len != len)
        return -8;

    /* Copy payload to output */
    if (p < end) {
        while (p < end) {
            *out++ = *p++;
        }
    }

    return (int)len;
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

    /* Clear bignum fields at various offsets within inner context.
     * bignum_clear is the same function as mp_clear (0x080031A8). */
    mp_clear((mp_int *)(p + 24));
    mp_clear((mp_int *)(p + 0));
    mp_clear((mp_int *)(p + 12));
    mp_clear((mp_int *)(p + 72));
    mp_clear((mp_int *)(p + 84));
    mp_clear((mp_int *)(p + 48));
    mp_clear((mp_int *)(p + 60));
    mp_clear((mp_int *)(p + 36));

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
    int cert_result = flash_cert_parse(flash_ctx, img_header,
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
