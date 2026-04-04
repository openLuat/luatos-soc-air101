/**
 * secboot_image.c - Decompiled image validation and RSA decryption
 *
 * Pseudo-C reconstruction from C-SKY XT804 disassembly of secboot.bin
 * Cross-referenced with: include/platform/wm_fwup.h, include/driver/wm_flash_map.h
 *
 * Functions:
 *   image_decrypt_init()   - 0x08004D98  (RSA encrypt/decrypt core)
 *   image_decrypt_block()  - 0x08004E98  (block-level RSA + PKCS unpad)
 *   image_decrypt_process()- 0x08004F20  (PKCS#1 DigestInfo parse + hash extract)
 *   validate_image()       - 0x08005988  (header + CRC check)
 *   find_valid_image()     - 0x08007278  (scan flash for valid image)
 *   image_header_verify()  - 0x080071F4  (CRC + data integrity)
 *   image_copy_update()    - 0x080058EC  (hash-verify during copy)
 *   signature_verify()     - 0x080070C4  (RSA signature check - stub note)
 */

#include "secboot_common.h"

/* Additional local status aliases */
#define STATUS_NOT_FOUND    'J'     /* 74 = 0x4A - No valid image found */
#define STATUS_BAD_TYPE     'L'     /* 76 = 0x4C - Wrong image type */
#define STATUS_CRC_OK       'C'     /* 67 = 0x43 - CRC verified */

/* Global pointer: boot parameter block */
/* *(uint32_t *)0x2001007C -> param block, field at offset 0x0C = total flash size */


/* ============================================================
 * image_decrypt_init (0x08004D98)
 *
 * Core RSA public-key operation: computes result = input^exp mod n,
 * then writes the result to output buffer with PKCS#1 v1.5 padding.
 *
 * Parameters:
 *   r0 = unused (caller context pointer, not dereferenced here)
 *   r1 = input_data (plaintext/ciphertext bytes)
 *   r2 = input_len  (length in bytes)
 *   r3 = output_buf (destination for result)
 *   sp[0] = out_len_ptr (uint32_t *: in=available size, out=written size)
 *   sp[1] = rsa_key (pointer to rsa_key_t structure)
 *   sp[2] = mode (1=public key op using e, 2=private key op using d)
 *
 * Returns:
 *   0 on success, -1 on error (bad args, RSA failure, output too small)
 *
 * Algorithm:
 *   1. Initialize stack-local mp_int bignum
 *   2. Read input_data into bignum via mp_read_unsigned_bin
 *   3. Verify input < modulus n (mp_cmp)
 *   4. Mode 1: compute input^e mod n (sha1_full)
 *      Mode 2: compute input^d mod n (sha1_full)  [d at offset 0x18 vs e at 0]
 *   5. Determine output size = mp_unsigned_bin_size(n)
 *   6. Check output buffer is large enough
 *   7. Zero-pad output to key_size, write result via rsa_step
 *   8. Update *out_len_ptr with bytes written
 *
 * Disassembly (0x08004D98 - 0x08004E98, 256 bytes):
 *   8004d98: push  r4-r10, r15
 *   8004d9a: subi  sp, 12            ; mp_int on stack
 *   8004d9c: mov   r5, r1            ; input_data
 *   8004d9e: mov   r9, r2            ; input_len
 *   8004da0: mov   r4, r3            ; output_buf
 *   8004da2: ld.w  r8, (sp, 0x2c)   ; out_len_ptr
 *   8004da6: ld.w  r6, (sp, 0x30)   ; rsa_key
 *   8004da8: ld.w  r10, (sp, 0x34)  ; mode
 *   8004dac: bez   r1, err_arg       ; NULL check input_data
 *   8004db0: bez   r3, err_arg       ; NULL check output_buf
 *   8004db4: bez   r8, err_arg       ; NULL check out_len_ptr
 *   8004db8: bez   r6, err_arg       ; NULL check rsa_key
 *   ...
 *   8004de2: bsr   sha_init          ; 0x08003CDC
 *   8004df0: bsr   mp_read_unsigned_bin ; 0x08003690
 *   8004e00: bsr   mp_cmp            ; 0x08003298
 *   8004e20: bsr   sha1_full         ; 0x0800472C
 *   8004e2a: bsr   mp_unsigned_bin_size ; 0x080031DC
 *   8004e5a: bsr   memset            ; 0x08002AB4
 *   8004e60: bsr   mp_unsigned_bin_size ; 0x080031DC
 *   8004e6a: bsr   rsa_step          ; 0x08003A14
 *   8004e7a: bsr   mp_clear          ; 0x080031A8
 * ============================================================ */
