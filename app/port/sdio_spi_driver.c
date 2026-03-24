#include "sdio_spi_driver.h"
#include "wm_sdio_host.h"
#include "wm_cpu.h"
#include "wm_dma.h"
#include "wm_pmu.h"
#include <string.h>

#define LCD_TASK_QUEUE_FILL              0x01
#define LCD_TASK_QUEUE_SIZE              0x01
#define LCD_TASK_STACK_SIZE              1024*4        // 4k
#define LCD_TASK_PRORITY                 55

static tls_os_queue_t *user_task_queue = NULL;
static OS_STK LcdTaskStk[LCD_TASK_STACK_SIZE/4];


static u8   sdio_spi_dma_channel  = 0xFF;
static u32  sdio_spi_dma_buf_size = 0;
static u32 *sdio_spi_dma_buf_addr = NULL;

// #if (USE_LCD_TASK == 0)
// static u32 *sdio_spi_dma_temp_buf = NULL;
// #endif

static tls_os_sem_t *sdio_spi_dma_ready_flag = NULL;

volatile bool sdio_spi_dma_ready = true;


void init_sdio_spi_mode(u32 clk)
{
    if(LCD_SCL == WM_IO_PA_09){
        tls_io_cfg_set(LCD_SCL, WM_IO_OPTION1);  /*CK*/
    }else{
        tls_io_cfg_set(LCD_SCL, WM_IO_OPTION2);   /*CK*/
    }

    if(LCD_MOSI == WM_IO_PA_10){
        tls_io_cfg_set(LCD_MOSI, WM_IO_OPTION1);  /*CMD*/
    }else{
        tls_io_cfg_set(LCD_MOSI, WM_IO_OPTION2);  /*CMD*/
    }

    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_SDIO_MASTER);
    tls_bitband_write(HR_CLK_RST_CTL, 27, 0);
    tls_bitband_write(HR_CLK_RST_CTL, 27, 1);
    while (tls_bitband_read(HR_CLK_RST_CTL, 27) == 0);
    tls_sys_clk sysclk;
    tls_sys_clk_get(&sysclk);

    SDIO_HOST->MMC_CARDSEL = 0xC0 | (sysclk.cpuclk / 2 - 1); // enable module, enable mmcclk
    
    // auto transfer, mmc mode. 
    if (clk>=120*1000*1000){
        SDIO_HOST->MMC_CTL = 0x542 | 0 << 3;        // 000 1/2
    }else if(clk>=60*1000*1000){
        SDIO_HOST->MMC_CTL = 0x542 | (0b001 << 3);  // 001 1/4
    }else if(clk>=30*1000*1000){
        SDIO_HOST->MMC_CTL = 0x542 | (0b010 << 3);  // 010 1/6
    }else if(clk>=15*1000*1000){
        SDIO_HOST->MMC_CTL = 0x542 | (0b011 << 3);  // 011 1/8
    }else{
        SDIO_HOST->MMC_CTL = 0x542 | 0 << 3;        // 000 1/2
    }

    SDIO_HOST->MMC_INT_MASK = 0x100; // unmask sdio data interrupt.
    SDIO_HOST->MMC_CRCCTL = 0x00; 
    SDIO_HOST->MMC_TIMEOUTCNT = 0;
    SDIO_HOST->MMC_BYTECNTL = 0;
    // 创建信号量 
    tls_os_sem_create(&sdio_spi_dma_ready_flag, 1);
}


static int sdio_spi_dma_cfg(u32*mbuf,u32 bufsize,u8 dir)
{
    int ch;
    u32 addr_inc = 0;

    ch = tls_dma_request(0, 0);
    DMA_CHNLCTRL_REG(ch) = DMA_CHNL_CTRL_CHNL_OFF;

    if(dir)
    {
        DMA_SRCADDR_REG(ch) = (unsigned int)mbuf;
        DMA_DESTADDR_REG(ch) = (unsigned int)SDIO_HOST->DATA_BUF;
        addr_inc = DMA_CTRL_SRC_ADDR_INC;
    }
    else
    {
        DMA_SRCADDR_REG(ch) = (unsigned int)SDIO_HOST->DATA_BUF;
        DMA_DESTADDR_REG(ch) = (unsigned int)mbuf;
        addr_inc =DMA_CTRL_DEST_ADDR_INC;
    }

    DMA_CTRL_REG(ch) = addr_inc | DMA_CTRL_DATA_SIZE_WORD | (bufsize << 8);
    DMA_MODE_REG(ch) = DMA_MODE_SEL_SDIOHOST | DMA_MODE_HARD_MODE;

    // tls_dma_irq_register(ch, (void (*))sdio_dma_callback, NULL, TLS_DMA_IRQ_TRANSFER_DONE);
    DMA_CHNLCTRL_REG(ch) = DMA_CHNL_CTRL_CHNL_ON;

    return ch;
}

