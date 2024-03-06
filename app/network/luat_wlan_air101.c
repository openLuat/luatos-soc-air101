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
#include "wm_debug.h"

#define LUAT_LOG_TAG "wlan"
#include "luat_log.h"

#include "lwip/netif.h"
#include "luat_network_adapter.h"
#include "luat_timer.h"
#include "net_lwip2.h"
#include "lwip/tcp.h"

void net_lwip2_set_link_state(uint8_t adapter_index, uint8_t updown);

#define SCAN_DONE       (0x73)
#define ONESHOT_RESULT  (0x74)

static int wlan_init;
static int wlan_state;

char luat_sta_hostname[32];

extern const char WiFiVer[];
extern u8 tx_gain_group[];
extern void *tls_wl_init(u8 *tx_gain, u8 *mac_addr, u8 *hwver);
extern int wpa_supplicant_init(u8 *mac_addr);
extern void tls_pmu_chipsleep_callback(int sleeptime);

static int l_wlan_cb(lua_State*L, void* ptr) {
    (void)ptr;
    u8 ssid[33]= {0};
    u8 pwd[65] = {0};
    char sta_ip[16] = {0};
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    lua_getglobal(L, "sys_pub");
    switch (msg->arg1)
    {
    case NETIF_IP_NET_UP:
        #ifdef LUAT_USE_NETWORK
        net_lwip2_set_link_state(NW_ADAPTER_INDEX_LWIP_WIFI_STA, 1);
        #endif
        luat_wlan_get_ip(0, sta_ip);
        LLOGD("sta ip %s", sta_ip);
        lua_pushstring(L, "IP_READY");
        lua_pushstring(L, sta_ip);
        lua_pushinteger(L, NW_ADAPTER_INDEX_LWIP_WIFI_STA);
        lua_call(L, 3, 0);
        break;
    case NETIF_WIFI_DISCONNECTED:
        #ifdef LUAT_USE_NETWORK
        net_lwip2_set_link_state(NW_ADAPTER_INDEX_LWIP_WIFI_STA, 0);
        #endif
        lua_pushstring(L, "IP_LOSE");
        lua_call(L, 1, 0); // 暂时只发个IP_LOSE
        break;
    case SCAN_DONE :
        lua_pushstring(L, "WLAN_SCAN_DONE");
        lua_call(L, 1, 0);
        break;
    case ONESHOT_RESULT:
        lua_pushstring(L, "SC_RESULT");
        tls_wifi_get_oneshot_ssidpwd(ssid, pwd);
        LLOGD("oneshot %s %s", ssid, pwd);
        lua_pushstring(L, (const char*)ssid);
        lua_pushstring(L, (const char*)pwd);
        lua_call(L, 3, 0);
        break;
    default:
        break;
    }
    return 0;
}

static void netif_event_cb(u8 status) {
    rtos_msg_t msg = {0};
    struct tls_param_ip ip_param;
    // LLOGD("netif_event %d", status);
    msg.handler = l_wlan_cb;
	switch (status)
    {
    case NETIF_WIFI_JOIN_FAILED:
        LLOGI("join failed");
        tls_auto_reconnect(3);
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
        tls_param_get(TLS_PARAM_ID_IP, (void *)&ip_param, false);
        if (!ip_param.dhcp_enable) {
            LLOGI("dhcp is disable, so 'join success' as 'IP_READY'");
            msg.arg1 = status;
            luat_msgbus_put(&msg, 0);
        }
        break;
    case NETIF_IP_NET_UP :
        LLOGI("IP READY");
        msg.arg1 = status;
        luat_msgbus_put(&msg, 0);
        break;
    case NETIF_WIFI_SOFTAP_SUCCESS :
        LLOGI("softap create success");
        #ifdef LUAT_USE_NETWORK
        net_lwip2_set_link_state(NW_ADAPTER_INDEX_LWIP_WIFI_AP, 1);
        #endif
        break;
    case NETIF_WIFI_SOFTAP_FAILED:
        LLOGI("softap create failed");
        #ifdef LUAT_USE_NETWORK
        net_lwip2_set_link_state(NW_ADAPTER_INDEX_LWIP_WIFI_AP, 0);
        #endif
        break;
    case NETIF_WIFI_SOFTAP_CLOSED:
        LLOGI("softap create closed");
        #ifdef LUAT_USE_NETWORK
        net_lwip2_set_link_state(NW_ADAPTER_INDEX_LWIP_WIFI_AP, 0);
        #endif
        break;
    case NETIF_IP_NET2_UP :
        LLOGI("softap netif up");
        break;
    default:
        break;
    }
}


