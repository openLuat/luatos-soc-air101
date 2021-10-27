#include "luat_base.h"
#include "luat_hwtimer.h"

#include "luat_msgbus.h"

#include "wm_timer.h"

static int l_hwtimer_handler(lua_State *L, void* ptr) {
    lua_getglobal(L, "sys_pub");
    if (lua_isfunction(L, -1)) {
        lua_pushstring(L, "HWTIMER_IRQ");
        lua_call(L, 1, 0);
    }
    return 0;
}

static void luat_hwtimer_cb(void *arg) {
    rtos_msg_t msg;
    msg.handler = l_hwtimer_handler;
    luat_msgbus_put(&msg, 0);
}

int luat_hwtimer_create(luat_hwtimer_conf_t *conf) {
    struct tls_timer_cfg cfg = {0};
    cfg.unit = conf->unit == 0 ? TLS_TIMER_UNIT_US : TLS_TIMER_UNIT_MS;
    cfg.timeout = conf->timeout;
    cfg.is_repeat = conf->is_repeat;
    cfg.callback = luat_hwtimer_cb;
    cfg.arg = NULL;
    u8 id = tls_timer_create(&cfg);
    if (id <= 5)
        return id;
    return -1;
}

int luat_hwtimer_start(int id) {
    if (id < 0 || id > 5) return -1;
    tls_timer_start(id);
    return 0;
}

int luat_hwtimer_stop(int id) {
    if (id < 0 || id > 5) return -1;
    tls_timer_stop(id);
    return 0;
}

int luat_hwtimer_read(int id) {
    if (id < 0 || id > 5) return -1;
    return tls_timer_read(id);
}

int luat_hwtimer_change(int id, uint32_t newtimeout) {
    if (id < 0 || id > 5) return -1;
    tls_timer_change(id, newtimeout);
    return 0;
}

int luat_hwtimer_destroy(int id)  {
    if (id < 0 || id > 5) return -1;
    tls_timer_destroy(id);
    return 0;
}