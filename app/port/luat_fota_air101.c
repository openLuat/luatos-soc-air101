
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
#include "miniz.h"

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

// 大缓冲区 - 减少Flash写入次数，每次写入约100ms
#define FOTA_BUFFER_SIZE (32 * 1024)  // 32KB缓冲区
static uint8_t* fota_buffer;  // 动态分配
static uint32_t fota_buffer_len;

// 用于脚本解压的变量和函数
static IMAGE_HEADER_PARAM_ST tmphead;
static uint32_t head_fill_count = 0;
static uint32_t image_skip_remain = 0;
extern uint32_t luadb_addr;

// 解压回调函数 - 处理解压后的数据，识别脚本区并写入
static int ota_gzcb(const void *pBuf, int len, void *pUser) {
    const char* tmp = pBuf;
next:
    if (len == 0) {
        return 1;
    }
    LLOGD("ota_gzcb状态: head_fill=%d skip_remain=%d img_len=%d type=%d", 
          head_fill_count, image_skip_remain, tmphead.img_len, tmphead.img_attr.b.img_type);
    
    if (image_skip_remain > 0) {
        // 正在处理某个IMG段的数据体
        if (tmphead.img_attr.b.img_type == 2) {
            LLOGD("写入脚本区: 地址=0x%08X, 长度=%d", 
                  luadb_addr + (tmphead.img_len - image_skip_remain), 
                  (len >= image_skip_remain) ? image_skip_remain : len);
        }
        
        if (len >= image_skip_remain) {
            // 当前数据足够完成这个IMG段
            if (tmphead.img_attr.b.img_type == 2) {
                tls_fls_write(luadb_addr + (tmphead.img_len - image_skip_remain), (u8*)tmp, image_skip_remain);
            }
            tmp += image_skip_remain;
            len -= image_skip_remain;
            image_skip_remain = 0;
            head_fill_count = 0;  // 准备读取下一个IMG头部
            goto next;
        } else {
            // 数据不够，先写入当前部分
            if (tmphead.img_attr.b.img_type == 2) {
                tls_fls_write(luadb_addr + (tmphead.img_len - image_skip_remain), (u8*)tmp, len);
            }
            image_skip_remain -= len;
            return 1;
        }
    }
    
    // 正在填充IMG头部
    if (head_fill_count < sizeof(IMAGE_HEADER_PARAM_ST)) {
        if (head_fill_count + len >= sizeof(IMAGE_HEADER_PARAM_ST)) {
            // 头部数据足够，填充完整头部
            memcpy((char*)&tmphead + head_fill_count, tmp, sizeof(IMAGE_HEADER_PARAM_ST) - head_fill_count);
            tmp += sizeof(IMAGE_HEADER_PARAM_ST) - head_fill_count;
            len -= sizeof(IMAGE_HEADER_PARAM_ST) - head_fill_count;
            head_fill_count = sizeof(IMAGE_HEADER_PARAM_ST);
            image_skip_remain = tmphead.img_len;  // 设置需要跳过的数据长度
            LLOGD("发现IMG段: 长度=%d 类型=%d", tmphead.img_len, tmphead.img_attr.b.img_type);
            goto next;
        } else {
            // 头部数据不够，继续等待
            memcpy((char*)&tmphead + head_fill_count, tmp, len);
            head_fill_count += len;
            return 1;
        }
    }
    return 1;
}

