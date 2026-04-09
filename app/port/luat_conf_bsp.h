
#ifndef LUAT_CONF_BSP
#define LUAT_CONF_BSP

#define LUAT_BSP_VERSION "V2001"

//------------------------------------------------------
// 以下custom --> 到  <-- custom 之间的内容,是供用户配置的
// 同时也是云编译可配置的部分. 提交代码时切勿删除会修改标识
//custom -->
//------------------------------------------------------


// 芯片型号选择
// 可选值: AIR101, AIR103, AIR601, AIR690, AIR6208
#define AIR6208

// 启用64位虚拟机
#define LUAT_CONF_VM_64bit

// 根据型号加载对应的分区内存头文件
// 注意：分区偏移与大小均由 partition/*.csv 自动生成
// LUAT_FS_SIZE 和 LUAT_SCRIPT_SIZE 为兼容旧代码保留的 KB 宏
#if defined(AIR101)
#include "partition_mem_AIR101.h"
#elif defined(AIR103)
#include "partition_mem_AIR103.h"
#elif defined(AIR601)
#include "partition_mem_AIR601.h"
#elif defined(AIR690)
#include "partition_mem_AIR690.h"
#elif defined(AIR6208)
#include "partition_mem_AIR6208.h"
#endif


//----------------------------
// 外设,按需启用, 最起码启用uart和wdt库
#define LUAT_USE_UART 1
#define LUAT_USE_GPIO 1
#define LUAT_USE_I2C  1
#define LUAT_USE_SPI  1
#define LUAT_USE_ADC  1
#define LUAT_USE_PWM  1
#define LUAT_USE_WDT  1
#define LUAT_USE_PM  1
#define LUAT_USE_MCU  1
#define LUAT_USE_RTC 1
// SDIO 仅支持TF/SD卡的挂载
// #define LUAT_USE_SDIO 1
// 段码屏/段式屏, 按需启用
// #define LUAT_USE_LCDSEG 1
#define LUAT_USE_OTP 1
// #define LUAT_USE_TOUCHKEY 1

// #define LUAT_USE_ICONV 1

// wlan库相关
#define LUAT_USE_WLAN 1
#define LUAT_USE_NETWORK 1
#define LUAT_USE_HTTP 1
#define LUAT_USE_MQTT 1
#define LUAT_USE_WEBSOCKET 1
#define LUAT_USE_SNTP 1
#define LUAT_USE_HTTPSRV 1
#define LUAT_USE_FTP 1
#define LUAT_USE_ERRDUMP 1
#define LUAT_USE_ICMP 1

// 开启TLS
#define LUAT_USE_TLS 1
// #define MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED 1
// #define LUAT_USE_CRYPTO_AES_MBEDTLS 1

// 其他网络功能

#define LUAT_USE_FOTA 1

// #define LUAT_USE_IOTAUTH 1

//----------------------------
// 常用工具库, 按需启用, cjson和pack是强烈推荐启用的
#define LUAT_USE_CRYPTO  1
#define LUAT_USE_CJSON  1
#define LUAT_USE_ZBUFF  1
#define LUAT_USE_PACK  1
// #define LUAT_USE_LIBGNSS  1
#define LUAT_USE_FS  1
// #define LUAT_USE_SENSOR  1
// #define LUAT_USE_SFUD  1
// #define LUAT_USE_IR 1
#define LUAT_USE_FSKV 1
// #define LUAT_USE_OTA 1
// #define LUAT_USE_I2CTOOLS 1
// #define LUAT_USE_LORA 1
// #define LUAT_USE_MLX90640 1
// #define LUAT_USE_MAX30102 1
// zlib压缩,更快更小的实现
#define LUAT_USE_MINIZ 1
// FASTLZ的内存需求小,压缩比不如miniz
// #define LUAT_USE_FASTLZ 1

// RSA 加解密,加签验签
// #define LUAT_USE_RSA 1
// #define LUAT_USE_XXTEA    1

// 国密算法 SM2/SM3/SM4
// #define LUAT_USE_GMSSL 1

// #define LUAT_USE_SQLITE3 1
// #define LUAT_USE_WS2812 1
// #define LUAT_USE_HT1621 1

// // 使用 TLSF 内存池, 实验性, 内存利用率更高一些
// #define LUAT_USE_TLSF 1

// 音频相关
#define LUAT_USE_I2S 1
#define LUAT_USE_MEDIA  1
#define LUAT_USE_AUDIO_G711 1
#define LUAT_SUPPORT_AMR 1

//---------------SDIO-FATFS特别配置
// sdio库对接的是fatfs
// fatfs的长文件名和非英文文件名支持需要180k的ROM, 非常奢侈
// 从v0006开始默认关闭之, 需要用到就打开吧
// #define LUAT_USE_FATFS
// #define LUAT_USE_FATFS_CHINESE 1

// #define LUAT_USE_YMODEM 1

