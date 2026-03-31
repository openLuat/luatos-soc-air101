
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
#include "wm_flash_map.h"

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
static uint32_t fota_head_check;
// static IMAGE_HEADER_PARAM_ST fota_head;

static int check_image_head(IMAGE_HEADER_PARAM_ST* imghead, const char* tag);
static uint32_t img_checksum(const char* ptr, size_t len);
static void check_ota_zone(void);

int luat_fota_init(uint32_t start_address, uint32_t len, luat_spi_device_t* spi_device, const char *path, uint32_t pathlen) {
    fota_state = FOTA_ONGO;
    fota_write_offset = 0;
    fota_head_check = 0;
    // 读取update区域位置及大小, 按4k对齐的方式, 清除对应的区域
    for (size_t i = 0; i < ota_zone_size / 4096; i++)
    {
        // LLOGD("清除ota区域: %08X", upgrade_img_addr + i * 4096);
        tls_fls_erase((upgrade_img_addr + i * 4096) / INSIDE_FLS_SECTOR_SIZE);
    }
    LLOGI("OTA区域初始化完成, 大小 %d kbyte", ota_zone_size / 1024);
    return 0;
}

int luat_fota_write(uint8_t *data, uint32_t len) {
    if (len + fota_write_offset > ota_zone_size) {
        LLOGD("write %p %d -> %08X %08X", data, len, fota_write_offset, upgrade_img_addr + fota_write_offset);
        LLOGE("OTA区域写满, 无法继续写入");
        return -1;
    }
    int ret = tls_fls_write_without_erase(upgrade_img_addr + fota_write_offset, data, len);
    fota_write_offset += len;
    LLOGD("write %p %d -> %08X %08X", data, len, fota_write_offset, upgrade_img_addr + fota_write_offset);
    if (ret) {
        LLOGD("tls_fls_write_without_erase ret %d", ret);
        return ret;
    }
    if (fota_head_check == 0 && fota_write_offset >= sizeof(IMAGE_HEADER_PARAM_ST)) {
        // 检查头部magic_no
        IMAGE_HEADER_PARAM_ST* imghead = (IMAGE_HEADER_PARAM_ST*)upgrade_img_addr;
        if (imghead->magic_no != MAGIC_NO) {
            LLOGD("fota包的magic_no错误, 0x%08X", imghead->magic_no);
            return -2;
        }
        // 检查头部校验和
        // 计算一下header的checksum
        uint32_t cm = img_checksum((const char*)imghead, sizeof(IMAGE_HEADER_PARAM_ST) - 4);
        if (cm != imghead->hd_checksum) {
            LLOGD("foto包的头部校验和不正确 expect %08X but %08X", imghead->hd_checksum, cm);
            return -3;
        }
        fota_head_check = 1;
    }
    return 0;
}

int luat_fota_done(void) {
    if (fota_write_offset == 0) {
        LLOGE("未写入任何数据, 无法完成OTA");
        return -1;
    }
    if (fota_write_offset < sizeof(IMAGE_HEADER_PARAM_ST)) {
        LLOGI("写入数据小于最小长度, 还不能判断");
        return -2;
    }
    // 写入长度已经超过最小长度, 判断是否是合法的镜像
    if (fota_write_offset < sizeof(IMAGE_HEADER_PARAM_ST)) {
        LLOGI("fota头部尚未接收完成");
        return -3;
    }
    IMAGE_HEADER_PARAM_ST* imghead = (IMAGE_HEADER_PARAM_ST*)upgrade_img_addr;
    if (imghead->img_len > fota_write_offset + sizeof(IMAGE_HEADER_PARAM_ST)) {
        LLOGI("fota数据还不够, 继续等数据");
        return -4;
    }

    // 写入长度足够, 判断是否是合法的镜像, 开始计算check sum
    uint32_t cm = img_checksum((const char*)upgrade_img_addr + sizeof(IMAGE_HEADER_PARAM_ST), imghead->img_len);
    if (cm != imghead->org_checksum) {
        LLOGD("foto包的头部校验和不正确 expect %08X but %08X", imghead->org_checksum, cm);
        return -3;
    }
    LLOGD("FOTA数据校验通过, 写入长度 %d", fota_write_offset);
    fota_state = FOTA_DONE;
    return 0;
}

