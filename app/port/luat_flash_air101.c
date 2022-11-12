#include "luat_base.h"
#include "luat_flash.h"

#include "luat_log.h"
#include "lfs_port.h"
#include "wm_include.h"
#include "luat_timer.h"
#include "stdio.h"
#include "luat_ota.h"
#include "wm_internal_flash.h"


int luat_flash_read(char* buff, size_t addr, size_t len) {
    return tls_fls_read(addr, buff, len);
}

int luat_flash_write(char* buff, size_t addr, size_t len) {
    return tls_fls_write(addr, buff, len);
}

int luat_flash_erase(size_t addr, size_t len) {
    tls_fls_erase(addr / 4096);
    return 0;
}
