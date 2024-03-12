
#include "string.h"
#include "wm_include.h"
#include "wm_crypto_hard.h"
#include "aes.h"
#include "wm_osal.h"
#include "wm_regs.h"
#include "wm_debug.h"
#include "wm_crypto_hard.h"
#include "wm_internal_flash.h"
#include "wm_pmu.h"
#include "wm_fwup.h"

#include "luat_base.h"
#include "luat_crypto.h"
#include "luat_malloc.h"
#define LUAT_LOG_TAG "fota"
#include "luat_log.h"

#include "FreeRTOS.h"
#include "task.h"

#include "luat_fota.h"

static const uint32_t MAGIC_NO = 0xA0FFFF9F;

enum {
    FOTA_IDLE,
    FOTA_ONGO,
    FOTA_DONE
};

static int fota_state;
static uint32_t fota_write_offset;
static uint32_t ota_zone_size;
static uint32_t upgrade_img_addr;

static int check_image_head(IMAGE_HEADER_PARAM_ST* imghead, const char* tag);

int luat_fota_init(uint32_t start_address, uint32_t len, luat_spi_device_t* spi_device, const char *path, uint32_t pathlen) {
    fota_state = FOTA_ONGO;
    fota_write_offset = 0;
    // 读取update区域位置及大小, 按4k对齐的方式, 清除对应的区域
    for (size_t i = 0; i < ota_zone_size / 4096; i++)
    {
        LLOGD("清除ota区域: %08X", upgrade_img_addr + i * 4096);
        tls_fls_erase((upgrade_img_addr + i * 4096) / INSIDE_FLS_SECTOR_SIZE);
    }
    LLOGI("OTA区域初始化完成");
    return 0;
}

int luat_fota_write(uint8_t *data, uint32_t len) {
    if (len + fota_write_offset > ota_zone_size) {
        LLOGE("OTA区域写满, 无法继续写入");
        return -1;
    }
    int ret = tls_fls_write_without_erase(upgrade_img_addr + fota_write_offset, data, len);
    fota_write_offset += len;
    return ret;
}

int luat_fota_done(void) {
    if (fota_write_offset == 0) {
        LLOGE("未写入任何数据, 无法完成OTA");
        return -1;
    }
    if (fota_write_offset < sizeof(IMAGE_HEADER_PARAM_ST)) {
        LLOGI("写入数据小于最小长度, 还不能判断");
        return 0;
    }
    // 写入长度已经超过最小长度, 判断是否是合法的镜像
    IMAGE_HEADER_PARAM_ST* imghead = (IMAGE_HEADER_PARAM_ST*)upgrade_img_addr;
    if (imghead->magic_no != MAGIC_NO) {
        LLOGE("image magic: %08x", imghead->magic_no);
        return -2;
    }
    if (imghead->img_len + sizeof(IMAGE_HEADER_PARAM_ST) < fota_write_offset) {
        LLOGI("fota数据还不够, 继续等数据");
        return 0;
    }
    // 写入长度足够, 判断是否是合法的镜像, 开始计算check sum
    if (check_image_head(imghead, "fota") == 0) {
        LLOGI("image check sum: %08x", imghead->hd_checksum);
        fota_state = FOTA_DONE;
        return 0;
    }

    return 0;
}

int luat_fota_end(uint8_t is_ok) {
    if (fota_state == FOTA_DONE) {
        return 0;
    }
    LLOGD("状态不正确, 要么数据没写完,要么校验没通过");
    return -1;
}

uint8_t luat_fota_wait_ready(void) {
    return 0;
}

static uint32_t img_checksum(const char* ptr, size_t len) {
    psCrcContext_t	crcContext;
	unsigned int crcvalue = 0;
	unsigned int crccallen = 0;
	unsigned int i = 0;

    crccallen = len;

	tls_crypto_crc_init(&crcContext, 0xFFFFFFFF, CRYPTO_CRC_TYPE_32, 3);
	for (i = 0; i <  crccallen/4; i++)
	{
		MEMCPY((unsigned char *)&crcvalue, (unsigned char *)ptr +i*4, 4); 
		tls_crypto_crc_update(&crcContext, (unsigned char *)&crcvalue, 4);
	}
	crcvalue = 0;
	tls_crypto_crc_final(&crcContext, &crcvalue);

    return crcvalue;
}

