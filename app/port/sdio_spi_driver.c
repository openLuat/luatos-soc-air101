#include "sdio_spi_driver.h"
#include "wm_sdio_host.h"
#include "wm_cpu.h"
#include "wm_dma.h"
#include "wm_pmu.h"
#include <string.h>

static tls_os_sem_t *sdio_spi_dma_ready = NULL;
static u8   sdio_spi_dma_channel  = 0xFF;

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
#if LCD_ASYNC_SEND
    // 创建信号量 
    tls_os_sem_create(&sdio_spi_dma_ready, 1);
#endif
}

void sdio_spi_put(u8 d)
{
    // printf("---> put: %2X\n", d);
    while ((SDIO_HOST->MMC_IO & 0x01));
    // 清除中断标志
    SDIO_HOST->MMC_INT_SRC |= 0x7ff; 
    SDIO_HOST->BUF_CTL = 0x4820;
    SDIO_HOST->DATA_BUF[0] = d;
    SDIO_HOST->MMC_BYTECNTL = 1;
    SDIO_HOST->MMC_IO = 0x01;
    while (!(SDIO_HOST->MMC_INT_SRC & 0x02));
}

void sdio_dma_sem_acquire()
{
    // 请求信号量
    tls_os_sem_acquire(sdio_spi_dma_ready, 0);
}

void wait_sdio_prev_data_completion()
{
    extern int delay_us();
    while (!(SDIO_HOST->MMC_INT_SRC & 0x02))
    {
        delay_us(1);
        // tls_os_time_delay(1);
    }
}

static void sdio_dma_callback(void)
{
    tls_dma_free(sdio_spi_dma_channel);
    // 释放信号量
    tls_os_sem_release(sdio_spi_dma_ready);
}

void sdio_spi_dma_write_async(u32* data, u32 len) 
{
    // printf("--->len: %d\n", len);
    // dma一次最大搬运65535,需要4的整数倍
    if(len>65532){
        printf("---> err: data_len > 65532!\n");
        return;
    }

    u32 dma_len = len;
    if(dma_len%4)
    {
        dma_len += (4 - (dma_len%4)); // 这个是dma搬运长度，len为sdio实际发送长度。
    }

    while ((SDIO_HOST->MMC_IO & 0x01));

    SDIO_HOST->BUF_CTL = 0x4000; // disable dma,
    sdio_spi_dma_channel = tls_dma_request(0, 0);
    DMA_CHNLCTRL_REG(sdio_spi_dma_channel) = DMA_CHNL_CTRL_CHNL_OFF;
    DMA_SRCADDR_REG(sdio_spi_dma_channel) = (unsigned int)data;
    DMA_DESTADDR_REG(sdio_spi_dma_channel) = (unsigned int)SDIO_HOST->DATA_BUF;
        
    DMA_CTRL_REG(sdio_spi_dma_channel) = DMA_CTRL_SRC_ADDR_INC | DMA_CTRL_DATA_SIZE_WORD | (dma_len << 8);
    DMA_MODE_REG(sdio_spi_dma_channel) = DMA_MODE_SEL_SDIOHOST | DMA_MODE_HARD_MODE;

    tls_dma_irq_register(sdio_spi_dma_channel, (void (*))sdio_dma_callback, NULL, TLS_DMA_IRQ_TRANSFER_DONE);
    DMA_CHNLCTRL_REG(sdio_spi_dma_channel) = DMA_CHNL_CTRL_CHNL_ON;

    SDIO_HOST->BUF_CTL = 0xC20; // enable dma, write sd card
    SDIO_HOST->MMC_INT_SRC |= 0x7ff; // clear all firstly
    SDIO_HOST->MMC_BYTECNTL = len;
    SDIO_HOST->MMC_IO = 0x01;
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

void write_sdio_spi_dma(u32* data, u32 len) 
{
    while ((SDIO_HOST->MMC_IO & 0x01));
    u32 offset=0;

    while(len > 0){
        int tx_len = len;
        if(len>65532) tx_len = 65532;
        u32 dma_len = tx_len;
        if(dma_len%4)
        {
            dma_len += (4 - (dma_len%4)); // 这个是dma搬运长度，len为sdio实际发送长度。
        }
        SDIO_HOST->BUF_CTL = 0x4000; //disable dma,
        sdio_spi_dma_channel = sdio_spi_dma_cfg((u32 *) data+offset, dma_len, 1);
        SDIO_HOST->BUF_CTL = 0xC20; //enable dma, write sd card
        SDIO_HOST->MMC_INT_SRC |= 0x7ff; // clear all firstly
        SDIO_HOST->MMC_BYTECNTL = tx_len;
        SDIO_HOST->MMC_IO = 0x01;
        offset += tx_len/4;
        len -= tx_len;
        while(!(SDIO_HOST->MMC_INT_SRC & 0x02));
        tls_dma_free(sdio_spi_dma_channel);
    }
}
