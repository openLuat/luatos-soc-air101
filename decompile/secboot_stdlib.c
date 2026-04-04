/**
 * secboot_stdlib.c - Decompiled standard library functions
 *
 * Pseudo-C reconstruction from C-SKY XT804 disassembly of secboot.bin
 * Cross-referenced with: GCC newlib/minilibc (libc/string, libc/stdio)
 *
 * These are standard C library functions compiled from GCC newlib/minilibc
 * and statically linked into the secboot binary.  Implementations follow
 * typical newlib patterns optimised for the C-SKY XT804 core.
 *
 * Functions:
 *   memcpy_words()     - 0x080025C0  (word-aligned memory copy with flag)
 *   printf_simple()    - 0x0800290C  (simplified printf, limited specifiers)
 *   calloc()           - 0x08002980  (allocate + zero-fill)
 *   memcmp_or_recv()   - 0x08002AB4  (memory compare / receive buffer op)
 *   strlen()           - 0x08002D7C  (string length)
 *   strcmp_or_strncpy() - 0x08002EAC (string compare or bounded copy)
 *   format_number()    - 0x08002FC8  (integer-to-string for printf)
 *   vsnprintf_core()   - 0x08003150  (core vsnprintf engine)
 *   snprintf()         - 0x08003178  (bounded formatted print)
 *   sprintf()          - 0x080031A8  (unbounded formatted print)
 *   printf()           - 0x080031DC  (formatted print to UART)
 */

#include <stdint.h>
#ifndef NULL
#define NULL ((void *)0)
#endif
#include <stdarg.h>

/* Forward declarations (defined in other decompile units) */
extern void *malloc(uint32_t size);                 /* secboot_memory.c 0x080029A0 */
extern void *memcpy(void *dst, const void *src, uint32_t n); /* secboot_memory.c 0x08002B54 */
extern void  uart_putchar(int ch);                  /* secboot_uart.c  0x0800717C */


/* ============================================================
 * memcpy_words (0x080025C0)
 *
 * Word-aligned memory copy used by startup / CRT0 code to
 * initialise .data (copy ROM -> RAM) and .bss (zero-fill).
 * The flag parameter selects the operation mode.
 *
 * r0 = dst   (destination, word-aligned)
 * r1 = src   (source, word-aligned; ignored when zeroing)
 * r2 = count (number of WORDS to copy/zero)
 * r3 = flag  (0 = copy src->dst, 1 = zero-fill dst)
 *
 * Returns: nothing (void)
 *
 * Disassembly (abbreviated — standard CRT init pattern):
 *   80025c0: bez   r2, done            ; if count == 0, return
 *   80025c2: cmpnei r3, 0
 *   80025c4: bt    zero_fill           ; flag != 0 -> zero mode
 * copy_loop:
 *   80025c6: ld.w  r12, (r1, 0x0)     ; r12 = *src
 *   80025c8: st.w  r12, (r0, 0x0)     ; *dst = r12
 *   80025ca: addi  r0, 4              ; dst += 4
 *   80025cc: addi  r1, 4              ; src += 4
 *   80025ce: subi  r2, 1              ; count--
 *   80025d0: bnez  r2, copy_loop
 *   80025d2: jmp   r15
 * zero_fill:
 *   80025d4: movi  r12, 0
 * zero_loop:
 *   80025d6: st.w  r12, (r0, 0x0)     ; *dst = 0
 *   80025d8: addi  r0, 4              ; dst += 4
 *   80025da: subi  r2, 1              ; count--
 *   80025dc: bnez  r2, zero_loop
 *   80025de: jmp   r15
 * done:
 *   80025e0: jmp   r15
 * ============================================================ */
void memcpy_words(uint32_t *dst, const uint32_t *src, uint32_t count, uint32_t flag)
{
    if (count == 0)
        return;

    if (flag != 0) {
        /* Zero-fill mode (.bss initialisation) */
        while (count--) {
            *dst++ = 0;
        }
    } else {
        /* Copy mode (.data initialisation: ROM -> RAM) */
        while (count--) {
            *dst++ = *src++;
        }
    }
}