// miniz解压函数 - 用于解压GZIP格式的OTA包
int my_tinfl_decompress_mem_to_callback(const void *pIn_buf, size_t *pIn_buf_size, 
    tinfl_put_buf_func_ptr pPut_buf_func, void *pPut_buf_user, int flags)
{
    int result = 0;
    tinfl_decompressor* decomp = luat_heap_malloc(sizeof(tinfl_decompressor));
    if (!decomp) {
        LLOGE("分配tinfl_decompressor失败");
        return TINFL_STATUS_FAILED;
    }
    mz_uint8 *pDict = (mz_uint8 *)luat_heap_malloc(TINFL_LZ_DICT_SIZE);
    size_t in_buf_ofs = 0, dict_ofs = 0;
    if (!pDict) {
        LLOGE("分配pDict失败");
        luat_heap_free(decomp);
        return TINFL_STATUS_FAILED;
    }
    memset(pDict, 0, TINFL_LZ_DICT_SIZE);
    tinfl_init(decomp);
    for (;;) {
        size_t in_buf_size = *pIn_buf_size - in_buf_ofs;
        size_t dst_buf_size = TINFL_LZ_DICT_SIZE - dict_ofs;
        tinfl_status status = tinfl_decompress(decomp, 
            (const mz_uint8 *)pIn_buf + in_buf_ofs, &in_buf_size, 
            pDict, pDict + dict_ofs, &dst_buf_size,
            (flags & ~(TINFL_FLAG_HAS_MORE_INPUT | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF)));
        in_buf_ofs += in_buf_size;
        if ((dst_buf_size) && (!(*pPut_buf_func)(pDict + dict_ofs, (int)dst_buf_size, pPut_buf_user)))
            break;
        if (status != TINFL_STATUS_HAS_MORE_OUTPUT) {
            result = (status == TINFL_STATUS_DONE);
            break;
        }
        dict_ofs = (dict_ofs + dst_buf_size) & (TINFL_LZ_DICT_SIZE - 1);
    }
    luat_heap_free(pDict);
    luat_heap_free(decomp);
    *pIn_buf_size = in_buf_ofs;
    return result;
}

static int check_image_head(IMAGE_HEADER_PARAM_ST* imghead, const char* tag);
static uint32_t img_checksum(const char* ptr, size_t len);
void luat_fota_boot_check(void);
static void check_ota_zone(void);

int luat_fota_init(uint32_t start_address, uint32_t len, luat_spi_device_t* spi_device, const char *path, uint32_t pathlen) {
    fota_state = FOTA_ONGO;
    fota_write_offset = 0;
    fota_head_check = 0;
    fota_buffer_len = 0;
    
    // 动态分配缓冲区（从PSRAM）
    if (fota_buffer == NULL) {
        fota_buffer = luat_heap_malloc(FOTA_BUFFER_SIZE);
        if (fota_buffer == NULL) {
            LLOGE("无法分配OTA缓冲区");
            return -1;
        }
    }
    for (size_t i = 0; i < ota_zone_size / 4096; i++)
    {
        // LLOGD("清除ota区域: %08X", upgrade_img_addr + i * 4096);
        tls_fls_erase((upgrade_img_addr + i * 4096) / INSIDE_FLS_SECTOR_SIZE);
    }
    LLOGI("OTA区域初始化完成, 大小 %d kbyte", ota_zone_size / 1024);
    return 0;
}

int luat_fota_write(uint8_t *data, uint32_t len) {
    if (fota_state != FOTA_ONGO) {
        return -1;
    }
    if (fota_buffer == NULL) {
        return -1;
    }
    
    uint32_t offset = 0;
    while (offset < len) {
        uint32_t can_copy = FOTA_BUFFER_SIZE - fota_buffer_len;
        uint32_t remaining = len - offset;
        uint32_t copy_len = (can_copy < remaining) ? can_copy : remaining;
        
        memcpy(fota_buffer + fota_buffer_len, data + offset, copy_len);
        fota_buffer_len += copy_len;
        offset += copy_len;
        
        // 缓冲区满时写入Flash
        if (fota_buffer_len >= FOTA_BUFFER_SIZE) {
            if (fota_write_offset + fota_buffer_len > ota_zone_size) {
                LLOGE("OTA区域写满");
                return -1;
            }
            
            tls_fls_write(upgrade_img_addr + fota_write_offset, fota_buffer, fota_buffer_len);
            fota_write_offset += fota_buffer_len;
            fota_buffer_len = 0;
        }
    }
    
    // 头部检查
    if (fota_head_check == 0 && fota_write_offset + fota_buffer_len >= sizeof(IMAGE_HEADER_PARAM_ST)) {
        // 先刷新缓冲区
        if (fota_buffer_len > 0) {
            tls_fls_write(upgrade_img_addr + fota_write_offset, fota_buffer, fota_buffer_len);
            fota_write_offset += fota_buffer_len;
            fota_buffer_len = 0;
        }
        
        IMAGE_HEADER_PARAM_ST* imghead = (IMAGE_HEADER_PARAM_ST*)upgrade_img_addr;
        if (imghead->magic_no != MAGIC_NO) {
            LLOGE("magic_no错误: %08X", imghead->magic_no);
            return -2;
        }
        uint32_t cm = img_checksum((const char*)imghead, sizeof(IMAGE_HEADER_PARAM_ST) - 4);
        if (cm != imghead->hd_checksum) {
            LLOGE("头部CRC错误");
            return -3;
        }
        fota_head_check = 1;
        LLOGI("固件头部校验通过");
    }
    
    return 0;
}

