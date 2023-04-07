#include "wm_osal.h"
#include "wm_mem.h"
#include "list.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"

#include "luat_conf_bsp.h"
#if 0

#ifdef LUAT_USE_PROFILER_XXX
void * mem_alloc_debug(u32 size) {
    void* ptr = malloc(size);
    printf("mem_alloc_debug %p %d\n", ptr, size);
    return ptr;
}
void mem_free_debug(void *ptr) {
    printf("mem_free_debug %p\n", ptr);
    free(ptr);
}
void * mem_realloc_debug(void *mem_address, u32 size) {
    void* ptr = realloc(mem_address, size);
    printf("mem_realloc_debug %p %d %p\n", mem_address, size, ptr);
    return ptr;
}

void *mem_calloc_debug(u32 length, u32 size) {
    void* ptr = calloc(length, size);
    printf("mem_calloc_debug %p %d\n", ptr, size * length);
    return ptr;
}
#else
TaskStatus_t stat;
void * mem_alloc_debug(u32 size) {
    if (size == 16) {
        printf("mem_alloc_debug %d\n", size);
        TaskHandle_t t = xTaskGetCurrentTaskHandle();
        if (t != NULL) {
            vTaskGetInfo(t, &stat, pdFALSE, eInvalid);
            printf("task %s %d\n", stat.pcTaskName, stat.xTaskNumber);
        }
    }
    return malloc(size);
}
void mem_free_debug(void *p) {
    // printf("free %p\n", p);
    free(p);
}
void * mem_realloc_debug(void *mem_address, u32 size) {
    return realloc(mem_address, size);
}

void *mem_calloc_debug(u32 length, u32 size) {
    return calloc(length, size);
}
#endif

#endif
