#include "luat_base.h"
#include "luat_nimble.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "wm_bt_config.h"

#include "wm_bt.h"
#include "wm_bt_util.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "transport/uart/ble_hci_uart.h"
#include "nimble/tls_nimble.h"

#include "wm_ble_gap.h"
#include "wm_ble_uart_if.h"
// #include "wm_ble_server_wifi_app.h"
#include "wm_ble_server_api_demo.h"
// #include "wm_ble_client_api_demo.h"
// #include "wm_ble_client_api_multi_conn_demo.h"

static bool ble_system_state_on = false;
static volatile tls_bt_state_t bt_adapter_state = WM_BT_STATE_OFF;

extern tls_bt_log_level_t tls_appl_trace_level;

int tls_bt_util_init(void);

int luat_nimble_trace_level(int level) {
    if (level >= 0 && level <= 6) {
        tls_appl_trace_level = (tls_bt_log_level_t)level;
    }
    return tls_appl_trace_level;
}

static void xxx_ble_income(uint8_t *p_data, uint32_t length) {
    printf("ble income len=%d ", length);
    for (size_t i = 0; i < length; i++)
    {
        printf("%02X ", p_data[i]);
    }
    printf("\n");
}

static void app_adapter_state_changed_callback(tls_bt_state_t status)
{
	TLS_BT_APPL_TRACE_DEBUG("adapter status = %s\r\n", status==WM_BT_STATE_ON?"bt_state_on":"bt_state_off");

    bt_adapter_state = status;
    
	#if (TLS_CONFIG_BLE == CFG_ON)

    if(status == WM_BT_STATE_ON)
    {
    	TLS_BT_APPL_TRACE_VERBOSE("init base application\r\n");

		//at here , user run their own applications;
        #if 1		
        //tls_ble_wifi_cfg_init();
        tls_ble_server_demo_api_init(xxx_ble_income);
        //tls_ble_client_demo_api_init(NULL);
        //tls_ble_server_demo_hid_init();
        //tls_ble_server_hid_uart_init();
        //tls_ble_client_multi_conn_demo_api_init();
        #endif        

    }else
    {
        TLS_BT_APPL_TRACE_VERBOSE("deinit base application\r\n");

        //here, user may free their application;
        #if 1
        // tls_ble_wifi_cfg_deinit(2);
        tls_ble_server_demo_api_deinit();
        // tls_ble_client_demo_api_deinit();
        // tls_ble_client_multi_conn_demo_api_deinit();
        #endif
    }

    #endif

}


static void
on_sync(void)
{
    //int rc;
    /* Make sure we have proper identity address set (public preferred) */
    //rc = ble_hs_util_ensure_addr(1);
    //assert(rc == 0);

    app_adapter_state_changed_callback(WM_BT_STATE_ON);
}
static void
on_reset(int reason)
{
    TLS_BT_APPL_TRACE_DEBUG("Resetting state; reason=%d\r\n", reason);
    app_adapter_state_changed_callback(WM_BT_STATE_OFF);
}
static void
on_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{

    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
        case BLE_GATT_REGISTER_OP_SVC:
            TLS_BT_APPL_TRACE_DEBUG("service,uuid16 %s handle=%d (%04X)\r\n",ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),ctxt->svc.handle, ctxt->svc.handle);
            break;

        case BLE_GATT_REGISTER_OP_CHR:
            TLS_BT_APPL_TRACE_DEBUG("charact,uuid16 %s arg %d def_handle=%d (%04X) val_handle=%d (%04X)\r\n",
                ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                (int)ctxt->chr.chr_def->arg,
                ctxt->chr.def_handle, ctxt->chr.def_handle,
                ctxt->chr.val_handle, ctxt->chr.val_handle);
            break;

        case BLE_GATT_REGISTER_OP_DSC:
            TLS_BT_APPL_TRACE_DEBUG("descrip, uuid16 %s arg %d handle=%d (%04X)\r\n",
                ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                (int)ctxt->dsc.dsc_def->arg,
                ctxt->dsc.handle, ctxt->dsc.handle);
            break;
    }

    return;

}


int
luat_nimble_init(uint8_t uart_idx)
{
    if(ble_system_state_on)
    {
        return BLE_HS_EALREADY;
    }

    memset(&ble_hs_cfg, 0, sizeof(ble_hs_cfg));

    /** Security manager settings. */
    ble_hs_cfg.sm_io_cap = MYNEWT_VAL(BLE_SM_IO_CAP),
    ble_hs_cfg.sm_oob_data_flag = MYNEWT_VAL(BLE_SM_OOB_DATA_FLAG),
    ble_hs_cfg.sm_bonding = MYNEWT_VAL(BLE_SM_BONDING),
    ble_hs_cfg.sm_mitm = MYNEWT_VAL(BLE_SM_MITM),
    ble_hs_cfg.sm_sc = MYNEWT_VAL(BLE_SM_SC),
    ble_hs_cfg.sm_keypress = MYNEWT_VAL(BLE_SM_KEYPRESS),
    ble_hs_cfg.sm_our_key_dist = MYNEWT_VAL(BLE_SM_OUR_KEY_DIST),
    ble_hs_cfg.sm_their_key_dist = MYNEWT_VAL(BLE_SM_THEIR_KEY_DIST),

    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.shutdown_cb = on_reset; /*same callback as on_reset */
    ble_hs_cfg.gatts_register_cb = on_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;   
    
    /* Initialize all packages. */
    nimble_port_init();



    /*Application levels code entry*/
    tls_ble_gap_init();
    tls_bt_util_init();

    /*Initialize the vuart interface and enable controller*/
    ble_hci_vuart_init(uart_idx);
    
    /* As the last thing, process events from default event queue. */
    tls_nimble_start();
    
    ble_system_state_on = true;

    return 0;
}


int
luat_nimble_deinit(void)
{
    int rc = 0;

    if(!ble_system_state_on)
    {
        return BLE_HS_EALREADY;
    }
    /*Stop hs system*/
    rc = nimble_port_stop();
    assert(rc == 0);
    
    /*Stop controller and free vuart resource */
    rc = ble_hci_vuart_deinit();
    assert(rc == 0);

    /*Free hs system resource*/
    nimble_port_deinit();
    
    /*Free task stack ptr and free hs task*/
    tls_nimble_stop();

    /*Application levels resource cleanup*/
    tls_ble_gap_deinit();
    tls_bt_util_deinit();
    
    ble_system_state_on = false;

    return rc;
}


//----------------------------------------
// 设置广播数据
int luat_nimble_gap_adv_set_fields() {
    
}
//----------------------------------------