static int check_image_head(IMAGE_HEADER_PARAM_ST* imghead, const char* tag) {
    if (imghead == NULL) {
        return -1;
    }
    if (imghead->magic_no != MAGIC_NO) {
        LLOGE("%s image magic: %08x", tag, imghead->magic_no);
        return -2;
    }
    LLOGD("%s image img_addr: %08X", tag, imghead->img_addr);
    LLOGD("%s image img_len: %08X", tag, imghead->img_len);
    LLOGD("%s image img_header_addr: %08X", tag, imghead->img_header_addr);
    LLOGD("%s image upgrade_img_addr: %08X", tag, imghead->upgrade_img_addr);
    LLOGD("%s image org_checksum: %08X", tag, imghead->org_checksum);
    LLOGD("%s image upd_no: %08X", tag, imghead->upd_no);
    LLOGD("%s image ver: %.16s", tag, imghead->ver);
    LLOGD("%s image hd_checksum: %08X", tag, imghead->hd_checksum);
    LLOGD("%s image next: %08X", tag, imghead->next);
    
    // image attr
    LLOGD("%s image attr img_type: %d", tag, imghead->img_attr.b.img_type);
    LLOGD("%s image attr zip_type: %d", tag, imghead->img_attr.b.zip_type);
    LLOGD("%s image attr psram_io: %d", tag, imghead->img_attr.b.psram_io);
    LLOGD("%s image attr erase_block_en: %d", tag, imghead->img_attr.b.erase_block_en);
    LLOGD("%s image attr erase_always: %d", tag, imghead->img_attr.b.erase_always);
    
    // 先判断一下magicno
    if (imghead->magic_no != MAGIC_NO) {
        return -2;
    }
    // 计算一下header的checksum
    uint32_t cm = img_checksum((const char*)imghead, sizeof(IMAGE_HEADER_PARAM_ST) - 4);
    LLOGD("%s head checksum: %08X %08X", tag, cm, imghead->hd_checksum);
    if (cm != imghead->hd_checksum) {
        return -3;
    }
    cm = img_checksum((const char*)imghead->img_addr, imghead->img_len);
    LLOGD("%s data checksum: %08X %08X", tag, cm, imghead->org_checksum);
    if (cm != imghead->org_checksum) {
        return -4;
    }

    return 0;
}

void luat_fota_boot_check(void) {
    LLOGD("启动fota开机检查");
    LLOGD("sizeof(IMAGE_HEADER_PARAM_ST) %d %d %d", sizeof(IMAGE_HEADER_PARAM_ST), sizeof(Img_Attr_Type), sizeof(unsigned int));
    // 读取secboot区域的信息, 大小1kb
    IMAGE_HEADER_PARAM_ST* secimg = (IMAGE_HEADER_PARAM_ST*)0x8002000;
    // check_image_head(secimg, "secboot");
    IMAGE_HEADER_PARAM_ST* upimg = (IMAGE_HEADER_PARAM_ST*)secimg->upgrade_img_addr;
    IMAGE_HEADER_PARAM_ST* runimg = (secimg->next);

    // check_image_head(upimg, "update");
    // check_image_head(runimg, "user");

    // 计算出OTA区域大小, 运行区镜像的大小
    uint32_t ota_size = ((uint32_t)runimg) - ((uint32_t)upimg);
    LLOGD("ota zone size: 0x%08X %dkb", ota_size, ota_size/1024);

    // 当前运行区镜像的大小
    LLOGD("run image size: 0x%08X %dkb", runimg->img_len, runimg->img_len/1024);

    // 把相关参数存起来
    upgrade_img_addr = secimg->upgrade_img_addr;
    
    ota_zone_size = ((uint32_t)runimg) - ((uint32_t)upimg);
    ota_zone_size = (ota_zone_size + 0x3FF) & (~0x3FF);
}
