
#include "luat_base.h"
#include "luat_ota.h"
#include "luat_fs.h"
#include "luat_timer.h"
#include "luat_malloc.h"
#include "luat_flash.h"

extern uint32_t luadb_addr;

int luat_ota_exec(void) {
    return luat_ota(luadb_addr);
}