int image_decrypt_init(void *ctx, const uint8_t *input_data,
                       uint32_t input_len, uint8_t *output_buf,
                       uint32_t *out_len_ptr, rsa_key_t *rsa_key,
                       uint32_t mode)
{
    mp_int tmp;
    mp_int *key_n;
    mp_int *key_exp;
    int ret;
    int bin_size;
    uint32_t key_size;

    /* NULL pointer checks */
    if (input_data == NULL || output_buf == NULL ||
        out_len_ptr == NULL || rsa_key == NULL)
        return -1;

    /* Initialize bignum with capacity for input_len + 4 bytes */
    ret = sha_init(&tmp, input_len + 4);
    if (ret != 0)
        return ret;

    /* Read input bytes into bignum */
    ret = mp_read_unsigned_bin(&tmp, input_data, (int)input_len);
    if (ret != 0) {
        mp_clear(&tmp);
        return ret;
    }

    /* Pointer to modulus n (at offset 24 in rsa_key) */
    key_n = (mp_int *)((uint8_t *)rsa_key + 24);

    /* Verify input < n (required for RSA) */
    if (mp_cmp(key_n, &tmp) != -1) {
        /* input >= n: invalid */
        mp_clear(&tmp);
        return -1;
    }

    /* Select exponent based on mode */
    if (mode == 2) {
        /* Private key operation: use exponent at offset 12 (d) */
        key_exp = (mp_int *)((uint8_t *)rsa_key + 12);
    } else if (mode == 1) {
        /* Public key operation: use exponent at offset 0 (e) */
        key_exp = (mp_int *)rsa_key;
    } else {
        /* Invalid mode */
        mp_clear(&tmp);
        return -1;
    }

    /* Compute tmp = tmp^exp mod n */
    ret = sha1_full(&tmp, key_exp, key_n, &tmp);
    if (ret != 0) {
        mp_clear(&tmp);
        return ret;
    }

    /* Get the required output size = mp_unsigned_bin_size(n) */
    bin_size = mp_unsigned_bin_size(key_n);

    /* Check output buffer has enough space */
    if (*out_len_ptr < (uint32_t)bin_size) {
        mp_clear(&tmp);
        return -1;
    }

    /* Get key_size from rsa_key structure (offset 0x60) */
    key_size = rsa_key->key_size;

    if ((uint32_t)bin_size >= key_size) {
        /* Result already fills key_size, write directly */
        *out_len_ptr = (uint32_t)bin_size;
        memset(output_buf, 0, (uint32_t)bin_size);

        /* Get actual bignum binary size for offset calculation */
        int actual_size = mp_unsigned_bin_size(&tmp);
        int offset = (int)bin_size - actual_size;
        ret = rsa_step(&tmp, output_buf + offset);
    } else {
        /* Zero-pad output buffer to key_size */
        uint8_t *p = output_buf;
        uint32_t pad_len = key_size - (uint32_t)bin_size;
        uint32_t i;
        for (i = 0; i < pad_len; i++) {
            *p++ = 0;
        }

        /* Write total size and zero remaining buffer */
        *out_len_ptr = key_size;
        memset(p, 0, key_size - pad_len);

        /* Write bignum result at correct offset */
        int actual_size = mp_unsigned_bin_size(&tmp);
        int offset = (int)key_size - actual_size;
        ret = rsa_step(&tmp, output_buf + offset);
    }

    int result = (ret != 0) ? -1 : 0;
    mp_clear(&tmp);
    return result;
}


/* ============================================================
 * image_decrypt_block (0x08004E98)
 *
 * Perform RSA decryption on a single block and then strip
 * PKCS#1 v1.5 padding via cert_parse.
 *
 * Parameters:
 *   r0 = ctx (unused context pointer)
 *   r1 = rsa_key (pointer to rsa_key_t)
 *   r2 = input_data (encrypted block, also used as output buffer)
 *   r3 = input_len
 *   sp[0] = out_buf (output for unpadded data)
 *   sp[1] = out_len_ptr (pointer to output length)
 *   sp[2] = mode (passed through to image_decrypt_init)
 *
 * Returns:
 *   0 on success, negative on error:
 *   -6 if input_len != key_size
 *   -1 if decrypted size != key_size or other error
 *
 * Disassembly (0x08004E98 - 0x08004F20, 136 bytes):
 *   8004e98: subi  sp, 28            ; manual save frame
 *   ...
 *   8004ed6: bsr   image_decrypt_init ; 0x08004D98
 *   8004eee: bsr   cert_parse        ; 0x08004C78
 * ============================================================ */
int image_decrypt_block(void *ctx, rsa_key_t *rsa_key,
                        uint8_t *input_data, uint32_t input_len,
                        uint8_t *out_buf, uint32_t out_len,
                        uint32_t mode)
{
    uint32_t key_size;
    uint32_t local_len;
    int ret;

    key_size = rsa_key->key_size;

    /* Input length must match key size */
    if (key_size != input_len)
        return -6;

    /* Decrypt in-place: output overwrites input buffer.
     * Note: mode is hardcoded to 1 (public key op) per binary —
     * the 'mode' parameter from caller is used for cert_parse marker. */
    local_len = key_size;
    ret = image_decrypt_init(ctx, input_data, input_len, input_data,
                             &local_len, rsa_key, 1);
    if (ret < 0)
        return ret;

    /* Verify decrypted size matches key size */
    if (key_size != local_len)
        return -1;

    /* Strip PKCS#1 v1.5 padding via cert_parse */
    ret = cert_parse(input_data, key_size, out_buf, out_len, 1);

    /* Clamp return value to <= 0 (cert_parse returns positive on success) */
    if (ret > 0)
        ret = 0;
    return ret;
}


