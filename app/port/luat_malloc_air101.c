
// 这个文件包含 系统heap和lua heap的默认实现


#include <stdlib.h>
#include <string.h>//add for memset
#include "bget.h"
#include "luat_malloc.h"
#include "FreeRTOS.h"

#define LUAT_LOG_TAG "heap"
#include "luat_log.h"
#include "wm_mem.h"

#ifdef LUAT_USE_WLAN
#define LUAT_HEAP_MIN_SIZE (100*1024)
#undef LUAT_HEAP_SIZE
#define LUAT_HEAP_SIZE LUAT_HEAP_MIN_SIZE
#else
#define LUAT_HEAP_MIN_SIZE (128*1024)
#endif

#ifndef LUAT_HEAP_SIZE
#ifdef LUAT_USE_NIMBLE
#define LUAT_HEAP_SIZE (128+16)*1024
#else
#define LUAT_HEAP_SIZE (128+48)*1024
#endif
#endif

#if (LUAT_HEAP_SIZE < LUAT_HEAP_MIN_SIZE)
#undef LUAT_HEAP_SIZE
#define LUAT_HEAP_SIZE LUAT_HEAP_MIN_SIZE
#endif

#if (LUAT_HEAP_SIZE > LUAT_HEAP_MIN_SIZE)
__attribute__((aligned(8))) static uint64_t heap_ext[(LUAT_HEAP_SIZE - LUAT_HEAP_MIN_SIZE) / 8];
#endif

#ifdef LUAT_USE_TLSF
#include "tlsf.h"
tlsf_t luavm_tlsf;
pool_t luavm_tlsf_ext;
#endif

void luat_heap_init(void) {


	// 毕竟sram还是快很多的, 优先sram吧
#ifndef LUAT_USE_TLSF
	bpool((void*)(0x20048000 - LUAT_HEAP_MIN_SIZE), LUAT_HEAP_MIN_SIZE);
#else
	luavm_tlsf = tlsf_create_with_pool((void*)(0x20048000 - LUAT_HEAP_MIN_SIZE), LUAT_HEAP_MIN_SIZE);
#endif

#ifdef LUAT_USE_PSRAM
	char test[] = {0xAA, 0xBB, 0xCC, 0xDD};
	char* psram_ptr = (void*)0x30010000;
	LLOGD("check psram ...");
	memcpy(psram_ptr, test, 4);
	if (memcmp(psram_ptr, test, 4)) {
		LLOGE("psram is enable, but can't access!!");
	}
	else {
		LLOGD("psram is ok");
		memset(psram_ptr, 0, 1024);
		// 存在psram, 加入到内存次, 就不使用系统额外的内存了.
		#ifdef LUAT_USE_TLSF
		luavm_tlsf_ext = tlsf_add_pool(luavm_tlsf, psram_ptr, 4*1024*1024);
		#else
		bpool(psram_ptr, 4*1024*1024); // 如果是8M内存, 改成 8也可以.
		#endif
		luat_main();
		return;
	}
#else
	// 暂时还是放在这里吧, 考虑改到0x20028000之前
	#if (LUAT_HEAP_SIZE > LUAT_HEAP_MIN_SIZE)
#ifndef LUAT_USE_TLSF
	bpool((void*)heap_ext, LUAT_HEAP_SIZE - LUAT_HEAP_MIN_SIZE);
#else
	luavm_tlsf_ext = tlsf_add_pool(luavm_tlsf, heap_ext, LUAT_HEAP_SIZE - LUAT_HEAP_MIN_SIZE);
#endif
	#endif
#endif
}

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

extern unsigned int heap_size_max;
extern unsigned int total_mem_size;
extern size_t __heap_start;
extern size_t __heap_end;
void luat_meminfo_sys(size_t* total, size_t* used, size_t* max_used)
{
#if configUSE_HEAP4
    extern void vPortGetHeapStats( HeapStats_t *pxHeapStats );
    HeapStats_t stat = {0};
    vPortGetHeapStats(&stat);
    *total = (size_t)(&__heap_end) - (size_t)(&__heap_start);
    *max_used = *total - stat.xMinimumEverFreeBytesRemaining;
    *used = (*total) - (stat.xAvailableHeapSpaceInBytes);
#else
    *used = heap_size_max - total_mem_size;
    *max_used = *used;
    *total = heap_size_max;
#endif
}

//------------------------------------------------

//------------------------------------------------
// ---------- 管理 LuaVM所使用的内存----------------
#ifdef LUAT_USE_TLSF
#include "tlsf.h"

// extern tlsf_t luavm_tlsf;
// extern pool_t luavm_tlsf_ext;

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
