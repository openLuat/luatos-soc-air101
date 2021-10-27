#include "luat_base.h"
#include "luat_pm.h"

#include "wm_pmu.h"
#include "wm_regs.h"

#define LUAT_LOG_TAG "pm"
#include "luat_log.h"

int luat_pm_request(int mode) {
    if (mode == LUAT_PM_SLEEP_MODE_LIGHT) {
        tls_pmu_sleep_start();
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

extern int rst_sta;
// extern int wake_src;
int luat_pm_last_state(int *lastState, int *rtcOrPad) {
    *rtcOrPad = 0; // 暂不支持
    if (rst_sta & 0x01) { // bit 0 , watchdog 复位
        *lastState = 8;
    }
    else if (rst_sta & 0x02) { // bit 1, 软件重启
        *lastState = 3;
    }
    else {
        *lastState = 0;
    }
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
