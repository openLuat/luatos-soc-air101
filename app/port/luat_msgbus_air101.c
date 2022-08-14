#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_malloc.h"
#include "wm_osal.h"
#include "FreeRTOS.h"
#include "rtosqueue.h"

#define QUEUE_MAX_SIZE (1024)

static xQueueHandle queue = NULL;
static u8 queue_buff[sizeof(rtos_msg_t) * QUEUE_MAX_SIZE];

void luat_msgbus_init(void)
{
    if (queue == NULL)
    {
        queue = xQueueCreateExt(queue_buff, QUEUE_MAX_SIZE, sizeof(rtos_msg_t));
    }
}
uint32_t luat_msgbus_put(rtos_msg_t *msg, size_t timeout)
{
    if (queue == NULL)
    {
        return 1;
    }
    portBASE_TYPE pxHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(queue, msg, &pxHigherPriorityTaskWoken);
    return 0;
}
uint32_t luat_msgbus_get(rtos_msg_t *msg, size_t timeout)
{
    if (queue == NULL)
    {
        return 1;
    }
    xQueueReceive(queue, msg, timeout);
    return 0;
}
uint32_t luat_msgbus_freesize(void)
{
    return 1;
}