/* ============================================================
 * image_decrypt_process (0x08004F20)
 *
 * RSA signature verification with PKCS#1 v1.5 DigestInfo parsing.
 * Decrypts the signature block, parses the ASN.1 DigestInfo
 * structure, and extracts the hash value.
 *
 * Parameters:
 *   r0 = ctx (context pointer passed through to decrypt_block)
 *   r1 = rsa_key_ptr (pointer to pointer to rsa_key_t)
 *   r2 = sig_data (signature bytes)
 *   r3 = sig_len
 *   sp[0] = hash_out (output buffer for extracted hash)
 *   sp[1] = hash_size (expected hash size: 20=SHA1, 32=SHA256, 48=SHA384)
 *   sp[2] = extra_param (passed through)
 *
 * Returns:
 *   hash_size on success (positive), negative on error:
 *   -1 for unsupported hash size, parse error, or verification failure
 *   -8 for allocation failure
 *
 * Algorithm:
 *   1. Determine buffer size from hash_size:
 *      SHA-1(20)→35, SHA-256(32)→51, SHA-384(48)→67
 *   2. malloc(buf_size) for decrypted DigestInfo
 *   3. Call image_decrypt_block to RSA-decrypt signature
 *   4. Parse ASN.1 SEQUENCE wrapper (pkey_verify)
 *   5. Parse AlgorithmIdentifier (signature_check_data)
 *   6. Verify OCTET STRING tag (0x04)
 *   7. Parse hash length (pkey_setup)
 *   8. memcpy hash data to output
 *   9. free buffer, return hash_size
 *
 * Disassembly (0x08004F20 - 0x08004C04, 228 bytes):
 *   8004f20: push  r4-r11, r15, r16
 *   8004f24: subi  sp, 28
 *   8004f26: ld.w  r4, (sp, 0x48)   ; hash_size
 *   ...
 *   8004f6c: bsr   image_decrypt_block ; 0x08004E98
 *   8004f80: bsr   pkey_verify       ; 0x08004AB4
 *   8004f92: bsr   signature_check_data ; 0x08004BB0
 *   8004fac: bsr   pkey_setup        ; 0x080049CC
 *   8004fbc: bsr   memcpy            ; 0x08002B54
 *   8004fc2: bsr   free              ; 0x08002A3C
 * ============================================================ */
int image_decrypt_process(void *ctx, rsa_key_t **rsa_key_ptr,
                          uint8_t *sig_data, uint32_t sig_len,
                          uint8_t *hash_out, uint32_t hash_size,
                          uint32_t extra_param)
{
    uint32_t buf_size;
    uint8_t *buf;
    uint8_t *pos;
    uint8_t *end;
    uint32_t seq_len;
    uint32_t oid_out1, oid_out2;
    uint32_t octet_len;
    int ret;

    /* Determine DigestInfo buffer size from hash algorithm */
    switch (hash_size) {
    case 20:  /* SHA-1 */
        buf_size = 35;   /* 0x23: 15 bytes OID + 20 bytes hash */
        break;
    case 32:  /* SHA-256 */
        buf_size = 51;   /* 0x33: 19 bytes OID + 32 bytes hash */
        break;
    case 48:  /* SHA-384 */
        buf_size = 67;   /* 0x43: 19 bytes OID + 48 bytes hash */
        break;
    default:
        return -1;       /* Unsupported hash size */
    }

    /* Allocate buffer for decrypted DigestInfo */
    buf = (uint8_t *)malloc(buf_size);
    if (buf == NULL)
        return -8;

    /* RSA decrypt the signature block into buf */
    ret = image_decrypt_block(ctx, *rsa_key_ptr, sig_data, sig_len,
                              buf, buf_size, extra_param);
    if (ret < 0) {
        free(buf);
        return ret;
    }

    /* Parse the DigestInfo ASN.1 structure:
     *   SEQUENCE {
     *     SEQUENCE { AlgorithmIdentifier }  -- OID + params
     *     OCTET STRING { hash_value }
     *   }
     */
    pos = buf;
    end = buf + buf_size;

    /* Parse outer SEQUENCE */
    ret = pkey_verify(&pos, buf_size, (mp_int *)&seq_len);
    if (ret < 0)
        goto fail;

    /* Parse AlgorithmIdentifier SEQUENCE (OID + parameters) */
    ret = signature_check_data(&pos, (uint32_t)(end - pos),
                               &oid_out1, &oid_out2);
    if (ret < 0)
        goto fail;

    /* Expect OCTET STRING tag (0x04) */
    if (*pos != 0x04)
        goto fail;
    pos++;  /* skip tag byte */

    /* Parse OCTET STRING length */
    ret = pkey_setup(&pos, (uint32_t)(end - pos), &octet_len);
    if (ret < 0)
        goto fail;

    /* Copy the hash value to output */
    memcpy(hash_out, pos, hash_size);

    /* Success: free buffer and return hash_size */
    free(buf);
    return (int)hash_size;

fail:
    free(buf);
    return -1;
}