/* ============================================================
 * printf_simple (0x0800290C)
 *
 * Simplified printf with limited format specifiers (%s, %x, %d).
 * Uses a small on-stack buffer and outputs via uart_putchar().
 * Typically called during early boot before full libc is usable.
 *
 * r0 = fmt (format string pointer)
 * ... = variadic arguments
 *
 * Disassembly (abbreviated):
 *   800290c: push  r4-r7, r15
 *   800290e: subi  sp, 64              ; char buf[64] on stack
 *   8002910: mov   r4, r0              ; r4 = fmt
 *   8002912: mov   r5, sp              ; r5 = va_list start
 *   8002914: addi  r5, 4               ; skip fmt on stack
 * loop:
 *   8002918: ld.b  r0, (r4, 0x0)       ; ch = *fmt
 *   800291a: bez   r0, done            ; NUL -> end
 *   800291c: cmpnei r0, '%'
 *   800291e: bf    format_char
 *   8002920: bsr   uart_putchar        ; 0x0800717C
 *   8002924: addi  r4, 1
 *   8002926: br    loop
 * format_char:
 *   8002928: addi  r4, 1
 *   800292a: ld.b  r0, (r4, 0x0)       ; specifier
 *   800292c: cmpnei r0, 's'
 *   800292e: bf    handle_s
 *   8002930: cmpnei r0, 'x'
 *   8002932: bf    handle_x
 *   8002934: cmpnei r0, 'd'
 *   8002936: bf    handle_d
 *   ;; ... outputs string / hex / decimal via uart_putchar
 * done:
 *   8002978: addi  sp, 64
 *   800297a: pop   r4-r7, r15
 * ============================================================ */
void printf_simple(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            uart_putchar(*fmt++);
            continue;
        }
        fmt++;  /* skip '%' */

        switch (*fmt) {
        case 's': {
            const char *s = va_arg(ap, const char *);
            while (s && *s)
                uart_putchar(*s++);
            break;
        }
        case 'x': {
            uint32_t val = va_arg(ap, uint32_t);
            /* Output hex digits (no leading "0x") */
            for (int i = 28; i >= 0; i -= 4) {
                int nib = (val >> i) & 0xF;
                uart_putchar(nib < 10 ? '0' + nib : 'a' + nib - 10);
            }
            break;
        }
        case 'd': {
            int val = va_arg(ap, int);
            char buf[12];
            int idx = 0;
            if (val < 0) {
                uart_putchar('-');
                val = -val;
            }
            do {
                buf[idx++] = '0' + (val % 10);
                val /= 10;
            } while (val);
            while (idx--)
                uart_putchar(buf[idx]);
            break;
        }
        default:
            uart_putchar(*fmt);
            break;
        }
        fmt++;
    }

    va_end(ap);
}


/* ============================================================
 * calloc (0x08002980)
 *
 * Standard calloc: allocate nmemb*size bytes, zero-filled.
 * Calls malloc() then memset() to zero.
 *
 * r0 = nmemb, r1 = size
 * Returns: pointer to allocated zeroed memory, or NULL
 *
 * Disassembly:
 *   8002980: push  r4, r15
 *   8002982: mult  r4, r0, r1          ; r4 = total = nmemb * size
 *   8002984: mov   r0, r4              ; r0 = total
 *   8002986: bsr   malloc              ; 0x080029A0
 *   800298a: bez   r0, done            ; NULL check
 *   800298c: mov   r1, r0              ; save ptr
 *   800298e: movi  r2, 0
 * zero_loop:
 *   8002990: stbi.b r2, (r1)           ; *ptr++ = 0
 *   8002992: subi  r4, 1
 *   8002994: bnez  r4, zero_loop
 * done:
 *   8002996: pop   r4, r15
 * ============================================================ */
void *calloc(uint32_t nmemb, uint32_t size)
{
    uint32_t total = nmemb * size;
    void *ptr = malloc(total);
    if (ptr == NULL)
        return NULL;

    /* Zero-fill (byte-by-byte in the binary) */
    uint8_t *p = (uint8_t *)ptr;
    for (uint32_t i = 0; i < total; i++)
        p[i] = 0;

    return ptr;
}


/* ============================================================
 * memcmp_or_recv (0x08002AB4)
 *
 * Memory compare or receive-buffer operation.  Compares two
 * memory regions byte-by-byte; may also serve as a receive-
 * buffer check in the UART/Xmodem path (dual-use in the
 * binary's call graph).
 *
 * r0 = ptr1/buf, r1 = ptr2/expected, r2 = n (byte count)
 * Returns: 0 if equal, non-zero difference on mismatch
 *
 * Disassembly (abbreviated — mirrors standard memcmp):
 *   8002ab4: push  r4
 *   8002ab6: bez   r2, match           ; n == 0 -> equal
 * byte_loop:
 *   8002ab8: ld.b  r3, (r0, 0x0)       ; a = *ptr1
 *   8002abc: ld.b  r4, (r1, 0x0)       ; b = *ptr2
 *   8002ac0: subu  r3, r4              ; diff = a - b
 *   8002ac2: bnez  r3, mismatch        ; if diff != 0 return
 *   8002ac4: addi  r0, 1
 *   8002ac6: addi  r1, 1
 *   8002ac8: subi  r2, 1
 *   8002aca: bnez  r2, byte_loop
 * match:
 *   8002acc: movi  r0, 0
 *   8002ace: pop   r4
 * mismatch:
 *   8002ad0: mov   r0, r3
 *   8002ad2: pop   r4
 * ============================================================ */
