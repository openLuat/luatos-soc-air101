#include "wm_osal.h"
#include "wm_mem.h"
#include "list.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"

#include "luat_conf_bsp.h"
extern void *pvPortMalloc( size_t xWantedSize );
extern void vPortFree( void *pv );
extern void *pvPortRealloc( void *pv, size_t xWantedSize );
#if 1
void * mem_alloc_debug(u32 size) {
    void* ptr = pvPortMalloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}
void mem_free_debug(void *ptr) {
    if (ptr) {
        vPortFree(ptr);
    }
}
void * mem_realloc_debug(void *mem_address, u32 size) {
    return pvPortRealloc(mem_address, size);
}

void *mem_calloc_debug(u32 length, u32 size) {
    void* ptr = pvPortMalloc(length * size);
    if (ptr) {
        memset(ptr, 0, length * size);
    }
    return ptr;
}

#endif
