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

static void sdio_lcd_wait_idle(void) {

}

int luat_sdio_lcd_write_cmd_data(luat_lcd_conf_t* conf,const uint8_t cmd, const uint8_t *data, uint8_t data_len){
    sdio_lcd_wait_idle();
    sdio_lcd_set_pin(conf->pin_dc, 0);
    sdio_lcd_set_pin(conf->lcd_cs_pin, 0);
    sdio_spi_put(cmd);

    sdio_lcd_wait_idle();
    sdio_lcd_set_pin(conf->pin_dc, 1);
    for (size_t i = 0; i < data_len; i++)
        sdio_spi_put( *data++);

    sdio_lcd_wait_idle();
    sdio_lcd_set_pin(conf->lcd_cs_pin, 1);
    return 0;
}

int luat_sdio_lcd_draw(luat_lcd_conf_t* conf, int16_t x1, int16_t y1, int16_t x2, int16_t y2, luat_color_t* color){
    uint32_t size = (x2 - x1 + 1) * (y2 - y1 + 1);
    luat_lcd_set_address(conf, x1, y1, x2, y2);
    sdio_lcd_set_pin(conf->lcd_cs_pin, 0);
    write_sdio_spi_dma((u32 *)color, size * 2);
    sdio_lcd_set_pin(conf->lcd_cs_pin, 1);
    return 0;
}

void luat_lcd_IF_init(luat_lcd_conf_t* conf){
    tls_gpio_cfg(conf->lcd_cs_pin,  WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_FLOATING);

    tls_gpio_write(conf->lcd_cs_pin, 1);
    tls_gpio_write(conf->pin_dc, 1);

    init_sdio_spi_mode(conf->bus_speed);
    
    conf->opts->write_cmd_data = luat_sdio_lcd_write_cmd_data;
    conf->opts->lcd_draw = luat_sdio_lcd_draw;
}
