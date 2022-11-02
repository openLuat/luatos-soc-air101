/*
 * Copyright (c) 2022 OpenLuat & AirM2M
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "luat_base.h"
#include "luat_rtos.h"
#include "c_common.h"
#include "luat_malloc.h"

#include "wm_osal.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#define LUAT_LOG_TAG "luat.rtos"
#include "luat_log.h"

int luat_thread_start(luat_thread_t* thread){
	thread->task_stk = luat_heap_malloc((thread->stack_size)*sizeof(uint32_t));
	return tls_os_task_create(NULL, thread->name,(void (*)(void *param))thread->entry,NULL,thread->task_stk,(thread->stack_size)*sizeof(uint32_t),thread->priority,0);
}

LUAT_RET luat_thread_stop(luat_thread_t* thread) {
    return LUAT_ERR_FAIL;
}

LUAT_RET luat_thread_delete(luat_thread_t* thread) {
	return LUAT_ERR_FAIL;
}



void *luat_queue_create(size_t msgcount, size_t msgsize){
	tls_os_queue_t *queue = (tls_os_queue_t *)luat_heap_malloc(sizeof(tls_os_queue_t));
	tls_os_queue_create(&queue, msgcount);
	return queue;
}

LUAT_RET luat_queue_send(void*   queue, void* msg,  size_t msg_size, size_t timeout){
	return tls_os_queue_send((tls_os_queue_t *)queue, msg, msg_size);
}

LUAT_RET luat_queue_recv(void*   queue, void* msg, size_t msg_size, size_t timeout){
	return tls_os_queue_receive((tls_os_queue_t *)queue, (void **) &msg, msg_size, timeout);
}

LUAT_RET luat_queue_reset(void*   queue){
	return tls_os_queue_flush((tls_os_queue_t *)queue);
}

LUAT_RET luat_queue_delete(void*   queue){
	return tls_os_queue_delete((tls_os_queue_t *)queue);
}

LUAT_RET luat_queue_free(void*   queue){
	luat_heap_free((tls_os_queue_t *)queue);
	return 0;
}

LUAT_RET luat_send_event_to_task(void *task_handle, uint32_t id, uint32_t param1, uint32_t param2, uint32_t param3) {
    return LUAT_ERR_OK;
}

LUAT_RET luat_wait_event_from_task(void *task_handle, uint32_t wait_event_id, void *out_event, void *call_back, uint32_t ms) {
    return LUAT_ERR_OK;
}

// int luat_sem_create(luat_sem_t* semaphore){
// 	semaphore->userdata = (tls_os_sem_t *)luat_heap_malloc(sizeof(tls_os_sem_t));
// 	return tls_os_sem_create((tls_os_sem_t **)&(semaphore->userdata),semaphore->value);
// }

// int luat_sem_delete(luat_sem_t* semaphore){
// 	tls_os_sem_delete((tls_os_sem_t *)(semaphore->userdata));
// 	luat_heap_free(semaphore->userdata);
// 	return 0;
// }

// int luat_sem_take(luat_sem_t* semaphore,uint32_t timeout){
// 	return tls_os_sem_acquire((tls_os_sem_t *)(semaphore->userdata),timeout);
// }

// int luat_sem_release(luat_sem_t* semaphore){
//     return tls_os_sem_release((tls_os_sem_t *)(semaphore->userdata));
// }

typedef struct luat_rtos_timer {
    tls_os_timer_t *timer;
	void *cb;
	void *param;
}luat_rtos_timer_t;

void *luat_create_rtos_timer(void *cb, void *param, void *task_handle){
	luat_rtos_timer_t *luat_timer = luat_heap_malloc(sizeof(luat_rtos_timer_t));
	luat_timer->cb = cb;
	luat_timer->param = param;
	return luat_timer;
}

int luat_start_rtos_timer(void *timer, uint32_t ms, uint8_t is_repeat){
	luat_rtos_timer_t *luat_timer = (luat_rtos_timer_t *)timer;
	tls_os_timer_create(&(luat_timer->timer), luat_timer->cb, luat_timer->param,ms, is_repeat, NULL);
	tls_os_timer_start(luat_timer->timer);
	return 0;
}

void luat_stop_rtos_timer(void *timer){
	luat_rtos_timer_t *luat_timer = (luat_rtos_timer_t *)timer;
	tls_os_timer_stop(luat_timer->timer);
}
void luat_release_rtos_timer(void *timer){
	luat_rtos_timer_t *luat_timer = (luat_rtos_timer_t *)timer;
	tls_os_timer_delete(luat_timer->timer);
	luat_heap_free(luat_timer);
}

void luat_task_suspend_all(void){
	vTaskSuspendAll();
}

void luat_task_resume_all(void){
	xTaskResumeAll();
}
