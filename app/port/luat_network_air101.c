#include "luat_base.h"
#include "luat_rtos.h"
#include "c_common.h"
#include "luat_malloc.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

void DBG_Printf(const char* format, ...) {
    
}

void DBG_HexPrintf(void *Data, unsigned int len) {

}

LUAT_RET luat_send_event_to_task(void *task_handle, uint32_t id, uint32_t param1, uint32_t param2, uint32_t param3) {
    return LUAT_ERR_OK;
}

LUAT_RET luat_wait_event_from_task(void *task_handle, uint32_t wait_event_id, void *out_event, void *call_back, uint32_t ms) {
    return LUAT_ERR_OK;
}

void luat_stop_rtos_timer(void *timer) {

}


void luat_release_rtos_timer(void *timer) {

}

void *luat_create_rtos_timer(void *cb, void *param, void *task_handle) {

}

int luat_start_rtos_timer(void *timer, uint32_t ms, uint8_t is_repeat) {
    return 0;
}
