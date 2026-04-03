/**
 * secboot_memory.c - Decompiled memory operations
 *
 * Pseudo-C reconstruction from C-SKY XT804 disassembly of secboot.bin
 * Cross-referenced with: platform/common/mem/wm_mem.c
 *
 * Functions:
 *   malloc()  - 0x080029A0  (best-fit allocator)
 *   free()    - 0x08002A3C  (coalescing free-list)
 *   memcpy()  - 0x08002B54  (word-optimized copy)
 *   memcmp()  - 0x08002BD4  (word-optimized compare)
 *   realloc() - 0x08002D20  (resize with copy)
 */

#include <stdint.h>

/* ============================================================
 * Heap layout (from analysis of malloc/free):
 *
 *   Heap start:  0x20011430  (after BSS)
 *   Heap end:    0x20028000  (top of 160KB SRAM)
 *
 * Free list is a singly-linked list of blocks.
 * Each block has an 8-byte header:
 *   [0] next_free  (pointer to next free block, or NULL)
 *   [4] size       (usable size in bytes, not including header)
 *
 * Global state at 0x20010060:
 *   [0] initialized   (0 = not yet, 1 = initialized)
 *   [4] free_list     (pointer to first free block)
 *
 * Lock variable at 0x2001142C (used with lock_acquire/lock_release)
 * ============================================================ */

#define HEAP_START      0x20011430
#define HEAP_END        0x20028000
#define HEAP_STATE      0x20010060  /* { initialized, free_list } */
#define HEAP_LOCK       0x2001142C

#define BLOCK_HEADER_SIZE   8
#define MIN_SPLIT_SIZE      15  /* Don't split if remainder < 15 bytes */
#define ALIGNMENT           8   /* All allocations aligned to 8 bytes */

typedef struct free_block {
    struct free_block *next;    /* Next free block in list */
    uint32_t size;              /* Usable size (excluding 8-byte header) */
} free_block_t;

typedef struct {
    uint32_t    initialized;    /* 0 = not initialized */
    free_block_t *free_list;    /* Head of free list */
} heap_state_t;


/* ============================================================
 * malloc (0x080029A0)
 *
 * Best-fit memory allocator. Walks the free list to find the
 * smallest block that satisfies the request.
 *
 * Algorithm:
 *   1. Acquire heap lock
 *   2. If heap not initialized, create single free block
 *      spanning entire heap (0x20011430 to 0x20028000)
 *   3. Align request size up to 8-byte boundary
 *   4. Walk free list, find best (smallest sufficient) block
 *   5. If block is large enough to split (remainder >= 15),
 *      split and return the first portion
 *   6. Otherwise, remove entire block from free list
 *   7. Release heap lock
 *   8. Return pointer to usable area (header + 8)
 *
 * Returns NULL (0) if no suitable block found.
 *
 * Key disassembly:
 *   80029a0: push  r4-r6
 *   80029a2: mov   r4, r0              ; r4 = requested_size
 *   80029a6: lrw   r0, 0x2001142c      ; Lock address
 *   80029a8: bsr   lock_acquire        ; 0x80071ac
 *   80029b0: lrw   r1, 0x20010060      ; heap_state
 *   80029b2: ld.w  r4, (r1, 0x0)       ; initialized?
 *   80029b4: cmpnei r4, 0
 *   80029b6: bt    skip_init
 *   ; --- Initialize heap ---
 *   80029b8: lrw   r2, 0x20011430      ; HEAP_START
 *   80029ba: st.w  r2, (r1, 0x4)       ; free_list = HEAP_START
 *   80029bc: st.w  r4, (r2, 0x0)       ; block->next = NULL (r4=0)
 *   80029be: lrw   r3, 0x20028000      ; HEAP_END
 *   80029c0: subu  r3, r2              ; size = HEAP_END - HEAP_START
 *   80029c2: subi  r3, 8              ; size -= BLOCK_HEADER_SIZE
 *   80029c4: st.w  r3, (r2, 0x4)       ; block->size = remaining
 *   80029c6: movi  r4, 1
 *   80029c8: st.w  r4, (r1, 0x0)       ; initialized = 1
 *   ; --- Align and search ---
 *   80029ca: addi  r4, r0, 7           ; aligned = (size + 7) & ~7
 *   80029cc: movi  r0, 7
 *   80029ce: andn  r4, r0              ; r4 = aligned size
 *   80029d0: movi  r0, 0              ; best_block = NULL
 *   ; Walk free list looking for best fit...
 *   80029d8: ld.w  r2, (r1, 0x4)       ; block->size
 *   80029da: cmphs r2, r4              ; size >= needed?
 *   ; ... (best-fit selection loop)
 *   ; --- Split or use whole block ---
 *   80029f8: ld.w  r1, (r0, 0x4)       ; block_size
 *   80029fa: subu  r1, r4              ; remainder = block_size - needed
 *   80029fc: cmplti r1, 15             ; remainder < MIN_SPLIT?
 *   8002a00: bt    use_whole           ; Yes -> don't split
 *   ; Split: create new free block at (block + 8 + aligned_size)
 *   8002a02: addi  r2, r4, 8
 *   8002a04: addu  r2, r0              ; new_block = block + 8 + size
 *   8002a06: st.w  r1, (r2, 0x0)       ; new_block->next = old_next
 *   8002a0e: st.w  r1, (r2, 0x4)       ; new_block->size = remainder-8
 *   8002a12: st.w  r4, (r0, 0x4)       ; block->size = aligned_size
 *   8002a18: addi  r0, 8              ; return block + 8
 *   ; --- Release lock and return ---
 *   8002a1e: lrw   r0, 0x2001142c
 *   8002a20: bsr   lock_release
 *   8002a28: pop   r4-r6
 * ============================================================ */
