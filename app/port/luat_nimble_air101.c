#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_nimble.h"

#include "wm_bt_config.h"
#include "wm_regs.h"

#include "wm_bt.h"
#include "wm_bt_util.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "transport/uart/ble_hci_uart.h"
#include "nimble/tls_nimble.h"

#include "wm_ble_gap.h"
#include "wm_ble_uart_if.h"

#include "syscfg/syscfg.h"

#define LUAT_LOG_TAG "nimble"
#include "luat_log.h"

static uint8_t inited;

int luat_nimble_deinit() {
    if (inited == 0)
        return 0;
    int rc;
    /*Stop hs system*/
    rc = nimble_port_stop();
    // assert(rc == 0);
    /*Stop controller and free vuart resource */
    rc = ble_hci_vuart_deinit();
    // assert(rc == 0);
    /*Free hs system resource*/
    nimble_port_deinit();
    /*Free task stack ptr and free hs task*/
    tls_nimble_stop();
    /*Application levels resource cleanup*/
    tls_ble_gap_deinit();
    // tls_bt_util_deinit(); // 关这个会死机, 还不知道为啥
    inited = 0;
    return 0;
}

int luat_nimble_trace_level(int level) {
    return 0;
}

int luat_nimble_init_peripheral(uint8_t uart_idx, char* name, int mode);
int luat_nimble_init_central(uint8_t uart_idx, char* name, int mode);
int luat_nimble_init_ibeacon(uint8_t uart_idx, char* name, int mode);

int luat_nimble_init(uint8_t uart_idx, char* name, int mode) {
    if (inited == 1)
        return 0;
    int ret = 0;
    // tls_reg_write32(HR_CLK_BBP_CLT_CTRL, 0x7F);

    ble_hs_cfg.sm_io_cap = MYNEWT_VAL(BLE_SM_IO_CAP),
    ble_hs_cfg.sm_oob_data_flag = MYNEWT_VAL(BLE_SM_OOB_DATA_FLAG),
    ble_hs_cfg.sm_bonding = MYNEWT_VAL(BLE_SM_BONDING),
    ble_hs_cfg.sm_mitm = MYNEWT_VAL(BLE_SM_MITM),
    ble_hs_cfg.sm_sc = MYNEWT_VAL(BLE_SM_SC),
    ble_hs_cfg.sm_keypress = MYNEWT_VAL(BLE_SM_KEYPRESS),
    ble_hs_cfg.sm_our_key_dist = MYNEWT_VAL(BLE_SM_OUR_KEY_DIST),
    ble_hs_cfg.sm_their_key_dist = MYNEWT_VAL(BLE_SM_THEIR_KEY_DIST);

    if (mode == 0) {
        LLOGD("CALL luat_nimble_init_peripheral");
        ret = luat_nimble_init_peripheral(uart_idx, name, mode);
    }
    else if (mode == 1) {
        LLOGD("CALL luat_nimble_init_central");
        ret = luat_nimble_init_central(uart_idx, name, mode);
    }
    else if (mode == 2) {
        ret = luat_nimble_init_ibeacon(uart_idx, name, mode);
    }
    else {
        return -1;
    }
    if (ret == 0) {
        /*Initialize the vuart interface and enable controller*/
        ble_hci_vuart_init(0xFF);

        /* As the last thing, process events from default event queue. */
        tls_nimble_start();
        inited = 1;
        #ifndef LUAT_USE_WLAN
        tls_bt_ctrl_sleep(true);
        #endif
    }
    return 0;
}