int luat_fota_end(uint8_t is_ok) {
    if (fota_state == FOTA_DONE && is_ok) {
        int ret = 0;
        IMAGE_HEADER_PARAM_ST* imghead = (IMAGE_HEADER_PARAM_ST*)upgrade_img_addr;
        if (imghead->img_attr.b.img_type == 1) {
#if defined(AIR6208)
            // AIR6208 使用 4MB Flash
            // Bootloader 可能使用默认的 2MB Flash 地址 (0x81FF000) 或正确的 4MB 地址 (0x83FF000)
            // 为确保兼容，在两个地址都写入 OTA 标志
            
            // 写入到 4MB Flash 的正确地址
            uint32_t ota_flag_addr_4mb = 0x083FF000;
            LLOGI("整包升级,写入升级标志(4MB) addr %08X checksum %08X", ota_flag_addr_4mb, imghead->org_checksum);
            ret = tls_fls_write(ota_flag_addr_4mb, (u8 *)&imghead->org_checksum, sizeof(imghead->org_checksum));
            if (ret) {
                LLOGE("写入升级标志位(4MB)失败, ret %d", ret);
            }
            
            // 同时写入到默认的 2MB Flash 地址，以防 Bootloader 使用硬编码地址
            uint32_t ota_flag_addr_2mb = 0x081FF000;
            LLOGI("整包升级,写入升级标志(2MB兼容) addr %08X checksum %08X", ota_flag_addr_2mb, imghead->org_checksum);
            ret = tls_fls_write(ota_flag_addr_2mb, (u8 *)&imghead->org_checksum, sizeof(imghead->org_checksum));
            if (ret) {
                LLOGE("写入升级标志位(2MB兼容)失败, ret %d", ret);
            }
#else
            // 2MB Flash (AIR101/AIR690) 使用动态地址
            LLOGI("整包升级,写入升级标志 addr %08X checksum %08X", TLS_FLASH_OTA_FLAG_ADDR, imghead->org_checksum);
            ret = tls_fls_write(TLS_FLASH_OTA_FLAG_ADDR, (u8 *)&imghead->org_checksum, sizeof(imghead->org_checksum));
            if (ret) {
                LLOGE("写入升级标志位失败, ret %d", ret);
            }
#endif
        }
        else {
            LLOGI("仅脚本升级, 重启后自动更新");
        }
        return ret;
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
    LLOGI("=== %s Image Header ===", tag);
    LLOGI("  magic_no:       %08X (期望: %08X)", imghead->magic_no, MAGIC_NO);
    LLOGI("  img_addr:       %08X (运行地址)", imghead->img_addr);
    LLOGI("  img_len:        %08X (%d bytes, %d KB)", imghead->img_len, imghead->img_len, imghead->img_len/1024);
    LLOGI("  img_header_addr:%08X", imghead->img_header_addr);
    LLOGI("  upgrade_img_addr:%08X (OTA区地址)", imghead->upgrade_img_addr);
    LLOGI("  org_checksum:   %08X (数据CRC32)", imghead->org_checksum);
    LLOGI("  upd_no:         %08X (升级版本号)", imghead->upd_no);
    LLOGI("  ver:            %.16s", imghead->ver);
    LLOGI("  hd_checksum:    %08X (头部CRC32)", imghead->hd_checksum);
    LLOGI("  next:           %08X (下一个镜像地址)", (uint32_t)imghead->next);
    
    // Image Attribute 详细信息
    LLOGI("  --- Image Attribute ---");
    LLOGI("    img_type:        %d (0=Bootloader, 1=User, 0xE=FT测试)", imghead->img_attr.b.img_type);
    LLOGI("    code_encrypt:    %d (0=明文, 1=加密)", imghead->img_attr.b.code_encrypt);
    LLOGI("    prikey_sel:      %d (私钥选择 0-7)", imghead->img_attr.b.prikey_sel);
    LLOGI("    signature:       %d (0=无签名, 1=含128字节签名)", imghead->img_attr.b.signature);
    LLOGI("    zip_type:        %d (0=不压缩, 1=GZIP)", imghead->img_attr.b.zip_type);
    LLOGI("    psram_io:        %d", imghead->img_attr.b.psram_io);
    LLOGI("    erase_block_en:  %d (0=4KB擦除, 1=支持64KB块擦除)", imghead->img_attr.b.erase_block_en);
    LLOGI("    erase_always:    %d (0=检查后擦除, 1=总是擦除)", imghead->img_attr.b.erase_always);
    LLOGI("    raw value:       %08X", imghead->img_attr.w);
    
    // 先判断一下magicno
    if (imghead->magic_no != MAGIC_NO) {
        return -2;
    }
    // 计算一下header的checksum
    uint32_t cm = img_checksum((const char*)imghead, sizeof(IMAGE_HEADER_PARAM_ST) - 4);
    if (cm != imghead->hd_checksum) {
        LLOGE("%s 头部CRC校验失败! 期望: %08X, 计算: %08X", tag, imghead->hd_checksum, cm);
        return -3;
    }
    LLOGI("  头部CRC校验通过");
    return 0;
}

void luat_fota_boot_check(void) {
    // 读取secboot区域的信息
    IMAGE_HEADER_PARAM_ST* secimg = (IMAGE_HEADER_PARAM_ST*)0x8002000;
    IMAGE_HEADER_PARAM_ST* runimg = (secimg->next);

    // 打印当前运行镜像的详细信息
    LLOGI("========================================");
    LLOGI("当前运行镜像信息:");
    check_image_head(runimg, "RUN");
    LLOGI("========================================");

    // 计算出OTA区域大小, 运行区镜像的大小

#if defined(AIR6208)
    // AIR6208: FOTA 区域在 0x280000, 大小 1536K
    // 完整的 Flash 地址 = 0x08000000 + 0x280000 = 0x08280000
    upgrade_img_addr = 0x08280000;
    ota_zone_size = 1536 * 1024;  // 1536K
    // 强制修正 TLS_FLASH_OTA_FLAG_ADDR 为 4MB Flash 的地址
    // 因为 tls_fls_sys_param_postion_init 可能检测到错误的 Flash 大小
    TLS_FLASH_OTA_FLAG_ADDR = 0x083FF000;  // 4MB Flash: 0x08000000 + 0x400000 - 0x1000
#else
    // 2MB Flash (AIR101/AIR690)
    upgrade_img_addr = secimg->upgrade_img_addr;
    ota_zone_size = ((uint32_t)runimg) - upgrade_img_addr;
    ota_zone_size = (ota_zone_size + 0x3FF) & (~0x3FF);
#endif

    LLOGI("OTA区域地址: 0x%08X, 大小: %d KB", upgrade_img_addr, ota_zone_size/1024);
    LLOGI("运行区地址: 0x%08X", (uint32_t)runimg);
    LLOGI("secimg->upgrade_img_addr: 0x%08X", secimg->upgrade_img_addr);

    // 检查OTA区域的数据
    check_ota_zone();
}

static void check_ota_zone(void) {
    // 首先, 检查是不是OTA数据
    IMAGE_HEADER_PARAM_ST* imghead = (IMAGE_HEADER_PARAM_ST*)upgrade_img_addr;
    
    LLOGI("========================================");
    LLOGI("OTA区域镜像信息 (地址: 0x%08X):", upgrade_img_addr);
    
    if (imghead->magic_no == 0xFFFFFFFF) {
        LLOGI("OTA区域为空 (全0xFF)");
        return; // 直接返回就行, 完全没有OTA数据
    }
    if (imghead->magic_no != MAGIC_NO) {
        LLOGE("OTA区域magic_no错误: %08X (期望: %08X)", imghead->magic_no, MAGIC_NO);
        goto clean;
    }
    
    // 打印OTA镜像的详细信息
    check_image_head(imghead, "OTA");
    
    // 检查头部校验和
    uint32_t cm = img_checksum((const char*)imghead, sizeof(IMAGE_HEADER_PARAM_ST) - 4);
    if (cm != imghead->hd_checksum) {
        LLOGE("OTA头部CRC校验失败! 期望: %08X, 计算: %08X", imghead->hd_checksum, cm);
        goto clean;
    }
    
    LLOGI("OTA数据头部校验通过");
    
    // 关键地址比较
    LLOGI("========================================");
    LLOGI("关键地址比较:");
    IMAGE_HEADER_PARAM_ST* secimg = (IMAGE_HEADER_PARAM_ST*)0x8002000;
    IMAGE_HEADER_PARAM_ST* runimg = (secimg->next);
    
    // 提取地址的低24位用于比较（去除Flash base）
    uint32_t runimg_low = (uint32_t)runimg & 0x00FFFFFF;
    uint32_t upgrade_low = upgrade_img_addr & 0x00FFFFFF;
    uint32_t ota_img_addr = imghead->img_addr & 0x00FFFFFF;
    uint32_t ota_upgrade_addr = imghead->upgrade_img_addr & 0x00FFFFFF;
    
    LLOGI("  OTA包中的 img_addr:        0x%08X (低24位: 0x%06X)", imghead->img_addr, ota_img_addr);
    LLOGI("  当前运行区地址 (runimg):    0x%08X (低24位: 0x%06X)", (uint32_t)runimg, runimg_low);
    LLOGI("  OTA包中的 upgrade_img_addr: 0x%08X (低24位: 0x%06X)", imghead->upgrade_img_addr, ota_upgrade_addr);
    LLOGI("  实际OTA区域地址:            0x%08X (低24位: 0x%06X)", upgrade_img_addr, upgrade_low);
    LLOGI("  secimg->upgrade_img_addr:   0x%08X", secimg->upgrade_img_addr);
    
    // 判断地址是否匹配（比较低24位）
    if (ota_img_addr != runimg_low) {
        LLOGI("警告: OTA包的img_addr低24位(0x%06X) 与运行区地址低24位(0x%06X)不匹配!", 
              ota_img_addr, runimg_low);
    } else {
        LLOGI("  ✓ img_addr 匹配");
    }
    if (ota_upgrade_addr != upgrade_low) {
        LLOGI("警告: OTA包的upgrade_img_addr低24位(0x%06X) 与实际OTA区低24位(0x%06X)不匹配!", 
              ota_upgrade_addr, upgrade_low);
    } else {
        LLOGI("  ✓ upgrade_img_addr 匹配");
    }
    LLOGI("========================================");
    
clean:
    // 注意: 这里不要清除OTA区域，保留数据供 Bootloader 使用
    ;
}

