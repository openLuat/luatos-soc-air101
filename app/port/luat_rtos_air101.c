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
#include "luat_malloc.h"

#include "wm_osal.h"

#define LUAT_LOG_TAG "luat.rtos"
#include "luat_log.h"

int luat_thread_start(luat_thread_t* thread){
	uint32_t* task_start_stk = luat_heap_malloc((thread->stack_size)*sizeof(uint32_t));
	return tls_os_task_create(NULL, thread->name,thread->entry,NULL,(void *)task_start_stk,(thread->stack_size)*sizeof(uint32_t),thread->priority,0);
}

LUAT_RET luat_thread_stop(luat_thread_t* thread) {
    return LUAT_ERR_FAIL;
}

LUAT_RET luat_thread_delete(luat_thread_t* thread) {
    return LUAT_ERR_FAIL;
}

LUAT_RET luat_queue_create(luat_rtos_queue_t* queue, size_t msgcount, size_t msgsize) {
	queue->userdata = (tls_os_queue_t *)luat_heap_malloc(sizeof(tls_os_queue_t));
	return tls_os_queue_create((tls_os_queue_t **)&(queue->userdata), msgcount);
}

LUAT_RET luat_queue_send(luat_rtos_queue_t*   queue, void* msg,  size_t msg_size, size_t timeout) {
	return tls_os_queue_send((tls_os_queue_t *)(queue->userdata), msg, msg_size);
}

LUAT_RET luat_queue_recv(luat_rtos_queue_t*   queue, void* msg, size_t msg_size, size_t timeout) {
	return tls_os_queue_receive((tls_os_queue_t *)(queue->userdata), (void **) &msg, msg_size, timeout);
}

LUAT_RET luat_queue_reset(luat_rtos_queue_t*   queue) {
	return tls_os_queue_flush((tls_os_queue_t *)(queue->userdata));
}

LUAT_RET luat_queue_delete(luat_rtos_queue_t*   queue) {
	tls_os_queue_delete((tls_os_queue_t *)(queue->userdata));
	luat_heap_free(queue->userdata);
	return 0;
}


int luat_sem_create(luat_sem_t* semaphore){
	semaphore->userdata = (tls_os_sem_t *)luat_heap_malloc(sizeof(tls_os_sem_t));
	return tls_os_sem_create((tls_os_sem_t **)&(semaphore->userdata),semaphore->value);
}

int luat_sem_delete(luat_sem_t* semaphore){
	tls_os_sem_delete((tls_os_sem_t *)(semaphore->userdata));
	luat_heap_free(semaphore->userdata);
	return 0;
}

int luat_sem_take(luat_sem_t* semaphore,uint32_t timeout){
	return tls_os_sem_acquire((tls_os_sem_t *)(semaphore->userdata),timeout);
}

int luat_sem_release(luat_sem_t* semaphore){
    return tls_os_sem_release((tls_os_sem_t *)(semaphore->userdata));
}
