
#include "string.h"
#include "wm_include.h"
#include "wm_crypto_hard.h"
#include "aes.h"

#include "luat_base.h"
#include "luat_crypto.h"
#define LUAT_LOG_TAG "crypto"
#include "luat_log.h"

static char trng_init = 0;

int luat_crypto_trng(char* buff, size_t len) {
    if (trng_init == 0) {
        trng_init = 1;
        tls_crypto_random_init(0, CRYPTO_RNG_SWITCH_32);
    }
    tls_crypto_random_bytes_range(buff, len, 256);
    return 0;
}

