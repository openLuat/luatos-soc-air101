
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
    LLOGD("FOTA数据校验通过, ");
    fota_state = FOTA_DONE;
    return 0;
}

int luat_fota_end(uint8_t is_ok) {
    if (fota_state == FOTA_DONE && is_ok) {
        IMAGE_HEADER_PARAM_ST* imghead = (IMAGE_HEADER_PARAM_ST*)upgrade_img_addr;
        LLOGI("准备写入升级标志 addr %08X checksum %08X", TLS_FLASH_OTA_FLAG_ADDR, imghead->org_checksum);
        int ret = tls_fls_write(TLS_FLASH_OTA_FLAG_ADDR, (u8 *)&imghead->org_checksum, sizeof(imghead->org_checksum));
        if (ret) {
            LLOGE("写入升级标志位失败, ret %d", ret);
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
    LLOGD("%s image img_addr: %08X", tag, imghead->img_addr);
    LLOGD("%s image img_len: %08X", tag, imghead->img_len);
    LLOGD("%s image img_header_addr: %08X", tag, imghead->img_header_addr);
    LLOGD("%s image upgrade_img_addr: %08X", tag, imghead->upgrade_img_addr);
    LLOGD("%s image org_checksum: %08X", tag, imghead->org_checksum);
    // LLOGD("%s image upd_no: %08X", tag, imghead->upd_no);
    // LLOGD("%s image ver: %.16s", tag, imghead->ver);
    LLOGD("%s image hd_checksum: %08X", tag, imghead->hd_checksum);
    // LLOGD("%s image next: %08X", tag, imghead->next);
    
    // image attr
    // LLOGD("%s image attr img_type: %d", tag, imghead->img_attr.b.img_type);
    // LLOGD("%s image attr zip_type: %d", tag, imghead->img_attr.b.zip_type);
    // LLOGD("%s image attr psram_io: %d", tag, imghead->img_attr.b.psram_io);
    // LLOGD("%s image attr erase_block_en: %d", tag, imghead->img_attr.b.erase_block_en);
    // LLOGD("%s image attr erase_always: %d", tag, imghead->img_attr.b.erase_always);
    
    // 先判断一下magicno
    if (imghead->magic_no != MAGIC_NO) {
        return -2;
    }
    // 计算一下header的checksum
    uint32_t cm = img_checksum((const char*)imghead, sizeof(IMAGE_HEADER_PARAM_ST) - 4);
    if (cm != imghead->hd_checksum) {
        LLOGD("%s head expect %08X but %08X", tag, imghead->hd_checksum, cm);
        return -3;
    }
    const char* dataptr = (const char*)imghead->img_addr;
    cm = img_checksum(dataptr, imghead->img_len);
    if (cm != imghead->org_checksum) {
        LLOGD("%s data expect %08X but %08X addr %08X", tag, imghead->org_checksum, cm, imghead->img_addr);
        return -4;
    }
    return 0;
}

void luat_fota_boot_check(void) {
    // LLOGD("启动fota开机检查");
    // LLOGD("sizeof(IMAGE_HEADER_PARAM_ST) %d %d %d", sizeof(IMAGE_HEADER_PARAM_ST), sizeof(Img_Attr_Type), sizeof(unsigned int));
    // 读取secboot区域的信息, 大小1kb
    IMAGE_HEADER_PARAM_ST* secimg = (IMAGE_HEADER_PARAM_ST*)0x8002000;
    // IMAGE_HEADER_PARAM_ST* upimg = (IMAGE_HEADER_PARAM_ST*)secimg->upgrade_img_addr;
    IMAGE_HEADER_PARAM_ST* runimg = (secimg->next);

    // check_image_head(secimg, "secboot");
    // check_image_head(upimg, "update");
    // check_image_head(runimg, "user");

    // 计算出OTA区域大小, 运行区镜像的大小

    // 把相关参数存起来
    upgrade_img_addr = secimg->upgrade_img_addr;
    
    ota_zone_size = ((uint32_t)runimg) - upgrade_img_addr;
    ota_zone_size = (ota_zone_size + 0x3FF) & (~0x3FF);

    LLOGD("ota zone : 0x%08X %dkb", upgrade_img_addr, ota_zone_size/1024);
    // 当前运行区镜像的大小
    LLOGD("run image size: 0x%08X %dkb", runimg->img_len, runimg->img_len/1024);

    // 检查OTA区域的数据, 如果存在更新包, 需要解析出脚本区, 然后清除数据
    check_ota_zone();
}

#include "miniz.h"
int my_tinfl_decompress_mem_to_callback(const void *pIn_buf, size_t *pIn_buf_size, tinfl_put_buf_func_ptr pPut_buf_func, void *pPut_buf_user, int flags)
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
    memset(pDict,0,TINFL_LZ_DICT_SIZE);
    tinfl_init(decomp);
    for (;;)
    {
        size_t in_buf_size = *pIn_buf_size - in_buf_ofs, dst_buf_size = TINFL_LZ_DICT_SIZE - dict_ofs;
        tinfl_status status = tinfl_decompress(decomp, (const mz_uint8 *)pIn_buf + in_buf_ofs, &in_buf_size, pDict, pDict + dict_ofs, &dst_buf_size,
                                               (flags & ~(TINFL_FLAG_HAS_MORE_INPUT | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF)));
        in_buf_ofs += in_buf_size;
        if ((dst_buf_size) && (!(*pPut_buf_func)(pDict + dict_ofs, (int)dst_buf_size, pPut_buf_user)))
            break;
        if (status != TINFL_STATUS_HAS_MORE_OUTPUT)
        {
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

static IMAGE_HEADER_PARAM_ST tmphead;
static uint32_t head_fill_count = 0;
static uint32_t image_skip_remain = 0;

static int ota_gzcb(const void *pBuf, int len, void *pUser) {
    const char* tmp = pBuf;
    LLOGD("得到解压数据 %p %d", tmp, len);
next:
    if (len == 0) {
        return 1;
    }
    if (image_skip_remain > 0) {
        if (len >= image_skip_remain) {
            tmp += image_skip_remain;
            len -= image_skip_remain;
            image_skip_remain = 0;
            head_fill_count = 0;
        }
        else {
            image_skip_remain -= len;
            return 1;
        }
    }
    if (head_fill_count < sizeof(IMAGE_HEADER_PARAM_ST)) {
        // 填充头部信息
        if (head_fill_count + len >= sizeof(IMAGE_HEADER_PARAM_ST)) {
            memcpy((char*)&tmphead + head_fill_count, tmp, sizeof(IMAGE_HEADER_PARAM_ST) - head_fill_count);
            tmp += sizeof(IMAGE_HEADER_PARAM_ST) - head_fill_count;
            len -= sizeof(IMAGE_HEADER_PARAM_ST) - head_fill_count;
            head_fill_count = sizeof(IMAGE_HEADER_PARAM_ST);
            image_skip_remain = tmphead.img_len;
            LLOGD("找到一个IMG数据段 长度 %d", tmphead.img_len);
            goto next;
        }
        else {
            // 继续等数据
            memcpy((char*)&tmphead + head_fill_count, tmp, len);
            head_fill_count += len;
            return 1;
        }
    }
    return 1;
}

static void check_ota_zone(void) {
    // 首先, 检查是不是OTA数据
    IMAGE_HEADER_PARAM_ST* imghead = (IMAGE_HEADER_PARAM_ST*)upgrade_img_addr;
    if (imghead->magic_no != MAGIC_NO) {
        LLOGD("OTA区域没有数据, 因为magic no不对 %08X", imghead->magic_no);
        return;
    }
    // 检查头部校验和
    // 计算一下header的checksum
    uint32_t cm = img_checksum((const char*)imghead, sizeof(IMAGE_HEADER_PARAM_ST) - 4);
    if (cm != imghead->hd_checksum) {
        LLOGD("foto包的头部校验和不正确 expect %08X but %08X", imghead->hd_checksum, cm);
        return;
    }
    cm = img_checksum((const char*)upgrade_img_addr + sizeof(IMAGE_HEADER_PARAM_ST), imghead->img_len);
    if (cm != imghead->org_checksum) {
        LLOGD("foto包的头部校验和不正确 expect %08X but %08X", imghead->org_checksum, cm);
        return;
    }
    // 当前肯定是压缩的, 需要引入miniz的API进行解压分析
    LLOGD("发现OTA数据, 继续进行脚本区更新");
    size_t inSize = imghead->img_len;
    uint8_t *ptr = (uint8_t *)upgrade_img_addr + sizeof(IMAGE_HEADER_PARAM_ST) + 10; // 跳过GZ的前10个字节

    int ret = my_tinfl_decompress_mem_to_callback(ptr, &inSize, ota_gzcb, NULL, 0);
    // LLOGD("OTA数据的前8个字节 %02X%02X%02X%02X%02X%02X%02X%02X", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7]);
    LLOGD("OTA解压函数的返回值 %d", ret);
}

