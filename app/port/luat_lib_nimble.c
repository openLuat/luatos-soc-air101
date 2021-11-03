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

static int l_nimble_gap_adv(lua_State* L) {
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
	{ NULL,             NULL ,              0}
};

LUAMOD_API int luaopen_nimble( lua_State *L ) {
    luat_newlib(L, reg_nimble);
    return 1;
}

