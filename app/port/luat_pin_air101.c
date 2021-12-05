

#include "wm_gpio_afsel.h"
#include "luat_base.h"
#include "luat_pin.h"

#define LUAT_LOG_TAG "pin"
#include "luat_log.h"

int luat_pin_to_gpio(const char* pin_name) {
    int zone = 0;
    int index = 0;
    int re = 0;
    re = luat_pin_parse(pin_name, &zone, &index);
    if (re < 0) {
        return -1;
    }
    // PA00 ~ PA15
    if (zone == 0 && index < 16) {
        return index;
    }
    // PB00 ~ PB32
    if (zone == 1 && index < 32) {
        return index + 16;
    }
    return -1;
}