int luat_wlan_init(luat_wlan_config_t *conf) {
    (void)conf;
    if (wlan_init == 0) {
        //------------------------------------
        //u8 enable = 0;
        u8 mac_addr[6];
        tls_get_tx_gain(&tx_gain_group[0]);
        TLS_DBGPRT_INFO("tx gain ");
        TLS_DBGPRT_DUMP((char *)(&tx_gain_group[0]), 27);
        // 计算wifi内存大小的公式 (TX+RX)*2*1638+4096
        // 例如 TX=6, RX=4, (6+4)*2*1638+4096=36856, 向上取整, 36k
        // 然后sram的最后位置是 0x20047FFF
        if (tls_wifi_mem_cfg((0x20048000u - (36*1024)), 6, 4)) /*wifi tx&rx mem customized interface*/
        {
            TLS_DBGPRT_INFO("wl mem initial failured\n");
        }
    
        tls_ft_param_get(CMD_WIFI_MAC, mac_addr, 6);
        TLS_DBGPRT_INFO("mac addr ");
        TLS_DBGPRT_DUMP((char *)(&mac_addr[0]), 6);
        if(tls_wl_init(NULL, &mac_addr[0], NULL) == NULL)
        {
            TLS_DBGPRT_INFO("wl driver initial failured\n");
        }
        if (wpa_supplicant_init(mac_addr))
        {
            TLS_DBGPRT_INFO("supplicant initial failured\n");
        }
    	/*wifi-temperature compensation,default:open*/
        if (tls_wifi_get_tempcomp_flag() != 0)
    	    tls_wifi_set_tempcomp_flag(0);
        if (tls_wifi_get_psm_chipsleep_flag() != 0)
    	    tls_wifi_set_psm_chipsleep_flag(0);
    	tls_wifi_psm_chipsleep_cb_register((tls_wifi_psm_chipsleep_callback)tls_pmu_chipsleep_callback, NULL, NULL);
    
        u8 wireless_protocol = 0;
        tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void *) &wireless_protocol, TRUE);
        // LLOGD("wireless_protocol %d", wireless_protocol);
        if (TLS_PARAM_IEEE80211_INFRA != wireless_protocol)
        {
            wireless_protocol = TLS_PARAM_IEEE80211_INFRA;
            tls_param_set(TLS_PARAM_ID_WPROTOCOL, (void *) &wireless_protocol, FALSE);
        }
        //tls_wifi_enable_log(1);
        luat_wlan_get_hostname(0); // 调用一下就行
        wlan_init = 1;

        #ifdef LUAT_USE_ZLINK_WLAN
        extern void luat_zlink_wlan_init(void);
        luat_zlink_wlan_init();
        #else
        tls_ethernet_init();
        tls_sys_init();
        tls_netif_add_status_event(netif_event_cb);
        tls_wifi_scan_result_cb_register(NULL);
        #endif

        //-----------------------------------
        #ifdef LUAT_USE_NETWORK
        struct netif *et0 = tls_get_netif();
        net_lwip2_set_netif(NW_ADAPTER_INDEX_LWIP_WIFI_STA, et0);
        #if TLS_CONFIG_AP
        extern struct netif *nif4apsta;
        if (nif4apsta) {
            net_lwip2_set_netif(NW_ADAPTER_INDEX_LWIP_WIFI_AP, nif4apsta);
            net_lwip2_register_adapter(NW_ADAPTER_INDEX_LWIP_WIFI_AP);
        }
        #endif
        net_lwip2_register_adapter(NW_ADAPTER_INDEX_LWIP_WIFI_STA);

        // 确保DHCP是默认开启
        struct tls_param_ip ip_param;
        tls_param_get(TLS_PARAM_ID_IP, (void *)&ip_param, false);
        if (!ip_param.dhcp_enable) {
            ip_param.dhcp_enable = 1;
            tls_param_set(TLS_PARAM_ID_IP, (void *)&ip_param, false);
        }
        #endif
    }
    tls_wifi_set_psflag(FALSE, FALSE);
	return 0;
}

int luat_wlan_mode(luat_wlan_config_t *conf) {
    #if TLS_CONFIG_AP
    // 不需要设置, 反正都能用
    (void)conf;
    if (conf->mode == LUAT_WLAN_MODE_STA) {
        tls_wifi_softap_destroy();
    }
    // else if (conf->mode == LUAT_WLAN_MODE_AP) {
    //     tls_wifi_disconnect();
    // }
    #endif
    return 0;
}