int memcmp_or_recv(const void *ptr1, const void *ptr2, uint32_t n)
{
    const uint8_t *a = (const uint8_t *)ptr1;
    const uint8_t *b = (const uint8_t *)ptr2;

    while (n--) {
        int diff = (int)*a++ - (int)*b++;
        if (diff != 0)
            return diff;
    }
    return 0;
}


/* ============================================================
 * strlen (0x08002D7C)
 *
 * Standard string length.  Scans for NUL terminator.
 * Uses a simple byte loop (no word-at-a-time trick).
 *
 * r0 = str
 * Returns: length (not including NUL)
 *
 * Disassembly:
 *   8002d7c: mov   r1, r0              ; r1 = start
 * loop:
 *   8002d7e: ld.b  r2, (r0, 0x0)       ; ch = *p
 *   8002d80: addi  r0, 1               ; p++
 *   8002d82: bnez  r2, loop            ; while (ch != '\0')
 *   8002d84: subi  r0, 1               ; back up past NUL
 *   8002d86: subu  r0, r1              ; length = p - start
 *   8002d88: jmp   r15
 * ============================================================ */
uint32_t strlen(const char *str)
{
    const char *p = str;
    while (*p != '\0')
        p++;
    return (uint32_t)(p - str);
}


/* ============================================================
 * strcmp_or_strncpy (0x08002EAC)
 *
 * String compare or bounded string copy.  Behaves like strncmp
 * when called from validation paths, or like strncpy when used
 * for copying bounded strings.  The disassembly matches a
 * standard newlib strncmp implementation.
 *
 * r0 = s1 (or dst), r1 = s2 (or src), r2 = n (max chars)
 * Returns: 0 if equal within n bytes, else difference
 *
 * Disassembly:
 *   8002eac: push  r4
 *   8002eae: bez   r2, equal           ; n == 0 -> equal
 * loop:
 *   8002eb0: ld.b  r3, (r0, 0x0)       ; c1 = *s1
 *   8002eb2: ld.b  r4, (r1, 0x0)       ; c2 = *s2
 *   8002eb4: subu  r12, r3, r4         ; diff = c1 - c2
 *   8002eb6: bnez  r12, done           ; mismatch -> return diff
 *   8002eb8: bez   r3, done            ; c1 == '\0' -> done
 *   8002eba: addi  r0, 1               ; s1++
 *   8002ebc: addi  r1, 1               ; s2++
 *   8002ebe: subi  r2, 1               ; n--
 *   8002ec0: bnez  r2, loop
 * equal:
 *   8002ec2: movi  r0, 0
 *   8002ec4: pop   r4
 * done:
 *   8002ec6: mov   r0, r12
 *   8002ec8: pop   r4
 * ============================================================ */
int strcmp_or_strncpy(const char *s1, const char *s2, uint32_t n)
{
    if (n == 0)
        return 0;

    while (n--) {
        unsigned char c1 = (unsigned char)*s1;
        unsigned char c2 = (unsigned char)*s2;
        int diff = c1 - c2;
        if (diff != 0)
            return diff;
        if (c1 == '\0')
            return 0;
        s1++;
        s2++;
    }
    return 0;
}


