#include "wm_include.h"
#include "FreeRTOS.h"

#include "stdio.h"

void* __wrap_malloc(size_t len) {
    return pvPortMalloc(len);
}

void __wrap_free(void* ptr) {
    if (ptr == NULL)
        return;
    u32 addr = (u32)ptr;
    if (addr >= 0x20000000 && addr <= 0x40000000) {
        // printf("free %p\n", ptr);
        vPortFree(ptr);
    }
}

void *pvPortRealloc( void *pv, size_t xWantedSize );
void* __wrap_realloc(void*ptr, size_t len) {
    return pvPortRealloc(ptr, len);
}

void* __wrap_calloc(size_t itemCount, size_t itemSize) {
    void* ptr = pvPortMalloc(itemCount * itemSize);
    if (ptr == NULL)
        return NULL;
    memset(ptr, 0, itemCount * itemSize);
    return ptr;
}

void* __wrap_zalloc(size_t size) {
    void* ptr = pvPortMalloc(size);
    if (ptr == NULL)
        return NULL;
    memset(ptr, 0, size);
    return ptr;
}