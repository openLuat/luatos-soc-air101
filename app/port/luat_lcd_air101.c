#include "wm_include.h"
#include "sdio_spi_driver.h"
#include "wm_sdio_host.h"
#include "wm_cpu.h"
#include "wm_dma.h"
#include "wm_pmu.h"
#include <string.h>

#include "luat_base.h"
#include "luat_mem.h"
#include "luat_lcd.h"
#include "luat_gpio.h"
#include "luat_rtos.h"

#include <os/os.h>

#define LUAT_LOG_TAG "lcd"
#include "luat_log.h"


static inline void sdio_lcd_set_pin(uint32_t pin, bool level) {
    if (pin < WM_IO_PB_00){
        tls_bitband_write(HR_GPIOA_DATA, pin, level);
    }else{
        tls_bitband_write(HR_GPIOB_DATA, pin - WM_IO_PB_00, level);
    }
}

int luat_sdio_lcd_write_cmd_data(luat_lcd_conf_t* conf,const uint8_t cmd, const uint8_t *data, uint8_t data_len){

    sdio_lcd_set_pin(conf->pin_dc, 0);
    sdio_lcd_set_pin(conf->lcd_cs_pin, 0);
    sdio_spi_put(cmd);

    sdio_lcd_set_pin(conf->pin_dc, 1);
    for (size_t i = 0; i < data_len; i++)
        sdio_spi_put( *data++);

    sdio_lcd_set_pin(conf->lcd_cs_pin, 1);
    return 0;
}

int luat_sdio_lcd_draw(luat_lcd_conf_t* conf, int16_t x1, int16_t y1, int16_t x2, int16_t y2, luat_color_t* color){
    uint32_t size = (x2 - x1 + 1) * (y2 - y1 + 1);
#if LCD_ASYNC_SEND
    wait_for_prev_frame_completion();
#endif
    luat_lcd_set_address(conf, x1, y1, x2, y2);

    sdio_lcd_set_pin(conf->lcd_cs_pin, 0);
    // write_sdio_spi_dma((u32 *)color, size * 2);
    // sdio_lcd_set_pin(conf->lcd_cs_pin, 1);

#if LCD_ASYNC_SEND
    lcd_dma_queue_write(x1, y1, x2, y2, (u32 *)color, size * 2);
#else
    write_sdio_spi_dma((u32 *)color, size * 2);
    sdio_lcd_set_pin(conf->lcd_cs_pin, 1);
#endif

    return 0;
}

void luat_lcd_IF_init(luat_lcd_conf_t* conf){
    tls_gpio_cfg(conf->lcd_cs_pin,  WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_FLOATING);

    tls_gpio_write(conf->lcd_cs_pin, 1);
    tls_gpio_write(conf->pin_dc, 1);

    init_sdio_spi_mode(conf->bus_speed);
#if LCD_ASYNC_SEND
    // 创建数据发送任务
    lcd_task_create(conf);
#endif
    conf->opts->write_cmd_data = luat_sdio_lcd_write_cmd_data;
    conf->opts->lcd_draw = luat_sdio_lcd_draw;
}


#define LCD_TASK_STACK_SIZE 1024
#define LCD_TASK_PRORITY 55

#define ONCE_DMA_MAX_LEN                  65532

#define USER_TASK_QUEUE_START             0x01
#define USER_TASK_QUEUE_SEND              0x02
#define USER_TASK_QUEUE_STOP              0x03
#define USER_TASK_QUEUE_SIZE              0x03

static tls_os_queue_t   *user_task_queue = NULL;
static OS_STK LcdTaskStk[LCD_TASK_STACK_SIZE/4];

// 用于标记上一帧发送完成状态的变量
static volatile bool prev_frame_completion_flag = true;

static u32  g_data_len    = 0;
static u32  g_data_offset = 0;
static u32 *g_data_addr   = NULL;

static u32  g_send_len    = 0;
static u32 *g_send_addr   = NULL;



// 等待上一帧发送完成
void wait_for_prev_frame_completion()
{
    // extern int delay_us();
    // tls_os_time_delay(10);
    while(!prev_frame_completion_flag){
        tls_os_time_delay(1);
        // delay_us(1);
    }
}

int lcd_dma_queue_write(u16 x_start, u16 y_start, u16 x_end, u16 y_end, u32* data, u32 len)
{
    // printf("---> len :%d\n", len);
    prev_frame_completion_flag = false;
    g_data_offset  = 0;
    g_data_len     = len;
    g_data_addr    = data;
    tls_os_queue_send(user_task_queue, (void *)USER_TASK_QUEUE_START, 0);
    return 0;
}

static int fill_buf()
{
    if(g_data_len == 0){
        return 1;
    }
    g_send_len = g_data_len;
    if(g_send_len > ONCE_DMA_MAX_LEN){
        g_send_len = ONCE_DMA_MAX_LEN;
    }
    g_send_addr = &g_data_addr[g_data_offset];
    g_data_offset += g_send_len/4;
    g_data_len    -= g_send_len;
    return 0;
}
    

static void lcd_task_start(void *data)
{
    void *msg;
    tls_os_status_t ret;
    luat_lcd_conf_t* conf = data;

    for ( ; ; )
    {
        // 接收队里消息
        ret= tls_os_queue_receive(user_task_queue, (void **)&msg, 0, 0);
        if (ret == TLS_OS_SUCCESS)
        {
            switch((u32)msg)
            {
            case USER_TASK_QUEUE_START: // 
            {
                // printf("---> start msg\r\n");
                // time_ms = tls_os_get_time();
                // printf("---> use %d ms\n", tls_os_get_time() - time_ms);
                tls_os_queue_send(user_task_queue, (void *)USER_TASK_QUEUE_SEND, 0);
                break;
            }
            case USER_TASK_QUEUE_SEND: //
            {
                // printf("---> 0start msg\r\n");
                if(fill_buf() == 1){
                    tls_os_queue_send(user_task_queue, (void *)USER_TASK_QUEUE_STOP, 0);
                    break;
                }
                // printf("---> 1start msg\r\n");
                sdio_dma_sem_acquire();
                // printf("---> 2start msg\r\n");
                wait_sdio_prev_data_completion();
                // printf("---> 3start msg\r\n");
                sdio_spi_dma_write_async((u32 *)g_send_addr, g_send_len);
                tls_os_queue_send(user_task_queue, (void *)USER_TASK_QUEUE_SEND, 0);
                break;
            }
            case USER_TASK_QUEUE_STOP: // 
            {
                // printf("---> stop msg\r\n");
                wait_sdio_prev_data_completion();
                sdio_lcd_set_pin(conf->lcd_cs_pin, 1);
                prev_frame_completion_flag = true;
                break;
            }
            default:
                break;
            }
        }
    }
}

void lcd_task_create(void* arg)
{
    // 创建消息队列
    tls_os_queue_create(&user_task_queue, USER_TASK_QUEUE_SIZE);
    tls_os_task_create(NULL, 
                      "lcd_task",
                       lcd_task_start,
                       arg,
                       (void *)LcdTaskStk,          /* task's stack start address */
                       LCD_TASK_STACK_SIZE, /* task's stack size, unit:byte */
                       LCD_TASK_PRORITY,
                       0);
}

