
#include "luat_base.h"
#include "luat_sdio.h"

#define LUAT_LOG_TAG "sdio"
#include "luat_log.h"

#include "wm_include.h"
#include "wm_gpio_afsel.h"
#include "wm_sdio_host.h"
#include "luat_fs.h"
#include "ff.h"
#include "diskio.h"		

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
// #ifdef AIR103
	else if (id == 1) {
		wm_sdio_host_config(1);
		return 0;
	}
// #endif
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

#include "wm_sdio_host.h"
#include <string.h>
#include "wm_include.h"

extern int wm_sd_card_set_blocklen(uint32_t blocklen);

#define BLOCK_SIZE  512
#define TRY_COUNT   10

static uint32_t fs_rca;

DSTATUS sdhc_sdio_initialize (
	luat_fatfs_sdio_t* userdata
)
{
		int ret;
	luat_sdio_init(0);
	ret= sdh_card_init(&fs_rca);
	if(ret)
		goto end;
	ret = wm_sd_card_set_bus_width(fs_rca, 2);
	if(ret)
		goto end;
	ret = wm_sd_card_set_blocklen(BLOCK_SIZE); //512
	if(ret)
		goto end;
end:
	return ret;
	return 0;
}

DSTATUS sdhc_sdio_status (
	luat_fatfs_sdio_t* userdata
)
{
	//if (drv) return STA_NOINIT;

	return 0;
}


DRESULT sdhc_sdio_read (
	luat_fatfs_sdio_t* userdata,
	BYTE *buff,			/* Pointer to the data buffer to store read data */
	DWORD sector,		/* Start sector number (LBA) */
	UINT count			/* Sector count (1..128) */
)
{
		int ret, i;
	int buflen = BLOCK_SIZE*count;
    BYTE *rdbuff = buff;

	if (((u32)buff)&0x3) /*non aligned 4*/
	{
	    rdbuff = tls_mem_alloc(buflen);
		if (rdbuff == NULL)
		{
			return -1;
		}
	}
    
	for( i=0; i<TRY_COUNT; i++ )
	{   
	    if(count == 1)
	    {
            ret = wm_sd_card_block_read(fs_rca, sector, (char *)rdbuff);
        }
        else if(count > 1)
        {
		    ret = wm_sd_card_blocks_read(fs_rca, sector, (char *)rdbuff, buflen);
        }
		if( ret == 0 ) 
			break;
	}

    if(rdbuff != buff)
    {
        if(ret == 0)
        {
            memcpy(buff, rdbuff, buflen);
        }
        tls_mem_free(rdbuff);
    }

	return ret;
	return 0;
}

DRESULT sdhc_sdio_write (
	luat_fatfs_sdio_t* userdata,
	const BYTE *buff,	/* Pointer to the data to be written */
	DWORD sector,		/* Start sector number (LBA) */
	UINT count			/* Sector count (1..128) */
)
{
		int ret, i;
	int buflen = BLOCK_SIZE*count;
    BYTE *wrbuff = buff;
    
	if (((u32)buff)&0x3)
	{
	    wrbuff = tls_mem_alloc(buflen);
		if (wrbuff == NULL) /*non aligned 4*/
		{
			return -1;
		}
        memcpy(wrbuff, buff, buflen);
	}
	
	for( i = 0; i < TRY_COUNT; i++ )
	{
	    if(count == 1)
	    {
            ret = wm_sd_card_block_write(fs_rca, sector, (char *)wrbuff);
        }
        else if(count > 1)
	    {
		    ret = wm_sd_card_blocks_write(fs_rca, sector, (char *)wrbuff, buflen);
	    }
		if( ret == 0 ) 
		{
			break;
		}
	}

    if(wrbuff != buff)
    {
        tls_mem_free(wrbuff);
    }

	return ret;
	return 0;
}

DRESULT sdhc_sdio_ioctl (
	luat_fatfs_sdio_t* userdata,
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	int res;
			// Process of the command for the MMC/SD card
		switch(ctrl)
		{
#if (FF_FS_READONLY == 0)
			case CTRL_SYNC:
				res = RES_OK;
				break;
#endif
			case GET_SECTOR_COUNT:
				*(DWORD*)buff = SDCardInfo.CardCapacity / BLOCK_SIZE;
				res = RES_OK;
				break;
			case GET_SECTOR_SIZE:
				*(WORD*)buff = BLOCK_SIZE;
				res = RES_OK;
				break;
			case GET_BLOCK_SIZE:
				*(DWORD*)buff = SDCardInfo.CardBlockSize;
				res = RES_OK;
				break;
#if (FF_USE_TRIM == 1)
			case CTRL_TRIM:
				break;
#endif
			default:
				break;
		}

		return res;
	return 0;
}


static const block_disk_opts_t sdhc_sdio_disk_opts = {
    .initialize = sdhc_sdio_initialize,
    .status = sdhc_sdio_status,
    .read = sdhc_sdio_read,
    .write = sdhc_sdio_write,
    .ioctl = sdhc_sdio_ioctl,
};

void luat_sdio_set_sdhc_ctrl(block_disk_t *disk){
	disk->opts = &sdhc_sdio_disk_opts;
}
