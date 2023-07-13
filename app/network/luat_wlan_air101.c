#include "luat_base.h"
#include "luat_malloc.h"
#include "luat_msgbus.h"
#include "luat_wlan.h"


#include <string.h>
#include "wm_irq.h"
#include "tls_sys.h"
#include "wm_ram_config.h"
#include "wm_efuse.h"
#include "wm_regs.h"
#include "wm_wifi.h"
#include "wm_netif.h"

#define LUAT_LOG_TAG "wlan"
#include "luat_log.h"

#include "lwip/netif.h"
#include "luat_network_adapter.h"
#include "luat_timer.h"
#include "net_lwip.h"
#include "lwip/tcp.h"

void net_lwip_set_link_state(uint8_t adapter_index, uint8_t updown);

#define SCAN_DONE       (0x73)
#define ONESHOT_RESULT  (0x74)

static int wlan_init;
static int wlan_state;

static int l_wlan_cb(lua_State*L, void* ptr) {
    u8 ssid[33]= {0};
    u8 pwd[65] = {0};
    char sta_ip[16] = {0};
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    lua_getglobal(L, "sys_pub");
    switch (msg->arg1)
    {
    case NETIF_IP_NET_UP:
        luat_wlan_get_ip(0, sta_ip);
        LLOGD("IP_EVENT_STA_GOT_IP %s", sta_ip);
        lua_pushstring(L, "IP_READY");
        lua_pushstring(L, sta_ip);
        lua_pushinteger(L, NW_ADAPTER_INDEX_LWIP_WIFI_STA);
        lua_call(L, 3, 0);
        net_lwip_set_link_state(NW_ADAPTER_INDEX_LWIP_WIFI_STA, 1);
        break;
    case NETIF_WIFI_DISCONNECTED:
        lua_pushstring(L, "IP_LOSE");
        lua_call(L, 1, 0); // 暂时只发个IP_LOSE
        net_lwip_set_link_state(NW_ADAPTER_INDEX_LWIP_WIFI_STA, 0);
        break;
    case SCAN_DONE :
        lua_pushstring(L, "WLAN_SCAN_DONE");
        lua_call(L, 1, 0);
        break;
    case ONESHOT_RESULT:
        lua_pushstring(L, "SC_RESULT");
        tls_wifi_get_oneshot_ssidpwd(ssid, pwd);
        LLOGD("oneshot %s %s", ssid, pwd);
        lua_pushstring(L, ssid);
        lua_pushstring(L, pwd);
        lua_call(L, 3, 0);
        break;
    default:
        break;
    }
    return 0;
}

static void netif_event_cb(u8 status) {
    rtos_msg_t msg = {0};
    msg.handler = l_wlan_cb;
	switch (status)
    {
    case NETIF_WIFI_JOIN_FAILED:
        LLOGI("join failed");
        break;
    case NETIF_WIFI_DISCONNECTED:
        wlan_state = 0;
        LLOGI("disconnected");
        msg.arg1 = status;
        luat_msgbus_put(&msg, 0);
        break;
    case NETIF_WIFI_JOIN_SUCCESS :
        wlan_state = 1;
        LLOGI("join success");
        break;
    case NETIF_IP_NET_UP :
        LLOGI("IP READY");
        msg.arg1 = status;
        luat_msgbus_put(&msg, 0);
        break;
    default:
        break;
    }
}

static void scan_event_cb(void) {
    rtos_msg_t msg = {0};
    msg.handler = l_wlan_cb;
    msg.arg1 = SCAN_DONE;
    luat_msgbus_put(&msg, 0);
}

int luat_wlan_init(luat_wlan_config_t *conf) {
    if (wlan_init == 0) {
        wlan_init = 1;
        tls_netif_add_status_event(netif_event_cb);
        tls_wifi_scan_result_cb_register(scan_event_cb);
        #ifdef LUAT_USE_NETWORK
        // LLOGD("CALL net_lwip_init");
        net_lwip_init();
        extern void soc_lwip_init_hook(void);
        // LLOGD("CALL net_lwip_register_adapter");
        struct netif *et0 = tls_get_netif();
        //extern void net_lwip_set_netif(uint8_t adapter_index, struct netif *netif, void *init, uint8_t is_default);
        //net_lwip_set_netif(NW_ADAPTER_INDEX_LWIP_WIFI_STA, et0, NULL, 1);
        // extern void net_lwip_set_netif(struct netif *netif);
        net_lwip_set_netif(et0);
        soc_lwip_init_hook();
        net_lwip_register_adapter(NW_ADAPTER_INDEX_LWIP_WIFI_STA);
        #endif
    }
	return 0;
}

int luat_wlan_mode(luat_wlan_config_t *conf) {
    // 不需要设置, 反正都能用
    return 0;
}

int luat_wlan_ready(void) {
    return wlan_state;
}

int luat_wlan_connect(luat_wlan_conninfo_t* info) {
	tls_wifi_connect(info->ssid, strlen(info->ssid), info->password, strlen(info->password));
    u8 opt = WIFI_AUTO_CNT_FLAG_SET;
    u8 mode = WIFI_AUTO_CNT_ON;
    tls_wifi_auto_connect_flag(opt, &mode);
    return 0;
}

