
#include "string.h"
#include "wm_include.h"
#include "wm_crypto_hard.h"
#include "aes.h"

#include "luat_base.h"
#include "luat_crypto.h"
#define LUAT_LOG_TAG "crypto"
#include "luat_log.h"

//#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
//#include "mbedtls/md5.h"

void luat_crypto_HmacSha1(const unsigned char *input, int ilen, unsigned char *output,const unsigned char *key, int keylen);
void luat_crypto_HmacSha256(const unsigned char *input, int ilen, unsigned char *output,const unsigned char *key, int keylen);
void luat_crypto_HmacSha512(const unsigned char *input, int ilen, unsigned char *output,const unsigned char *key, int keylen);
void luat_crypto_HmacMd5(const unsigned char *input, int ilen, unsigned char *output,const unsigned char *key, int keylen);

int luat_crypto_md5_simple(const char* str, size_t str_size, void* out_ptr) {
    psDigestContext_t ctx;
    tls_crypto_md5_init(&ctx);

    tls_crypto_md5_update(&ctx, (const unsigned char *)str, str_size);
    tls_crypto_md5_final(&ctx, (unsigned char *)out_ptr);
    return 0;
}
int luat_crypto_sha1_simple(const char* str, size_t str_size, void* out_ptr) {
    psDigestContext_t ctx;
    tls_crypto_sha1_init(&ctx);

    tls_crypto_sha1_update(&ctx, (const unsigned char *)str, str_size);
    tls_crypto_sha1_final(&ctx, (unsigned char *)out_ptr);
    return 0;
}

int luat_crypto_sha256_simple(const char* str, size_t str_size, void* out_ptr) {
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);

    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, (const unsigned char *)str, str_size);
    mbedtls_sha256_finish(&ctx, (unsigned char *)out_ptr);
    mbedtls_sha256_free(&ctx);
    return 0;
}

int luat_crypto_sha512_simple(const char* str, size_t str_size, void* out_ptr) {
    mbedtls_sha512_context ctx;
    mbedtls_sha512_init(&ctx);

    mbedtls_sha512_starts(&ctx, 0);
    mbedtls_sha512_update(&ctx, (const unsigned char *)str, str_size);
    mbedtls_sha512_finish(&ctx, (unsigned char *)out_ptr);
    mbedtls_sha512_free(&ctx);
    return 0;
}

int luat_crypto_hmac_md5_simple(const char* str, size_t str_size, const char* mac, size_t mac_size, void* out_ptr) {
    luat_crypto_HmacMd5((const unsigned char *)str, str_size, (unsigned char *)out_ptr, (const unsigned char *)mac, mac_size);
    return 0;
}

int luat_crypto_hmac_sha1_simple(const char* str, size_t str_size, const char* mac, size_t mac_size, void* out_ptr) {
    luat_crypto_HmacSha1((const unsigned char *)str, str_size, (unsigned char *)out_ptr, (const unsigned char *)mac, mac_size);
    return 0;
}

int luat_crypto_hmac_sha256_simple(const char* str, size_t str_size, const char* mac, size_t mac_size, void* out_ptr) {
    luat_crypto_HmacSha256((const unsigned char *)str, str_size, (unsigned char *)out_ptr, (const unsigned char *)mac, mac_size);
    return 0;
}

int luat_crypto_hmac_sha512_simple(const char* str, size_t str_size, const char* mac, size_t mac_size, void* out_ptr) {
    luat_crypto_HmacSha512((const unsigned char *)str, str_size, (unsigned char *)out_ptr, (const unsigned char *)mac, mac_size);
    return 0;
}


///----------------------------

#define ALI_SHA1_KEY_IOPAD_SIZE (64)
#define ALI_SHA1_DIGEST_SIZE    (20)

#define ALI_SHA256_KEY_IOPAD_SIZE   (64)
#define ALI_SHA256_DIGEST_SIZE      (32)

#define ALI_SHA512_KEY_IOPAD_SIZE   (128)
#define ALI_SHA512_DIGEST_SIZE      (64)

#define ALI_MD5_KEY_IOPAD_SIZE  (64)
#define ALI_MD5_DIGEST_SIZE     (16)

