#include "luat_base.h"
#include "luat_pm.h"

#include "wm_pmu.h"
#include "wm_regs.h"
#include "wm_timer.h"
#include "wm_watchdog.h"
#include "wm_ram_config.h"

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
        tls_pmu_clk_select(1);
        return 0;
    }
    else if (id == 1 && timeout > 0) {
        // 单位毫妙
        tls_pmu_timer1_start(timeout);
        tls_pmu_clk_select(1);
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
    int reson = tls_sys_get_reboot_reason();
    switch (tls_sys_get_reboot_reason())
    {
    case REBOOT_REASON_POWER_ON:// 硬件复位开机
        *lastState = 0;
        *rtcOrPad = 0;
        break;
    case REBOOT_REASON_STANDBY: // 唤醒重启
        *lastState = 3;
        *rtcOrPad = 3;
        break;
    case REBOOT_REASON_EXCEPTION: // 异常重启
        *lastState = 0;
        *rtcOrPad = 1;
        break;
    case REBOOT_REASON_WDG_TIMEOUT: // 硬狗超时
        *lastState = 0;
        *rtcOrPad = 1;
        break;
    case REBOOT_REASON_ACTIVE: // 用户主动复位
        *lastState = 0;
        *rtcOrPad = 0;
        break;
    case REBOOT_REASON_SLEEP: // 不可能出现
        *lastState = 0;
        *rtcOrPad = 4;
        break;

    default:
        break;
    }

    return 0;
}

int luat_pm_get_poweron_reason(void)
{
int reson = tls_sys_get_reboot_reason();
    switch (tls_sys_get_reboot_reason())
    {
    case REBOOT_REASON_POWER_ON:// 硬件复位开机
        return 0;
    case REBOOT_REASON_STANDBY:
        return 2;
    case REBOOT_REASON_EXCEPTION:
        return 6;
    case REBOOT_REASON_WDG_TIMEOUT:
        return 8;
    case REBOOT_REASON_ACTIVE:
        return 3;
    case REBOOT_REASON_SLEEP:
        return 2;

    default:
        break;
    }

    return 4;
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
