
// 这个文件包含 系统heap和lua heap的默认实现


#include <stdlib.h>
#include <string.h>//add for memset
#include "bget.h"
#include "luat_malloc.h"
#include "FreeRTOS.h"

#define LUAT_LOG_TAG "heap"
#include "luat_log.h"
#include "wm_mem.h"
#include "wm_ram_config.h"

void* __wrap_malloc(size_t len);
void  __wrap_free(void* ptr);
void* __wrap_calloc(size_t itemCount, size_t itemSize);
void* __wrap_zalloc(size_t size);
void* __wrap_realloc(void*ptr, size_t len);

// 内存分2个区域, P1 和 P2
// 其中P1是160k, 同时也是sys的主要区域, 栈内存也必须分配在这里
// P2默认是WIFI和外设的内存, 不启用WIFI时全部归lua使用, 启用WIFI时部分使用
#ifdef LUAT_USE_WLAN
// 与tls_wifi_mem_cfg正相关, 假设tx是7, rx是3
// (7+3)*2*1638+4096 = 36k , 然后lua可用 128 - 36 = 92
// (6+4)*2*1638+4096 = 36k , 然后lua可用 128 - 36 = 92
// (7+7)*2*1638+4096 = 49k , 然后lua可用 128 - 49 = 79
#define LUAT_HEAP_P2_SIZE (92*1024)
#else
#define LUAT_HEAP_P2_SIZE (128*1024)
#endif

#if defined(LUAT_USE_NIMBLE) || defined(LUAT_USE_TLS)
#ifdef LUAT_USE_WLAN
#define LUAT_HEAP_P1_SIZE 0
#else
#define LUAT_HEAP_P1_SIZE 24*1024
#endif
#else
#ifdef LUAT_USE_WLAN
#define LUAT_HEAP_P1_SIZE 24* 1024
#else
#define LUAT_HEAP_P1_SIZE 48*1024
#endif
#endif

#if (LUAT_HEAP_P1_SIZE > 0)
#ifndef LUAT_USE_PSRAM
__attribute__((aligned(8))) static uint64_t heap_ext[(LUAT_HEAP_P1_SIZE) / 8];
#else
static uint64_t* heap_ext;
#endif
#endif

#ifdef LUAT_USE_TLSF
#include "tlsf.h"
tlsf_t luavm_tlsf;
pool_t luavm_tlsf_ext;
#endif

#ifdef LUAT_USE_PROFILER
#include "luat_profiler.h"
extern int luat_profiler_memdebug;
extern luat_profiler_mem_t profiler_memregs[];
#endif

extern uint32_t __ram_end;
#ifdef LUAT_USE_PSRAM
#ifdef LUAT_USE_PSRAM_1M
const size_t psram_size = 1 * 1024 * 1024;
#elif defined(LUAT_USE_PSRAM_2M)
const size_t psram_size = 2 * 1024 * 1024;
#elif defined(LUAT_USE_PSRAM_4M)
const size_t psram_size = 4 * 1024 * 1024;
#else
const size_t psram_size = 8 * 1024 * 1024;
#endif
size_t psram_lua_size = 0;
size_t psram_sys_size = 0;
#include "luat_bget.h"
luat_bget_t psram_bget;
#endif