void *malloc(uint32_t size)
{
    heap_state_t *state = (heap_state_t *)HEAP_STATE;

    lock_acquire((uint32_t *)HEAP_LOCK);

    /* Initialize heap on first call */
    if (!state->initialized) {
        free_block_t *initial = (free_block_t *)HEAP_START;
        state->free_list = initial;
        initial->next = NULL;
        initial->size = HEAP_END - HEAP_START - BLOCK_HEADER_SIZE;
        state->initialized = 1;
    }

    /* Align request to 8-byte boundary */
    uint32_t aligned = (size + 7) & ~7;

    /* Best-fit search: find smallest block >= aligned */
    free_block_t *best = NULL;
    uint32_t best_size = 0;
    free_block_t *best_prev = NULL;
    free_block_t *prev = (free_block_t *)&state->free_list;  /* Sentinel */
    free_block_t *curr = state->free_list;

    while (curr != NULL) {
        if (curr->size >= aligned) {
            if (best == NULL || curr->size < best_size) {
                best_size = curr->size;
                best_prev = prev;
                best = curr;
            }
        }
        prev = curr;
        curr = curr->next;
    }

    void *result = NULL;

    if (best != NULL) {
        uint32_t remainder = best->size - aligned;

        if (remainder >= MIN_SPLIT_SIZE) {
            /* Split block: create new free block after allocated portion */
            free_block_t *new_block = (free_block_t *)((uint8_t *)best + BLOCK_HEADER_SIZE + aligned);
            new_block->next = best->next;
            new_block->size = remainder - BLOCK_HEADER_SIZE;
            best_prev->next = new_block;
            best->size = aligned;
        } else {
            /* Use whole block: remove from free list */
            best_prev->next = best->next;
        }

        result = (uint8_t *)best + BLOCK_HEADER_SIZE;
    }

    lock_release((uint32_t *)HEAP_LOCK);
    return result;
}


/* ============================================================
 * free (0x08002A3C)
 *
 * Returns a block to the free list with coalescing.
 * Attempts to merge with adjacent free blocks.
 *
 * Algorithm:
 *   1. Acquire heap lock
 *   2. If ptr is NULL, return immediately
 *   3. Get block header (ptr - 8)
 *   4. Calculate block end (header + size)
 *   5. Walk free list to find insertion point
 *   6. If block end touches next free block, merge forward
 *   7. If previous free block end touches this block, merge backward
 *   8. Otherwise insert into free list at sorted position
 *   9. Release heap lock
 *
 * Key disassembly:
 *   8002a3c: push  r4-r5
 *   8002a3e: mov   r4, r0              ; r4 = ptr
 *   8002a42: lrw   r0, 0x2001142c      ; Lock
 *   8002a44: bsr   lock_acquire
 *   8002a4c: cmpnei r0, 0             ; if (ptr == NULL) goto end
 *   8002a4e: bf    end
 *   8002a50: subi  r2, r0, 8           ; r2 = block_header = ptr - 8
 *   8002a52: ld.w  r3, (r2, 0x4)       ; r3 = block->size
 *   8002a54: addu  r0, r3              ; r0 = block_end = ptr + size
 *   8002a56: lrw   r4, 0x20010064      ; free_list_ptr
 *   ; Walk free list looking for merge opportunities...
 *   8002a5a: ld.w  r3, (r4, 0x0)       ; next_free
 *   8002a5c: cmpne r0, r3             ; block_end == next_free?
 *   ; ... (coalescing logic)
 *   8002aa0: lrw   r0, 0x2001142c
 *   8002aa4: bsr   lock_release
 *   8002aaa: pop   r4-r5
 * ============================================================ */
