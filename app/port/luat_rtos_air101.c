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


int luat_rtos_task_create(luat_rtos_task_handle *task_handle, uint32_t stack_size, uint8_t priority, const char *task_name, luat_rtos_task_entry task_fun, void* user_data, uint16_t event_cout)
{
	if (!task_handle) return -1;
	return xTaskCreate(task_fun,
		task_name,
		stack_size/sizeof(uint32_t),
		user_data,
		priority,
		task_handle	);
}

int luat_rtos_task_delete(luat_rtos_task_handle task_handle)
{
	if (!task_handle) return -1;
	vTaskDelete(task_handle);
	return 0;
}

int luat_rtos_task_suspend(luat_rtos_task_handle task_handle)
{
	if (!task_handle) return -1;
	vTaskSuspend(task_handle);
	return 0;
}

int luat_rtos_task_resume(luat_rtos_task_handle task_handle)
{
	if (!task_handle) return -1;
	vTaskResume(task_handle);
	return 0;
}
uint32_t luat_rtos_task_HighWaterMark(luat_rtos_task_handle task_handle)
{
	if (!task_handle) return -1;
	return uxTaskGetStackHighWaterMark(task_handle);
}
void luat_rtos_task_sleep(uint32_t ms)
{
	vTaskDelay(ms);
}

void luat_task_suspend_all(void)
{
	vTaskSuspendAll();
}
void luat_task_resume_all(void)
{
	xTaskResumeAll();
}

void *luat_get_current_task(void)
{
	return xTaskGetCurrentTaskHandle();
}


int luat_rtos_queue_create(luat_rtos_queue_t *queue_handle, uint32_t item_count, uint32_t item_size)
{
	if (!queue_handle) return -1;
	xQueueHandle pxNewQueue;
	pxNewQueue = xQueueCreate(item_count, item_size);
	if (!pxNewQueue)
		return -1;
	*queue_handle = pxNewQueue;
	return 0;
}

int luat_rtos_queue_delete(luat_rtos_queue_t queue_handle)
{
	if (!queue_handle) return -1;
    vQueueDelete ((xQueueHandle)queue_handle);
	return 0;
}

int luat_rtos_queue_send(luat_rtos_queue_t queue_handle, void *item, uint32_t item_size, uint32_t timeout)
{
	if (!queue_handle || !item) return -1;
	if (tls_get_isr_count()>0)
	{
		BaseType_t pxHigherPriorityTaskWoken;
		if (xQueueSendToBackFromISR(queue_handle, item, &pxHigherPriorityTaskWoken) != pdPASS)
			return -1;
		portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
		return 0;
	}
	else
	{
		if (xQueueSendToBack (queue_handle, item, timeout) != pdPASS)
			return -1;
	}
	return 0;
}

int luat_rtos_queue_recv(luat_rtos_queue_t queue_handle, void *item, uint32_t item_size, uint32_t timeout)
{
	if (!queue_handle || !item)
		return -1;
	BaseType_t yield = pdFALSE;
	if (tls_get_isr_count()>0)
	{
		if (xQueueReceiveFromISR(queue_handle, item, &yield) != pdPASS)
			return -1;
		portYIELD_FROM_ISR(yield);
		return 0;
	}
	else
	{
		if (xQueueReceive(queue_handle, item, timeout) != pdPASS)
			return -1;
	}
	return 0;
}


// LUAT_RET luat_send_event_to_task(void *task_handle, uint32_t id, uint32_t param1, uint32_t param2, uint32_t param3) {
//     return LUAT_ERR_OK;
// }

// LUAT_RET luat_wait_event_from_task(void *task_handle, uint32_t wait_event_id, void *out_event, void *call_back, uint32_t ms) {
//     return LUAT_ERR_OK;
// }

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
	luat_rtos_timer_callback_t rtos_cb;
}air101_timer;

void *luat_create_rtos_timer(void *cb, void *param, void *task_handle){
	air101_timer *luat_timer = luat_heap_malloc(sizeof(air101_timer));
	if (luat_timer == NULL)
		return NULL;
	luat_timer->timer = NULL;
	luat_timer->cb = cb;
	luat_timer->param = param;
	return luat_timer;
}

int luat_start_rtos_timer(void *timer, uint32_t ms, uint8_t is_repeat){
	air101_timer *luat_timer = (air101_timer *)timer;
	tls_os_timer_create(&(luat_timer->timer), luat_timer->cb, luat_timer->param,ms, is_repeat, NULL);
	tls_os_timer_start(luat_timer->timer);
	return 0;
}

void luat_stop_rtos_timer(void *timer){
	air101_timer *luat_timer = (air101_timer *)timer;
	tls_os_timer_stop(luat_timer->timer);
}
void luat_release_rtos_timer(void *timer){
	air101_timer *luat_timer = (air101_timer *)timer;
	tls_os_timer_delete(luat_timer->timer);
	luat_heap_free(luat_timer);
}

int luat_rtos_timer_create(luat_rtos_timer_t *timer_handle) {
	*timer_handle = luat_heap_malloc(sizeof(air101_timer));
	if (*timer_handle != NULL)
		return 0;
	return -1;
}

/**
 * @brief 删除软件定时器
 * 
 * @param timer_handle 定时器句柄
 * @return int =0成功，其他失败
 */
int luat_rtos_timer_delete(luat_rtos_timer_t timer_handle) {
	if (timer_handle != NULL) {
		luat_heap_free(timer_handle);
		return 0;
	}
	return -1;
}

static void rtos_cb(void*ptimer, void* args) {
	air101_timer* timer = (air101_timer*)args;
	if (timer != NULL)
		timer->rtos_cb(timer->param);
}

/**
 * @brief 启动软件定时器
 * 
 * @param timer_handle 定时器句柄
 * @param timeout 超时时间，单位ms，没有特殊值
 * @param repeat 0不重复，其他重复
 * @param callback_fun 定时时间到后的回调函数
 * @param user_param 回调函数时的最后一个输入参数
 * @return int =0成功，其他失败
 */
int luat_rtos_timer_start(luat_rtos_timer_t timer_handle, uint32_t timeout, uint8_t repeat, luat_rtos_timer_callback_t callback_fun, void *user_param) {
	air101_timer* timer = (air101_timer*)timer_handle;
	if (timer->timer != NULL) {
		tls_os_timer_stop(timer->timer);
		tls_os_timer_delete(timer->timer);
		timer->timer = NULL;
	}
	timer->param = user_param;
	timer->rtos_cb = callback_fun;
	tls_os_timer_create(&(timer->timer), rtos_cb, timer, timeout, repeat, NULL);
	tls_os_timer_start(timer->timer);
	return 0;
}
/**
 * @brief 停止软件定时器
 * 
 * @param timer_handle 定时器句柄
 * @return int =0成功，其他失败
 */
int luat_rtos_timer_stop(luat_rtos_timer_t timer_handle) {
	air101_timer* timer = (air101_timer*)timer_handle;
	tls_os_timer_stop(timer->timer);
	tls_os_timer_delete(timer->timer);
	timer->timer = NULL;
	return 0;
}
