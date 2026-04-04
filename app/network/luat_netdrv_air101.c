#include "luat_base.h"
#include "luat_netdrv.h"
#include "luat_network_adapter.h"
#include "net_lwip2.h"
#include "luat_ulwip.h"

#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"

#include <string.h>

#define LUAT_LOG_TAG "netdrv"
#include "luat_log.h"

// ============================================================================
// External declarations
// ============================================================================

// STA netif getter from SDK
extern struct netif *tls_get_netif(void);

// AP netif (defined in src/network/lwip2.1.3/netif/wm_ethernet.c)
extern struct netif *nif4apsta;

// ============================================================================
// Static driver instances (STA + AP only, no ETH)
// ============================================================================

static luat_netdrv_t g_netdrv_sta;
static luat_netdrv_t g_netdrv_ap;

// ulwip context for STA DHCP management
static ulwip_ctx_t g_sta_ulwip_ctx = {0};

// Initialization flags
static uint8_t g_sta_init_ok = 0;
static uint8_t g_ap_init_ok = 0;

// ============================================================================
// Forward declarations
// ============================================================================

static int sta_bootup_cb(luat_netdrv_t* drv, void* userdata);
static int ap_bootup_cb(luat_netdrv_t* drv, void* userdata);
static int sta_dhcp(luat_netdrv_t* drv, void* userdata, int enable);
static void netif_dataout(luat_netdrv_t* drv, void* userdata, uint8_t* buff, uint16_t len);

// ============================================================================
// dataout callback - send packets through netif->linkoutput
// ============================================================================

static void netif_dataout(luat_netdrv_t* drv, void* userdata, uint8_t* buff, uint16_t len) {
    if (userdata == NULL || len < 16) {
        LLOGD("netif_dataout: invalid params %p len=%d", userdata, len);
        return;
    }

    struct netif *netif = (struct netif *)userdata;
    if (netif->linkoutput == NULL) {
        LLOGW("netif_dataout: linkoutput is NULL");
        return;
    }

    struct pbuf *p = pbuf_alloc(PBUF_RAW_TX, len, PBUF_RAM);
    if (p == NULL) {
        LLOGW("netif_dataout: pbuf_alloc failed len=%d", len);
        return;
    }

    pbuf_take(p, buff, len);
    err_t ret = netif->linkoutput(netif, p);
    (void)ret;  // Suppress unused variable warning
    pbuf_free(p);
}

// ============================================================================
// STA boot callback
// ============================================================================

static int sta_bootup_cb(luat_netdrv_t* drv, void* userdata) {
    (void)userdata;  // Unused

    if (g_sta_init_ok) {
        return 0;  // Already initialized
    }

    struct netif *sta = tls_get_netif();
    if (sta == NULL) {
        LLOGE("sta_bootup: tls_get_netif() returned NULL");
        return -1;
    }

    // Register with net_lwip2 adapter layer
    net_lwip2_set_netif(NW_ADAPTER_INDEX_LWIP_WIFI_STA, sta);
    net_lwip2_register_adapter(NW_ADAPTER_INDEX_LWIP_WIFI_STA);

    // Store netif reference
    g_netdrv_sta.netif = sta;
    g_netdrv_sta.userdata = sta;

    // Initialize ulwip context for DHCP management
    memcpy(g_sta_ulwip_ctx.hwaddr, sta->hwaddr, 6);
    g_sta_ulwip_ctx.netif = sta;
    g_sta_ulwip_ctx.adapter_index = NW_ADAPTER_INDEX_LWIP_WIFI_STA;
    g_sta_ulwip_ctx.dhcp_enable = 1;  // Enable DHCP by default

    // Link ulwip context to driver
    g_netdrv_sta.ulwip = &g_sta_ulwip_ctx;

    LLOGI("STA netdrv registered, netif=%p", sta);
    g_sta_init_ok = 1;
    return 0;
}

// ============================================================================
// AP boot callback
// ============================================================================

static int ap_bootup_cb(luat_netdrv_t* drv, void* userdata) {
    (void)userdata;  // Unused

    if (g_ap_init_ok) {
        return 0;  // Already initialized
    }

    // AP netif is created when softap starts
    if (nif4apsta == NULL) {
        LLOGD("ap_bootup: nif4apsta is NULL, AP not started yet");
        return -1;
    }

    // Register with net_lwip2 adapter layer
    net_lwip2_set_netif(NW_ADAPTER_INDEX_LWIP_WIFI_AP, nif4apsta);
    net_lwip2_register_adapter(NW_ADAPTER_INDEX_LWIP_WIFI_AP);

    // Store netif reference
    g_netdrv_ap.netif = nif4apsta;
    g_netdrv_ap.userdata = nif4apsta;

    LLOGI("AP netdrv registered, netif=%p", nif4apsta);
    g_ap_init_ok = 1;
    return 0;
}