int luat_fota_done(void) {
    // 刷新剩余数据
    if (fota_buffer_len > 0) {
        if (fota_write_offset + fota_buffer_len > ota_zone_size) {
            LLOGE("OTA区域写满");
            return -1;
        }
        tls_fls_write(upgrade_img_addr + fota_write_offset, fota_buffer, fota_buffer_len);
        LLOGI("写入数据: %08X, 大小: %d", upgrade_img_addr + fota_write_offset, fota_buffer_len);
        fota_write_offset += fota_buffer_len;
        fota_buffer_len = 0;
    }
    
    if (fota_write_offset == 0) {
        LLOGE("未写入数据");
        return -1;
    }
    
    if (fota_write_offset < sizeof(IMAGE_HEADER_PARAM_ST)) {
        return -2;
    }
    
    IMAGE_HEADER_PARAM_ST* imghead = (IMAGE_HEADER_PARAM_ST*)upgrade_img_addr;
    if (imghead->img_len + sizeof(IMAGE_HEADER_PARAM_ST) > fota_write_offset) {
        return -4;
    }

    uint32_t cm = img_checksum((const char*)upgrade_img_addr + sizeof(IMAGE_HEADER_PARAM_ST), imghead->img_len);
    if (cm != imghead->org_checksum) {
        LLOGE("CRC校验失败");
        return -3;
    }
    
    LLOGI("FOTA数据校验通过, 写入长度 %d", fota_write_offset);
    fota_state = FOTA_DONE;
    return 0;
}

