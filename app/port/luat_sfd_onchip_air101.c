#include "luat_base.h"
#include "luat_sfd.h"

// #ifdef LUAT_SFD_ONCHIP

#define LUAT_LOG_TAG "onchip"
#include "luat_log.h"

#include "wm_include.h"
#include "wm_flash_map.h"
#include "wm_internal_flash.h"

extern uint32_t kv_addr;
extern uint32_t kv_size_kb;

int sfd_onchip_init (void* userdata) {
    sfd_onchip_t* onchip = (sfd_onchip_t*)userdata;
    if (onchip == NULL)
       return -1;
    //LLOGD("kv addr 0x%08X", kv_addr);
    //LLOGD("kv addr2 0x%08X", (0x1FC000 - 112 * 1024 - 64 * 1024));
    onchip->addr = kv_addr;
    return 0;
}

int sfd_onchip_status (void* userdata) {
    return 0;
}

int sfd_onchip_read (void* userdata, char* buff, size_t offset, size_t len) {
    int ret;
    sfd_onchip_t* onchip = (sfd_onchip_t*)userdata;
    if (onchip == NULL)
       return -1;
    ret = tls_fls_read(offset + onchip->addr, (u8 *)buff, len);
    if (ret != TLS_FLS_STATUS_OK)
    {
        return -1;
    }
    return 0;
}

int sfd_onchip_write (void* userdata, const char* buff, size_t offset, size_t len) {
    int ret;
    sfd_onchip_t* onchip = (sfd_onchip_t*)userdata;
    if (onchip == NULL)
       return -1;
    ret = tls_fls_write(offset + onchip->addr, (u8 *)buff, len);
    if (ret != TLS_FLS_STATUS_OK)
    {
        return -1;
    }
    return 0;
}
int sfd_onchip_erase (void* userdata, size_t offset, size_t len) {
    int ret;
    sfd_onchip_t* onchip = (sfd_onchip_t*)userdata;
    if (onchip == NULL)
       return -1;
    ret = tls_fls_erase((offset + onchip->addr) / INSIDE_FLS_SECTOR_SIZE);
    if (ret != TLS_FLS_STATUS_OK)
    {
        return -1;
    }
    return 0;
}

int sfd_onchip_ioctl (void* userdata, size_t cmd, void* buff) {
    return 0;
}

// #endif