// ============================================================================
// STA DHCP callback
// ============================================================================

static int sta_dhcp(luat_netdrv_t* drv, void* userdata, int enable) {
    (void)drv;
    (void)userdata;

    struct netif *sta = tls_get_netif();
    if (sta == NULL) {
        LLOGW("sta_dhcp: STA netif is NULL");
        return -2;
    }

    g_sta_ulwip_ctx.dhcp_enable = enable;

    if (enable && netif_is_up(sta) && netif_is_link_up(sta)) {
        ulwip_dhcp_client_start(&g_sta_ulwip_ctx);
        LLOGD("sta_dhcp: DHCP client started");
    } else {
        ulwip_dhcp_client_stop(&g_sta_ulwip_ctx);
        LLOGD("sta_dhcp: DHCP client stopped");
    }

    return 0;
}

// ============================================================================
// DHCP event callback - called when DHCP succeeds or times out
// ============================================================================

static void sta_dhcp_event_cb(int32_t event, void* ctx) {
    (void)ctx;
    extern void net_lwip2_set_link_state(uint8_t adapter_index, uint8_t updown);

    if (event == LUAT_ULWIP_DHCP_EVENT_GOT_IP) {
        LLOGI("DHCP: Got IP address");
        // Set link state up for net_lwip2
        net_lwip2_set_link_state(NW_ADAPTER_INDEX_LWIP_WIFI_STA, 1);
    } else if (event == LUAT_ULWIP_DHCP_EVENT_TIMEOUT) {
        LLOGW("DHCP: Timeout, failed to get IP");
    }
}

// ============================================================================
// Public function: Start DHCP when WiFi connects
// ============================================================================

void luat_netdrv_sta_start_dhcp(void) {
    struct netif *sta = tls_get_netif();
    if (sta == NULL) {
        LLOGW("luat_netdrv_sta_start_dhcp: STA netif is NULL");
        return;
    }

    // Initialize ulwip context if not already done
    if (g_sta_ulwip_ctx.netif == NULL) {
        g_sta_ulwip_ctx.netif = sta;
        g_sta_ulwip_ctx.adapter_index = NW_ADAPTER_INDEX_LWIP_WIFI_STA;
        memcpy(g_sta_ulwip_ctx.hwaddr, sta->hwaddr, 6);
        g_netdrv_sta.netif = sta;
        g_netdrv_sta.userdata = sta;

        // Register with net_lwip2 if not done
        net_lwip2_set_netif(NW_ADAPTER_INDEX_LWIP_WIFI_STA, sta);
        net_lwip2_register_adapter(NW_ADAPTER_INDEX_LWIP_WIFI_STA);
        g_sta_init_ok = 1;
    }

    // Ensure netif is up
    if (!netif_is_up(sta)) {
        netif_set_up(sta);
    }

    // Set link state
    netif_set_link_up(sta);

    // Set up event callback
    g_sta_ulwip_ctx.event_cb = sta_dhcp_event_cb;
    g_sta_ulwip_ctx.dhcp_enable = 1;

    // Start DHCP client
    LLOGI("Starting ulwip DHCP client, netif=%p", sta);
    ulwip_dhcp_client_start(&g_sta_ulwip_ctx);
}

// ============================================================================
// Public function: Initialize AP netdrv when softap starts
// ============================================================================