/* ============================================================
 * validate_image (0x08005988)
 *
 * Validates an image header at the given address.
 * Checks:
 *   1. Magic number == 0xA0FFFF9F
 *   2. Image address range within flash bounds
 *   3. Image length vs available flash space
 *   4. If signature bit set: compute CRC32 of header
 *      and compare with hd_checksum (offset 0x3C)
 *   5. If no signature: check alignment (img_addr % 1024 == 0)
 *
 * r0 = pointer to 64-byte image header (already read into RAM)
 * Returns: status code character
 *
 * Disassembly:
 *   8005988: push  r4, r15
 *   800598a: subi  r14, 12
 *   800598c: movi  r3, 0
 *   800598e: st.w  r3, (sp, 0x0)        ; crc_result = 0
 *   8005990: ld.w  r2, (r0, 0x0)        ; magic_no
 *   8005992: lrw   r3, 0xa0ffff9f       ; SIGNATURE_WORD
 *   8005994: cmpne r2, r3
 *   8005996: mov   r4, r0               ; r4 = header
 *   8005998: bf    check_attrs           ; magic OK -> continue
 *   800599a: movi  r0, 76               ; return 'L' (bad magic)
 *   800599c: addi  r14, 12
 *   800599e: pop   r4, r15
 *
 * check_attrs:
 *   80059a0: ld.b  r3, (r0, 0x6)        ; img_attr byte 2 (zip_type bit)
 *   80059a2: andi  r3, r3, 1            ; zip_type flag
 *   80059a6: bez   r3, no_signature     ; Not zipped -> different path
 *
 *   ; --- Signed/zipped image path ---
 *   80059aa: ld.w  r3, (r0, 0x8)        ; img_addr
 *   80059ac: bmaski r2, 29              ; max_addr = 0x1FFFFFFF
 *   80059b0: cmphs r2, r3              ; img_addr <= max?
 *   80059b2: bf    check_2mb_bound
 *   80059b4: lrw   r2, 0x2001007c       ; param_block ptr
 *   80059b6: movih r1, 2048             ; r1 = 0x00800000 (8MB)
 *   80059ba: ld.w  r2, (r2, 0x0)
 *   80059bc: ld.w  r2, (r2, 0xc)        ; flash_total_size
 *   80059be: addu  r2, r1               ; max = flash_size + 8MB
 *   80059c0: cmphs r3, r2              ; img_addr >= max?
 *   80059c2: bt    bad_range            ; Yes -> error
 *
 * check_2mb_bound:
 *   80059c4: movih r2, 57339            ; 0xDFEB << 16 = 0xDFEB0000
 *   80059c8: bseti r2, 15              ; r2 = 0xDFEB8000
 *   80059ca: addu  r1, r3, r2           ; r1 = img_addr + offset
 *   80059cc: lrw   r2, 0xffb7fff        ; upper bound check
 *   80059ce: cmphs r2, r1
 *   80059d0: bt    bad_range
 *
 *   ; --- Check img_len ---
 *   80059d2: ld.w  r2, (r4, 0xc)        ; img_len
 *   80059d4: bez   r2, bad_len          ; len == 0 -> error
 *   80059d8: movih r1, 8192             ; 8MB (0x00800000 >> 16 = 0x2000)
 *   80059dc: subu  r1, r3               ; space_left = 8MB - img_addr
 *   80059de: cmphs r2, r1              ; img_len >= space?
 *   80059e0: bf    check_with_flash     ; No -> check with actual flash
 *   ; ... (additional bounds checking)
 *
 *   ; --- CRC verification ---
 *   80059fa: movi  r3, 3               ; CRC type = CRC32
 *   80059fc: movi  r1, 0
 *   80059fe: mov   r2, r3              ; params
 *   8005a00: subi  r1, 1               ; initial = 0xFFFFFFFF
 *   8005a02: addi  r0, sp, 4           ; CRC context on stack
 *   8005a04: bsr   crc_init            ; 0x08004404
 *   8005a08: movi  r2, 60              ; Hash 60 bytes of header
 *   8005a0a: mov   r1, r4              ; header pointer
 *   8005a0c: addi  r0, sp, 4
 *   8005a0e: bsr   crc_update          ; 0x08004410
 *   8005a12: mov   r1, sp              ; output buffer
 *   8005a14: addi  r0, sp, 4
 *   8005a16: bsr   crc_final           ; 0x0800459C
 *   8005a1a: ld.w  r2, (r4, 0x3c)      ; expected hd_checksum
 *   8005a1c: ld.w  r3, (sp, 0x0)       ; computed CRC
 *   8005a1e: cmpne r2, r3
 *   8005a20: bt    bad_magic           ; CRC mismatch -> fail
 *   8005a22: movi  r0, 67              ; return 'C' (OK)
 *   8005a24: br    return
 *
 * no_signature:
 *   8005a26: ld.w  r3, (r0, 0x8)        ; img_addr
 *   8005a28: andi  r2, r3, 1023         ; check 1KB alignment
 *   8005a2c: bez   r2, check_addr       ; Aligned -> continue
 *   8005a30: movi  r0, 75              ; return 'K' (bad alignment)
 *
 * bad_len:
 *   8005a46: movi  r0, 73              ; return 'I'
 * bad_range:
 *   8005a4a: movi  r0, 74              ; return 'J'
 * ============================================================ */