void sdio_spi_put(u8 d)
{
    SDIO_HOST->BUF_CTL = 0x4820;
    SDIO_HOST->DATA_BUF[0] = d;

    SDIO_HOST->MMC_BYTECNTL = 1;
    SDIO_HOST->MMC_IO = 0x01;
    while (1) {
        if ((SDIO_HOST->MMC_IO & 0x01) == 0x00)
            break;
    }
}

void write_sdio_spi_dma(u32* data, u32 len) 
{
    while(1){
        if ((SDIO_HOST->MMC_IO & 0x01) == 0x00)
            break;
    }
    u32 offset=0;
    while(len>0){
        int datalen=len;
        if(len>0xfffc)
            datalen=0xfffc;
        len-=datalen;

        SDIO_HOST->BUF_CTL = 0x4000; //disable dma,
        sdio_spi_dma_channel = sdio_spi_dma_cfg((u32 *) data+offset, datalen, 1);
        SDIO_HOST->BUF_CTL = 0xC20; //enable dma, write sd card
        SDIO_HOST->MMC_INT_SRC |= 0x7ff; // clear all firstly
        SDIO_HOST->MMC_BYTECNTL = datalen;
        SDIO_HOST->MMC_IO = 0x01;
        offset+=datalen/4;
        while(1){
            if ((SDIO_HOST->BUF_CTL & 0x400) == 0x00)
                break;
        }
        tls_dma_free(sdio_spi_dma_channel);
 
    }
}

static void sdio_dma_callback(void)
{
    // printf("sdio_dma_callback\n");
    // printf("--->buf_size:%d\n", sdio_spi_dma_buf_size);
    tls_os_queue_send(user_task_queue, (void *)LCD_TASK_QUEUE_FILL, 0);
}

// 两次数据之间需要加延时，否则会导致屏幕死机，延时时间可以自行调整。
void wait_sdio_spi_dma_ready()
{
    extern int delay_us();
    while(!sdio_spi_dma_ready){
        tls_os_time_delay(1);
        // delay_us(1);
    }
}

void write_sdio_spi_dma_async(u32* data, u32 len) 
{
    // printf("---> len: %d\n", len);
    // 等待信号量 直到上一次DMA数据发送完成
    // printf("--->:%d %s\r\n", __LINE__, __func__);
    tls_os_sem_acquire(sdio_spi_dma_ready_flag, 0);
    sdio_spi_dma_ready = false;

    if(len<4){
        printf("send err\n");
        return;
    }

    if(len%4)
    {
        len += (4 - (len%4)); // dma发送长度必须为4的倍数。
    }

    sdio_spi_dma_buf_addr = data;
    sdio_spi_dma_buf_size = len;
    while(1){
        if ((SDIO_HOST->MMC_IO & 0x01) == 0x00)
            break;
    }
    SDIO_HOST->BUF_CTL = 0x4000; //disable dma,
    sdio_spi_dma_channel = tls_dma_request(0, 0);
    DMA_CHNLCTRL_REG(sdio_spi_dma_channel) = DMA_CHNL_CTRL_CHNL_OFF;
    DMA_SRCADDR_REG(sdio_spi_dma_channel) = (unsigned int)sdio_spi_dma_buf_addr;
    DMA_DESTADDR_REG(sdio_spi_dma_channel) = (unsigned int)SDIO_HOST->DATA_BUF;
    u32 bufsize = sdio_spi_dma_buf_size;
    if(bufsize > 65532){
        bufsize = 65532;
    }
    sdio_spi_dma_buf_size -= bufsize;
        
    DMA_CTRL_REG(sdio_spi_dma_channel) = DMA_CTRL_SRC_ADDR_INC | DMA_CTRL_DATA_SIZE_WORD | (bufsize << 8);
    DMA_MODE_REG(sdio_spi_dma_channel) = DMA_MODE_SEL_SDIOHOST | DMA_MODE_HARD_MODE;

    tls_dma_irq_register(sdio_spi_dma_channel, (void (*))sdio_dma_callback, NULL, TLS_DMA_IRQ_TRANSFER_DONE);
    DMA_CHNLCTRL_REG(sdio_spi_dma_channel) = DMA_CHNL_CTRL_CHNL_ON;

    SDIO_HOST->BUF_CTL = 0xC20; //enable dma, write sd card
    SDIO_HOST->MMC_INT_SRC |= 0x7ff; // clear all firstly
    SDIO_HOST->MMC_BYTECNTL = bufsize;
    SDIO_HOST->MMC_IO = 0x01;
}

