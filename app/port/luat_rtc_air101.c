#include "luat_rtc.h"
#include "luat_msgbus.h"
#include "wm_rtc.h"

#define LUAT_LOG_TAG "rtc"
#include "luat_log.h"

extern int Base_year;

static int luat_rtc_handler(lua_State *L, void* ptr) {
    lua_getglobal(L, "sys_pub");
    if (lua_isfunction(L, -1)) {
        lua_pushstring(L, "RTC_IRQ");
        lua_call(L, 1, 0);
    }
    return 0;
}

static void luat_rtc_cb(void *arg) {
    //LLOGI("rtc irq");
    rtos_msg_t msg;
    msg.handler = luat_rtc_handler;
    luat_msgbus_put(&msg, 0);
}

int luat_rtc_set(struct tm *tblock) {
    if (tblock == NULL)
        return -1;
    tblock->tm_year -= Base_year;
    tblock->tm_mon--;
    tls_set_rtc(tblock);
    return 0;
}

int luat_rtc_get(struct tm *tblock) {
    if (tblock == NULL)
        return -1;
    tls_get_rtc(tblock);
    tblock->tm_year += Base_year;
    tblock->tm_mon++;
    return 0;
}

int luat_rtc_timer_start(int id, struct tm *tblock) {
    if (id || tblock == NULL)
        return -1;
    tblock->tm_year -= Base_year;
    tblock->tm_mon--;
    tls_rtc_isr_register(luat_rtc_cb, NULL);
    tls_rtc_timer_start(tblock);
}

int luat_rtc_timer_stop(int id) {
    if (id)
        return -1;
    tls_rtc_timer_stop();
}