int luat_wlan_disconnect(void) {
    u8 opt = WIFI_AUTO_CNT_FLAG_SET;
    u8 mode = WIFI_AUTO_CNT_OFF;
    tls_wifi_auto_connect_flag(opt, &mode);
    tls_wifi_disconnect();
    return 0;
}

int luat_wlan_scan(void) {
    int ret = tls_wifi_scan();
    LLOGD("tls_wifi_scan %d", ret);
    return ret;
}

int luat_wlan_scan_get_result(luat_wlan_scan_result_t *results, int ap_limit) {
    size_t buffsize = ap_limit * 48 + 8;
    u8* buff = luat_heap_malloc(buffsize);
    if (buff == NULL)
        return 0;
    memset(buff, 0, buffsize);
    tls_wifi_get_scan_rslt((u8*)buff, buffsize);
    struct tls_scan_bss_t *wsr = (struct tls_scan_bss_t*)buff;
    struct tls_bss_info_t *bss_info;
    if (ap_limit > wsr->count)
        ap_limit = wsr->count;
    bss_info = wsr->bss;
    for (size_t i = 0; i < ap_limit; i++)
    {
        memset(results[i].ssid, 0, 33);
        memcpy(results[i].ssid, bss_info[i].ssid, bss_info[i].ssid_len);
        memcpy(results[i].bssid, bss_info[i].bssid, ETH_ALEN);
        results[i].rssi = bss_info[i].rssi;
        results[i].ch = bss_info[i].channel;
        break;
    }
    luat_heap_free(buff);
    return ap_limit;
}

static void oneshot_result_callback(enum tls_wifi_oneshot_result_type type) {
    if (type == WM_WIFI_ONESHOT_TYPE_SSIDPWD) {
        LLOGD("oneshot Got!!");
        rtos_msg_t msg = {.handler = l_wlan_cb, .arg1=ONESHOT_RESULT};
        luat_msgbus_put(&msg, 0);
    }
}

int luat_wlan_smartconfig_start(int tp) {
    tls_wifi_oneshot_result_cb_register(oneshot_result_callback);
    return tls_wifi_set_oneshot_flag(1);
}

int luat_wlan_smartconfig_stop(void) {
    return tls_wifi_set_oneshot_flag(0);
}

// 数据类
int luat_wlan_get_mac(int id, char* mac) {
    tls_get_mac_addr(mac);
    return 0;
}

int luat_wlan_set_mac(int id, char* mac) {
    tls_set_mac_addr(mac);
    return 0;
}

int luat_wlan_get_ip(int type, char* data) {
    struct netif *et0 = tls_get_netif();
    if (et0 == NULL || et0->ip_addr.addr == 0)
        return -1;
    ipaddr_ntoa_r(&et0->ip_addr, data, 16);
    return 0;
}

// 设置和获取省电模式
int luat_wlan_set_ps(int mode) {
    tls_wifi_set_psm_chipsleep_flag(mode == 0 ? 0 : 1);
    return 0;
}

int luat_wlan_get_ps(void) {
    return tls_wifi_get_psm_chipsleep_flag();
}

int luat_wlan_get_ap_bssid(char* buff) {
    struct tls_curr_bss_t bss = {0};
    tls_wifi_get_current_bss(&bss);
    memcpy(buff, bss.bssid, ETH_ALEN);
    return 0;
}

int luat_wlan_get_ap_rssi(void) {
    struct tls_curr_bss_t bss = {0};
    tls_wifi_get_current_bss(&bss);
    return bss.rssi;
}

int luat_wlan_get_ap_gateway(char* buff) {
    struct netif *et0 = tls_get_netif();
    if (et0 == NULL || et0->ip_addr.addr == 0)
        return -1;
    ipaddr_ntoa_r(&et0->gw, buff, 16);
    return 0;
}

// AP类
int luat_wlan_ap_start(luat_wlan_apinfo_t *apinfo2) {
    int ret = 0;
    struct tls_softap_info_t apinfo = {0};
    struct tls_ip_info_t ipinfo = {
        .ip_addr = {192, 168, 4, 1},
        .netmask = {255, 255, 255, 0}
    };
    memcpy(apinfo.ssid, apinfo2->ssid, strlen(apinfo2->ssid) + 1);
    if (strlen(apinfo2->password)) {
        apinfo.keyinfo.format = 1;
        apinfo.keyinfo.key_len = strlen(apinfo2->password);
        apinfo.keyinfo.index = 1;
        memcpy(apinfo.keyinfo.key, apinfo2->password, strlen(apinfo2->password)+1);
        apinfo.encrypt = IEEE80211_ENCRYT_CCMP_WPA;
    }
    else {
        apinfo.encrypt = IEEE80211_ENCRYT_NONE;
    }
    apinfo.channel = 6;
    // ----------------------------
    // 这部分有必要不?? 拿不准
    // ----------------------------
    u8 mac[6] = {0};
    tls_get_mac_addr(mac);
    sprintf_(ipinfo.dnsname, "LUATOS_%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    //------------------------------

    ret = tls_wifi_softap_create(&apinfo, &ipinfo);
    LLOGD("tls_wifi_softap_create %d", ret);
    return ret;
}
