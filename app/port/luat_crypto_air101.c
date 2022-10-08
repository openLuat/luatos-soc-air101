
#include "string.h"
#include "wm_include.h"
#include "wm_crypto_hard.h"
#include "aes.h"

#include "luat_base.h"
#include "luat_crypto.h"
#define LUAT_LOG_TAG "crypto"
#include "luat_log.h"

#include "FreeRTOS.h"
#include "task.h"

int luat_crypto_trng(char* buff, size_t len) {
    // tls_crypto_random_init(0, CRYPTO_RNG_SWITCH_32);
    // vTaskDelay(5);
    // tls_crypto_random_bytes_range(buff, len, 256);
    // tls_crypto_random_bytes_range(buff, len, 256);
    // tls_crypto_random_bytes_range(buff, len, 256);
    // tls_crypto_random_stop();
    // tls_crypto_trng(buff, len);
    extern int random_get_bytes(void *buf, size_t len);
	random_get_bytes(buff, len);
    return 0;
}