void luat_heap_init(void) {
	// 毕竟sram还是快很多的, 优先sram吧
    //LLOGD("VM MEM P1 %08X P2 %08X", LUAT_HEAP_P1_SIZE, LUAT_HEAP_P2_SIZE);
#if (LUAT_HEAP_P1_SIZE > 0)
    if (heap_ext) {
    #ifndef LUAT_USE_TLSF
	    bpool((void*)heap_ext, LUAT_HEAP_P1_SIZE);
    #else
	    luavm_tlsf = tlsf_create_with_pool((void*)heap_ext, LUAT_HEAP_P1_SIZE);
    #endif
    }
#endif

#ifdef LUAT_USE_PSRAM
	char* psram_ptr = (void*)0x30010000;
    LLOGD("PSRAM size %dkb", psram_size / 1024);
	if (psram_size == 0) {
		// LLOGE("psram is enable, but can't access!!");
        // #if (LUAT_HEAP_P1_SIZE > 0)
        //     heap_ext = tls_mem_alloc(LUAT_HEAP_P1_SIZE);
        //     bpool((void*)heap_ext, LUAT_HEAP_P1_SIZE);
        // #endif
	}
	else {
		// LLOGD("psram is ok");
		// 存在psram, 加入到内存次, 就不使用系统额外的内存了.
        if (psram_size >= 6 * 1024 * 1024) {
            psram_lua_size = 5*1024*1024;
        }
        else if (psram_size >= 4 * 1024 * 1024) {
            psram_lua_size = 3*1024*1024;
        }
        else if (psram_size >= 2 * 1024 * 1024) {
            psram_lua_size = 1*1024*1024;
        }
        else {
            psram_lua_size = 512*1024;
        }
        psram_sys_size = psram_size - psram_lua_size;
        LLOGD("PSRAM 内存分配 --> Lua %dkb Sys %dkb", psram_lua_size / 1024, psram_sys_size / 1024);
		#ifdef LUAT_USE_TLSF
		luavm_tlsf_ext = tlsf_add_pool(luavm_tlsf, psram_ptr, psram_lua_size);
		#else
        bpool(psram_ptr, psram_lua_size); // 如果是8M内存, 改成 8也可以
		#endif
        if (psram_sys_size > 0) {
            luat_bget_init(&psram_bget);
            luat_bpool(&psram_bget, psram_ptr + psram_lua_size, psram_sys_size);
            char* tmp = luat_bget(&psram_bget, 1024);
            if (tmp) {
                luat_brel(&psram_bget, tmp);
            }
        }
		return;
	}
#else
    // LLOGD("__ram_end %08X", (uint32_t)&__ram_end);
    char* ptr = (void*)(0x20028000);
    size_t heap2_size = LUAT_HEAP_P2_SIZE;
    #if defined(LUAT_USE_NIMBLE) && defined(LUAT_USE_TLS) && defined(LUAT_USE_WLAN)
    heap2_size -= 32*1024;
    ptr += 32*1024;
    #endif
    #if defined(LUAT_USE_SPI_SLAVE)
    heap2_size -= SLAVE_HSPI_MAX_SIZE;
    ptr += SLAVE_HSPI_MAX_SIZE;
    #endif

#ifndef LUAT_USE_TLSF
	bpool(ptr, heap2_size);
#else
	luavm_tlsf_ext = tlsf_add_pool(ptr, heap2_size);
#endif
	#endif
}

//------------------------------------------------
//  管理系统内存
#ifdef LUAT_USE_PROFILER_XXX
void* luat_heap_malloc(size_t len) {
    void* ptr = __wrap_malloc(len);
    printf("luat_heap_malloc %p %d\n", ptr, len);
    return ptr;
}

void luat_heap_free(void* ptr) {
    if (ptr == NULL)
        return;
    printf("luat_heap_free %p\n", ptr);
    __wrap_free(ptr);
}

void* luat_heap_realloc(void* ptr, size_t len) {
    void* nptr = __wrap_realloc(ptr, len);
    printf("luat_heap_realloc %p %d %p\n", ptr, len, nptr);
    return nptr;
}

void* luat_heap_calloc(size_t count, size_t _size) {
    void* ptr = __wrap_calloc(count, _size);
    printf("luat_heap_calloc %p\n", ptr);
    return ptr;
}
#else
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
#endif

extern unsigned int heap_size_max;
extern unsigned int total_mem_size;
extern unsigned int min_free_size;
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
    *max_used = heap_size_max - min_free_size;
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
    (void)ud;
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

#include "wm_include.h"
#include "FreeRTOS.h"

#include "stdio.h"

