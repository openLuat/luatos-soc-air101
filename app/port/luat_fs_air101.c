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

uint32_t kv_addr;
uint32_t kv_size_kb;
uint32_t luadb_addr;
uint32_t luadb_size_kb;
uint32_t lfs_addr;
uint32_t lfs_size_kb;


extern const struct luat_vfs_filesystem vfs_fs_lfs2;
extern const char luadb_inline_sys[];
extern const struct luat_vfs_filesystem vfs_fs_luadb;
extern const struct luat_vfs_filesystem vfs_fs_ram;
extern size_t luat_luadb_act_size;

#ifdef LUAT_USE_LVGL
#include "lvgl.h"
void luat_lv_fs_init(void);
// void lv_bmp_init(void);
// void lv_png_init(void);
void lv_split_jpeg_init(void);
#endif

void luat_fs_update_addr(void) {
	kv_addr = LUAT_PARTITION_KV_ADDR;
	kv_size_kb = LUAT_PARTITION_KV_SIZE / 1024U;
	luadb_addr = LUAT_PARTITION_SCRIPT_ADDR;
	luadb_size_kb = LUAT_PARTITION_SCRIPT_SIZE / 1024U;
	lfs_addr = LUAT_PARTITION_FS_ADDR;
	lfs_size_kb = LUAT_PARTITION_FS_SIZE / 1024U;
    
	if (lfs_size_kb != 48)
		LLOGD("可读写文件系统大小 %dkb flash偏移量 %08X", lfs_size_kb, lfs_addr);
}

int luat_fs_init(void) {
    uint8_t *ptr = (uint8_t*)(luadb_addr + 0x8000000); //0x80E0000
    LFS_Init();
    luat_vfs_reg(&vfs_fs_lfs2);
	luat_fs_conf_t conf = {
		.busname = (char*)&lfs,
		.type = "lfs2",
		.filesystem = "lfs2",
		.mount_point = "/"
	};
	luat_fs_mount(&conf);

    luat_vfs_reg(&vfs_fs_ram);
    luat_fs_conf_t conf3 = {
		.busname = NULL,
		.type = "ram",
		.filesystem = "ram",
		.mount_point = "/ram/"
	};
	luat_fs_mount(&conf3);
    
	luat_vfs_reg(&vfs_fs_luadb);
	luat_fs_conf_t conf2 = {
		.busname = (char*)(ptr),
		.type = "luadb",
		.filesystem = "luadb",
		.mount_point = "/luadb/",
	};
	luat_fs_mount(&conf2);
	luat_luadb_act_size = luadb_size_kb;

	#ifdef LUAT_USE_LVGL
	luat_lv_fs_init();
	// lv_bmp_init();
	// lv_png_init();
	lv_split_jpeg_init();
	#endif

	return 0;
}