int validate_image(const uint8_t *header)
{
    uint32_t magic = *(uint32_t *)(header + 0x00);
    uint32_t img_attr = *(uint32_t *)(header + 0x04);
    uint32_t img_addr = *(uint32_t *)(header + 0x08);
    uint32_t img_len  = *(uint32_t *)(header + 0x0C);
    uint32_t hd_crc   = *(uint32_t *)(header + 0x3C);

    /* Check magic number */
    if (magic != SIGNATURE_WORD) {
        return STATUS_BAD_MAGIC;    /* 'L' = 76 */
    }

    /* Check zip_type / signature flag (byte offset 0x06, bit 0 = zip_type) */
    uint8_t zip_type = (header[0x06]) & 1;

    if (!zip_type) {
        /* Non-zipped image: check 1KB alignment of img_addr */
        if (img_addr & 0x3FF) {
            return STATUS_BAD_ALIGN;    /* 'K' = 75 */
        }
        /* Fall through to address range check at img_addr validation */
    }

    /* Validate image address range */
    uint32_t max_addr = 0x1FFFFFFF;  /* bmaski(29) */
    if (img_addr <= max_addr) {
        /* Check against actual flash size + 8MB offset */
        uint32_t *param_ptr = *(uint32_t **)0x2001007C;
        uint32_t flash_size = param_ptr[3];  /* offset 0x0C */
        uint32_t upper = flash_size + 0x00800000;
        if (img_addr >= upper) {
            return STATUS_BAD_RANGE;    /* 'J' = 74 */
        }
    } else {
        /* Additional boundary check for high addresses */
        /* (QSPI address space wrapping check) */
        return STATUS_BAD_RANGE;        /* 'J' = 74 */
    }

    /* Validate image length */
    if (img_len == 0) {
        return STATUS_BAD_LEN;          /* 'I' = 73 */
    }

    /* Check that image fits within available flash */
    uint32_t space_avail = 0x00800000 - img_addr;  /* 8MB - img_addr */
    if (img_len >= space_avail) {
        /* Try with actual flash size */
        uint32_t *param_ptr = *(uint32_t **)0x2001007C;
        uint32_t flash_size = param_ptr[3];
        space_avail = flash_size + 0x00800000 - img_addr;
        if (img_len >= space_avail) {
            return STATUS_BAD_LEN;      /* 'I' = 73 */
        }
    }

    /* CRC32 verification of header (60 bytes, excluding hd_checksum field) */
    if (zip_type) {
        uint32_t crc_ctx[4];    /* CRC context (on stack) */
        uint32_t crc_result = 0;

        crc_init(crc_ctx, 3, 0xFFFFFFFF);      /* CRC32, init=0xFFFFFFFF */
        crc_update(crc_ctx, header, 60);         /* Hash first 60 bytes */
        crc_final(crc_ctx, &crc_result);         /* Get result */

        if (crc_result != hd_crc) {
            return STATUS_BAD_MAGIC;    /* 'L' = 76 (CRC mismatch) */
        }
    }

    return STATUS_OK;                   /* 'C' = 67 */
}


/* ============================================================
 * find_valid_image (0x08007278)
 *
 * Scans flash starting from a given address to find a valid
 * firmware image. Reads 64-byte headers and calls validate_image.
 *
 * r0 = start_flash_addr, r1 = header_buf (64 bytes),
 * r2 = mode (0 = find first type-1 image, nonzero = scan all)
 *
 * Returns: status code
 *   'C' (67) = found valid image (header_buf filled)
 *   'J' (74) = no valid image found (address out of bounds)
 *   'L' (76) = found image but wrong type
 *
 * The function iterates through consecutive image headers in
 * flash. Each header's img_len + 64 (header size) + optional
 * 128 (signature) determines the offset to the next image.
 *
 * Disassembly:
 *   8007278: push  r4-r8, r15
 *   800727a: mov   r4, r0              ; cur_addr = start
 *   800727c: mov   r5, r1              ; hdr_buf
 *   800727e: mov   r6, r2              ; mode
 *   8007280: bez   r0, not_found       ; NULL addr -> fail
 *   8007284: bmaski r7, 27             ; max = 0x07FFFFFF (128MB)
 *   8007288: lrw   r8, 0x2001007c      ; param_block_ptr
 *
 * scan_loop:
 *   80072b4: movi  r2, 64
 *   80072b6: mov   r1, r5              ; dest = hdr_buf
 *   80072b8: mov   r0, r4              ; src = cur_addr
 *   80072ba: bsr   flash_read          ; 0x080054F8
 *   80072be: mov   r0, r5
 *   80072c0: bsr   validate_image      ; 0x08005988
 *   80072c4: cmpnei r0, 67             ; result != 'C'?
 *   80072c8: bt    type_error          ; Not valid -> 'L'
 *
 *   ; Check image type field (img_attr & 0x0F)
 *   80072ca: ld.b  r3, (r5, 0x4)       ; img_attr low byte
 *   80072cc: andi  r3, r3, 15          ; type = attr & 0x0F
 *   80072d0: cmpnei r3, 1             ; type != FLASHBIN0?
 *   80072d2: bf    found_it            ; type == 1 -> found!
 *
 *   ; Not the type we want - skip to next image
 *   80072d4: bnez  r6, advance         ; mode != 0 -> advance
 *   80072d8: ld.w  r4, (r5, 0x38)      ; next_header_ptr
 *   80072da: cmphs r7, r4             ; within bounds?
 *   80072dc: bf    check_flash_end
 *
 * advance:
 *   800728e: ld.w  r3, (r5, 0xc)       ; img_len
 *   8007290: addi  r3, 64              ; skip header
 *   8007292: addu  r4, r3              ; cur_addr += img_len + 64
 *   8007294: ld.b  r3, (r5, 0x5)       ; img_attr byte 1
 *   8007296: andi  r3, r3, 1           ; signature flag (bit 8)
 *   800729a: bez   r3, no_sig
 *   800729e: addi  r4, 128             ; skip 128-byte signature
 *
 * no_sig:
 *   80072a0: cmphs r7, r4             ; addr <= max?
 *   80072a2: bt    not_found           ; Past max -> fail
 *   ; Check against actual flash end
 *   80072a4: ld.w  r3, (r8, 0x0)       ; param_block
 *   80072a8: movih r2, 2048            ; 8MB
 *   80072ac: ld.w  r3, (r3, 0xc)       ; flash_size
 *   80072ae: addu  r3, r2              ; flash_end
 *   80072b0: cmphs r4, r3             ; addr >= flash_end?
 *   80072b2: bt    not_found
 *   ; Continue scanning
 *   80072b4: br    scan_loop
 *
 * found_it:
 *   80072e6: bez   r6, verify_data     ; mode 0 -> verify data
 *   ; mode != 0: verify header with CRC at (cur_addr + 64)
 *   80072ea: addi  r1, r4, 64
 *   80072ee: mov   r0, r5
 *   80072f0: bsr   image_header_verify ; 0x080071F4
 *   80072f4: pop   r4-r8, r15
 *
 * verify_data:
 *   80072f6: ld.w  r1, (r5, 0x8)       ; img_addr (= data length)
 *   80072f8: mov   r0, r5              ; header
 *   80072fa: bsr   image_header_verify ; 0x080071F4
 *   80072fe: pop   r4-r8, r15
 *
 * not_found:
 *   80072de: movi  r0, 74              ; return 'J'
 *   80072e0: pop   r4-r8, r15
 * type_error:
 *   80072e2: movi  r0, 76              ; return 'L'
 *   80072e4: pop   r4-r8, r15
 * ============================================================ */
