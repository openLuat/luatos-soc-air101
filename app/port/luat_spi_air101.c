

#include "luat_base.h"
#include "luat_spi.h"

#include "wm_include.h"

#include "wm_hostspi.h"
#include "wm_gpio_afsel.h"
#include "wm_cpu.h"

#define LUAT_LOG_TAG "luat.spi"
#include "luat_log.h"
#include "luat_timer.h"

static uint8_t luat_spi_mode = 1;

int luat_spi_device_config(luat_spi_device_t* spi_dev) {
    unsigned int clk;
    uint8_t TLS_SPI_MODE = 0x00 ;
    clk = spi_dev->spi_config.bandrate;
    tls_sys_clk sysclk;
    tls_sys_clk_get(&sysclk);
    if(clk / (UNIT_MHZ/2) > sysclk.apbclk)
        clk = sysclk.apbclk * (UNIT_MHZ/2);
    if(spi_dev->spi_config.CPHA)
        TLS_SPI_MODE |= SPI_CPHA;
    if(spi_dev->spi_config.CPOL)
        TLS_SPI_MODE |= SPI_CPOL;
    return tls_spi_setup(TLS_SPI_MODE, TLS_SPI_CS_LOW, clk);
}

int luat_spi_bus_setup(luat_spi_device_t* spi_dev){
    int bus_id = spi_dev->bus_id;
    if (bus_id == 0){
#ifdef LUAT_USE_PSRAM
        LLOGE("psram/spi0 use the same pin, pls switch to spi1");
        luat_timer_mdelay(1000);
        return -1;
#endif
        wm_spi_ck_config(WM_IO_PB_02);
        wm_spi_di_config(WM_IO_PB_03);
        wm_spi_do_config(WM_IO_PB_05);
    }
    // #ifdef AIR103
    else if (bus_id == 1) { // 本质上是mode=1,不是spi1,该模式下psram可用
	    wm_spi_ck_config(WM_IO_PB_15);
	    wm_spi_di_config(WM_IO_PB_16);
	    wm_spi_do_config(WM_IO_PB_17);
    }
    else if (bus_id == 2) { // 本质上是mode=2,不是spi2,该模式下psram可用
	    wm_spi_ck_config(WM_IO_PB_24);
	    wm_spi_di_config(WM_IO_PB_25);
	    wm_spi_do_config(WM_IO_PB_26);
    }
    // #endif
    else{
        LLOGD("spi_bus error");
        return -1;
    }
    tls_spi_trans_type(SPI_DMA_TRANSFER);
}

//初始化配置SPI各项参数，并打开SPI
//成功返回0
int luat_spi_setup(luat_spi_t* spi) {
    int ret;
    unsigned int clk;
    uint8_t TLS_SPI_MODE = 0x00 ;
    if (spi->id == 0) {
#ifdef LUAT_USE_PSRAM
        LLOGE("psram/spi0 use the same pin, pls switch to spi1");
        luat_timer_mdelay(1000);
        return -1;
#endif
	    // 兼容CS=0,默认配置, 也兼容CS为GPIO20的配置,其他配置不受控,自然不应该配置CS脚
        if (spi->cs == 0 || spi->cs == WM_IO_PB_04)
            wm_spi_cs_config(WM_IO_PB_04);
        wm_spi_ck_config(WM_IO_PB_02);
        wm_spi_di_config(WM_IO_PB_03);
        wm_spi_do_config(WM_IO_PB_05);
    }
    // #ifdef AIR103
    else if (spi->id == 1) { // 本质上是mode=1,不是spi1,该模式下psram可用
        if (spi->cs == 0 || spi->cs == WM_IO_PB_14)
	        wm_spi_cs_config(WM_IO_PB_14);
	    wm_spi_ck_config(WM_IO_PB_15);
	    wm_spi_di_config(WM_IO_PB_16);
	    wm_spi_do_config(WM_IO_PB_17);
    }
    else if (spi->id == 2) { // 本质上是mode=2,不是spi2,该模式下psram可用
        if (spi->cs == 0 || spi->cs == WM_IO_PB_23)
	        wm_spi_cs_config(WM_IO_PB_23);
	    wm_spi_ck_config(WM_IO_PB_24);
	    wm_spi_di_config(WM_IO_PB_25);
	    wm_spi_do_config(WM_IO_PB_26);
    }
    // #endif
    else {
        return -1;
    }

    if(spi->CPHA)
        TLS_SPI_MODE |= SPI_CPHA;
    if(spi->CPOL)
        TLS_SPI_MODE |= SPI_CPOL;
    clk = spi->bandrate;

    tls_sys_clk sysclk;
    tls_sys_clk_get(&sysclk);
    if(clk / (UNIT_MHZ/2) > sysclk.apbclk)
        clk = sysclk.apbclk * (UNIT_MHZ/2);

    tls_spi_trans_type(SPI_DMA_TRANSFER);
    ret = tls_spi_setup(TLS_SPI_MODE, TLS_SPI_CS_LOW, clk);

    luat_spi_mode = spi->mode;

    return ret;
}

