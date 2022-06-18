#include "luat_base.h"
#include "c_common.h"

#include "FreeRTOS.h"
#include "task.h"


void luat_task_suspend_all(void) {
    vTaskSuspendAll();
}

void luat_task_resume_all(void) {
    xTaskResumeAll();
}

uint64_t GetSysTickMS() {
    return xTaskGetTickCount();
}


void OS_Free(void* ptr) {
    luat_heap_free(ptr);
}

void* OS_Zalloc(size_t len) {
    void* ptr = luat_heap_malloc(len);
	if (ptr) {
		memset(ptr, 0, len);
	}
	return ptr;
}
