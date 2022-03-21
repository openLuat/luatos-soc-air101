#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_malloc.h"
#include "luat_spi.h"

#include "luat_nimble.h"

#define LUAT_LOG_TAG "nimble"
#include "luat_log.h"

static int l_nimble_init(lua_State* L) {
    int rc = 0;
    char* name = NULL;
    if(lua_isstring(L, 1)) {
        name = lua_tostring(L, 1);
    }
    rc = luat_nimble_init(0xFF,name);
    if (rc) {
        lua_pushboolean(L, 0);
        lua_pushinteger(L, rc);
        return 2;
    }
    else {
        lua_pushboolean(L, 1);
        return 1;
    }
}

static int l_nimble_deinit(lua_State* L) {
    int rc = 0;
    rc = luat_nimble_deinit();
    if (rc) {
        lua_pushboolean(L, 0);
        lua_pushinteger(L, rc);
        return 2;
    }
    else {
        lua_pushboolean(L, 1);
        return 1;
    }
}

static int l_nimble_debug(lua_State* L) {
    int level;
    if (lua_gettop(L) > 0)
        level = luat_nimble_trace_level(luaL_checkinteger(L, 1));
    else
        level = luat_nimble_trace_level(-1);
    lua_pushinteger(L, level);
    return 1;
}

static int l_nimble_server_init(lua_State* L) {
    test_server_api_init();
    return 0;
}


static int l_nimble_server_deinit(lua_State* L) {
    test_server_api_deinit();
    return 0;
}

#include "wm_bt_config.h"
#include "wm_params.h"
#include "wm_param.h"
#include "wm_ble.h"

int tls_nimble_gap_adv(wm_ble_adv_type_t type, int duration);

static int l_nimble_gap_adv(lua_State* L) {
    wm_ble_adv_type_t type = luaL_checkinteger(L, 1);
    int duration = luaL_optinteger(L, 2, 0);

    int ret = tls_nimble_gap_adv(type, duration);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    lua_pushinteger(L, ret);
    return 2;
}

static int l_nimble_send_msg(lua_State *L) {
    int conn_id = luaL_checkinteger(L, 1);
    int handle_id = luaL_checkinteger(L, 2);
    size_t len;
    const char* data = luaL_checklstring(L, 3, &len);
    int ret = tls_ble_server_api_send_msg(data, len);

    lua_pushboolean(L, ret == 0 ? 1 : 0);
    lua_pushinteger(L, ret);
    return 2;
}

int
ble_ibeacon_set_adv_data(void *uuid128, uint16_t major,
                         uint16_t minor, int8_t measured_power);

static int l_ibeacon_set_adv_data(lua_State *L) {
    uint8_t buff[16];
    size_t len = 0;
    const char* data = luaL_checklstring(L, 1, &len);
    memset(buff, 0, 16);
    memcpy(buff, data, len > 16 ? 16 : len);
    uint16_t major = luaL_optinteger(L, 2, 2);
    uint16_t minor = luaL_optinteger(L, 3, 10);
    int8_t measured_power = luaL_optinteger(L, 4, 0);

    int ret = ble_ibeacon_set_adv_data(buff, major, minor, measured_power);
    lua_pushboolean(L, ret == 0 ? 1 : 0);
    lua_pushinteger(L, ret);
    return 2;
}

static int l_gap_adv_start(lua_State *L) {
    return 0;
}

static int l_gap_adv_stop(lua_State *L) {
    return 0;
}

#include "rotable.h"
static const rotable_Reg reg_nimble[] =
{
	{ "init",           l_nimble_init,      0},
    { "deinit",         l_nimble_deinit,      0},
    { "debug",          l_nimble_debug,     0},
    { "server_init",    l_nimble_server_init,      0},
    { "server_deinit",  l_nimble_server_deinit,     0},
    { "gap_adv",        l_nimble_gap_adv,      0},
    { "send_msg",       l_nimble_send_msg,     0},

    // for ibeacon
    { "ibeacon_set_adv_data", l_ibeacon_set_adv_data, 0},
    // { "gap_adv_start", l_gap_adv_start, 0},
    // { "gap_adv_stop", l_gap_adv_stop, 0},
	{ NULL,             NULL ,              0}
};

LUAMOD_API int luaopen_nimble( lua_State *L ) {
    luat_newlib(L, reg_nimble);
    return 1;
}