/* ============================================================
 * format_number (0x08002FC8)
 *
 * Number formatting helper for the printf family.  Converts an
 * unsigned integer to a string representation in a given base,
 * with optional padding (spaces or zeroes) and sign handling.
 *
 * This is the internal itoa-style routine called by
 * vsnprintf_core() when processing %d, %u, %x, %X, %o.
 *
 * r0 = output_cb  (callback: void (*)(char, void*) or buffer ctx)
 * r1 = ctx         (opaque context for output_cb)
 * r2 = value       (unsigned integer to format)
 * r3 = base        (radix: 10, 16, 8)
 * [sp+0] = width   (minimum field width)
 * [sp+4] = flags   (bit 0 = zero-pad, bit 1 = left-align,
 *                    bit 2 = sign/space, bit 3 = uppercase hex)
 *
 * Disassembly (abbreviated — follows typical newlib _ntoa_long):
 *   8002fc8: push  r4-r11, r15
 *   8002fca: subi  sp, 36              ; char tmp[36] on stack
 *   8002fcc: mov   r4, r0              ; output callback
 *   8002fce: mov   r5, r1              ; context
 *   8002fd0: mov   r6, r2              ; value
 *   8002fd2: mov   r7, r3              ; base
 *   8002fd4: ld.w  r8, (sp, 72)        ; width (from caller stack)
 *   8002fd8: ld.w  r9, (sp, 76)        ; flags
 *   ;; --- digit extraction loop ---
 *   8002fdc: movi  r10, 0              ; digit count = 0
 * digit_loop:
 *   8002fde: divs  r11, r6, r7         ; quotient
 *   8002fe2: mult  r12, r11, r7
 *   8002fe4: subu  r12, r6, r12        ; remainder = value - quot*base
 *   8002fe6: cmplti r12, 10
 *   8002fe8: bt    is_digit
 *   8002fea: addi  r12, 'a'-10         ; hex letter (lowercase)
 *   8002fec: br    store
 * is_digit:
 *   8002fee: addi  r12, '0'            ; decimal digit
 * store:
 *   8002ff0: st.b  r12, (sp, r10)      ; tmp[count] = ch
 *   8002ff2: addi  r10, 1
 *   8002ff4: mov   r6, r11             ; value = quotient
 *   8002ff6: bnez  r6, digit_loop
 *   ;; --- padding + output (digits in reverse) ---
 *   8002ff8: subu  r8, r10             ; pad_count = width - digits
 *   ;; ... (padding logic, then reverse output of tmp[])
 *   8003148: addi  sp, 36
 *   800314a: pop   r4-r11, r15
 * ============================================================ */
static void format_number(void (*output_cb)(char, void *), void *ctx,
                          uint32_t value, uint32_t base,
                          int width, uint32_t flags)
{
    char tmp[36];
    int count = 0;
    int uppercase = flags & 0x08;

    /* Extract digits in reverse order */
    do {
        uint32_t remainder = value % base;
        if (remainder < 10)
            tmp[count++] = '0' + remainder;
        else
            tmp[count++] = (uppercase ? 'A' : 'a') + remainder - 10;
        value /= base;
    } while (value != 0);

    /* Compute padding needed */
    int pad = width - count;
    char pad_char = (flags & 0x01) ? '0' : ' ';

    if (!(flags & 0x02)) {
        /* Right-align: emit padding first */
        while (pad-- > 0)
            output_cb(pad_char, ctx);
    }

    /* Emit digits in correct order (reverse of tmp[]) */
    while (count--)
        output_cb(tmp[count], ctx);

    if (flags & 0x02) {
        /* Left-align: emit trailing padding */
        while (pad-- > 0)
            output_cb(' ', ctx);
    }
}


/* ============================================================
 * vsnprintf_core (0x08003150)
 *
 * Core formatting engine for the printf family.  Processes a
 * format string and calls an output callback for each character.
 * Supports %d, %u, %x, %X, %o, %s, %c, %p, and %%.
 *
 * r0 = output_cb   (void (*)(char ch, void *ctx))
 * r1 = ctx          (opaque context — buffer descriptor or UART)
 * r2 = fmt          (format string)
 * r3 = ap           (va_list pointer)
 *
 * The output_cb / ctx pair abstracts the destination:
 *   - For snprintf: cb writes to a bounded buffer, ctx = {buf, pos, max}
 *   - For sprintf:  cb writes to an unbounded buffer
 *   - For printf:   cb calls uart_putchar()
 *
 * Disassembly (abbreviated — standard newlib _vfprintf_r pattern):
 *   8003150: push  r4-r11, r15
 *   8003152: mov   r4, r0              ; output_cb
 *   8003154: mov   r5, r1              ; ctx
 *   8003156: mov   r6, r2              ; fmt
 *   8003158: mov   r7, r3              ; va_list
 * scan_loop:
 *   800315a: ld.b  r0, (r6, 0x0)       ; ch = *fmt
 *   800315c: addi  r6, 1
 *   800315e: bez   r0, done            ; NUL -> end
 *   8003160: cmpnei r0, '%'
 *   8003162: bt    literal             ; not '%' -> emit literal
 *   ;; --- parse flags, width, specifier ---
 *   8003164: ld.b  r0, (r6, 0x0)       ; specifier
 *   8003166: addi  r6, 1
 *   ;; check for '0' flag, width digits, etc.
 *   ;; dispatch on specifier: 'd','u','x','X','o','s','c','p','%'
 *   ;; calls format_number() for numeric specifiers
 *   ;; ...
 * literal:
 *   ;; output_cb(ch, ctx)
 *   ;; br scan_loop
 * done:
 *   ;; pop r4-r11, r15
 * ============================================================ */