void luat_netdrv_ap_init(void) {
    // AP netif is created when softap starts
    if (nif4apsta == NULL) {
        LLOGW("luat_netdrv_ap_init: nif4apsta is NULL");
        return;
    }

    // Initialize AP driver if not done
    if (!g_ap_init_ok) {
        // Register with net_lwip2 adapter layer
        net_lwip2_set_netif(NW_ADAPTER_INDEX_LWIP_WIFI_AP, nif4apsta);
        net_lwip2_register_adapter(NW_ADAPTER_INDEX_LWIP_WIFI_AP);

        // Store netif reference
        g_netdrv_ap.netif = nif4apsta;
        g_netdrv_ap.userdata = nif4apsta;

        // Ensure netif is up synchronously (SDK uses async netifapi which may not complete yet)
        // Note: SDK's tls_netif2_set_up() uses netif_set_link_up() (sync) + netifapi_netif_set_up() (async)
        // We call netif_set_up() directly to ensure netif_is_up() returns true immediately
        if (!netif_is_up(nif4apsta)) {
            netif_set_up(nif4apsta);
        }
        if (!netif_is_link_up(nif4apsta)) {
            netif_set_link_up(nif4apsta);
        }

        LLOGI("AP netdrv initialized, netif=%p, link_up=%d, up=%d, ip=%s",
              nif4apsta,
              netif_is_link_up(nif4apsta),
              netif_is_up(nif4apsta),
              ipaddr_ntoa(&nif4apsta->ip_addr));

        g_ap_init_ok = 1;
    }
}

// ============================================================================
// Registration function - called from luat_wlan_init()
// ============================================================================

void luat_netdrv_register_xt804(void) {
    // Initialize driver instances
    memset(&g_netdrv_sta, 0, sizeof(luat_netdrv_t));
    memset(&g_netdrv_ap, 0, sizeof(luat_netdrv_t));
    memset(&g_sta_ulwip_ctx, 0, sizeof(ulwip_ctx_t));

    // Configure STA driver
    g_netdrv_sta.id = NW_ADAPTER_INDEX_LWIP_WIFI_STA;
    g_netdrv_sta.boot = sta_bootup_cb;
    g_netdrv_sta.dataout = netif_dataout;
    g_netdrv_sta.dhcp = sta_dhcp;
    g_netdrv_sta.ulwip = &g_sta_ulwip_ctx;

    // Configure AP driver (no DHCP client for AP mode)
    g_netdrv_ap.id = NW_ADAPTER_INDEX_LWIP_WIFI_AP;
    g_netdrv_ap.boot = ap_bootup_cb;
    g_netdrv_ap.dataout = netif_dataout;

    // Set AP netif immediately if already available
    // nif4apsta is created during Tcpip_stack_init(), which runs before luat_wlan_init()
    if (nif4apsta != NULL) {
        g_netdrv_ap.netif = nif4apsta;
        g_netdrv_ap.userdata = nif4apsta;
        LLOGI("AP netif pre-set during registration, netif=%p", nif4apsta);
    }

    // Register drivers with netdrv framework
    luat_netdrv_register(NW_ADAPTER_INDEX_LWIP_WIFI_STA, &g_netdrv_sta);
    luat_netdrv_register(NW_ADAPTER_INDEX_LWIP_WIFI_AP, &g_netdrv_ap);

    
    // netif association is handled by netdrv boot callbacks
    // netdrv was registered early in main.c, trigger boot now
    luat_netdrv_conf_t drvconf = {0};
    drvconf.id = NW_ADAPTER_INDEX_LWIP_WIFI_STA;
    luat_netdrv_setup(&drvconf);
    extern struct netif *nif4apsta;
    if (nif4apsta) {
        drvconf.id = NW_ADAPTER_INDEX_LWIP_WIFI_AP;
        luat_netdrv_setup(&drvconf);
    }

    // Set STA as default network adapter
    network_register_set_default(NW_ADAPTER_INDEX_LWIP_WIFI_STA);

    LLOGI("xt804 netdrv registered (STA + AP)");
}

// ============================================================================
// Optional: Static IP configuration for STA
// ============================================================================

void luat_netdrv_sta_set_static_ip(ip_addr_t* ip, ip_addr_t* gateway, ip_addr_t* mask) {
    struct netif *sta = tls_get_netif();
    if (sta == NULL) {
        LLOGW("luat_netdrv_sta_set_static_ip: STA netif is NULL");
        return;
    }

    // Stop DHCP if running
    if (g_sta_ulwip_ctx.dhcp_enable) {
        ulwip_dhcp_client_stop(&g_sta_ulwip_ctx);
        g_sta_ulwip_ctx.dhcp_enable = 0;
    }

    // Set static IP
    netif_set_ipaddr(sta, ip_2_ip4(ip));
    netif_set_gw(sta, ip_2_ip4(gateway));
    netif_set_netmask(sta, ip_2_ip4(mask));

    LLOGI("STA static IP configured: %s / %s / %s",
          ipaddr_ntoa(ip), ipaddr_ntoa(mask), ipaddr_ntoa(gateway));
}