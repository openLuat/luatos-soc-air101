/***************************************************************************** 
* 
* File Name : wm_mem.c
* 
* Description: memory manager Module 
* 
* Copyright (c) 2014 Winner Micro Electronic Design Co., Ltd. 
* All rights reserved. 
* 
* Author : dave
* 
* Date : 2014-6-12 
*****************************************************************************/ 

#include "wm_osal.h"
#include "wm_mem.h"
#include "list.h"
#include <string.h>
#include "FreeRTOS.h"


#if configUSE_HEAP3

void *pvPortMalloc( u32 xWantedSize );
void vPortFree( void *pv );
void *vPortRealloc(void *pv, u32 size);
void *vPortCalloc(u32 length, u32 size);

void * mem_alloc_debug(u32 size) {
    return pvPortMalloc(size);
}
void mem_free_debug(void *p) {
    vPortFree(p);
}
void * mem_realloc_debug(void *mem_address, u32 size) {
    return vPortRealloc(mem_address, size);
}
void *mem_calloc_debug(u32 length, u32 size) {
    return vPortCalloc(length, size);
}

#else

extern u8 tls_get_isr_count(void);
bool         memory_manager_initialized = false;
/**
 * This mutex is used to synchronize the list of allocated
 * memory blocks. This is a debug version only feature
 */
tls_os_sem_t    *mem_sem;

extern u32 total_mem_size;
void * mem_alloc_debug(u32 size) {
    u32 cpu_sr = 0;
    u32 *buffer = NULL;
	u32 length = size;


	//printf("size:%d\n", size);
    if (!memory_manager_initialized) {
        tls_os_status_t os_status;
        memory_manager_initialized = true;
        //
        // NOTE: If two thread allocate the very first allocation simultaneously
        // it could cause double initialization of the memory manager. This is a
        // highly unlikely scenario and will occur in debug versions only.
        //
        os_status = tls_os_sem_create(&mem_sem, 1);
        if(os_status != TLS_OS_SUCCESS)
            printf("mem_alloc_debug: tls_os_sem_create mem_sem error\r\n");
    }
	tls_os_sem_acquire(mem_sem, 0);
    cpu_sr = tls_os_set_critical();
    buffer = (u32*)malloc(length);
    tls_os_release_critical(cpu_sr); 
	tls_os_sem_release(mem_sem);
	return buffer;
}

extern const uint32_t __ram_end;
void mem_free_debug(void *p) {
    if (((uint32_t)p >= (uint32_t)(&__heap_start)) && ((uint32_t)p <= (uint32_t)(&__ram_end)))
	{
        u32 cpu_sr = 0;
        tls_os_sem_acquire(mem_sem, 0);
        cpu_sr = tls_os_set_critical();
        free(p);
        tls_os_release_critical(cpu_sr);	
        tls_os_sem_release(mem_sem);
    }
}
void * mem_realloc_debug(void *mem_address, u32 size) {
    u32 * mem_re_addr = NULL;
    u32 cpu_sr = 0;
	u32 length = size;
	tls_os_sem_acquire(mem_sem, 0);
	cpu_sr = tls_os_set_critical();
	mem_re_addr = realloc(mem_address, length);
	tls_os_release_critical(cpu_sr);
	tls_os_sem_release(mem_sem);
    return mem_re_addr;
}
void *mem_calloc_debug(u32 length, u32 size) {
    u32 cpu_sr = 0;
    u32 *buffer = NULL;
	//u32 length = 0;
	tls_os_sem_acquire(mem_sem, 0);
    cpu_sr = tls_os_set_critical();
    buffer = (u32*)calloc(length, size);
    tls_os_release_critical(cpu_sr); 
	tls_os_sem_release(mem_sem);
    return buffer;
}

#endif



