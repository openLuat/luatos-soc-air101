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
extern int zlib_decompress(FILE *source, FILE *dest);
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

    //检测是否有升级文件
    if(luat_fs_fexist("/update")){
        FILE *fd_in = luat_fs_fopen("/update", "r");
        FILE *fd_out = luat_fs_fopen("/update.bin", "w+");
        zlib_decompress(fd_in, fd_out);
        luat_fs_fclose(fd_in);
        luat_fs_fclose(fd_out);
        size_t binsize = luat_fs_fsize("/update.bin");
        LLOGI("update.bin size:%d",binsize);
        uint8_t* binbuff = (uint8_t*)luat_heap_malloc(binsize * sizeof(uint8_t));
        FILE * fd = luat_fs_fopen("/update.bin", "rb");
        if (fd) {
            luat_fs_fread(binbuff, sizeof(uint8_t), binsize, fd);
            //做一下校验
            if (binbuff[0] != 0x01 || binbuff[1] != 0x04 || binbuff[2]+(binbuff[3]<<8) != 0xA55A || binbuff[4]+(binbuff[5]<<8) != 0xA55A){
                LLOGI("Magic error");
                goto _close;
            }
            LLOGI("Magic OK");
            if (binbuff[6] != 0x02 || binbuff[7] != 0x02 || binbuff[8] != 0x02 || binbuff[9] != 0x00){
                LLOGI("Version error");
                goto _close;
            }
            LLOGI("Version OK");
            if (binbuff[10] != 0x03 || binbuff[11] != 0x04){
                LLOGI("Header error");
                goto _close;
            }
            uint32_t headsize = binbuff[12]+(binbuff[13]<<8)+(binbuff[14]<<16)+(binbuff[15]<<24);
            LLOGI("Header OK headers:%08x",headsize);

            if (binbuff[16] != 0x04 || binbuff[17] != 0x02){
                LLOGI("file count error");
                goto _close;
            }
            uint16_t filecount = binbuff[18]+(binbuff[19]<<8);
            LLOGI("file count:%04x",filecount);
            if (binbuff[20] != 0xFE || binbuff[21] != 0x02){
                LLOGI("CRC16 error");
                goto _close;
            }
            uint16_t CRC16 = binbuff[22]+(binbuff[23]<<8);
            // LLOGI("CRC16:%04x",CRC16);

            tls_fls_write(luadb_addr, binbuff, binsize);
_close:
            luat_fs_fclose(fd);
        }
        luat_heap_free(binbuff);
        //不论成功与否都删掉避免每次启动都执行一遍
        luat_fs_remove("/update.bin");
        luat_fs_remove("/update");
    }

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
