#include "luat_base.h"
#include "luat_otp.h"

#include "wm_include.h"
#include "wm_internal_flash.h"


int luat_otp_read(int zone, char* buff, size_t offset, size_t len) {
    if (zone < 0 || zone > 2)
        return -1;
    int addr = (zone + 1) * 0x1000 + offset;
    if (tls_fls_otp_read(addr, (u8*)buff, len) == TLS_FLS_STATUS_OK) {
        return len;
    }
    return -1;
}

int luat_otp_write(int zone, char* buff, size_t offset, size_t len) {
    if (zone < 0 || zone > 2)
        return -1;
    int addr = (zone + 1) * 0x1000 + offset;
    if (tls_fls_otp_write(addr, (u8*)buff, len) == TLS_FLS_STATUS_OK) {
        return 0;
    }
    return -1;
}

int luat_otp_erase(int zone, size_t offset, size_t len) {
    return 0; // 无需主动擦除
}

int luat_otp_lock(int zone) {
    tls_fls_otp_lock();
    return 0;
}
