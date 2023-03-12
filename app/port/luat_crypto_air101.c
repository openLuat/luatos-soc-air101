
#include "string.h"
#include "wm_include.h"
#include "wm_crypto_hard.h"
#include "aes.h"
#include "wm_osal.h"

#include "luat_base.h"
#include "luat_crypto.h"
#define LUAT_LOG_TAG "crypto"
#include "luat_log.h"

#include "FreeRTOS.h"
#include "task.h"

int tls_crypto_random_bytes_range(unsigned char *out, u32 len, u32 range);

int luat_crypto_trng(char* buff, size_t len) {
    #define TMP_LEN (8)
    char tmp[TMP_LEN];
    tls_crypto_random_init(tls_os_get_time(), CRYPTO_RNG_SWITCH_32);
    vTaskDelay(1);
    tls_crypto_random_bytes_range(tmp, TMP_LEN, 255);
    tls_crypto_random_stop();

    extern int random_get_bytes(void *buf, size_t len);
	random_get_bytes(buff, len);
    for (size_t i = 0; i < len; i++)
    {
        buff[i] ^= tmp[i % TMP_LEN];
    }
    return 0;
}

