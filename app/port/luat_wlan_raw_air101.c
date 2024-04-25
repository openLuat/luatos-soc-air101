#include "luat_base.h"
#include "luat_malloc.h"
#include "luat_msgbus.h"
#include "luat_wlan_raw.h"


#include <string.h>
#include "wm_irq.h"
#include "tls_sys.h"
#include "wm_ram_config.h"
#include "wm_efuse.h"
#include "wm_regs.h"
#include "wm_wifi.h"
#include "wm_netif.h"
#include "wm_debug.h"

#define LUAT_LOG_TAG "wlanraw"
#include "luat_log.h"

extern u8* tls_wifi_buffer_acquire(int total_len);
extern void tls_wifi_buffer_release(bool is_apsta, u8* buffer);
extern u8 *hostapd_get_mac(void);
extern u8 *wpa_supplicant_get_mac(void);


int luat_wlan_raw_write(int is_apsta, uint8_t* buff, size_t len) {
    u8* tmp = tls_wifi_buffer_acquire(len);
    if (tmp == NULL) {
        LLOGW("tls_wifi_buffer_acquire failed, len:%d", len);
        return -1;
    }
    memcpy(tmp, buff, len);
    //LLOGD("写入数据 %s %d", is_apsta ? "AP" : "STA", len);
    tls_wifi_buffer_release(is_apsta, tmp);
    return 0;
}

int luat_wlan_get_mac(int id, char* mac);
static char wlan_raw_ap_mac[6];
int luat_wlan_raw_in(const u8 *bssid, u8 *buf, u32 buf_len) {
    const u8* ap_mac = hostapd_get_mac();
    // printf("pkgin %02X%02X%02X%02X%02X%02X %p %d\n", 
    //     bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5], 
    //     buf, buf_len);
    // printf("AP MAC %02X%02X%02X%02X%02X%02X\n", ap_mac[0], ap_mac[1], ap_mac[2], ap_mac[3], ap_mac[4], ap_mac[5]);
    if (memcmp(bssid, ap_mac, 6) == 0) {
        // LLOGD("AP data");
        l_wlan_raw_event(1, buf, buf_len); // AP数据包
    }
    else {
        // LLOGD("STA data");
        l_wlan_raw_event(0, buf, buf_len); // STA数据包
    }
    return 0;
}

int luat_wlan_raw_setup(luat_wlan_raw_conf_t *conf) {
    luat_wlan_get_mac(1, wlan_raw_ap_mac);
    return 0;
}
