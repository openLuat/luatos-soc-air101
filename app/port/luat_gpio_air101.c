#include "luat_base.h"
#include "luat_malloc.h"
#include "luat_msgbus.h"
#include "luat_timer.h"
#include "luat_gpio.h"
#include "wm_include.h"
#include "luat_irq.h"

typedef struct wm_gpio_conf
{
    luat_gpio_irq_cb cb;
    void* args;
}wm_gpio_conf_t;


// static wm_gpio_conf_t confs[WM_IO_PB_31 + 1];

static void luat_gpio_irq_callback(void *ptr)
{
    int pin = (int)ptr;
    int ret = tls_get_gpio_irq_status(pin);
	if(ret)
	{
		tls_clr_gpio_irq_status(pin);
		luat_irq_gpio_cb(pin, NULL);
	}
}

int luat_gpio_setup(luat_gpio_t *gpio){
    int dir = 0;
    int attr = 0;
    int irq = 0;
    int ret;
    if (gpio->pin < 0 || gpio->pin > WM_IO_PB_31) return 0;
    switch (gpio->mode){
        case Luat_GPIO_OUTPUT:
            dir = WM_GPIO_DIR_OUTPUT;
            attr = WM_GPIO_ATTR_FLOATING;
            break;
        case Luat_GPIO_INPUT:
        case Luat_GPIO_IRQ:
        {
            dir = WM_GPIO_DIR_INPUT;
            switch (gpio->pull)
            {
            case Luat_GPIO_PULLUP:
                attr = WM_GPIO_ATTR_PULLHIGH;
                break;
            case Luat_GPIO_PULLDOWN:
                attr = WM_GPIO_ATTR_PULLLOW;
                break;
            case Luat_GPIO_DEFAULT:
            default:
                attr = WM_GPIO_ATTR_FLOATING;
                break;
            }
        }
        break;
        default:
            dir = WM_GPIO_DIR_INPUT;
            attr = WM_GPIO_ATTR_FLOATING;
            break;
    }
    tls_gpio_cfg(gpio->pin, dir, attr);

    if (gpio->mode == Luat_GPIO_IRQ)
    {

        if (gpio->irq == Luat_GPIO_RISING)
        {
            irq = WM_GPIO_IRQ_TRIG_RISING_EDGE;
        }
        else if (gpio->irq == Luat_GPIO_FALLING)
        {
            irq = WM_GPIO_IRQ_TRIG_FALLING_EDGE;
        }
        else
        {
            irq = WM_GPIO_IRQ_TRIG_DOUBLE_EDGE;
        }
        tls_clr_gpio_irq_status(gpio->pin);
        tls_gpio_isr_register(gpio->pin, luat_gpio_irq_callback, (void *)gpio->pin);
        // if (gpio->irq_cb) {
        //     confs[gpio->pin].cb = gpio->irq_cb;
        //     confs[gpio->pin].args = gpio->irq_args;
        // }
        tls_gpio_irq_enable(gpio->pin, irq);
        return 0;
    }
    else{
        tls_gpio_irq_disable(gpio->pin);
    }
    return 0;
}

int luat_gpio_set(int pin, int level)
{
    if (pin < 0 || pin > WM_IO_PB_31) return 0;
    tls_gpio_write(pin, level);
    return 0;
}

//hyj
void luat_gpio_pulse(int pin, uint8_t *level, uint16_t len,uint16_t delay_ns)
{
    if (pin < 0 || pin > WM_IO_PB_31) return 0;
    tls_gpio_pulse(pin,level,len,delay_ns);
    return 0;
}

int luat_gpio_get(int pin)
{
    if (pin < 0 || pin > WM_IO_PB_31) return 0;
    int re = tls_gpio_read(pin);
    return re;
}

void luat_gpio_close(int pin)
{
    if (pin < 0 || pin > WM_IO_PB_31) return;
    tls_gpio_cfg(pin, WM_GPIO_DIR_INPUT, WM_GPIO_ATTR_FLOATING);
    tls_gpio_irq_disable(pin);
    // confs[pin].cb = NULL;
}

// int luat_gpio_set_irq_cb(int pin, luat_gpio_irq_cb cb, void* args) {
//     if (pin < 0 || pin > WM_IO_PB_31) return -1;
//     if (cb) {
//         confs[pin].cb = cb;
//         confs[pin].args = args;
//     }
//     return 0;
// }
