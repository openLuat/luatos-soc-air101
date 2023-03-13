#include "wm_osal.h"
#include "wm_mem.h"
#include "list.h"
#include <string.h>
#include "FreeRTOS.h"
#include "stdio.h"

void * mem_alloc_debug(u32 size) {
    return malloc(size);
}
void mem_free_debug(void *p) {
    free(p);
}
void * mem_realloc_debug(void *mem_address, u32 size) {
    return realloc(mem_address, size);
}

void *mem_calloc_debug(u32 length, u32 size) {
    return calloc(length, size);
}
