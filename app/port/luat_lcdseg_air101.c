#include "luat_base.h"
#include "luat_lcdseg.h"

#include "wm_include.h"
#include "wm_lcd.h"
#include "wm_regs.h"
#include "wm_pmu.h"

#define LUAT_LOG_TAG "lcdseg"
#include "luat_log.h"

int luat_lcdseg_setup(luat_lcd_options_t *opts) {
    tls_lcd_options_t conf = {0};
    conf.enable = 1;
    switch (opts->bias)
    {
    case 0:
        conf.bias = BIAS_STATIC;
        break;
    case 2:
        conf.bias = BIAS_ONEHALF;
        break;
    case 3:
        conf.bias = BIAS_ONETHIRD;
        break;
    case 4:
        conf.bias = BIAS_ONEFOURTH;
        break;
    default:
        LLOGD("invaild bias %d", opts->bias);
        return -1;
    }

    switch (opts->duty)
    {
    case 0:
        conf.duty = DUTY_STATIC;
        break;
    case 2:
        conf.duty = DUTY_ONEHALF;
        break;
    case 3:
        conf.duty = DUTY_ONETHIRD;
        break;
    case 4:
        conf.duty = DUTY_ONEFOURTH;
        break;
    case 5:
        conf.duty = DUTY_ONEFIFTH;
        break;
    case 6:
        conf.duty = DUTY_ONESIXTH;
        break;
    case 7:
        conf.duty = DUTY_ONESEVENTH;
        break;
    case 8:
        conf.duty = DUTY_ONEEIGHTH;
        break;
    default:
        LLOGD("invaild duty %d", opts->bias);
        return -1;
    }


    switch(opts->vlcd) {
    case 27:
        conf.vlcd = VLCD27;
        break;
    case 29:
        conf.vlcd = VLCD29;
        break;
    case 31:
        conf.vlcd = VLCD31;
        break;
    case 33:
        conf.vlcd = VLCD33;
        break;
    default:
        LLOGD("invaild vlcd %d", opts->vlcd);
        return -1;
    }

    if (opts->com_number > 4) {
        LLOGD("invaild com_number %d", opts->com_number);
        return -1;
    }
    conf.com_number = opts->com_number;
    conf.fresh_rate = opts->fresh_rate;
    /**
	tls_io_cfg_set(WM_IO_PB_25, WM_IO_OPTION6);
	tls_io_cfg_set(WM_IO_PB_21, WM_IO_OPTION6);
	tls_io_cfg_set(WM_IO_PB_22, WM_IO_OPTION6);
	tls_io_cfg_set(WM_IO_PB_27, WM_IO_OPTION6);
     */
    if (opts->com_mark & 0x01)
        tls_io_cfg_set(WM_IO_PB_25, WM_IO_OPTION6);
    if (opts->com_mark & 0x02)
        tls_io_cfg_set(WM_IO_PB_21, WM_IO_OPTION6);
    if (opts->com_mark & 0x04)
        tls_io_cfg_set(WM_IO_PB_22, WM_IO_OPTION6);
    if (opts->com_mark & 0x08)
        tls_io_cfg_set(WM_IO_PB_27, WM_IO_OPTION6);
    
    // seg 0-5
    //if (opts->seg_mark & (1 << 0))
	//    tls_io_cfg_set(WM_IO_PB_23, WM_IO_OPTION6);
	if (opts->seg_mark & (1 << 1))
	    tls_io_cfg_set(WM_IO_PB_26, WM_IO_OPTION6);
	if (opts->seg_mark & (1 << 2))
	    tls_io_cfg_set(WM_IO_PB_24, WM_IO_OPTION6);

	if (opts->seg_mark & (1 << 3))
	    tls_io_cfg_set(WM_IO_PA_07, WM_IO_OPTION6);
	if (opts->seg_mark & (1 << 4))
	    tls_io_cfg_set(WM_IO_PA_08, WM_IO_OPTION6);
	if (opts->seg_mark & (1 << 5))
	    tls_io_cfg_set(WM_IO_PA_09, WM_IO_OPTION6);
	if (opts->seg_mark & (1 << 6))
	    tls_io_cfg_set(WM_IO_PA_10, WM_IO_OPTION6);	
	if (opts->seg_mark & (1 << 7))
	    tls_io_cfg_set(WM_IO_PA_11, WM_IO_OPTION6);	
	if (opts->seg_mark & (1 << 8))
	    tls_io_cfg_set(WM_IO_PA_12, WM_IO_OPTION6);	
	if (opts->seg_mark & (1 << 9))
	    tls_io_cfg_set(WM_IO_PA_13, WM_IO_OPTION6);	
	if (opts->seg_mark & (1 << 10))
	    tls_io_cfg_set(WM_IO_PA_14, WM_IO_OPTION6);	
	if (opts->seg_mark & (1 << 11))
	    tls_io_cfg_set(WM_IO_PA_15, WM_IO_OPTION6);	
	if (opts->seg_mark & (1 << 12))
	    tls_io_cfg_set(WM_IO_PB_00, WM_IO_OPTION6);	
	if (opts->seg_mark & (1 << 13))
	    tls_io_cfg_set(WM_IO_PB_01, WM_IO_OPTION6);	
	if (opts->seg_mark & (1 << 14))
	    tls_io_cfg_set(WM_IO_PB_02, WM_IO_OPTION6);	
	if (opts->seg_mark & (1 << 15))
	    tls_io_cfg_set(WM_IO_PB_03, WM_IO_OPTION6);

    if (opts->seg_mark & (1 << 16))
	    tls_io_cfg_set(WM_IO_PB_04, WM_IO_OPTION6);	
	if (opts->seg_mark & (1 << 17))
	    tls_io_cfg_set(WM_IO_PB_05, WM_IO_OPTION6);	
	if (opts->seg_mark & (1 << 18))
	    tls_io_cfg_set(WM_IO_PB_06, WM_IO_OPTION6);	
	if (opts->seg_mark & (1 << 19))
	    tls_io_cfg_set(WM_IO_PB_07, WM_IO_OPTION6);	
	if (opts->seg_mark & (1 << 20))
	    tls_io_cfg_set(WM_IO_PB_08, WM_IO_OPTION6);	
	if (opts->seg_mark & (1 << 21))
	    tls_io_cfg_set(WM_IO_PB_09, WM_IO_OPTION6);	
	if (opts->seg_mark & (1 << 22))
	    tls_io_cfg_set(WM_IO_PB_10, WM_IO_OPTION6);	
	if (opts->seg_mark & (1 << 23))
	    tls_io_cfg_set(WM_IO_PB_11, WM_IO_OPTION6);
    
    
	if (opts->seg_mark & (1 << 24))
	    tls_io_cfg_set(WM_IO_PB_12, WM_IO_OPTION6);
	if (opts->seg_mark & (1 << 25))
	    tls_io_cfg_set(WM_IO_PB_13, WM_IO_OPTION6);
	if (opts->seg_mark & (1 << 26))
	    tls_io_cfg_set(WM_IO_PB_14, WM_IO_OPTION6);
	if (opts->seg_mark & (1 << 27))
	    tls_io_cfg_set(WM_IO_PB_15, WM_IO_OPTION6);
	if (opts->seg_mark & (1 << 28))
	    tls_io_cfg_set(WM_IO_PB_16, WM_IO_OPTION6);
	if (opts->seg_mark & (1 << 29))
	    tls_io_cfg_set(WM_IO_PB_17, WM_IO_OPTION6);
	if (opts->seg_mark & (1 << 30))
	    tls_io_cfg_set(WM_IO_PB_18, WM_IO_OPTION6);
	if (opts->seg_mark & (1 << 31))
	    tls_io_cfg_set(WM_IO_PA_06, WM_IO_OPTION6);

    tls_open_peripheral_clock(TLS_PERIPHERAL_TYPE_LCD);
    tls_reg_write32(HR_LCD_COM_EN, opts->com_mark);
    tls_reg_write32(HR_LCD_SEG_EN, opts->seg_mark);
    tls_lcd_init(&conf);
    return 0;
}

int luat_lcdseg_enable(uint8_t enable) {
    TLS_LCD_ENABLE(enable == 0 ? 0 : 1);
    return 0;
}

int luat_lcdseg_power(uint8_t enable) {
    TLS_LCD_POWERDOWM(enable == 0 ? 1 : 0);
    return 0;
}

int luat_lcdseg_seg_set(uint8_t com, uint32_t seg, uint8_t val) {
    if (com > 4)
        return -1;
    if (seg > 32)
        return -1;
    tls_lcd_seg_set(com, seg, val == 0 ? 0 : 1);
    return 0;
}