#ifdef LUAT_USE_PROFILER

void* __wrap_malloc(size_t len) {
    #ifdef LUAT_USE_PROFILER
    void* ptr = pvPortMalloc(len);
    if (luat_profiler_memdebug) {
        // printf("malloc %d %p\n", len, ptr);
        if (ptr == NULL)
            return NULL;
        for (size_t i = 0; i < LUAT_PROFILER_MEMDEBUG_ADDR_COUNT; i++)
        {
            if (profiler_memregs[i].addr == 0) {
                profiler_memregs[i].addr = (uint32)ptr;
                profiler_memregs[i].len = len;
                break;
            }
        }
        
    }
    return ptr;
    #else
    return pvPortMalloc(len);
    #endif
}

void __wrap_free(void* ptr) {
    if (ptr == NULL)
        return;
    #ifdef LUAT_USE_PROFILER
    // if (luat_profiler_memdebug)
    //     printf("free %p\n", ptr);
    #endif
    u32 addr = (u32)ptr;
    if (addr >= 0x20000000 && addr <= 0x40000000) {
        // printf("free %p\n", ptr);
        vPortFree(ptr);
        #ifdef LUAT_USE_PROFILER
            for (size_t i = 0; i < LUAT_PROFILER_MEMDEBUG_ADDR_COUNT; i++)
            {
                if (profiler_memregs[i].addr == addr) {
                    profiler_memregs[i].len = 0;
                    profiler_memregs[i].addr = 0;
                    break;
                }
            }
        #endif
    }
}

void *pvPortRealloc( void *pv, size_t xWantedSize );
void* __wrap_realloc(void*ptr, size_t len) {
    #ifdef LUAT_USE_PROFILER
    void* newptr = pvPortRealloc(ptr, len);
    if (luat_profiler_memdebug && newptr) {
        // printf("realloc %p %d %p\n", ptr, len, newptr);
        uint32_t addr = (uint32_t)ptr;
        uint32_t naddr = (uint32_t)newptr;
        if (ptr == newptr) {
            // 相同内存地址, 只是扩容了
            for (size_t i = 0; i < LUAT_PROFILER_MEMDEBUG_ADDR_COUNT; i++)
            {
                if (profiler_memregs[i].addr == addr) {
                    profiler_memregs[i].len = len;
                    addr = 0;
                    break;
                }
            }
            if (0 != addr) {
                for (size_t i = 0; i < LUAT_PROFILER_MEMDEBUG_ADDR_COUNT; i++)
                {
                    if (profiler_memregs[i].addr == 0) {
                        profiler_memregs[i].addr = addr;
                        profiler_memregs[i].len = len;
                        break;
                    }
                }
            }
        }
        else {
            if (ptr == NULL) {
                for (size_t i = 0; i < LUAT_PROFILER_MEMDEBUG_ADDR_COUNT; i++)
                {
                    if (profiler_memregs[i].addr == 0) {
                        profiler_memregs[i].addr = naddr;
                        profiler_memregs[i].len = len;
                        break;
                    }
                }
            }
            else {
                for (size_t i = 0; i < LUAT_PROFILER_MEMDEBUG_ADDR_COUNT; i++)
                {
                    if (profiler_memregs[i].addr == addr) {
                        profiler_memregs[i].addr = 0;
                        profiler_memregs[i].len = 0;
                        break;
                    }
                }
                for (size_t i = 0; i < LUAT_PROFILER_MEMDEBUG_ADDR_COUNT; i++)
                {
                    if (profiler_memregs[i].addr == 0) {
                        profiler_memregs[i].addr = naddr;
                        profiler_memregs[i].len = len;
                        break;
                    }
                }
            }
        }
    }
    return newptr;
    #endif
    return pvPortRealloc(ptr, len);
}