int luat_fota_end(uint8_t is_ok) {
    if (fota_state == FOTA_DONE && is_ok) {
        int ret = 0;
        IMAGE_HEADER_PARAM_ST* imghead = (IMAGE_HEADER_PARAM_ST*)upgrade_img_addr;
        if (imghead->img_attr.b.img_type == 1) {
            // 2MB Flash (AIR101/AIR690) 使用动态地址
            LLOGI("整包升级,写入升级标志 addr %08X checksum %08X", TLS_FLASH_OTA_FLAG_ADDR, imghead->org_checksum);
            ret = tls_fls_write(TLS_FLASH_OTA_FLAG_ADDR, (u8 *)&imghead->org_checksum, sizeof(imghead->org_checksum));
            if (ret) {
                LLOGE("写入升级标志位失败, ret %d", ret);
            }
            
            // 验证写入结果
            uint32_t verify_checksum = 0;
            tls_fls_read(TLS_FLASH_OTA_FLAG_ADDR, (u8 *)&verify_checksum, sizeof(verify_checksum));
            LLOGI("验证写入: 期望 %08X, 实际读取 %08X", imghead->org_checksum, verify_checksum);
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
    // LLOGI("=== %s Image Header ===", tag);
    // LLOGI("  magic_no:       %08X (期望: %08X)", imghead->magic_no, MAGIC_NO);
    // LLOGI("  img_addr:       %08X (运行地址)", imghead->img_addr);
    // LLOGI("  img_len:        %08X (%d bytes, %d KB)", imghead->img_len, imghead->img_len, imghead->img_len/1024);
    // LLOGI("  img_header_addr:%08X", imghead->img_header_addr);
    // LLOGI("  upgrade_img_addr:%08X (OTA区地址)", imghead->upgrade_img_addr);
    // LLOGI("  org_checksum:   %08X (数据CRC32)", imghead->org_checksum);
    // LLOGI("  upd_no:         %08X (升级版本号)", imghead->upd_no);
    // LLOGI("  ver:            %.16s", imghead->ver);
    // LLOGI("  hd_checksum:    %08X (头部CRC32)", imghead->hd_checksum);
    // LLOGI("  next:           %08X (下一个镜像地址)", (uint32_t)imghead->next);
    
    // Image Attribute 详细信息
    // LLOGI("  --- Image Attribute ---");
    // LLOGI("    img_type:        %d (0=Bootloader, 1=User, 0xE=FT测试)", imghead->img_attr.b.img_type);
    // LLOGI("    code_encrypt:    %d (0=明文, 1=加密)", imghead->img_attr.b.code_encrypt);
    // LLOGI("    prikey_sel:      %d (私钥选择 0-7)", imghead->img_attr.b.prikey_sel);
    // LLOGI("    signature:       %d (0=无签名, 1=含128字节签名)", imghead->img_attr.b.signature);
    // LLOGI("    zip_type:        %d (0=不压缩, 1=GZIP)", imghead->img_attr.b.zip_type);
    // LLOGI("    psram_io:        %d", imghead->img_attr.b.psram_io);
    // LLOGI("    erase_block_en:  %d (0=4KB擦除, 1=支持64KB块擦除)", imghead->img_attr.b.erase_block_en);
    // LLOGI("    erase_always:    %d (0=检查后擦除, 1=总是擦除)", imghead->img_attr.b.erase_always);
    // LLOGI("    raw value:       %08X", imghead->img_attr.w);
    
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
    check_image_head(runimg, "RUN");

    // 计算出OTA区域大小, 运行区镜像的大小

#if defined(AIR6208)
    // AIR6208: 使用 8M Flash
    // 分区表配置 (新顺序: secboot -> fota -> app -> kv -> script -> fs):
    // secboot: 0x08000000 - 0x08010000 (64K)
    // fota:    0x08010000 - 0x08210000 (2048K)
    // app:     0x08210000 - 0x08420000 (2112K)
    // kv:      0x08420000 - 0x08430000 (64K)
    // script:  0x08430000 - 0x084B0000 (512K)
    // fs:      0x084B0000 - 0x08800000 (3392K)
    upgrade_img_addr = 0x08010000;  // fota分区起始
    ota_zone_size = 2048 * 1024;    // 2048K
#else
    // 2MB Flash (AIR101/AIR690)
    upgrade_img_addr = secimg->upgrade_img_addr;
    ota_zone_size = ((uint32_t)runimg) - upgrade_img_addr;
    ota_zone_size = (ota_zone_size + 0x3FF) & (~0x3FF);
#endif

    // LLOGI("OTA区域地址: 0x%08X, 大小: %d KB", upgrade_img_addr, ota_zone_size/1024);
    // LLOGI("运行区地址: 0x%08X", (uint32_t)runimg);
    // LLOGI("secimg->upgrade_img_addr: 0x%08X", secimg->upgrade_img_addr);

#if defined(AIR6208)
    // 调试: 检查OTA标志地址的内容
    LLOGI("OTA标志状态检查:");
    uint32_t flag_8mb = 0;
    tls_fls_read(TLS_FLASH_OTA_FLAG_ADDR, (u8 *)&flag_8mb, sizeof(flag_8mb));
    LLOGI(" %08X: %08X %s", TLS_FLASH_OTA_FLAG_ADDR, flag_8mb, 
          (flag_8mb != 0xFFFFFFFF) ? "<- 有OTA标志!" : "(空)");
#endif

    // 检查OTA区域的数据
    check_ota_zone();
}

static void check_ota_zone(void) {
    // 首先, 检查是不是OTA数据
    IMAGE_HEADER_PARAM_ST* imghead = (IMAGE_HEADER_PARAM_ST*)upgrade_img_addr;
    
    // LLOGI("========================================");
    // LLOGI("OTA区域镜像信息 (地址: 0x%08X):", upgrade_img_addr);
    
    if (imghead->magic_no == 0xFFFFFFFF) {
        LLOGW("OTA区域为空 (全0xFF)");
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
    IMAGE_HEADER_PARAM_ST* secimg = (IMAGE_HEADER_PARAM_ST*)0x8002000;
    IMAGE_HEADER_PARAM_ST* runimg_header = (secimg->next);
    
    // 获取当前运行镜像的 img_addr（代码运行地址）
    uint32_t run_img_addr = runimg_header->img_addr;
    
    // 提取地址的低24位用于比较（去除Flash base）
    uint32_t run_img_addr_low = run_img_addr & 0x00FFFFFF;
    uint32_t upgrade_low = upgrade_img_addr & 0x00FFFFFF;
    uint32_t ota_img_addr = imghead->img_addr & 0x00FFFFFF;
    uint32_t ota_upgrade_addr = imghead->upgrade_img_addr & 0x00FFFFFF;
    
    // LLOGI("  当前运行镜像:");
    // LLOGI("    Image Header 地址:          0x%08X", (uint32_t)runimg_header);
    // LLOGI("    img_addr (代码运行地址):    0x%08X (低24位: 0x%06X)", run_img_addr, run_img_addr_low);
    // LLOGI("  OTA包:");
    // LLOGI("    img_addr (代码运行地址):    0x%08X (低24位: 0x%06X)", imghead->img_addr, ota_img_addr);
    // LLOGI("    upgrade_img_addr (OTA区):   0x%08X (低24位: 0x%06X)", imghead->upgrade_img_addr, ota_upgrade_addr);
    // LLOGI("  系统配置:");
    // LLOGI("    实际OTA区域地址:            0x%08X (低24位: 0x%06X)", upgrade_img_addr, upgrade_low);
    // LLOGI("    secimg->upgrade_img_addr:   0x%08X", secimg->upgrade_img_addr);
    
    // 判断地址是否匹配（比较低24位）
    if (ota_img_addr != run_img_addr_low) {
        LLOGW("警告: OTA包的img_addr(0x%08X) 与当前运行镜像的img_addr(0x%08X)不匹配!", 
              imghead->img_addr, run_img_addr);
    } else {
        LLOGW("  ✓ img_addr 匹配 (代码运行地址)");
    }
    if (ota_upgrade_addr != upgrade_low) {
        LLOGW("警告: OTA包的upgrade_img_addr(0x%08X) 与实际OTA区(0x%08X)不匹配!", 
              imghead->upgrade_img_addr, upgrade_img_addr);
    } else {
        LLOGW("  ✓ upgrade_img_addr 匹配");
    }
    
    // 检查OTA包类型，如果是全量升级包(类型1)，需要解压并升级脚本区
    if (imghead->img_attr.b.img_type == 1) {
        LLOGI("检测到全量升级包，开始解压并升级脚本区...");
        
        // 重置解压状态变量
        head_fill_count = 0;
        image_skip_remain = 0;
        memset(&tmphead, 0, sizeof(tmphead));
        
        // 解压OTA数据并通过回调写入脚本区
        size_t inSize = imghead->img_len;
        uint8_t *ptr = (uint8_t *)upgrade_img_addr + sizeof(IMAGE_HEADER_PARAM_ST) + 10; // 跳过GZ的前10个字节
        
        int ret = my_tinfl_decompress_mem_to_callback(ptr, &inSize, ota_gzcb, NULL, 0);
        LLOGD("OTA解压函数返回值: %d", ret);
        
        if (ret) {
            LLOGI("脚本区升级完成");
        } else {
            LLOGE("脚本区升级失败");
        }
    } else if (imghead->img_attr.b.img_type == 2) {
        LLOGI("检测到仅脚本升级包，将在重启后处理");
    }
    
    // 清理OTA区域
    LLOGI("清理OTA区域...");
clean:
    tls_fls_erase((upgrade_img_addr) / INSIDE_FLS_SECTOR_SIZE);
    LLOGI("OTA区域清理完成");
}