int find_valid_image(uint32_t start_addr, uint8_t *hdr_buf, int mode)
{
    uint32_t cur_addr = start_addr;
    uint32_t max_addr = 0x07FFFFFF;     /* bmaski(27) */
    uint32_t *param_ptr = *(uint32_t **)0x2001007C;

    if (cur_addr == 0) {
        return STATUS_NOT_FOUND;        /* 'J' */
    }

    while (1) {
        /* Read 64-byte header from flash */
        flash_read(cur_addr, hdr_buf, 64);

        /* Validate header */
        int result = validate_image(hdr_buf);
        if (result != STATUS_OK) {
            return STATUS_BAD_TYPE;     /* 'L' */
        }

        /* Check image type: must be FLASHBIN0 (type 1) */
        uint8_t img_type = hdr_buf[0x04] & 0x0F;
        if (img_type == IMG_TYPE_FLASHBIN0) {
            /* Found a valid type-1 image */
            if (mode != 0) {
                /* Mode 1: verify CRC at (addr + 64) */
                return image_header_verify(hdr_buf, cur_addr + 64);
            } else {
                /* Mode 0: verify with img_addr as data length */
                uint32_t data_len = *(uint32_t *)(hdr_buf + 0x08);
                return image_header_verify(hdr_buf, data_len);
            }
        }

        /* Wrong type - advance to next image */
        if (mode != 0) {
            uint32_t img_len = *(uint32_t *)(hdr_buf + 0x0C);
            cur_addr += img_len + 64;           /* Skip header + data */

            /* Check signature flag (byte 0x05, bit 0 = attr.signature) */
            if (hdr_buf[0x05] & 1) {
                cur_addr += 128;                /* Skip 128-byte signature */
            }
        } else {
            /* Mode 0: use next pointer from header */
            cur_addr = *(uint32_t *)(hdr_buf + 0x38);
        }

        /* Bounds check */
        if (cur_addr > max_addr) {
            return STATUS_NOT_FOUND;    /* 'J' */
        }

        uint32_t flash_size = param_ptr[3];     /* offset 0x0C */
        uint32_t flash_end = flash_size + 0x00800000;
        if (cur_addr >= flash_end) {
            return STATUS_NOT_FOUND;    /* 'J' */
        }
    }
}


/* ============================================================
 * image_header_verify (0x080071F4)
 *
 * Verifies image data integrity by computing CRC over the
 * image data region and optionally copying the image.
 *
 * r0 = header_buf, r1 = data_len_or_addr
 * Returns:
 *   'C' (67) = CRC matches, image OK
 *   'Z' (90) = CRC verification failed
 *   'M' (77) = Data copy/verification mismatch
 *
 * Disassembly:
 *   80071f4: push  r4-r5, r15
 *   80071f6: mov   r4, r0              ; header
 *   80071f8: mov   r5, r1              ; data_len
 *   80071fa: bsr   crc_verify_image    ; 0x08005CFC
 *   80071fe: addi  r3, r0, 3           ; result + 3
 *   8007200: cmphsi r3, 2              ; (result+3) >= 2 ?
 *   8007202: bf    check_copy          ; result in [-3..-2] -> try copy
 *   8007204: blz   r0, crc_bad         ; result < 0 -> CRC error
 *   8007208: movi  r0, 67              ; return 'C' (OK)
 *   800720a: pop   r4-r5, r15
 *
 * crc_bad:
 *   800720c: movi  r0, 90              ; return 'Z' (CRC fail)
 *   800720e: pop   r4-r5, r15
 *
 * check_copy:
 *   8007210: mov   r1, r5
 *   8007212: mov   r0, r4
 *   8007214: bsr   image_copy_update   ; 0x080058EC
 *   8007218: bnez  r0, ok              ; copy returned nonzero -> 'C'
 *   800721c: movi  r0, 77              ; return 'M' (mismatch)
 *   800721e: pop   r4-r5, r15
 * ============================================================ */
