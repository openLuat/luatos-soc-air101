#include "luat_adc.h"
#include "luat_base.h"
#include "wm_adc.h"
#include "wm_gpio_afsel.h"
#include "wm_include.h"

// 0,1 标准ADC
// 3 - 内部温度传感

int luat_adc_open(int ch, void *args)
{
    switch (ch)
    {
    case 0:
        wm_adc_config(ch);
        break;
    case 1:
        wm_adc_config(ch);
        break;
// #ifdef AIR103
    case 2:
        wm_adc_config(ch);
        break;
    case 3:
        wm_adc_config(ch);
        break;
// #endif
    case 10:
        return 0; // 温度传感器
    case 11:
        return 0; // VBAT电压
    default:
        return 1;
    }
    return 0;
}

int luat_adc_read(int ch, int *val, int *val2)
{
    int voltage = 0;

    switch (ch)
    {
    case 0:
        voltage = adc_get_inputVolt(ch);
        break;
    case 1:
        voltage = adc_get_inputVolt(ch);
        break;
    case 2:
        voltage = adc_get_inputVolt(ch);
        break;
    case 3:
        voltage = adc_get_inputVolt(ch);
        break;
    case 10:
        voltage = adc_temp();
        return 0;
    case 11:
        voltage = adc_get_interVolt();
        return 0;
    default:
        return 1;
    }
    *val = voltage;
    *val2 = voltage;
    if (*val < 46134) {
        *val2 = 0;
    }
    else if (*val > 123405) {
        *val2 = 2300;
    }
    else {
        *val2 = (int)((double)(*val - 46134) / (double)(32.196));
    }
    return 0;
}

int luat_adc_close(int ch)
{
    switch (ch)
    {
    case 0:
        tls_io_cfg_set(WM_IO_PA_01, WM_IO_OPTION5);
        break;
    case 1:
        tls_io_cfg_set(WM_IO_PA_04, WM_IO_OPTION5);
        break;
// #ifdef AIR103
    case 2:
        tls_io_cfg_set(WM_IO_PA_03, WM_IO_OPTION5);
        break;
    case 3:
        tls_io_cfg_set(WM_IO_PA_02, WM_IO_OPTION5);
        break;
// #endif
    case 10:
        break; // 温度
    case 11:
        break; // 内部电压
    default:
        return 1;
    }
    return 0;
}
