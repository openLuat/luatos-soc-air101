#include "wm_gpio_afsel.h"
#include "luat_base.h"

#define LUAT_LOG_TAG "pin"
#include "luat_log.h"

static int get_pin_index(lua_State *L){
    char pin_sn[3]={0};
    int pin_n;
    const char* pin_name = luaL_checkstring(L, 1);
    if (memcmp("PA",pin_name, 2)==0){
        strncpy(pin_sn, pin_name+2, 2);
        pin_n = atoi(pin_sn);
        if (pin_n >= 0 && pin_n < 48){
            lua_pushinteger(L,pin_n);
            return 1;
        }else
            goto error;
    }else if(memcmp("PB",pin_name, 2)==0){
        strncpy(pin_sn, pin_name+2, 2);
        pin_n = atoi(pin_sn);
        if (pin_n >= 0 && pin_n < 48){
            lua_pushinteger(L,pin_n+16);
            return 1;
        }else
            goto error;
    }
error:
    LLOGW("pin does not exist");
    return 0;
}

#include "rotable.h"
static const rotable_Reg reg_pin[] =
{
    {"__index", get_pin_index, 0},
	{ NULL, NULL , 0}
};

LUAMOD_API int luaopen_pin( lua_State *L ) {
    luat_newlib(L, reg_pin);
    return 1;
}