//----------------------------
// 高级功能, 推荐使用REPL, 因为SHELL已废弃
// #define LUAT_USE_SHELL 1
// #define LUAT_USE_DBG
// NIMBLE 是蓝牙功能, 名为BLE, 但绝非低功耗.
// #define LUAT_USE_NIMBLE 1
// 多虚拟机支持,实验性,一般不启用
// #define LUAT_USE_VMX 1
// #define LUAT_USE_PROTOBUF 1
// #define LUAT_USE_REPL 1


// airlink和netdrv相关的功能
// #define LUAT_USE_AIRLINK 1
// #define LUAT_USE_AIRLINK_EXEC_SDATA 1
// #define LUAT_USE_AIRLINK_EXEC_NOTIFY 1
// #define LUAT_USE_AIRLINK_EXEC_WIFI 1
// #define LUAT_USE_AIRLINK_SPI_MASTER 1
// #define LUAT_USE_AIRLINK_UART 1

#define LUAT_USE_NETDRV 1
#define LUAT_USE_NETDRV_NAPT 1
#define LUAT_USE_NETDRV_CH390H 1

//---------------------
// UI
#define LUAT_USE_TP
// LCD  是彩屏, 若使用LVGL就必须启用LCD
#define LUAT_USE_LCD
#define LUAT_USE_TJPGD
// EINK 是墨水屏
// #define LUAT_USE_EINK

//---------------------
// U8G2
// 单色屏, 支持i2c/spi
// #define LUAT_USE_U8G2

/**************FONT*****************/
// #define LUAT_USE_FONTS
/**********U8G2&LCD&EINK FONT*************/
// #define USE_U8G2_OPPOSANSM_ENGLISH 1
// #define USE_U8G2_OPPOSANSM8_CHINESE
// #define USE_U8G2_OPPOSANSM10_CHINESE
// #define USE_U8G2_OPPOSANSM12_CHINESE
// #define USE_U8G2_OPPOSANSM16_CHINESE


// -------------------------------------
// PSRAM
#define LUAT_USE_PSRAM 1
#define LUAT_USE_PSRAM_PORT 0
#define LUAT_USE_PSRAM_2M 1

#define LUAT_USE_AIRUI 1
#define LUAT_USE_AIRUI_LUATOS 1
#define LUAT_USE_PINYIN 1
#define LUAT_USE_HZFONT 1
#define LUAT_USE_AIRUI_MISANS_FONT_16 1

#define LV_HOR_RES_MAX          (480)
#define LV_VER_RES_MAX          (480)
#define LV_COLOR_DEPTH          16

#define LV_COLOR_16_SWAP   1
//-------------------------------------------------------------------------------
//<-- custom
//------------------------------------------------------------------------------

#define LUAT_USE_LWIP 1

#if defined(LUAT_USE_HTTP) || defined(LUAT_USE_MQTT) || defined(LUAT_USE_FTP) || defined(LUAT_USE_SNTP) || defined(LUAT_USE_ERRDUMP)
#ifndef LUAT_USE_NETWORK
#define LUAT_USE_NETWORK
#endif
#endif

#if defined(LUAT_USE_NETWORK) || defined(LUAT_USE_ULWIP)
#ifndef LUAT_USE_DNS
#define LUAT_USE_DNS 1
#endif
#endif


// 内存优化: 减少内存消耗, 会稍微减低性能
// #define LUAT_USE_MEMORY_OPTIMIZATION_CODE_MMAP 1

//----------------------------------
// 使用VFS(虚拟文件系统)和内置库文件, 必须启用
#define LUAT_USE_FS_VFS 1
#define LUAT_USE_VFS_INLINE_LIB 1
//----------------------------------

#define LUAT_RET int
#define LUAT_RT_RET_TYPE	void
#define LUAT_RT_CB_PARAM void *param
// #define LUAT_USE_LOG2 1

// 单纯为了生成文档的宏
#define LUAT_USE_PIN 1

#define LUAT_GPIO_PIN_MAX (48)
#define LUAT_CONF_SPI_HALF_DUPLEX_ONLY 1

#ifdef LUAT_USE_SHELL
#undef LUAT_USE_REPL
#endif

#ifdef LUAT_USE_MEDIA
#ifndef LUAT_USE_I2S
#define LUAT_USE_I2S 1
#endif
#endif

#ifndef LUAT_USE_PSRAM
#if defined(LUAT_USE_PSRAM_1M) || defined(LUAT_USE_PSRAM_2M) || defined(LUAT_USE_PSRAM_4M) || defined(LUAT_USE_PSRAM_8M)
#define LUAT_USE_PSRAM 1
#endif
#endif

#ifdef LUAT_USE_SOFT_UART
#undef LUAT_USE_SOFT_UART
#endif

#endif

#define LUAT_USE_MEM_LOGOUT 1
// 暂时写这里了
#ifndef LUAT_CONF_FIRMWARE_TYPE_NUM
#ifdef LUAT_CONF_VM_64bit
#define LUAT_CONF_FIRMWARE_TYPE_NUM 101
#else
#define LUAT_CONF_FIRMWARE_TYPE_NUM 1
#endif
#endif