int luat_wlan_ready(void) {
    return wlan_state;
}

int luat_wlan_connect(luat_wlan_conninfo_t* info) {
	tls_wifi_connect((u8*)info->ssid, strlen(info->ssid), (u8*)info->password, strlen(info->password));
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

static void scan_event_cb(void *ptmr, void *parg) {
    (void)ptmr;
    (void)parg;
    rtos_msg_t msg = {0};
    msg.handler = l_wlan_cb;
    msg.arg1 = SCAN_DONE;
    luat_msgbus_put(&msg, 0);
}

int luat_wlan_scan(void) {
    int ret = tls_wifi_scan();
    if (ret)
        LLOGD("tls_wifi_scan %d", ret);
    static tls_os_timer_t *scan_timer = NULL;
	tls_os_timer_create(&scan_timer, scan_event_cb, NULL, 3000, 0, NULL);
	tls_os_timer_start(scan_timer);
    return ret;
}

int luat_wlan_scan_get_result(luat_wlan_scan_result_t *results, size_t ap_limit) {
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
        // break;
    }
    luat_heap_free(buff);
    return ap_limit;
}

void luat_sc_callback(enum tls_wifi_oneshot_result_type type) {
    if (type == WM_WIFI_ONESHOT_TYPE_SSIDPWD) {
        LLOGD("oneshot Got!!");
        rtos_msg_t msg = {.handler = l_wlan_cb, .arg1=ONESHOT_RESULT};
        luat_msgbus_put(&msg, 0);
    }
}

extern u8 gucssidData[];
extern u8 gucpwdData[];
int luat_wlan_smartconfig_start(int tp) {
    (void)tp;
    gucssidData[0] = 0;
    gucpwdData[0] = 0;
    tls_wifi_oneshot_result_cb_register(luat_sc_callback);
    return tls_wifi_set_oneshot_flag(1);
}

int luat_wlan_smartconfig_stop(void) {
    return tls_wifi_set_oneshot_flag(0);
}

// 数据类
int luat_wlan_get_mac(int id, char* mac) {
    if (id == 0)
        tls_get_mac_addr((u8*)mac);
    else
        tls_ft_param_get(CMD_WIFI_MACAP, mac, 6);
    return 0;
}

int luat_wlan_set_mac(int id, const char* mac_addr) {
    u8 mac[8] = {0};
    int ret = 0;
    if (id == 0) {
        tls_ft_param_get(CMD_WIFI_MAC, mac, 6);
    }
    else {
        tls_ft_param_get(CMD_WIFI_MACAP, mac, 6);
    }
    if (memcmp(mac_addr, mac, 6) == 0) {
        // 完全相同, 不需要设置
        return 0;
    }
    if (id == 0) {
        ret = tls_ft_param_set(CMD_WIFI_MAC, (void*)mac_addr, 6);
    }
    else {
        ret = tls_ft_param_set(CMD_WIFI_MACAP, (void*)mac_addr, 6);
    }
    return ret;
}

