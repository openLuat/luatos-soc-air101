#include "luat_base.h"
#include "luat_i2s.h"

#include "wm_include.h"
#include "wm_i2s.h"
#include "wm_gpio_afsel.h"

#define LUAT_LOG_TAG "i2s"
#include "luat_log.h"

int tls_i2s_io_init(void)
{
    wm_i2s_ck_config(WM_IO_PB_08);
    wm_i2s_ws_config(WM_IO_PB_09);
    wm_i2s_di_config(WM_IO_PB_10);
    wm_i2s_do_config(WM_IO_PB_11);
    LLOGD("ck--PB08, ws--PB09, di--PB10, do--PB11");
    return WM_SUCCESS;
}

int luat_i2s_setup(luat_i2s_conf_t *conf) {
    // 首先, 配置io复用
    tls_i2s_io_init();

    // 然后转本地i2s配置
    I2S_InitDef opts = { I2S_MODE_MASTER, I2S_CTRL_STEREO, I2S_RIGHT_CHANNEL, I2S_Standard, I2S_DataFormat_16, 8000, 5000000 };

    int mode = I2S_MODE_MASTER; // 当前强制master
    int stereo = 0;
    int format = 0; // i2s
    int datawidth = 16;
    int freq = 44100;

    opts.I2S_Mode_MS = mode;
    opts.I2S_Mode_SS = (stereo<<22);
    opts.I2S_Mode_LR = I2S_LEFT_CHANNEL;
    opts.I2S_Trans_STD = (format*0x1000000);
    opts.I2S_DataFormat = (datawidth/8 - 1)*0x10;
    opts.I2S_AudioFreq = freq;
    opts.I2S_MclkFreq = 80000000;

    wm_i2s_port_init(&opts);
    wm_i2s_register_callback(NULL);

    return 0;
}

int luat_i2s_send(uint8_t id, char* buff, size_t len) {
    wm_i2s_tx_int((int16_t *)buff, len / 2, NULL);
    return len;
}

int luat_i2s_recv(uint8_t id, char* buff, size_t len) {
    wm_i2s_rx_int((int16_t *)buff, len / 2);
    return len;
}

int luat_i2s_close(uint8_t id) {
    wm_i2s_tx_rx_stop();
    return 0;
}
