
#include "string.h"
#include "wm_include.h"
#include "wm_crypto_hard.h"
#include "aes.h"
#include "wm_osal.h"
#include "wm_regs.h"
#include "wm_debug.h"
#include "wm_crypto_hard.h"
#include "wm_internal_flash.h"
#include "wm_pmu.h"

#include "luat_base.h"
#include "luat_crypto.h"
#define LUAT_LOG_TAG "crypto"
#include "luat_log.h"

#include "FreeRTOS.h"
#include "task.h"


static unsigned char trng_wait;
static unsigned char trng_pool[24];

void rngGenRandom() {
    uint32_t ret;
    tls_crypto_random_init(tls_os_get_time(), CRYPTO_RNG_SWITCH_32);
    vTaskDelay(1);
    for (size_t i = 0; i < sizeof(trng_pool) / sizeof(uint32_t); i++)
    {
        tls_reg_read32(HR_CRYPTO_SEC_CFG);
        ret = tls_reg_read32(HR_CRYPTO_RNG_RESULT);
        memcpy(trng_pool + i * sizeof(uint32_t), &ret, sizeof(uint32_t));
    }
    tls_crypto_random_stop();
}

int luat_crypto_trng(char* buff, size_t len)  {
    char* dst = buff;
    while (len > 0) {
        // 池内没有剩余的随机值? 生成一次
        if (trng_wait == 0) {
            // LLOGD("生成一次随机数 24字节,放入池中");
            rngGenRandom();
            trng_wait = 24;
        }
        // 剩余随机值够用, 直接拷贝
        if (len <= trng_wait) {
            memcpy(dst, trng_pool + (24 - trng_wait), len);
            trng_wait -= len;
            return 0;
        }
        // 不够用, 先把现有的用完, 然后下一个循环
        memcpy(dst, trng_pool + (24 - trng_wait), trng_wait);
        dst += trng_wait;
        len -= trng_wait;
        trng_wait = 0;
    }
    return 0;
}