//关闭SPI，成功返回0
int luat_spi_close(int spi_id) {
    return 0;
}

//收发SPI数据，返回接收字节数
int32_t tls_spi_xfer(const void *data_out, void *data_in, uint32_t num_out, uint32_t num_in);
int luat_spi_transfer(int spi_id, const char* send_buf, size_t send_length, char* recv_buf, size_t recv_length){
    //tls_spi_read_with_cmd(send_buf, send_length, recv_buf, recv_length);
    // if (luat_spi_mode == 1)
        // tls_spi_xfer(send_buf, recv_buf, send_buf, recv_length);
    // else {
        if (send_length > 0)
            tls_spi_write(send_buf, send_length);
        if (recv_length > 0)
            tls_spi_read(recv_buf, recv_length);
    // }
    return recv_length;
}
//收SPI数据，返回接收字节数
int luat_spi_recv(int spi_id, char* recv_buf, size_t length) {
    int ret;
    if(length <= SPI_DMA_BUF_MAX_SIZE)
    {
        ret = tls_spi_read(recv_buf,length);
    }
    else
    {
        size_t i;
        for(i=0;i<length;i+=SPI_DMA_BUF_MAX_SIZE)
        {
            ret = tls_spi_read((u8*)(recv_buf+i), length - i > SPI_DMA_BUF_MAX_SIZE ? SPI_DMA_BUF_MAX_SIZE : length - i);
            if(ret != TLS_SPI_STATUS_OK)
                break;
        }
    }
    if (ret == TLS_SPI_STATUS_OK)
        return length;
    else
        return -1;
}
//发SPI数据，返回发送字节数
int luat_spi_send(int spi_id, const char* send_buf, size_t length) {
    int ret;
    if(length <= SPI_DMA_BUF_MAX_SIZE)
    {
        ret = tls_spi_write(send_buf, length);
    }
    else
    {
        size_t i;
        for(i=0;i<length;i+=SPI_DMA_BUF_MAX_SIZE)
        {
            ret = tls_spi_write((const u8*)(send_buf+i), length - i > SPI_DMA_BUF_MAX_SIZE ? SPI_DMA_BUF_MAX_SIZE : length - i);
            if(ret != TLS_SPI_STATUS_OK)
                break;
        }
    }
    if (ret == TLS_SPI_STATUS_OK)
        return length;
    else
        return -1;
}

int luat_spi_config_dma(int spi_id, uint32_t tx_channel, uint32_t rx_channel) {
    return 0;
}

int luat_spi_change_speed(int spi_id, uint32_t speed) {
    int ret = tls_spi_set_speed(speed);
    if (ret != 0) {
        LLOGW("spi set speed %d fail %d", speed, ret);
    }
    return ret;
}