// char atHb2Hex(unsigned char hb)
// {
//     hb = hb&0xF;
//     return (char)(hb<10 ? '0'+hb : hb-10+'a');
// }


/*
 * output = SHA-1( input buffer )
 */
void luat_crypto_HmacSha1(const unsigned char *input, int ilen, unsigned char *output,const unsigned char *key, int keylen)
{
    int i;
    psDigestContext_t ctx;
    unsigned char k_ipad[ALI_SHA1_KEY_IOPAD_SIZE] = {0};
    unsigned char k_opad[ALI_SHA1_KEY_IOPAD_SIZE] = {0};
    unsigned char tempbuf[ALI_SHA1_DIGEST_SIZE];

    memset(k_ipad, 0x36, ALI_SHA1_KEY_IOPAD_SIZE);
    memset(k_opad, 0x5C, ALI_SHA1_KEY_IOPAD_SIZE);

    for(i=0; i<keylen; i++)
    {
        if(i>=ALI_SHA1_KEY_IOPAD_SIZE)
        {
            break;
        }
        k_ipad[i] ^=key[i];
        k_opad[i] ^=key[i];
    }

    tls_crypto_sha1_init(&ctx);

    tls_crypto_sha1_update(&ctx, k_ipad, ALI_SHA1_KEY_IOPAD_SIZE);
    tls_crypto_sha1_update(&ctx, input, ilen);
    tls_crypto_sha1_final(&ctx, tempbuf);

    tls_crypto_sha1_init(&ctx);

    tls_crypto_sha1_update(&ctx, k_opad, ALI_SHA1_KEY_IOPAD_SIZE);
    tls_crypto_sha1_update(&ctx, tempbuf, ALI_SHA1_DIGEST_SIZE);
    tls_crypto_sha1_final(&ctx, tempbuf);

    memcpy(output, tempbuf, ALI_SHA1_DIGEST_SIZE);
}
/*
 * output = SHA-256( input buffer )
 */
void luat_crypto_HmacSha256(const unsigned char *input, int ilen, unsigned char *output,const unsigned char *key, int keylen)
{
    int i;
    mbedtls_sha256_context ctx;
    unsigned char k_ipad[ALI_SHA256_KEY_IOPAD_SIZE] = {0};
    unsigned char k_opad[ALI_SHA256_KEY_IOPAD_SIZE] = {0};

    memset(k_ipad, 0x36, 64);
    memset(k_opad, 0x5C, 64);

    if ((NULL == input) || (NULL == key) || (NULL == output)) {
        return;
    }

    if (keylen > ALI_SHA256_KEY_IOPAD_SIZE) {
        return;
    }

    for(i=0; i<keylen; i++)
    {
        if(i>=ALI_SHA256_KEY_IOPAD_SIZE)
        {
            break;
        }
        k_ipad[i] ^=key[i];
        k_opad[i] ^=key[i];
    }
    mbedtls_sha256_init(&ctx);

    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, k_ipad, ALI_SHA256_KEY_IOPAD_SIZE);
    mbedtls_sha256_update(&ctx, input, ilen);
    mbedtls_sha256_finish(&ctx, output);

    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, k_opad, ALI_SHA256_KEY_IOPAD_SIZE);
    mbedtls_sha256_update(&ctx, output, ALI_SHA256_DIGEST_SIZE);
    mbedtls_sha256_finish(&ctx, output);

    mbedtls_sha256_free(&ctx);
}
/*
 * output = SHA-512( input buffer )
 */
