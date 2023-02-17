
// 这个文件包含 系统heap和lua heap的默认实现


#include <stdlib.h>
#include <string.h>//add for memset
#include "bget.h"
#include "luat_malloc.h"

#define LUAT_LOG_TAG "heap"
#include "luat_log.h"
#include "wm_mem.h"


// const uint32_t luat_rom_addr_start = 0x8010000;
// #ifdef AIR103
// const uint32_t luat_rom_addr_end   = 0x80FFFFF;
// #else
// const uint32_t luat_rom_addr_end   = 0x81FFFFF;
// #endif

//------------------------------------------------
//  管理系统内存

void* luat_heap_malloc(size_t len) {
    return tls_mem_alloc(len);
}

void luat_heap_free(void* ptr) {
    if (ptr == NULL)
        return;
    tls_mem_free(ptr);
}

void* luat_heap_realloc(void* ptr, size_t len) {
    return tls_mem_realloc(ptr, len);
}

void* luat_heap_calloc(size_t count, size_t _size) {
    return tls_mem_calloc(count, _size);
}

#if configUSE_HEAP3
extern size_t xTotalHeapSize;
extern size_t xFreeBytesRemaining;
extern size_t xFreeBytesMin;
#endif

extern unsigned int heap_size_max;
extern unsigned int total_mem_size;
void luat_meminfo_sys(size_t* total, size_t* used, size_t* max_used)
{
#if configUSE_HEAP3
    *used = xTotalHeapSize - xFreeBytesRemaining;
    *max_used = xTotalHeapSize - xFreeBytesMin;
    *total = xTotalHeapSize;
#else
    *used = heap_size_max - total_mem_size - xPortGetFreeHeapSize();
    *max_used = *used;
    *total = heap_size_max;
#endif
}

//------------------------------------------------

//------------------------------------------------
// ---------- 管理 LuaVM所使用的内存----------------
#ifdef LUAT_USE_TLSF
#include "tlsf.h"

extern tlsf_t luavm_tlsf;
extern pool_t luavm_tlsf_ext;

void* __attribute__((section (".ram_run"))) luat_heap_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    if (ptr == NULL && nsize == 0)
        return NULL;
#ifdef LUAT_USE_MEMORY_OPTIMIZATION_CODE_MMAP
    if (ptr != NULL) {
        uint32_t ptrv = (uint32_t)ptr;
        if (ptrv >= 0x08000000 && ptrv < 0x08200000) {
            //LLOGD("??? %p %d %d", ptr, osize, nsize);
            if (nsize == 0)
                return NULL;
        }
    }
#endif
    return tlsf_realloc(luavm_tlsf, ptr, nsize);
}

void luat_meminfo_luavm(size_t *total, size_t *used, size_t *max_used) {
    *total = 0;
    *used = 0;
    *max_used = 0;
    tlsf_stat(tlsf_get_pool(luavm_tlsf), total, used, max_used);
    if (luavm_tlsf_ext) {
        tlsf_stat(luavm_tlsf_ext, total, used, max_used);
    }
}

#else
void* __attribute__((section (".ram_run"))) luat_heap_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    if (ptr == NULL && nsize == 0)
        return NULL;
#ifdef LUAT_USE_MEMORY_OPTIMIZATION_CODE_MMAP
    if (ptr != NULL) {
        uint32_t ptrv = (uint32_t)ptr;
        if (ptrv >= 0x08000000 && ptrv < 0x08200000) {
            // LLOGD("??? %p %d %d", ptr, osize, nsize);
            return NULL;
        }
    }
#endif
    if (0) {
        if (ptr) {
            if (nsize) {
                // 缩放内存块
                LLOGD("realloc %p from %d to %d", ptr, osize, nsize);
            }
            else {
                // 释放内存块
                LLOGD("free %p ", ptr);
                brel(ptr);
                return NULL;
            }
        }
        else {
            // 申请内存块
            ptr = bget(nsize);
            LLOGD("malloc %p type=%d size=%d", ptr, osize, nsize);
            return ptr;
        }
    }

    if (nsize)
    {
    	void* ptmp = bgetr(ptr, nsize);
    	if(ptmp == NULL && osize >= nsize)
    	{
    		return ptr;
    	}
        return ptmp;
    }

    brel(ptr);
    return NULL;
}

void luat_meminfo_luavm(size_t *total, size_t *used, size_t *max_used) {
	long curalloc, totfree, maxfree;
	unsigned long nget, nrel;
	bstats(&curalloc, &totfree, &maxfree, &nget, &nrel);
	*used = curalloc;
	*max_used = bstatsmaxget();
    *total = curalloc + totfree;
}
#endif
//-----------------------------------------------------------------------------