void free(void *ptr)
{
    if (ptr == NULL) return;

    lock_acquire((uint32_t *)HEAP_LOCK);

    free_block_t *block = (free_block_t *)((uint8_t *)ptr - BLOCK_HEADER_SIZE);
    uint8_t *block_end = (uint8_t *)ptr + block->size;

    /* 0x20010064 stores the "rover" / insertion hint pointer */
    free_block_t **rover = (free_block_t **)0x20010064;
    free_block_t *curr = *rover;

    /* Try to merge with the next adjacent free block */
    if ((uint8_t *)curr == block_end) {
        /* Forward merge: absorb curr into block */
        block->size += curr->size + BLOCK_HEADER_SIZE;
        curr = curr->next;
    }

    if (curr != NULL) {
        uint8_t *curr_end = (uint8_t *)curr + BLOCK_HEADER_SIZE + curr->size;

        if (curr_end == (uint8_t *)block) {
            /* Backward merge: absorb block into curr */
            curr->size += block->size + BLOCK_HEADER_SIZE;
            block->size = 0;
            block->next = curr;
            *rover = curr;
        } else if ((uint8_t *)block < (uint8_t *)curr) {
            /* Insert before curr */
            *rover = block;
            block->next = curr;
        } else {
            /* Walk to find correct sorted position */
            while (curr->next != NULL) {
                free_block_t *next = curr->next;
                /* Check adjacency with next block */
                /* ... coalescing logic continues ... */
                curr = next;
            }
        }
    } else {
        /* Empty free list - this block becomes the only entry */
        block->next = NULL;
        *rover = block;
    }

    lock_release((uint32_t *)HEAP_LOCK);
}


/* ============================================================
 * memcpy (0x08002B54)
 *
 * Optimized memory copy. Uses word-sized copies when both
 * source and destination are aligned to 4-byte boundaries,
 * with 16-byte unrolled loop for large copies.
 *
 * r0 = dest, r1 = src, r2 = length
 * Returns: dest (original r0)
 *
 * Disassembly:
 *   8002b54: push  r4
 *   8002b56: mov   r12, r0             ; save dest for return
 *   8002b58: or    r13, r1, r0         ; check alignment
 *   8002b5c: andi  r4, r13, 3          ; r4 = (src|dest) & 3
 *   8002b60: bez   r4, aligned_copy    ; Both aligned? Use word copy
 *   ; --- Byte-by-byte copy (unaligned) ---
 *   8002b64: bez   r2, done
 * byte_loop:
 *   8002b68: ld.b  r3, (r1, 0x0)       ; Load byte
 *   8002b6c: st.b  r3, (r12, 0x0)      ; Store byte
 *   8002b72: addi  r12, 1
 *   8002b76: bnez  r2, byte_loop
 *   8002b7a: pop   r4
 *   ; --- 16-byte unrolled word copy ---
 * aligned_copy:
 *   8002b7c: cmplti r2, 16             ; Less than 16 bytes?
 *   8002b7e: bt    word_copy           ; Use 4-byte loop
 * fast_loop:
 *   8002b80: ld.w  r13, (r1, 0x0)      ; Load 16 bytes (4 words)
 *   8002b84: ld.w  r3,  (r1, 0x4)
 *   8002b86: ld.w  r4,  (r1, 0x8)
 *   8002b88: st.w  r13, (r12, 0x0)     ; Store 16 bytes
 *   8002b90: st.w  r3,  (r12, 0x4)
 *   8002b94: st.w  r4,  (r12, 0x8)
 *   8002b98: st.w  r13, (r12, 0xc)
 *   8002b9c: subi  r2, 16
 *   8002ba6: bf    fast_loop           ; Loop if >= 16 remaining
 *   ; --- 4-byte word copy ---
 * word_copy:
 *   8002ba8: cmplti r2, 4
 *   8002baa: bt    tail_bytes
 * word_loop:
 *   8002bac: ld.w  r3, (r1, 0x0)
 *   8002bb2: st.w  r3, (r12, 0x0)
 *   ; --- Remaining bytes ---
 * tail_bytes:
 *   8002bbe: bez   r2, done            ; No remainder?
 *   8002bc2: ld.b  r3, (r1, 0x0)       ; Byte copy loop
 *   8002bc8: st.b  r3, (r12, 0x0)
 * ============================================================ */
