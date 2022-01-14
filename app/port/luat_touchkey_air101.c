#include "luat_base.h"
#include "luat_touchkey.h"
#include "luat_msgbus.h"

#include "wm_include.h"
#include "wm_touchsensor.h"
#include "wm_gpio_afsel.h"
#include "wm_io.h"
#include "wm_regs.h"

static enum tls_io_name touchkeymap[] = {
  WM_IO_PA_00, // NOP
  WM_IO_PA_07, /*touch sensor 1*/
  WM_IO_PA_09, /*touch sensor 2*/
  WM_IO_PA_10, /*touch sensor 3*/
  WM_IO_PB_00, /*touch sensor 4*/			
  WM_IO_PB_01, /*touch sensor 5*/			
  WM_IO_PB_02, /*touch sensor 6*/			
  WM_IO_PB_03, /*touch sensor 7*/			
  WM_IO_PB_04, /*touch sensor 8*/			
  WM_IO_PB_05, /*touch sensor 9*/			
  WM_IO_PB_06, /*touch sensor 10*/			
  WM_IO_PB_07, /*touch sensor 11*/			
  WM_IO_PB_08, /*touch sensor 12*/			
  WM_IO_PB_09, /*touch sensor 13*/
  WM_IO_PA_12, /*touch sensor 14*/
  WM_IO_PA_14, /*touch sensor 15*/
};

static void luat_touchkey_cb(uint32_t value) {
    rtos_msg_t msg;
    msg.handler = l_touchkey_handler;
    msg.ptr = NULL;
	for (uint8_t i = 0; i < 15; i++)
	{
		if (value&BIT(i))
		{
			msg.arg1 = i + 1;
            msg.arg2 = tls_touchsensor_countnum_get(i+1);
			luat_msgbus_put(&msg, 0);
		}	
	}
}


int luat_touchkey_setup(luat_touchkey_conf_t *conf) {
    if (conf->id < 1 || conf->id > 15 )
        return -1;
    wm_touch_sensor_config(touchkeymap[(enum tls_io_name)conf->id]);
    tls_touchsensor_init_config(conf->id, conf->scan_period, conf->window, 1);
    tls_touchsensor_irq_enable(conf->id);
    tls_touchsensor_irq_register(luat_touchkey_cb);
    if (conf->threshold)
        tls_touchsensor_threshold_config(conf->id, conf->threshold);
    return 0;
}

int luat_touchkey_close(uint8_t id) {
    if (id < 1 || id > 15 )
        return -1;
    tls_touchsensor_irq_disable(id);
    tls_touchsensor_deinit(id);
    return 0;
}

