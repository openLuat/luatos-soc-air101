// #include "luat_conf_bsp.h"
#include "luat_base.h"
#include "luat_fs.h"
#define LUAT_LOG_TAG "fs"
#include "luat_log.h"
#include "lfs_port.h"
#include "wm_include.h"
#include "luat_timer.h"
#include "stdio.h"
#include "luat_ota.h"
#include "wm_internal_flash.h"

extern struct lfs_config lfs_cfg;
extern lfs_t lfs;

// 分区信息

// KV -- 64k
// luadb -- N k
// lfs - (112 + 64 - N)k
uint32_t kv_addr;
uint32_t kv_size_kb = 64;
uint32_t luadb_addr;
uint32_t luadb_size_kb;
uint32_t lfs_addr;
uint32_t lfs_size_kb;

#ifndef FLASH_FS_REGION_SIZE
#define FLASH_FS_REGION_SIZE 112
#endif

extern const struct luat_vfs_filesystem vfs_fs_lfs2;
extern const char luadb_inline_sys[];
extern const struct luat_vfs_filesystem vfs_fs_luadb;

#ifdef LUAT_USE_LVGL
#include "lvgl.h"
void luat_lv_fs_init(void);
// void lv_bmp_init(void);
// void lv_png_init(void);
void lv_split_jpeg_init(void);
#endif

int luat_fs_init(void) {
    //luat_timer_mdelay(1000);
#ifdef AIR103
    luadb_addr =  0x0E0000 - (FLASH_FS_REGION_SIZE - 112) * 1024U;
#else
    luadb_addr =  0x1E0000 - (FLASH_FS_REGION_SIZE - 112) * 1024U;
#endif
    //LLOGD("luadb_addr 0x%08X", luadb_addr);
    uint8_t *ptr = (uint8_t*)(luadb_addr + 0x8000000); //0x80E0000
    //LLOGD("luadb_addr ptr %p", ptr);

    // 兼容老的LuaTools, 并提示更新
    static const uint8_t luadb_magic[] = {0x01, 0x04, 0x5A, 0xA5};
    uint8_t header[4];
    memcpy(header, ptr, 4);
    //LLOGD(">> %02X %02X %02X %02X", header[0], header[1], header[2], header[3]);
    if (memcmp(header, luadb_magic, 4)) {
        // 老的布局
        LLOGW("Legacy non-LuaDB download, please upgrade your LuatIDE or LuatTools %p", ptr);
        // lfs_addr = luadb_addr;
        // kv_addr = lfs_addr - kv_size_kb*1024U;
        // lfs_size_kb = FLASH_FS_REGION_SIZE;
        // luadb_addr = 0;
    }
    // else {
        //LLOGI("Using LuaDB as script zone format %p", ptr);
#ifdef AIR103
        lfs_addr = 0x0F0000;
        lfs_size_kb = 48;
#else
        lfs_addr = 0x1F0000;
        lfs_size_kb = 48;
#endif
        kv_addr = luadb_addr - kv_size_kb*1024U;
    // }

    //LLOGD("lfs addr4 %p", &lfs_addr);
    //LLOGD("lfs addr5 0x%08X", lfs_addr);
    //LLOGD("luadb_addr 0x%08X", luadb_addr);

    LFS_Init();
    luat_vfs_reg(&vfs_fs_lfs2);
	luat_fs_conf_t conf = {
		.busname = &lfs,
		.type = "lfs2",
		.filesystem = "lfs2",
		.mount_point = "/"
	};
	luat_fs_mount(&conf);

    #ifdef LUAT_USE_OTA
    //OTA检测升级
    luat_ota(luadb_addr);
    #endif
    
	luat_vfs_reg(&vfs_fs_luadb);
	luat_fs_conf_t conf2 = {
		.busname = (char*)(ptr),
		.type = "luadb",
		.filesystem = "luadb",
		.mount_point = "/luadb/",
	};
	luat_fs_mount(&conf2);

	#ifdef LUAT_USE_LVGL
	luat_lv_fs_init();
	// lv_bmp_init();
	// lv_png_init();
	lv_split_jpeg_init();
	#endif

	return 0;
}

#ifdef LUAT_USE_OTA
int luat_flash_write(uint32_t addr, uint8_t * buf, uint32_t len){
    return tls_fls_write(addr, buf, len);
}
#endif