void luat_crypto_HmacSha512(const unsigned char *input, int ilen, unsigned char *output,const unsigned char *key, int keylen)
{
    int i;
    mbedtls_sha512_context ctx;
    unsigned char k_ipad[ALI_SHA512_KEY_IOPAD_SIZE] = {0};
    unsigned char k_opad[ALI_SHA512_KEY_IOPAD_SIZE] = {0};

    memset(k_ipad, 0x36, ALI_SHA512_KEY_IOPAD_SIZE);
    memset(k_opad, 0x5C, ALI_SHA512_KEY_IOPAD_SIZE);

    if ((NULL == input) || (NULL == key) || (NULL == output)) {
        return;
    }

    if (keylen > ALI_SHA512_KEY_IOPAD_SIZE) {
        return;
    }

    for(i=0; i<keylen; i++)
    {
        if(i>=ALI_SHA512_KEY_IOPAD_SIZE)
        {
            break;
        }
        k_ipad[i] ^=key[i];
        k_opad[i] ^=key[i];
    }
    mbedtls_sha512_init(&ctx);

    mbedtls_sha512_starts(&ctx, 0);
    mbedtls_sha512_update(&ctx, k_ipad, ALI_SHA512_KEY_IOPAD_SIZE);
    mbedtls_sha512_update(&ctx, input, ilen);
    mbedtls_sha512_finish(&ctx, output);

    mbedtls_sha512_starts(&ctx, 0);
    mbedtls_sha512_update(&ctx, k_opad, ALI_SHA512_KEY_IOPAD_SIZE);
    mbedtls_sha512_update(&ctx, output, ALI_SHA512_DIGEST_SIZE);
    mbedtls_sha512_finish(&ctx, output);

    mbedtls_sha512_free(&ctx);
}
/*
 * output = MD-5( input buffer )
 */
void luat_crypto_HmacMd5(const unsigned char *input, int ilen, unsigned char *output,const unsigned char *key, int keylen)
{
    int i;
    psDigestContext_t ctx;
    unsigned char k_ipad[ALI_MD5_KEY_IOPAD_SIZE] = {0};
    unsigned char k_opad[ALI_MD5_KEY_IOPAD_SIZE] = {0};
    unsigned char tempbuf[ALI_MD5_DIGEST_SIZE];

    memset(k_ipad, 0x36, ALI_MD5_KEY_IOPAD_SIZE);
    memset(k_opad, 0x5C, ALI_MD5_KEY_IOPAD_SIZE);

    for(i=0; i<keylen; i++)
    {
        if(i>=ALI_MD5_KEY_IOPAD_SIZE)
        {
            break;
        }
        k_ipad[i] ^=key[i];
        k_opad[i] ^=key[i];
    }

    tls_crypto_md5_init(&ctx);

    tls_crypto_md5_update(&ctx, k_ipad, ALI_MD5_KEY_IOPAD_SIZE);
    tls_crypto_md5_update(&ctx, input, ilen);
    tls_crypto_md5_final(&ctx, tempbuf);

    tls_crypto_md5_init(&ctx);

    tls_crypto_md5_update(&ctx, k_opad, ALI_MD5_KEY_IOPAD_SIZE);
    tls_crypto_md5_update(&ctx, tempbuf, ALI_MD5_DIGEST_SIZE);
    tls_crypto_md5_final(&ctx, tempbuf);

    memcpy(output, tempbuf, ALI_MD5_DIGEST_SIZE);
}

int l_crypto_cipher_xxx(lua_State *L, uint8_t flags) {
    int ret = -1;
    size_t cipher_size = 0;
    size_t pad_size = 0;
    size_t str_size = 0;
    size_t key_size = 0;
    size_t iv_size = 0;
    const char* cipher = luaL_optlstring(L, 1, "AES-128-ECB", &cipher_size);
    const char* pad = luaL_optlstring(L, 2, "PKCS7", &pad_size);
    const char* str = luaL_checklstring(L, 3, &str_size);
    const char* key = luaL_checklstring(L, 4, &key_size);
    const char* iv = luaL_optlstring(L, 5, "", &iv_size);

    luaL_Buffer buff;

    if (!strcmp("AES-128-CBC", cipher)) {
        luaL_buffinitsize(L, &buff, str_size);
        memcpy(buff.b, str, str_size);
        if (flags) {
            ret = aes_128_cbc_encrypt((const u8*)key, (const u8*)iv, (u8*)buff.b, str_size);
        }
        else {
            ret = aes_128_cbc_decrypt((const u8*)key, (const u8*)iv, (u8*)buff.b, str_size);
        }
        if (ret == 0) {
            luaL_pushresultsize(&buff, str_size);
            return 1;
        }
    }
    lua_pushstring(L, "");
    return 1;
}

int luat_crypto_trng(char* buff, size_t len) {
    return tls_crypto_trng(buff, len);
}