int luat_wlan_get_ip(int type, char* data) {
    (void)type;
    struct netif *et0 = tls_get_netif();
    if (et0 == NULL || ip_addr_isany(&et0->ip_addr))
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
    if (et0 == NULL || ip_addr_isany(&et0->gw))
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
    if (apinfo2->gateway[0]) {
        memcpy(ipinfo.ip_addr, apinfo2->gateway, 4);
    }
    if (apinfo2->netmask[0]) {
        memcpy(ipinfo.netmask, apinfo2->netmask, 4);
    }
    memcpy(apinfo.ssid, apinfo2->ssid, strlen(apinfo2->ssid) + 1);
    if (strlen(apinfo2->password)) {
        apinfo.keyinfo.format = 1;
        apinfo.keyinfo.key_len = strlen(apinfo2->password);
        apinfo.keyinfo.index = 1;
        memcpy(apinfo.keyinfo.key, apinfo2->password, strlen(apinfo2->password)+1);
        apinfo.encrypt = IEEE80211_ENCRYT_CCMP_WPA2;
    }
    else {
        apinfo.encrypt = IEEE80211_ENCRYT_NONE;
    }
    if (apinfo2->channel > 0)
        apinfo.channel = apinfo2->channel;
    else {
        apinfo.channel = 6;
    }
    LLOGD("AP GW %d.%d.%d.%d MASK %d.%d.%d.%d", 
                ipinfo.ip_addr[0],ipinfo.ip_addr[1],ipinfo.ip_addr[2],ipinfo.ip_addr[3],
                ipinfo.netmask[0],ipinfo.netmask[1],ipinfo.netmask[2],ipinfo.netmask[3]
    );
    // ----------------------------
    u8 mac[6] = {0};
    tls_get_mac_addr(mac);
    sprintf_((char*)ipinfo.dnsname, "LUATOS_%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    //------------------------------

    u8 value = 0;
    tls_param_get(TLS_PARAM_ID_WPROTOCOL, (void *) &value, TRUE);
    // LLOGD("wireless_protocol %d", wireless_protocol);
    if (TLS_PARAM_IEEE80211_SOFTAP != value)
    {
        value = TLS_PARAM_IEEE80211_SOFTAP;
        tls_param_set(TLS_PARAM_ID_WPROTOCOL, (void *) &value, FALSE);
    }
    if (apinfo2->hidden) {
        value = 0;
        tls_param_set(TLS_PARAM_ID_BRDSSID, (void *) &value, FALSE);
    }
    else {
        value = 1;
        tls_param_set(TLS_PARAM_ID_BRDSSID, (void *) &value, FALSE);
    }

    ret = tls_wifi_softap_create(&apinfo, &ipinfo);
    if (ret)
        LLOGD("tls_wifi_softap_create %d", ret);
    return ret;
}

extern u8 *wpa_supplicant_get_mac(void);
const char* luat_wlan_get_hostname(int id) {
    (void)id;
    if (luat_sta_hostname[0] == 0) {
        u8* mac_addr = wpa_supplicant_get_mac();
        sprintf_(luat_sta_hostname, "LUATOS_%02X%02X%02X%02X%02X%02X", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    }
    return (const char*)luat_sta_hostname;
}

int luat_wlan_set_hostname(int id, const char* hostname) {
    (void)id;
    if (hostname == NULL || hostname[0] == 0) {
        return 0;
    }
    memcpy(luat_sta_hostname, hostname, strlen(hostname) + 1);
    return 0;
}

int luat_wlan_ap_stop(void) {
    #if TLS_CONFIG_AP
    tls_wifi_softap_destroy();
    #endif
    return 0;
}

int luat_wlan_set_station_ip(luat_wlan_station_info_t *info) {
    struct tls_ethif *ethif;
    struct tls_param_ip param_ip;
    if (info == NULL) {
        return -1;
    }
    ethif=tls_netif_get_ethif();
	if (ethif == NULL)
	{
        LLOGE("call wlan.init() first!!");
		return -1;
	}

    if (info->dhcp_enable) {
        LLOGD("dhcp enable");
    }
    else {
        LLOGD("dhcp disable");
        LLOGD("Sta IP %d.%d.%d.%d MASK %d.%d.%d.%d GW %d.%d.%d.%d", 
                info->ipv4_addr[0],   info->ipv4_addr[1],   info->ipv4_addr[2],   info->ipv4_addr[3],
                info->ipv4_netmask[0],info->ipv4_netmask[1],info->ipv4_netmask[2],info->ipv4_netmask[3],
                info->ipv4_gateway[0],info->ipv4_gateway[1],info->ipv4_gateway[2],info->ipv4_gateway[3]
        );
    }

	if (WM_WIFI_JOINED == tls_wifi_get_state())
	{
	    if (info->dhcp_enable) {
	        /* enable dhcp */
	        tls_dhcp_start();
	    } else {
	        tls_dhcp_stop();

			MEMCPY((char *)ip_2_ip4(&ethif->ip_addr) , info->ipv4_addr, 4);
            // MEMCPY((char *)ip_2_ip4(&ethif->dns1), &params->dns1, 4);
	        MEMCPY((char *)ip_2_ip4(&ethif->netmask), info->ipv4_netmask, 4);
	        MEMCPY((char *)ip_2_ip4(&ethif->gw), info->ipv4_gateway, 4);
	        tls_netif_set_addr(&ethif->ip_addr, &ethif->netmask, &ethif->gw);
	    }
	}

    /* update flash params */
    param_ip.dhcp_enable = info->dhcp_enable;
    MEMCPY((char *)param_ip.gateway, info->ipv4_gateway, 4);
    MEMCPY((char *)param_ip.ip, info->ipv4_addr, 4);
    MEMCPY((char *)param_ip.netmask, info->ipv4_netmask, 4);
    tls_param_set(TLS_PARAM_ID_IP, (void *)&param_ip, false);

    return 0;
}
