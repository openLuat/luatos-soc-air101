#include "luat_base.h"
#include "luat_spi_slave.h"

#define LUAT_LOG_TAG "spislave"
#include "luat_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wm_hspi.h"
#include "wm_regs.h"
#include "wm_config.h"
#include "wm_mem.h"
#include "wm_osal.h"
#include "wm_irq.h"
//#include "lwip/mem.h"
#include "wm_io.h"
#include "wm_gpio_afsel.h"

static int inited;


static s16 hsp_rx_cmd_cb(char *buf) {
    printf("hsp_rx_cmd_cb %p %d", buf, buf[0]);
    l_spi_slave_event(0, 0, buf, 256);
    return WM_SUCCESS;
}
static s16 hsp_rx_data_cb(char *buf) {
    printf("hsp_rx_data_cb %p %d %d", buf, buf[0], buf[1]);
    l_spi_slave_event(0, 1, buf, 1500);
    return WM_SUCCESS;
}
static s16 hsp_tx_data_cb(char *buf) {
    // printf("hsp_tx_data_cb %p %d %d", buf, buf[0], buf[1]);
    return WM_SUCCESS;
}

int luat_spi_slave_open(luat_spi_slave_conf_t *conf) {
    int rc = 0;
    if (!inited) {
        rc = tls_slave_spi_init();
        if (rc) {
            LLOGE("spi slave 初始化失败 %d", rc);
            return rc;
        }
    }
    if (conf->id == HSPI_INTERFACE_SPI) {
        LLOGD("初始化SPI从机为高速SPI模式");
        wm_hspi_gpio_config(0);
        tls_set_high_speed_interface_type(HSPI_INTERFACE_SPI);
    }
    else if (conf->id == HSPI_INTERFACE_SDIO) {
        LLOGD("初始化SPI从机为高速SDIO模式");
        wm_sdio_slave_config(0);
        tls_set_high_speed_interface_type(HSPI_INTERFACE_SDIO);
    }
    else {
        LLOGE("不支持的SPI从机模式 %d", conf->id);
        return -1;
    }
    tls_set_hspi_user_mode(1);
    // 注册消息回调
    tls_hspi_rx_cmd_callback_register(hsp_rx_cmd_cb);
    tls_hspi_rx_data_callback_register(hsp_rx_data_cb);
    // tls_hspi_tx_data_callback_register(hsp_tx_data_cb);

    return 0;
}

int luat_spi_slave_close(luat_spi_slave_conf_t *conf) {
    LLOGW("当前不支持关闭SPI从机");
    return 0;
}
int luat_spi_slave_read(luat_spi_slave_conf_t *conf, uint8_t* src, uint8_t* buf, size_t len) {
    memcpy(buf, src, len);
    return len;
}

int luat_spi_slave_write(luat_spi_slave_conf_t *conf, uint8_t* buf, size_t len) {
    // LLOGD("从机-->主机 写入 %p %d", buf, len);
    int ret = tls_hspi_tx_data((char*)buf, len);
    // LLOGD("从机-->主机 写入 %p %d %d", buf, len, ret);
    return ret;
}

int tls_hspi_writable(void);
int luat_spi_slave_writable(luat_spi_slave_conf_t *conf) {
    return tls_hspi_writable();
}

