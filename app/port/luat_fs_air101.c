// #include "luat_conf_bsp.h"
#include "luat_base.h"
#include "luat_fs.h"
#define LUAT_LOG_TAG "fs"
#include "luat_log.h"
#include "lfs_port.h"
#include "wm_include.h"
#include "luat_timer.h"
#include "stdio.h"

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

#define UPDATE_TGZ_PATH "/update.tgz"
#define UPDATE_BIN_PATH "/update.bin"

extern int zlib_decompress(FILE *source, FILE *dest);
extern int luat_luadb_checkfile(const char* path);

static int luat_ota(void){
    int ret  = 0;
    //检测是否有压缩升级文件
    if(luat_fs_fexist(UPDATE_TGZ_PATH)){
        LLOGI("found update.tgz, decompress ...");
        FILE *fd_in = luat_fs_fopen(UPDATE_TGZ_PATH, "r");
        if (fd_in == NULL){
            LLOGE("open the input file : %s error!", UPDATE_TGZ_PATH);
            ret = -1;
            goto _close_decompress;
        }
        luat_fs_remove(UPDATE_BIN_PATH);
        FILE *fd_out = luat_fs_fopen(UPDATE_BIN_PATH, "w+");
        if (fd_out == NULL){
            LLOGE("open the output file : %s error!", UPDATE_BIN_PATH);
            ret = -1;
            goto _close_decompress;
        }
        ret = zlib_decompress(fd_in, fd_out);
        if (ret != 0){
            LLOGE("decompress file error!");
        }
_close_decompress:
        if(fd_in != NULL){
            luat_fs_fclose(fd_in);
        }
        if(fd_out != NULL){
            luat_fs_fclose(fd_out);
        }
        //不论成功与否都删掉避免每次启动都执行一遍
        luat_fs_remove(UPDATE_TGZ_PATH);
    }

    //检测是否有升级文件
    if(luat_fs_fexist(UPDATE_BIN_PATH)){
        LLOGI("found update.bin, checking");
        if (luat_luadb_checkfile(UPDATE_BIN_PATH) == 0) {
            LLOGI("update.bin ok, updating...");
            #define UPDATE_BUFF_SIZE 4096
            char* buff = luat_heap_malloc(UPDATE_BUFF_SIZE);
            int len = 0;
            int offset = 0;
            if (buff != NULL) {
                FILE* fd = luat_fs_fopen(UPDATE_BIN_PATH, "rb");
                while (1) {
                    len = luat_fs_fread(buff, UPDATE_BUFF_SIZE, 1, fd);
                    if (len < 1)
                        break;
                    tls_fls_write(luadb_addr + offset, buff, len);
                    offset += len;
                }
            }
        }
        else {
            ret = -1;
            LLOGW("update.bin NOT ok, skip");
        }
        luat_fs_remove(UPDATE_BIN_PATH);
    }
    return ret;
}


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
        lfs_addr = luadb_addr;
        kv_addr = lfs_addr - 64*1024U;
        lfs_size_kb = FLASH_FS_REGION_SIZE;
        luadb_addr = 0;
    }
    else {
        LLOGI("Using LuaDB as script zone format %p", ptr);
        // TODO 根据LuaDB的区域动态调整?
#ifdef AIR103
        lfs_addr = 0x0F0000;
        lfs_size_kb = 48;
#else
        lfs_addr = 0x1F0000;
        lfs_size_kb = 48;
#endif
        kv_addr = luadb_addr - 64*1024U;
    }

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
    //OTA检测升级
    luat_ota();
	luat_vfs_reg(&vfs_fs_luadb);
	luat_fs_conf_t conf2 = {
		.busname = (char*)(luadb_addr == 0 ? luadb_inline_sys : ptr),
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
