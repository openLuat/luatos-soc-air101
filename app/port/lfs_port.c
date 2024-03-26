#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "lfs_port.h"
#include "lfs_util.h"

#include "luat_base.h"

#define LUAT_LOG_TAG "lfs"
#include "luat_log.h"
#include "wm_include.h"
#include "lfs.h"
#include "wm_flash_map.h"
#include "wm_internal_flash.h"
#include "luat_timer.h"


extern uint32_t lfs_addr;
extern uint32_t lfs_size_kb;


#ifdef LFS_START_ADDR
#undef LFS_START_ADDR
#endif

#define FLASH_FS_REGION_SIZE lfs_size_kb
#define LFS_START_ADDR lfs_addr

/***************************************************
 ***************       MACRO      ******************
 ***************************************************/
#define LFS_BLOCK_DEVICE_READ_SIZE (256)
#define LFS_BLOCK_DEVICE_PROG_SIZE (256)
#define LFS_BLOCK_DEVICE_CACHE_SIZE (256)
#define LFS_BLOCK_DEVICE_ERASE_SIZE (4096) // one sector 4KB
#define LFS_BLOCK_DEVICE_TOTOAL_SIZE (FLASH_FS_REGION_SIZE * 1024)
#define LFS_BLOCK_DEVICE_LOOK_AHEAD (16)
/***************************************************
 *******    FUNCTION FORWARD DECLARTION     ********
 ***************************************************/



// Read a block
static int block_device_read(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size);

// Program a block
//
// The block must have previously been erased.
static int block_device_prog(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size);

// Erase a block
//
// A block must be erased before being programmed. The
// state of an erased block is undefined.
static int block_device_erase(const struct lfs_config *cfg, lfs_block_t block);

// Sync the block device
static int block_device_sync(const struct lfs_config *cfg);

// utility functions for traversals
static int lfs_statfs_count(void *p, lfs_block_t b);

/***************************************************
 ***************  GLOBAL VARIABLE  *****************
 ***************************************************/

#ifdef LFS_THREAD_SAFE_MUTEX
static osMutexId_t lfs_mutex;
#endif

static char lfs_read_buf[256];
static char lfs_prog_buf[256];
// static __ALIGNED(4) char lfs_lookahead_buf[LFS_BLOCK_DEVICE_LOOK_AHEAD];
static  char  __attribute__((aligned(4))) lfs_lookahead_buf[LFS_BLOCK_DEVICE_LOOK_AHEAD];

// configuration of the filesystem is provided by this struct
struct lfs_config lfs_cfg =
{
    .context = NULL,
    // block device operations
    .read = block_device_read,
    .prog = block_device_prog,
    .erase = block_device_erase,
    .sync = block_device_sync,

    // block device configuration
    .read_size = LFS_BLOCK_DEVICE_READ_SIZE,
    .prog_size = LFS_BLOCK_DEVICE_PROG_SIZE,
    .block_size = LFS_BLOCK_DEVICE_ERASE_SIZE,
    //.block_count = LFS_BLOCK_DEVICE_TOTOAL_SIZE / LFS_BLOCK_DEVICE_ERASE_SIZE,
    .block_cycles = 200,
    .cache_size = LFS_BLOCK_DEVICE_CACHE_SIZE,
    .lookahead_size = LFS_BLOCK_DEVICE_LOOK_AHEAD,

    .read_buffer = lfs_read_buf,
    .prog_buffer = lfs_prog_buf,
    .lookahead_buffer = lfs_lookahead_buf,
    .name_max = 63,
    .file_max = 0,
    .attr_max = 0
};

lfs_t lfs;

/***************************************************
 *******         INTERANL FUNCTION          ********
 ***************************************************/

static int block_device_read(const struct lfs_config *cfg, lfs_block_t block,
                             lfs_off_t off, void *buffer, lfs_size_t size)
{
    int ret;
    //LLOGD("block_device_read ,block = %d, off = %d,  size = %d",block, off, size);
    ret = tls_fls_read(block * 4096 + off + LFS_START_ADDR, (u8 *)buffer, size);
    //LLOGD("block_device_read return val : %d",ret);
    if (ret != TLS_FLS_STATUS_OK)
    {
        return -1;
    }
    return 0;
}

static int block_device_prog(const struct lfs_config *cfg, lfs_block_t block,
                             lfs_off_t off, const void *buffer, lfs_size_t size)
{
    int ret;
    //LLOGD("block_device_prog ,block = %d, off = %d,  size = %d",block, off, size);
    ret = tls_fls_write_without_erase(block * 4096 + off + LFS_START_ADDR, (u8 *)buffer, size);
    //LLOGD("block_device_prog return val : %d",ret);
    if (ret != TLS_FLS_STATUS_OK)
    {
        return -1;
    }

    return 0;
}

static int block_device_erase(const struct lfs_config *cfg, lfs_block_t block)
{
    int ret;
    ret = tls_fls_erase((block * 4096 + LFS_START_ADDR) / INSIDE_FLS_SECTOR_SIZE);
    if (ret != TLS_FLS_STATUS_OK)
    {
        return -1;
    }
    return 0;
}

static int block_device_sync(const struct lfs_config *cfg)
{
    return 0;
}

static int lfs_statfs_count(void *p, lfs_block_t b)
{
    *(lfs_size_t *)p += 1;

    return 0;
}

// Initialize
int LFS_Init(void)
{
    //luat_timer_mdelay(1000);
    //LLOGD("lfs addr %08X", LFS_START_ADDR);
    // LLOGD("lfs addr2 %08X", lfs_addr);
    // LLOGD("lfs addr3 %p", &lfs_addr);
    // LLOGD("lfs block_count %d", LFS_BLOCK_DEVICE_TOTOAL_SIZE / LFS_BLOCK_DEVICE_ERASE_SIZE);
    //printf("------------LFS_Init------------\r\n");
    // mount the filesystem
    lfs_cfg.block_count = LFS_BLOCK_DEVICE_TOTOAL_SIZE / LFS_BLOCK_DEVICE_ERASE_SIZE;
    int err = lfs_mount(&lfs, &lfs_cfg);
    //printf("lfs_mount %d\r\n",err);
    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err)
    {
        luat_timer_mdelay(100);
        LLOGI("lfs mount fail ret=%d , exec format", err);
        err = lfs_format(&lfs, &lfs_cfg);
        //printf("lfs_format %d\r\n",err);
        if(err) {
            LLOGW("lfs format fail ret=%d", err);
            return err;
        }

        err = lfs_mount(&lfs, &lfs_cfg);
        //printf("lfs_mount %d\r\n",err);
        if(err)
            return err;
    }
    return 0;
}
