#include "luat_base.h"
#include "luat_pm.h"

#include "wm_pmu.h"
#include "wm_regs.h"
#include "wm_timer.h"

#define LUAT_LOG_TAG "pm"
#include "luat_log.h"

int luat_pm_request(int mode) {
    if (mode == LUAT_PM_SLEEP_MODE_LIGHT) {
        tls_close_peripheral_clock(TLS_PERIPHERAL_TYPE_TIMER);
        tls_pmu_sleep_start();
        tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_TIMER);
        return 0;
    }
    else if (mode == LUAT_PM_SLEEP_MODE_DEEP || mode == LUAT_PM_SLEEP_MODE_STANDBY) {
        tls_pmu_standby_start();
        return 0;
    }
    return -1;
}

//int luat_pm_release(int mode);

int luat_pm_dtimer_start(int id, size_t timeout) {
    if (id == 0 && timeout > 0) {
        // 单位秒
        tls_pmu_timer0_start((timeout + 999) / 1000);
        return 0;
    }
    else if (id == 1 && timeout > 0) {
        // 单位毫妙
        tls_pmu_timer1_start(timeout);
        return 0;
    }
    return -1;
}

int luat_pm_dtimer_stop(int id) {
    if (id == 0) {
        tls_pmu_timer0_stop();
        return 0;
    }
    else if (id == 1) {
        tls_pmu_timer1_stop();
        return 0;
    }
    return -1;
}

int luat_pm_dtimer_check(int id) {
    return -1;
}

//void luat_pm_cb(int event, int arg, void* args);

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
extern int power_bk_reg; // from wm_main.c
// extern int wake_src;
int luat_pm_last_state(int *lastState, int *rtcOrPad) {
    // 实际情况与寄存器手册的描述不符
    // 复位开机,   是 00D90344
    // RTC或wakeup 是 00D10240
    if (CHECK_BIT(power_bk_reg, 8)) {
        *lastState = 0;
        *rtcOrPad = 0;
    }
    else {
        *lastState = 1;
        *rtcOrPad = 4;
    }

    // if (CHECK_BIT(power_bk_reg, 8)) {
    //     if (CHECK_BIT(power_bk_reg, 5)) {
    //         *lastState = 3;
    //         *rtcOrPad = 1;
    //     }
    //     else if (CHECK_BIT(power_bk_reg, 2)) {
    //         *lastState = 3;
    //         *rtcOrPad = 2;
    //     }
    //     else {
    //         *lastState = 99;
    //         *rtcOrPad = 0;
    //     }
    // }
    // else {
    //     *lastState = 0;
    //     *rtcOrPad = 0;
    // }
    return 0;
}

int luat_pm_force(int mode) {
    return luat_pm_request(mode);
}

int luat_pm_check(void) {
    return 0;
}

int luat_pm_dtimer_wakeup_id(int* id) {
    return 0;
}

int luat_pm_dtimer_list(size_t* count, size_t* list) {
    *count = 0;
    return 0;
}

int luat_pm_power_ctrl(int id, uint8_t onoff) {
    LLOGW("not support yet");
    return -1;
}

int luat_pm_wakeup_pin(int pin, int val){
    LLOGW("not support yet");
    return -1;
}
