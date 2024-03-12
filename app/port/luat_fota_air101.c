
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
#include "wm_fwup.h"

#include "luat_base.h"
#include "luat_crypto.h"
#define LUAT_LOG_TAG "fota"
#include "luat_log.h"

#include "FreeRTOS.h"
#include "task.h"

#include "luat_fota.h"

int luat_fota_init(uint32_t start_address, uint32_t len, luat_spi_device_t* spi_device, const char *path, uint32_t pathlen) {
    return -1;
}

int luat_fota_write(uint8_t *data, uint32_t len) {
    return -1;
}

int luat_fota_done(void) {
    return -1;
}

int luat_fota_end(uint8_t is_ok) {
    return -1;
}

uint8_t luat_fota_wait_ready(void) {
    return 0;
}
