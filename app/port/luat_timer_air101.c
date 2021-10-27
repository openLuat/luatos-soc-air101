
#include "luat_base.h"
#include "luat_malloc.h"
#include "luat_timer.h"
#include "luat_msgbus.h"
#include "FreeRTOS.h"
#include "task.h"
#include "wm_osal.h"
#define LUAT_LOG_TAG "luat.timer"
#include "luat_log.h"

#define FREERTOS_TIMER_COUNT 64
static luat_timer_t* timers[FREERTOS_TIMER_COUNT] = {0};

static void luat_timer_callback(void *ptmr, void *parg) {
    // LLOGD("timer callback");
    rtos_msg_t msg;
    luat_timer_t *timer = (luat_timer_t*)parg;
    msg.handler = timer->func;
    msg.ptr = timer;
    msg.arg1 = 0;
    msg.arg2 = 0;
    luat_msgbus_put(&msg, 1);
    // LLOGD("timer msgbus re=%ld", re);
}

static int nextTimerSlot() {
    for (size_t i = 0; i < FREERTOS_TIMER_COUNT; i++)
    {
        if (timers[i] == NULL) {
            return i;
        }
    }
    return -1;
}

int luat_timer_start(luat_timer_t* timer) {
    tls_os_timer_t *os_timer = NULL;
    int timerIndex;
    int ret;
    // LLOGD(">>luat_timer_start timeout=%ld", timer->timeout);
    timerIndex = nextTimerSlot();
    // LLOGD("timer id=%ld", timerIndex);
    if (timerIndex < 0) {
        return 1; // too many timer!!
    }
    if (timer->timeout < (1000 / configTICK_RATE_HZ))
        timer->timeout = 1000 / configTICK_RATE_HZ;
    ret =  tls_os_timer_create(&os_timer, luat_timer_callback, timer,(timer->timeout)/(1000 / configTICK_RATE_HZ), timer->repeat ? 1 : 0, NULL);
    // LLOGD("timer id=%ld, osTimerNew=%08X", timerIndex, (int)timer);
    if (TLS_OS_SUCCESS != ret)
    {
       return 1;
    }
    timers[timerIndex] = timer;
    timer->os_timer = os_timer;
    tls_os_timer_start(os_timer);
    return 0;
}

int luat_timer_stop(luat_timer_t* timer) {
    if (!timer)
        return 1;
    for (size_t i = 0; i < FREERTOS_TIMER_COUNT; i++)
    {
        if (timers[i] == timer) {
            timers[i] = NULL;
            break;
        }
    }
    tls_os_timer_stop(timer->os_timer);
    tls_os_timer_delete(timer->os_timer);
    return 0;
}

luat_timer_t* luat_timer_get(size_t timer_id) {
    for (size_t i = 0; i < FREERTOS_TIMER_COUNT; i++)
    {
        if (timers[i] && timers[i]->id == timer_id) {
            return timers[i];
        }
    }
    return NULL;
}


int luat_timer_mdelay(size_t ms) {
    if (ms >= (1000 / configTICK_RATE_HZ)) {
        vTaskDelay(ms / (1000 / configTICK_RATE_HZ));
    }
    return 0;
}

