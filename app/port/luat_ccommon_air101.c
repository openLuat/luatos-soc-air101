#include "luat_base.h"
#include "c_common.h"

#include "FreeRTOS.h"
#include "task.h"


uint64_t GetSysTickMS() {
    return xTaskGetTickCount();
}