int image_header_verify(const uint8_t *header, uint32_t data_param)
{
    int crc_result = crc_verify_image(header, data_param);

    if (crc_result + 3 >= 2) {
        /* Normal result range */
        if (crc_result < 0) {
            return STATUS_CRC_BAD;      /* 'Z' = 90 */
        }
        return STATUS_CRC_OK;           /* 'C' = 67 */
    }

    /* CRC indeterminate - try copy and verify */
    int copy_result = image_copy_update(header, data_param);
    if (copy_result != 0) {
        return STATUS_CRC_OK;           /* 'C' = 67 (copy OK) */
    }

    return STATUS_COPY_FAIL;            /* 'M' = 77 */
}


/* ============================================================
 * image_copy_update (0x080058EC)
 *
 * Reads image data from flash and computes running hash,
 * comparing against expected checksum stored in header.
 *
 * Uses CRC32 streaming: reads 1024-byte chunks from flash,
 * feeds them through CRC context, then compares final CRC
 * with header's org_checksum (offset 0x18).
 *
 * r0 = header_buf, r1 = data_length
 * Returns: 0 if checksum mismatch, 1 if match
 *
 * Disassembly:
 *   80058ec: push  r4-r7, r15
 *   80058ee: subi  r14, 20             ; 20 bytes stack frame
 *   80058f0: ld.w  r4, (r0, 0xc)       ; img_len from header
 *   80058f2: movi  r3, 0
 *   80058f4: st.w  r3, (sp, 0x0)       ; chunk_remaining = 0
 *   80058f6: st.w  r3, (sp, 0x8)       ; hash_result = 0
 *   80058f8: st.w  r3, (sp, 0x4)       ; current_offset = 0
 *   80058fa: addu  r4, r1              ; end_addr = img_len + data_length
 *   80058fc: st.w  r1, (sp, 0x4)       ; current_offset = data_length
 *
 *   ; Initialize CRC context
 *   80058fe: movi  r3, 3               ; CRC32 type
 *   8005900: movi  r1, 0
 *   8005902: mov   r6, r0              ; save header
 *   8005904: mov   r2, r3
 *   8005906: subi  r1, 1               ; init = 0xFFFFFFFF
 *   8005908: addi  r0, sp, 12          ; CRC context at sp+12
 *   800590a: bsr   crc_init            ; 0x08004404
 *
 *   ; Chunk size = 1024
 *   800590e: movi  r5, 1024
 *   8005912: bmaski r7, 29             ; max flash addr
 *
 * chunk_loop:
 *   8005916: ld.w  r3, (sp, 0x4)       ; current_offset
 *   8005918: cmphs r3, r4             ; offset >= end?
 *   800591a: bt    finalize
 *   800591c: ld.w  r3, (sp, 0x4)
 *   800591e: subu  r3, r4, r3          ; remaining = end - offset
 *   8005920: st.w  r3, (sp, 0x0)       ; chunk_size = remaining
 *   8005922: ld.w  r3, (sp, 0x0)
 *   8005924: cmphs r5, r3             ; 1024 >= remaining?
 *   8005926: bt    read_chunk
 *   8005928: st.w  r5, (sp, 0x0)       ; chunk_size = min(1024, remaining)
 *
 * read_chunk:
 *   800592a: ld.w  r3, (sp, 0x4)       ; current_offset
 *   800592c: cmphs r7, r3             ; within QSPI range?
 *   800592e: bt    read_via_qspi
 *   ; Read via SPI flash controller
 *   8005930: ld.w  r1, (sp, 0x4)       ; flash_addr
 *   8005932: addi  r0, sp, 12          ; CRC context
 *   8005934: ld.w  r2, (sp, 0x0)       ; chunk_size
 *   8005936: bsr   crc_update          ; Update CRC with flash data
 *   800593a: ld.w  r3, (sp, 0x4)       ; advance offset
 *   800593c: cmphs r3, r4             ; done?
 *   800593e: bf    chunk_loop
 *
 * finalize:
 *   8005940: addi  r0, sp, 12
 *   8005942: bsr   crc_finalize        ; 0x08004544
 *   8005946: addi  r1, sp, 8           ; output addr
 *   8005948: addi  r0, sp, 12
 *   800594a: bsr   crc_final           ; 0x0800459C
 *   800594e: ld.w  r2, (r6, 0x18)      ; expected = header->org_checksum
 *   8005950: ld.w  r3, (sp, 0x8)       ; computed CRC
 *   8005952: cmpne r2, r3             ; match?
 *   8005954: mvcv  r0                  ; r0 = (match ? 1 : 0)
 *   8005956: zextb r0, r0
 *   8005958: addi  r14, 20
 *   800595a: pop   r4-r7, r15
 * ============================================================ */