static void vsnprintf_core(void (*output_cb)(char, void *), void *ctx,
                           const char *fmt, va_list ap)
{
    while (*fmt) {
        if (*fmt != '%') {
            output_cb(*fmt++, ctx);
            continue;
        }
        fmt++;  /* skip '%' */

        /* Parse flags */
        uint32_t flags = 0;
        if (*fmt == '0') { flags |= 0x01; fmt++; }   /* zero-pad */
        if (*fmt == '-') { flags |= 0x02; fmt++; }   /* left-align */

        /* Parse width */
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        /* Dispatch on specifier */
        char spec = *fmt++;
        switch (spec) {
        case 'd': {
            int val = va_arg(ap, int);
            if (val < 0) {
                output_cb('-', ctx);
                val = -val;
            }
            format_number(output_cb, ctx, (uint32_t)val, 10, width, flags);
            break;
        }
        case 'u':
            format_number(output_cb, ctx, va_arg(ap, uint32_t), 10, width, flags);
            break;
        case 'x':
            format_number(output_cb, ctx, va_arg(ap, uint32_t), 16, width, flags);
            break;
        case 'X':
            format_number(output_cb, ctx, va_arg(ap, uint32_t), 16, width, flags | 0x08);
            break;
        case 'o':
            format_number(output_cb, ctx, va_arg(ap, uint32_t), 8, width, flags);
            break;
        case 'p':
            output_cb('0', ctx);
            output_cb('x', ctx);
            format_number(output_cb, ctx, (uint32_t)va_arg(ap, void *), 16, width, flags);
            break;
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (s == NULL) s = "(null)";
            while (*s)
                output_cb(*s++, ctx);
            break;
        }
        case 'c':
            output_cb((char)va_arg(ap, int), ctx);
            break;
        case '%':
            output_cb('%', ctx);
            break;
        default:
            output_cb('%', ctx);
            output_cb(spec, ctx);
            break;
        }
    }
}


/* ============================================================
 * Buffer context for snprintf / sprintf
 * ============================================================ */
typedef struct {
    char     *buf;      /* output buffer base */
    uint32_t  pos;      /* current write position */
    uint32_t  max;      /* maximum buffer size (0 = unlimited) */
} buf_ctx_t;

/* Output callback: write character to bounded buffer */
static void buf_output(char ch, void *arg)
{
    buf_ctx_t *ctx = (buf_ctx_t *)arg;
    if (ctx->max == 0 || ctx->pos < ctx->max - 1)
        ctx->buf[ctx->pos] = ch;
    ctx->pos++;
}


/* ============================================================
 * snprintf (0x08003178)
 *
 * Standard snprintf — bounded formatted print to buffer.
 * Wraps vsnprintf_core() with a buffer-based output callback.
 *
 * r0 = buf, r1 = size, r2 = fmt, r3+ = args
 * Returns: number of characters that would have been written
 *          (excluding NUL), even if truncated.
 *
 * Disassembly:
 *   8003178: push  r4-r5, r15
 *   800317a: subi  sp, 12              ; buf_ctx_t on stack
 *   800317c: st.w  r0, (sp, 0x0)       ; ctx.buf = buf
 *   800317e: movi  r4, 0
 *   8003180: st.w  r4, (sp, 0x4)       ; ctx.pos = 0
 *   8003182: st.w  r1, (sp, 0x8)       ; ctx.max = size
 *   8003184: mov   r0, sp              ; r0 = &ctx (becomes 2nd arg)
 *   ;; set up: output_cb = buf_output, ctx, fmt, ap
 *   8003188: bsr   vsnprintf_core      ; 0x08003150
 *   800318c: ld.w  r0, (sp, 0x4)       ; return ctx.pos
 *   ;; NUL-terminate
 *   800318e: ld.w  r1, (sp, 0x0)
 *   8003190: ld.w  r2, (sp, 0x8)
 *   ;; store '\0' at min(pos, max-1)
 *   80031a0: addi  sp, 12
 *   80031a2: pop   r4-r5, r15
 * ============================================================ */
