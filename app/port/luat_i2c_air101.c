
#include "luat_base.h"
#include "luat_i2c.h"

#include "wm_include.h"
#include "wm_i2c.h"
#include <string.h>
#include "wm_gpio_afsel.h"

#define LUAT_LOG_TAG "i2c"
#include "luat_log.h"

int luat_i2c_exist(int id) {
    return id == 0;
}

int luat_i2c_setup(int id, int speed) {
    //if (luat_i2c_exist(id) != 0) return -1;
    if (speed == 0)
        speed = 100 * 1000; // SLOW
    else if (speed == 1)
        speed = 400 * 1000; // FAST
    wm_i2c_scl_config(WM_IO_PA_01);
    wm_i2c_sda_config(WM_IO_PA_04);
    tls_i2c_init(speed);
    return 0;
}

int luat_i2c_close(int id) {
    return 0;
}

int luat_i2c_send(int id, int addr, void* buff, size_t len , uint8_t stop) {
    tls_i2c_write_byte(addr << 1, 1);
    if(WM_FAILED == tls_i2c_wait_ack())
        return -1;
    for (size_t i = 0; i < len; i++){
        tls_i2c_write_byte(((u8*)buff)[i], 0);
        if(WM_FAILED == tls_i2c_wait_ack())
            return -1;
    }
    if (stop){
        tls_i2c_stop();
    }
    return 0;
}

int luat_i2c_recv(int id, int addr, void* buff, size_t len) {
    tls_i2c_write_byte((addr << 1) + 1, 1);
    if(WM_FAILED == tls_i2c_wait_ack())
        return -1;
    if (len < 1) 
        return -1;
    else if (len == 1){
        ((u8*)buff)[0] = tls_i2c_read_byte(0, 1);
    }else {
        for (size_t i = 0; i < len; i++){
            if (i == 0)
                ((u8*)buff)[i] = tls_i2c_read_byte(1, 0);
            else if (i == len - 1){
                ((u8*)buff)[i] = tls_i2c_read_byte(0, 1);
                break;
            }
            else
                ((u8*)buff)[i] = tls_i2c_read_byte(1, 0);
            if(WM_FAILED == tls_i2c_wait_ack())
                break;
        }
    }
    
    return 0;
}