void* __wrap_calloc(size_t itemCount, size_t itemSize) {
    void* ptr = pvPortMalloc(itemCount * itemSize);
    #ifdef LUAT_USE_PROFILER
    if (luat_profiler_memdebug) {
        // printf("calloc %p %d\n", ptr, itemCount * itemSize);
        if (ptr) {
            for (size_t i = 0; i < LUAT_PROFILER_MEMDEBUG_ADDR_COUNT; i++)
            {
                if (profiler_memregs[i].addr == 0) {
                    profiler_memregs[i].addr = (uint32_t)ptr;
                    profiler_memregs[i].len = itemCount * itemSize;
                    break;
                }
            }
        }
    }
    #endif
    if (ptr == NULL)
        return NULL;
    memset(ptr, 0, itemCount * itemSize);
    return ptr;
}

void* __wrap_zalloc(size_t size) {
    void* ptr = pvPortMalloc(size);
    #ifdef LUAT_USE_PROFILER
    if (luat_profiler_memdebug) {
        // printf("zalloc %p %d\n", ptr, size);
        if (ptr) {
            for (size_t i = 0; i < LUAT_PROFILER_MEMDEBUG_ADDR_COUNT; i++)
            {
                if (profiler_memregs[i].addr == 0) {
                    profiler_memregs[i].addr = (uint32_t)ptr;
                    profiler_memregs[i].len = size;
                    break;
                }
            }
        }
    }
    #endif
    if (ptr == NULL)
        return NULL;
    memset(ptr, 0, size);
    return ptr;
}
#else

// 标准实现
void* __wrap_malloc(size_t len) {
    return pvPortMalloc(len);
}

void __wrap_free(void* ptr) {
    if (ptr == NULL)
        return;
    u32 addr = (u32)ptr;
    if (addr >= 0x20000000 && addr <= 0x40000000) {
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
#endif

#ifdef LUAT_USE_PSRAM
#include "luat_mem.h"

void* luat_heap_opt_malloc(LUAT_HEAP_TYPE_E type,size_t len){
    if (type == LUAT_HEAP_PSRAM && psram_sys_size) {
        return luat_bget(&psram_bget, len);
    }
    return luat_heap_malloc(len);
}

void luat_heap_opt_free(LUAT_HEAP_TYPE_E type,void* ptr){
    if (type == LUAT_HEAP_PSRAM && psram_sys_size && ((uint32_t)ptr) > 0x30010000) {
        return luat_brel(&psram_bget, ptr);
    }
    luat_heap_free(ptr);
}

void* luat_heap_opt_realloc(LUAT_HEAP_TYPE_E type,void* ptr, size_t len){
    if (type == LUAT_HEAP_PSRAM && psram_sys_size &&  (!ptr || ((uint32_t)ptr) > 0x30010000)) {
        return luat_bgetr(&psram_bget, ptr, len);
    }
    return luat_heap_realloc(ptr, len);
}

void* luat_heap_opt_calloc(LUAT_HEAP_TYPE_E type,size_t count, size_t size){
    if (type == LUAT_HEAP_PSRAM && psram_sys_size) {
        return luat_bgetz(&psram_bget, count*size);
    }
    return luat_heap_opt_zalloc(type,count*size);
}

void* luat_heap_opt_zalloc(LUAT_HEAP_TYPE_E type,size_t size){
    void *ptr = luat_heap_opt_malloc(type,size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void luat_meminfo_opt_sys(LUAT_HEAP_TYPE_E type,size_t* total, size_t* used, size_t* max_used){
    if (type == LUAT_HEAP_PSRAM){
        if (psram_sys_size > 0) {
            long curalloc, totfree, maxfree;
	        unsigned long nget, nrel;
	        luat_bstats(&psram_bget, &curalloc, &totfree, &maxfree, &nget, &nrel);
	        *used = curalloc;
	        *max_used = luat_bstatsmaxget(&psram_bget);
            *total = curalloc + totfree;
        }
        else {
            *total = 0;
            *used = 0;
            *max_used = 0;
        }
    }else
        luat_meminfo_sys(total, used, max_used);
}

#endif