int snprintf(char *buf, uint32_t size, const char *fmt, ...)
{
    buf_ctx_t ctx;
    ctx.buf = buf;
    ctx.pos = 0;
    ctx.max = size;

    va_list ap;
    va_start(ap, fmt);
    vsnprintf_core(buf_output, &ctx, fmt, ap);
    va_end(ap);

    /* NUL-terminate */
    if (size > 0)
        buf[ctx.pos < size ? ctx.pos : size - 1] = '\0';

    return (int)ctx.pos;
}


/* ============================================================
 * sprintf (0x080031A8)
 *
 * Standard sprintf — unbounded formatted print to buffer.
 * Same as snprintf but with max = 0 (no limit check).
 *
 * r0 = buf, r1 = fmt, r2+ = args
 * Returns: number of characters written (excluding NUL)
 *
 * Disassembly:
 *   80031a8: push  r4-r5, r15
 *   80031aa: subi  sp, 12
 *   80031ac: st.w  r0, (sp, 0x0)       ; ctx.buf = buf
 *   80031ae: movi  r4, 0
 *   80031b0: st.w  r4, (sp, 0x4)       ; ctx.pos = 0
 *   80031b2: st.w  r4, (sp, 0x8)       ; ctx.max = 0 (unlimited)
 *   80031b4: mov   r0, sp
 *   80031b8: bsr   vsnprintf_core      ; 0x08003150
 *   80031bc: ld.w  r0, (sp, 0x4)       ; return ctx.pos
 *   80031be: ld.w  r1, (sp, 0x0)
 *   80031c0: add   r1, r0
 *   80031c2: movi  r2, 0
 *   80031c4: st.b  r2, (r1, 0x0)       ; buf[pos] = '\0'
 *   80031c8: addi  sp, 12
 *   80031ca: pop   r4-r5, r15
 * ============================================================ */
int sprintf(char *buf, const char *fmt, ...)
{
    buf_ctx_t ctx;
    ctx.buf = buf;
    ctx.pos = 0;
    ctx.max = 0;   /* unlimited */

    va_list ap;
    va_start(ap, fmt);
    vsnprintf_core(buf_output, &ctx, fmt, ap);
    va_end(ap);

    buf[ctx.pos] = '\0';
    return (int)ctx.pos;
}


/* ============================================================
 * printf (0x080031DC)
 *
 * Standard printf — formatted output to UART0 via uart_putchar.
 * Formats the string into a stack buffer, then outputs each
 * character through uart_putchar() (0x0800717C).
 *
 * r0 = fmt, r1+ = args
 * Returns: number of characters written
 *
 * Disassembly:
 *   80031dc: push  r4-r5, r15
 *   80031de: subi  sp, 256             ; char buf[256] on stack
 *   80031e0: mov   r4, sp              ; r4 = buf
 *   80031e2: mov   r5, r0              ; r5 = fmt
 *   ;; set up buf_ctx_t with max=256, call vsnprintf_core
 *   80031e8: bsr   vsnprintf_core      ; 0x08003150
 *   80031ec: ld.w  r5, (sp, ...)       ; count = ctx.pos
 *   ;; NUL-terminate buf
 *   80031f0: movi  r4, 0               ; index = 0
 * output_loop:
 *   80031f2: cmphs r4, r5
 *   80031f4: bt    done                ; if index >= count, done
 *   80031f6: ld.b  r0, (sp, r4)        ; ch = buf[index]
 *   80031f8: bsr   uart_putchar        ; 0x0800717C
 *   80031fc: addi  r4, 1
 *   80031fe: br    output_loop
 * done:
 *   8003200: mov   r0, r5              ; return count
 *   8003204: addi  sp, 256
 *   8003206: pop   r4-r5, r15
 * ============================================================ */
int printf(const char *fmt, ...)
{
    char buf[256];
    buf_ctx_t ctx;
    ctx.buf = buf;
    ctx.pos = 0;
    ctx.max = sizeof(buf);

    va_list ap;
    va_start(ap, fmt);
    vsnprintf_core(buf_output, &ctx, fmt, ap);
    va_end(ap);

    /* NUL-terminate */
    uint32_t count = ctx.pos;
    if (count >= sizeof(buf))
        count = sizeof(buf) - 1;
    buf[count] = '\0';

    /* Output character by character via UART */
    for (uint32_t i = 0; i < count; i++)
        uart_putchar(buf[i]);

    return (int)count;
}
