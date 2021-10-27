
#include "luat_base.h"
#include "luat_sdio.h"

#define LUAT_LOG_TAG "luat.sdio"
#include "luat_log.h"

#include "wm_sdio_host.h"
#include "luat_fs.h"
#include "ff.h"

FATFS fs;
BYTE work[FF_MAX_SS];

#ifdef LUAT_USE_FS_VFS
extern const struct luat_vfs_filesystem vfs_fs_fatfs;
#endif

int luat_sdio_init(int id){
	if (id == 0) {
		wm_sdio_host_config(0);
		return 0;
	}
#ifdef AIR103
	else if (id == 1) {
		wm_sdio_host_config(1);
		return 0;
	}
#endif
	return -1;
}

int luat_sdio_sd_read(int id, int rca, char* buff, size_t offset, size_t len){
	return wm_sd_card_blocks_read(rca, offset, buff, len);
}

int luat_sdio_sd_write(int id, int rca, char* buff, size_t offset, size_t len){
	return wm_sd_card_blocks_write(rca, offset, buff, len);
}

int luat_sdio_sd_mount(int id, int *rca, char* path,int auto_format){
	FRESULT res_sd;
	res_sd = f_mount(&fs, "/", 1);
	if (res_sd) {
		LLOGD("mount ret = %d", res_sd);
		if (res_sd == FR_NO_FILESYSTEM && auto_format) {
			res_sd = f_mkfs("/", 0, work, sizeof(work));
			if(res_sd == FR_OK){
				res_sd = f_mount(NULL, "/", 1);
				res_sd = f_mount(&fs, "/", 1);
			}
			else {
				LLOGD("format ret = %d", res_sd);
			}
		}
	}
	if (res_sd != FR_OK) {
		return res_sd;
	}
#ifdef LUAT_USE_FS_VFS
	luat_vfs_reg(&vfs_fs_fatfs);
	luat_fs_conf_t conf = {
		.busname = &fs,
		.type = "fatfs",
		.filesystem = "fatfs",
		.mount_point = path, 
	};
	luat_fs_mount(&conf);
#endif
	return 0;
}

int luat_sdio_sd_unmount(int id, int rca){
	return f_mount(NULL, "/", 1);
}

int luat_sdio_sd_format(int id, int rca){
	f_mkfs("/", 0, work, sizeof(work));
	return 0;
}
