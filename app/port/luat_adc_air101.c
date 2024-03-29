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
    case 2:
        wm_adc_config(ch);
        break;
    case 3:
        wm_adc_config(ch);
        break;
    case 10:
    case LUAT_ADC_CH_CPU:
        return 0; // 温度传感器
    case 11:
    case LUAT_ADC_CH_VBAT:
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
        voltage = adc_get_inputVolt(ch, val);
        break;
    case 1:
        voltage = adc_get_inputVolt(ch, val);
        break;
    case 2:
        voltage = adc_get_inputVolt(ch, val);
        break;
    case 3:
        voltage = adc_get_inputVolt(ch, val);
        break;
    case LUAT_ADC_CH_CPU:
        voltage = adc_temp();
        *val = voltage;
        *val2 = voltage;
        return 0;
    case LUAT_ADC_CH_VBAT:
        voltage = adc_get_interVolt();
        *val = voltage;
        *val2 = voltage;
        return 0;
    default:
        return 1;
    }
    *val2 = voltage;
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
    case 2:
        tls_io_cfg_set(WM_IO_PA_03, WM_IO_OPTION5);
        break;
    case 3:
        tls_io_cfg_set(WM_IO_PA_02, WM_IO_OPTION5);
        break;
    case LUAT_ADC_CH_CPU:
        break; // 温度
    case LUAT_ADC_CH_VBAT:
        break; // 内部电压
    default:
        return 1;
    }
    return 0;
}

int luat_adc_global_config(int tp, int val) {
    return 0;
}
