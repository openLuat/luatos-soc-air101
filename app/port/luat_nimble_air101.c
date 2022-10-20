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

int luat_nimble_deinit() {
    LLOGE("deinit not support yet");
    return -1;
}

int luat_nimble_trace_level(int level) {
    return 0;
}

int luat_nimble_init_server(uint8_t uart_idx, char* name, int mode);

int luat_nimble_init(uint8_t uart_idx, char* name, int mode) {

    int ret = 0;
    tls_reg_write32(HR_CLK_BBP_CLT_CTRL, 0x7F);

    ble_hs_cfg.sm_io_cap = MYNEWT_VAL(BLE_SM_IO_CAP),
    ble_hs_cfg.sm_oob_data_flag = MYNEWT_VAL(BLE_SM_OOB_DATA_FLAG),
    ble_hs_cfg.sm_bonding = MYNEWT_VAL(BLE_SM_BONDING),
    ble_hs_cfg.sm_mitm = MYNEWT_VAL(BLE_SM_MITM),
    ble_hs_cfg.sm_sc = MYNEWT_VAL(BLE_SM_SC),
    ble_hs_cfg.sm_keypress = MYNEWT_VAL(BLE_SM_KEYPRESS),
    ble_hs_cfg.sm_our_key_dist = MYNEWT_VAL(BLE_SM_OUR_KEY_DIST),
    ble_hs_cfg.sm_their_key_dist = MYNEWT_VAL(BLE_SM_THEIR_KEY_DIST);

    ret = luat_nimble_init_server(uart_idx, name, mode);
    if (ret == 0) {
        /*Initialize the vuart interface and enable controller*/
        ble_hci_vuart_init(0xFF);

        /* As the last thing, process events from default event queue. */
        tls_nimble_start();
    }
    return 0;
}
