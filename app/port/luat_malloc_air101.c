
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
    tls_mem_free(ptr);
}

void* luat_heap_realloc(void* ptr, size_t len) {
    return tls_mem_realloc(ptr, len);
}

void* luat_heap_calloc(size_t count, size_t _size) {
    return tls_mem_calloc(count, _size);
}
//------------------------------------------------

//------------------------------------------------
// ---------- 管理 LuaVM所使用的内存----------------
void* __attribute__((section (".ram_run"))) luat_heap_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
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
// #if 0
#ifdef LUAT_USE_MEMORY_OPTIMIZATION_CODE_MMAP
    if (ptr != NULL) {
        uint32_t ptrv = (uint32_t)ptr;
        if (ptrv >= 0X40000000U) {
            // nop 无需释放
        }
        else {
            brel(ptr);
        }
    }
#else
    brel(ptr);
#endif
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

//-----------------------------------------------------------------------------
