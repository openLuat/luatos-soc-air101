#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_malloc.h"
#include "wm_osal.h"
static tls_os_queue_t *queue = NULL;

void luat_msgbus_init(void)
{
    if (queue == NULL)
    {
        tls_os_queue_create(&queue, 256);
    }
}
uint32_t luat_msgbus_put(rtos_msg_t *msg, size_t timeout)
{
    if (queue == NULL)
    {
        return 1;
    }
    rtos_msg_t* dst = (rtos_msg_t*)luat_heap_malloc(sizeof(rtos_msg_t));
    memcpy(dst, msg, sizeof(rtos_msg_t));
    int ret = tls_os_queue_send(queue, (void *)dst, sizeof(rtos_msg_t));
    return ret;
}
uint32_t luat_msgbus_get(rtos_msg_t *_msg, size_t timeout)
{
    if (queue == NULL)
    {
        return 1;
    }
    void* msg;
    int ret = tls_os_queue_receive(queue, (void **)&msg, sizeof(rtos_msg_t), timeout);
    if (ret == TLS_OS_SUCCESS) {
        memcpy(_msg, (rtos_msg_t*)msg, sizeof(rtos_msg_t));
        luat_heap_free(msg);
        return 0;
    }
    return -1;
}
uint32_t luat_msgbus_freesize(void)
{
    if (queue == NULL)
    {
        return 1;
    }
    return 1;
}