static void fillRemainingBuf()
{
    // printf("---> size: %d\r\n", sdio_spi_dma_buf_size);
    if(sdio_spi_dma_buf_size > 0)
    {
        while(1){
            if ((SDIO_HOST->MMC_IO & 0x01) == 0x00)
                break;
        }
        sdio_spi_dma_buf_addr += 65532/4;
        DMA_CHNLCTRL_REG(sdio_spi_dma_channel) = DMA_CHNL_CTRL_CHNL_OFF;

        DMA_SRCADDR_REG(sdio_spi_dma_channel) = (unsigned int)sdio_spi_dma_buf_addr;
        DMA_DESTADDR_REG(sdio_spi_dma_channel) = (unsigned int)SDIO_HOST->DATA_BUF;
    
        u32 bufsize = sdio_spi_dma_buf_size;
        if(bufsize > 65532){
            bufsize = 65532;
        }
        sdio_spi_dma_buf_size -= bufsize;
            
        DMA_CTRL_REG(sdio_spi_dma_channel) = DMA_CTRL_SRC_ADDR_INC | DMA_CTRL_DATA_SIZE_WORD | (bufsize << 8);
        DMA_MODE_REG(sdio_spi_dma_channel) = DMA_MODE_SEL_SDIOHOST | DMA_MODE_HARD_MODE;

        DMA_CHNLCTRL_REG(sdio_spi_dma_channel) = DMA_CHNL_CTRL_CHNL_ON;
        SDIO_HOST->BUF_CTL = 0xC20; //enable dma, write sd card
        SDIO_HOST->MMC_INT_SRC |= 0x7ff; // clear all firstly
        SDIO_HOST->MMC_BYTECNTL = bufsize;
        SDIO_HOST->MMC_IO = 0x01;
    }
    else
    {
        tls_dma_free(sdio_spi_dma_channel);
        tls_os_sem_release(sdio_spi_dma_ready_flag);
        sdio_spi_dma_ready = true;
    }
}

static void lcd_task_start(void *data)
{
    void *msg;
    tls_os_status_t ret;

    for ( ; ; )
    {
        // 接收队里消息
        ret= tls_os_queue_receive(user_task_queue, (void **)&msg, 0, 0);
        if (ret == TLS_OS_SUCCESS)
        {
            switch((u32)msg)
            {
            case LCD_TASK_QUEUE_FILL: // 填充剩余buf
            {
                fillRemainingBuf();
                break;
            }
            default:
                break;
            }
        }
    }
}

void lcd_task_create(void)
{
    // 创建消息队列
    tls_os_queue_create(&user_task_queue, LCD_TASK_QUEUE_SIZE);
    tls_os_task_create(NULL, 
                      "lcd",
                       lcd_task_start,
                       NULL,
                       (void *)LcdTaskStk,          /* task's stack start address */
                       LCD_TASK_STACK_SIZE, /* task's stack size, unit:byte */
                       LCD_TASK_PRORITY,
                       0);
    // 创建消息队列
    tls_os_queue_create(&user_task_queue, LCD_TASK_QUEUE_SIZE);
}