void *memcpy(void *dest, const void *src, uint32_t n)
{
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    /* Check if both pointers are word-aligned */
    if (((uintptr_t)s | (uintptr_t)d) & 3) {
        /* Unaligned: byte-by-byte copy */
        while (n--) {
            *d++ = *s++;
        }
        return dest;
    }

    /* 16-byte unrolled copy for large blocks */
    uint32_t *wd = (uint32_t *)d;
    const uint32_t *ws = (const uint32_t *)s;
    while (n >= 16) {
        wd[0] = ws[0];
        wd[1] = ws[1];
        wd[2] = ws[2];
        wd[3] = ws[3];
        wd += 4;
        ws += 4;
        n -= 16;
    }

    /* 4-byte copy for medium remainder */
    while (n >= 4) {
        *wd++ = *ws++;
        n -= 4;
    }

    /* Byte copy for tail */
    d = (uint8_t *)wd;
    s = (const uint8_t *)ws;
    while (n--) {
        *d++ = *s++;
    }

    return dest;
}


/* ============================================================
 * memcmp (0x08002BD4)
 *
 * Optimized memory comparison. Uses word-sized compares when
 * both pointers are aligned, with 16-byte unrolled loop.
 *
 * r0 = ptr1, r1 = ptr2, r2 = length
 * Returns: 0 if equal, non-zero if different
 *
 * Disassembly:
 *   8002bd4: push  r4
 *   8002bd6: bez   r2, return_zero     ; length == 0 -> equal
 *   8002bda: mov   r12, r0             ; save ptr1
 *   8002bdc: or    r0, r1              ; check alignment
 *   8002bde: andi  r0, r0, 3
 *   8002be2: bnez  r0, byte_compare    ; Not aligned -> byte compare
 *   ; --- Word-aligned fast compare ---
 *   8002be6: ld.w  r0, (r12, 0x0)      ; Load word from ptr1
 *   8002bea: ld.w  r13, (r1, 0x0)      ; Load word from ptr2
 *   8002bee: cmpne r0, r13             ; Compare
 *   8002bf0: bt    find_diff           ; Not equal -> find byte diff
 *   ; ... (unrolled 4-word compare continues)
 *   ; --- Byte-by-byte compare ---
 * byte_compare:
 *   8002cc8: ld.b  r0, (r12, 0x0)
 *   8002ccc: ld.b  r3, (r1, 0x0)
 *   8002cd0: subu  r0, r3              ; diff = *ptr1 - *ptr2
 *   8002cd2: bnez  r0, return_diff
 *   ; ---
 * return_zero:
 *   8002d1a: movi  r0, 0
 *   8002d1c: pop   r4
 * ============================================================ */
int memcmp(const void *ptr1, const void *ptr2, uint32_t n)
{
    const uint8_t *p1 = (const uint8_t *)ptr1;
    const uint8_t *p2 = (const uint8_t *)ptr2;

    if (n == 0) return 0;

    /* Check alignment */
    if (((uintptr_t)p1 | (uintptr_t)p2) & 3) {
        goto byte_compare;
    }

    /* Word-aligned fast compare with 16-byte unrolling */
    {
        const uint32_t *w1 = (const uint32_t *)p1;
        const uint32_t *w2 = (const uint32_t *)p2;

        while (n >= 16) {
            if (w1[0] != w2[0]) { p1 = (const uint8_t *)&w1[0]; p2 = (const uint8_t *)&w2[0]; n = 4; goto byte_compare; }
            if (w1[1] != w2[1]) { p1 = (const uint8_t *)&w1[1]; p2 = (const uint8_t *)&w2[1]; n = 4; goto byte_compare; }
            if (w1[2] != w2[2]) { p1 = (const uint8_t *)&w1[2]; p2 = (const uint8_t *)&w2[2]; n = 4; goto byte_compare; }
            if (w1[3] != w2[3]) { p1 = (const uint8_t *)&w1[3]; p2 = (const uint8_t *)&w2[3]; n = 4; goto byte_compare; }
            w1 += 4; w2 += 4; n -= 16;
        }

        while (n >= 4) {
            if (*w1 != *w2) { p1 = (const uint8_t *)w1; p2 = (const uint8_t *)w2; n = 4; goto byte_compare; }
            w1++; w2++; n -= 4;
        }

        p1 = (const uint8_t *)w1;
        p2 = (const uint8_t *)w2;
    }

byte_compare:
    while (n--) {
        if (*p1 != *p2) return (int)*p1 - (int)*p2;
        p1++; p2++;
    }
    return 0;
}