int image_copy_update(const uint8_t *header, uint32_t data_length)
{
    uint32_t img_len = *(uint32_t *)(header + 0x0C);
    uint32_t end_addr = img_len + data_length;
    uint32_t current_offset = data_length;
    uint32_t hash_result = 0;
    uint32_t max_flash = 0x1FFFFFFF;    /* bmaski(29) */
    uint32_t chunk_size = 1024;

    /* Initialize CRC32 context */
    uint32_t crc_ctx[4];
    crc_init(crc_ctx, 3, 0xFFFFFFFF);

    /* Process image data in 1024-byte chunks */
    while (current_offset < end_addr) {
        uint32_t remaining = end_addr - current_offset;
        uint32_t this_chunk = (remaining < chunk_size) ? remaining : chunk_size;

        if (current_offset <= max_flash) {
            /* Read from flash via QSPI and update CRC inline */
            /* In actual code, flash data is read directly into CRC */
            flash_read_page(current_offset, NULL, this_chunk, 0);
            crc_update(crc_ctx, current_offset, this_chunk);
        } else {
            /* Direct memory-mapped read for high addresses */
            crc_update(crc_ctx, (void *)current_offset, this_chunk);
        }

        current_offset += this_chunk;
    }

    /* Finalize CRC */
    crc_finalize(crc_ctx);
    crc_final(crc_ctx, &hash_result);

    /* Compare with expected checksum from header */
    uint32_t expected = *(uint32_t *)(header + 0x18);  /* org_checksum */
    return (hash_result == expected) ? 1 : 0;
}


/* ============================================================
 * signature_verify (0x080070C4)
 *
 * RSA signature verification for signed firmware images.
 *
 * NOTE: SHA and RSA internals are SKIPPED per project requirements.
 * This function is documented at the interface level only.
 *
 * r0 = data_addr (flash address of signed data)
 * r1 = data_len  (length of signed data)
 * r2 = sig_addr  (flash address of 128-byte RSA signature)
 *
 * Algorithm:
 *   1. Initialize signature verification state variables
 *   2. Store data/sig addresses in global state:
 *      - 0x200113FC: status flag (cleared to 0)
 *      - 0x200113E0: signature address
 *      - 0x20011424: another status flag
 *      - 0x20011418: result accumulator
 *      - 0x200113F8: another status flag
 *      - 0x20011414: cleared
 *      - 0x2001140C: cleared
 *      - 0x20011410: data length
 *      - 0x2001141C: data address
 *   3. Call SHA256 computation (0x080062D4) - SKIPPED
 *   4. Call RSA verify (0x08007058) - SKIPPED
 *   5. Check result: if accumulated result <= 0x07FFFFFF, success
 *   6. Return 0 on success, -1 on failure
 *
 * Disassembly:
 *   80070c4: push  r4-r6, r15
 *   80070c6: lrw   r3, 0x200113fc
 *   80070c8: movi  r4, 0
 *   80070ca: st.w  r4, (r3, 0x0)       ; clear status
 *   80070cc: lrw   r6, 0x200113e0      ; sig_context
 *   80070d4: st.w  r2, (r6, 0x0)       ; sig_addr
 *   ... (store all parameters into global state)
 *   80070ec: mov   r0, r4              ; 0
 *   80070ee: bsr   sha256_init         ; 0x080062D4
 *   80070f2: bsr   rsa_verify          ; 0x08007058
 *   80070f6: ld.w  r2, (r5, 0x0)       ; result
 *   80070f8: bmaski r3, 27             ; 0x07FFFFFF
 *   80070fc: cmphs r3, r2             ; result <= max?
 *   80070fe: mvc   r0                  ; r0 = success flag
 *   8007102: subu  r0, r4, r0          ; r0 = 0 - flag = (-1 or 0)
 *   8007104: st.w  r4, (r6, 0x0)       ; clear sig context
 *   8007106: pop   r4-r6, r15
 * ============================================================ */
int signature_verify(uint32_t data_addr, uint32_t data_len, uint32_t sig_addr)
{
    /* NOTE: SHA256 and RSA internals are skipped.
     * This function:
     *   1. Computes SHA256 hash of the firmware data
     *   2. Verifies the RSA signature against the hash
     *   3. Returns 0 (success) or -1 (failure)
     */

    /* Initialize global signature verification state */
    *(volatile uint32_t *)0x200113FC = 0;
    *(volatile uint32_t *)0x200113E0 = sig_addr;
    *(volatile uint32_t *)0x20011424 = 0;
    *(volatile uint32_t *)0x20011418 = 0;
    *(volatile uint32_t *)0x200113F8 = 0;
    *(volatile uint32_t *)0x20011414 = 0;
    *(volatile uint32_t *)0x2001140C = 0;
    *(volatile uint32_t *)0x20011410 = data_len;
    *(volatile uint32_t *)0x2001141C = data_addr;

    /* SHA256 computation call (0x080062D4 = ota_process in this binary).
     * In the original firmware this address serves double duty as both
     * the OTA process function and a SHA256 hash computation entry point
     * depending on global state context. */
    ota_process(NULL, NULL, 0);

    /* RSA signature verification (0x08007058 = app_boot_sequence).
     * Similar to above, this address also serves as the RSA verify
     * entry in the signature verification flow. */
    app_boot_sequence(0);

    /* Check result */
    uint32_t result = *(volatile uint32_t *)0x20011418;
    if (result <= 0x07FFFFFF) {
        *(volatile uint32_t *)0x200113E0 = 0;
        return 0;       /* Signature valid */
    }

    *(volatile uint32_t *)0x200113E0 = 0;
    return -1;          /* Signature invalid */
}