/* ============================================================
 * realloc (0x08002D20)
 *
 * Resize an allocated buffer. This is a simplified realloc
 * used for dynamic array/buffer management.
 *
 * r0 = descriptor (struct with: [0] ?, [2] current_cap, [8] buffer_ptr)
 * r1 = new_count (signed, may be negative for shrink hint)
 *
 * Algorithm:
 *   1. Compute new capacity = (count & ~7) + 16  (round up, min 16)
 *   2. Allocate new_cap * 4 bytes via realloc/malloc
 *   3. If new_cap > old_cap, zero-fill the extension
 *   4. Update descriptor's buffer pointer and capacity
 *   5. Return 0 on success, -2 on allocation failure
 *
 * Key disassembly:
 *   8002d20: push  r4-r5, r15
 *   8002d22: bmaski r3, 31            ; r3 = 0x7FFFFFFF
 *   8002d26: addi  r3, 8              ; r3 = 0x80000008 (-8 unsigned)
 *   8002d28: and   r2, r1, r3         ; r2 = count & ~7
 *   8002d2c: mov   r5, r0             ; r5 = descriptor
 *   8002d2e: blz   r2, negative_adj   ; Handle negative counts
 *   8002d32: subu  r2, r1, r2         ; (adjustment)
 *   8002d34: addi  r4, r2, 16         ; new_cap = rounded + 16
 *   8002d38: lsli  r1, r4, 2          ; byte_size = new_cap * 4
 *   8002d3a: ld.w  r0, (r5, 0x8)      ; old buffer
 *   8002d3c: bsr   realloc_internal   ; 0x08002958
 *   8002d40: bez   r0, fail           ; allocation failed?
 *   8002d44: ld.hs r3, (r5, 0x2)      ; old capacity
 *   8002d48: sexth r2, r4             ; new capacity
 *   8002d4a: cmplt r3, r2             ; grew?
 *   8002d4c: st.w  r0, (r5, 0x8)      ; update buffer ptr
 *   8002d4e: st.h  r2, (r5, 0x2)      ; update capacity
 *   8002d50: bf    done               ; No growth -> done
 *   ; Zero-fill new entries
 *   8002d52: lsli  r2, r2, 2          ; new_end = new_cap * 4
 *   8002d54: lsli  r3, r3, 2          ; old_end = old_cap * 4
 *   8002d56: addu  r3, r0             ; start = buf + old_end
 *   8002d58: addu  r0, r2             ; end = buf + new_end
 * zero_loop:
 *   8002d5c: stbi.w r2, (r3)          ; *r3++ = 0
 *   8002d60: cmpne r3, r0
 *   8002d62: bt    zero_loop
 *   8002d64: movi  r0, 0              ; return 0 (success)
 *   8002d66: pop   r4-r5, r15
 * fail:
 *   8002d74: movi  r0, 0
 *   8002d76: subi  r0, 2              ; return -2
 *   8002d78: pop   r4-r5, r15
 * ============================================================ */
int realloc_buffer(void *descriptor, int new_count)
{
    typedef struct {
        uint16_t _field0;
        int16_t  capacity;
        uint32_t _field4;
        void     *buffer;
    } buf_desc_t;

    buf_desc_t *desc = (buf_desc_t *)descriptor;

    /* Round count to 8-byte boundary, add 16 for headroom */
    int rounded = new_count & ~7;
    if (new_count < 0) {
        rounded = ((new_count - 1) | (~8 + 1)) + 1;  /* Negative rounding */
    }
    int new_cap = rounded + 16;

    /* Reallocate buffer (new_cap * 4 bytes) */
    void *new_buf = realloc_internal(desc->buffer, new_cap * 4);
    if (new_buf == NULL) {
        return -2;
    }

    int old_cap = desc->capacity;
    desc->buffer = new_buf;
    desc->capacity = (int16_t)new_cap;

    /* Zero-fill newly allocated entries */
    if (old_cap < new_cap) {
        uint32_t *start = (uint32_t *)new_buf + old_cap;
        uint32_t *end = (uint32_t *)new_buf + new_cap;
        while (start < end) {
            *start++ = 0;
        }
    }

    return 0;
}
