/**
 * 
 * 
 * 
 * 
 * 
这个头文件单纯就是为了做发行版本的时候覆盖luat_conf_bsp.h

日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件
日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件
日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件
日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件
日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件
日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件
日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件
日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件
日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件
日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件
日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件
日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件
日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件
日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件
日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件
日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件
日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件
日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件
日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件
日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件
日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件
日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件
日常编译改 luat_conf_bsp.h 就行, 不用管这个头文件, 也不要引用这个头文件

 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 */

#ifndef LUAT_CONF_BSP
#define LUAT_CONF_BSP

#define LUAT_BSP_VERSION "V1023"

// Air601
#define AIR601

// 启用64位虚拟机
// #define LUAT_CONF_VM_64bit



// FLASH_FS_REGION_SIZE包含2部分: 脚本区和文件系统区
// 其中文件系统区固定48k, 脚本区默认64k, 两者加起来就是默认值 64+48=112
// 若需要增加脚本区的大小, 那么大小必须是64的整数倍, 例如变成 128,192,256
// 128k脚本区, 对应的 FLASH_FS_REGION_SIZE 为 176, 因为 128+48=176 
// 192k脚本区, 对应的 FLASH_FS_REGION_SIZE 为 240, 因为 192+48=240 
// 256k脚本区, 对应的 FLASH_FS_REGION_SIZE 为 304, 因为 256+48=304 
#define LUAT_FS_SIZE                48      //文件系统大小
#define LUAT_SCRIPT_SIZE            64      //脚本大小
#define FLASH_FS_REGION_SIZE        (LUAT_FS_SIZE + LUAT_SCRIPT_SIZE)

// 内存优化: 减少内存消耗, 会稍微减低性能
// #define LUAT_USE_MEMORY_OPTIMIZATION_CODE_MMAP 1

//----------------------------------
// 使用VFS(虚拟文件系统)和内置库文件, 必须启用
#define LUAT_USE_FS_VFS 1
#define LUAT_USE_VFS_INLINE_LIB 1
//----------------------------------

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
// OTP
// #define LUAT_USE_OTP 1
// #define LUAT_USE_TOUCHKEY 1

// #define LUAT_USE_ICONV 1

// wlan库相关
#define LUAT_USE_WLAN
#define LUAT_USE_LWIP
#define LUAT_USE_NETWORK
#define LUAT_USE_DNS
#define LUAT_USE_SNTP
#define LUAT_USE_HTTPSRV

// 外置网络支持
// #define LUAT_USE_W5500_XXX
// #define LUAT_USE_DHCP

// 内存不足, 无法开启TLS
// #define LUAT_USE_TLS


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
// #define LUAT_USE_STATEM 1
// 性能测试,跑完就是玩,不要与lvgl一起启用,生产环境的固件别加这个库
// #define LUAT_USE_COREMARK 1
// #define LUAT_USE_ZLIB 1 
// #define LUAT_USE_IR 1
// FDB 提供kv数据库, 与nvm库类似
// #define LUAT_USE_FDB 1
// fskv提供与fdb库兼容的API,旨在替换fdb库
#define LUAT_USE_FSKV 1
#define LUAT_USE_OTA 1
// #define LUAT_USE_I2CTOOLS 1
// #define LUAT_USE_LORA 1
// #define LUAT_USE_MLX90640 1
// #define LUAT_USE_MAX30102 1
// zlib压缩,更快更小的实现
// #define LUAT_USE_MINIZ 1

// RSA 加解密,加签验签
// #define LUAT_USE_RSA 1

// 国密算法 SM2/SM3/SM4
// #define LUAT_USE_GMSSL 1


// // 使用 TLSF 内存池, 实验性, 内存利用率更高一些
// #define LUAT_USE_TLSF 1

//---------------SDIO-FATFS特别配置
// sdio库对接的是fatfs
// fatfs的长文件名和非英文文件名支持需要180k的ROM, 非常奢侈
// 从v0006开始默认关闭之, 需要用到就打开吧
// #define LUAT_USE_FATFS
// #define LUAT_USE_FATFS_CHINESE

//----------------------------
// 高通字体, 需配合芯片使用
// #define LUAT_USE_GTFONT 1
// #define LUAT_USE_GTFONT_UTF8

// #define LUAT_USE_YMODEM

//----------------------------
// 高级功能, 推荐使用REPL, 因为SHELL已废弃
// #define LUAT_USE_SHELL 1
// #define LUAT_USE_DBG
// NIMBLE 是蓝牙功能, 名为BLE, 但绝非低功耗.
// #define LUAT_USE_NIMBLE 1
// 多虚拟机支持,实验性,一般不启用
// #define LUAT_USE_VMX 1
// #define LUAT_USE_NES
// #define LUAT_USE_PROTOBUF 1
#define LUAT_USE_REPL 1

//---------------------
// UI
// LCD  是彩屏, 若使用LVGL就必须启用LCD
// #define LUAT_USE_LCD
// #define LUAT_USE_TJPGD
// EINK 是墨水屏
// #define LUAT_USE_EINK

//---------------------
// U8G2
// 单色屏, 支持i2c/spi
// #define LUAT_USE_DISP
// #define LUAT_USE_U8G2

/**************FONT*****************/
// #define LUAT_USE_FONTS
/**********U8G2&LCD&EINK FONT*************/
// #define USE_U8G2_OPPOSANSM_ENGLISH 1
// #define USE_U8G2_OPPOSANSM8_CHINESE
// #define USE_U8G2_OPPOSANSM10_CHINESE
// #define USE_U8G2_OPPOSANSM12_CHINESE
// #define USE_U8G2_OPPOSANSM16_CHINESE
// #define USE_U8G2_OPPOSANSM24_CHINESE
// #define USE_U8G2_OPPOSANSM32_CHINESE
// SARASA
// #define USE_U8G2_SARASA_M8_CHINESE
// #define USE_U8G2_SARASA_M10_CHINESE
// #define USE_U8G2_SARASA_M12_CHINESE
// #define USE_U8G2_SARASA_M14_CHINESE
// #define USE_U8G2_SARASA_M16_CHINESE
// #define USE_U8G2_SARASA_M18_CHINESE
// #define USE_U8G2_SARASA_M20_CHINESE
// #define USE_U8G2_SARASA_M22_CHINESE
// #define USE_U8G2_SARASA_M24_CHINESE
// #define USE_U8G2_SARASA_M26_CHINESE
// #define USE_U8G2_SARASA_M28_CHINESE
/**********LVGL FONT*************/
// #define LV_FONT_OPPOSANS_M_8
// #define LV_FONT_OPPOSANS_M_10
// #define LV_FONT_OPPOSANS_M_12
// #define LV_FONT_OPPOSANS_M_16

// -------------------------------------
// PSRAM
// 需要外挂PSRAM芯片, 否则不要启用, 必死机
// air101虽然支持psram,但与spi存在复用冲突
// air103支持psram与spi同时使用,复用不冲突
// #define LUAT_USE_PSRAM 1
// LVGL推荐把部分方法放入内存, 按需采用
// #define LV_ATTRIBUTE_FAST_MEM __attribute__((section (".ram_run")))
// ROTABLE技术是节省内存的关键技术, 启用PSRAM后内存不缺, 禁用可提高性能
// #define LUAT_CONF_DISABLE_ROTABLE 1
//---------------------------------------


//---------------------
// LVGL
// 主推的UI库, 功能强大但API繁琐
// #define LUAT_USE_LVGL
#define LV_DISP_DEF_REFR_PERIOD 30
#define LUAT_LV_DEBUG 0

#define LV_MEM_CUSTOM 1

#define LUAT_USE_LVGL_INDEV 1 // 输入设备

#define LUAT_USE_LVGL_ARC   //圆弧 无依赖
#define LUAT_USE_LVGL_BAR   //进度条 无依赖
#define LUAT_USE_LVGL_BTN   //按钮 依赖容器CONT
#define LUAT_USE_LVGL_BTNMATRIX   //按钮矩阵 无依赖
#define LUAT_USE_LVGL_CALENDAR   //日历 无依赖
#define LUAT_USE_LVGL_CANVAS   //画布 依赖图片IMG
#define LUAT_USE_LVGL_CHECKBOX   //复选框 依赖按钮BTN 标签LABEL
#define LUAT_USE_LVGL_CHART   //图表 无依赖
#define LUAT_USE_LVGL_CONT   //容器 无依赖
#define LUAT_USE_LVGL_CPICKER   //颜色选择器 无依赖
#define LUAT_USE_LVGL_DROPDOWN   //下拉列表 依赖页面PAGE 标签LABEL
#define LUAT_USE_LVGL_GAUGE   //仪表 依赖进度条BAR 仪表(弧形刻度)LINEMETER
#define LUAT_USE_LVGL_IMG   //图片 依赖标签LABEL
#define LUAT_USE_LVGL_IMGBTN   //图片按钮 依赖按钮BTN
#define LUAT_USE_LVGL_KEYBOARD   //键盘 依赖图片按钮IMGBTN
#define LUAT_USE_LVGL_LABEL   //标签 无依赖
#define LUAT_USE_LVGL_LED   //LED 无依赖
#define LUAT_USE_LVGL_LINE   //线 无依赖
#define LUAT_USE_LVGL_LIST   //列表 依赖页面PAGE 按钮BTN 标签LABEL
#define LUAT_USE_LVGL_LINEMETER   //仪表(弧形刻度) 无依赖
#define LUAT_USE_LVGL_OBJMASK   //对象蒙版 无依赖
#define LUAT_USE_LVGL_MSGBOX   //消息框 依赖图片按钮IMGBTN 标签LABEL
#define LUAT_USE_LVGL_PAGE   //页面 依赖容器CONT
#define LUAT_USE_LVGL_SPINNER   //旋转器 依赖圆弧ARC 动画ANIM
#define LUAT_USE_LVGL_ROLLER   //滚筒 无依赖
#define LUAT_USE_LVGL_SLIDER   //滑杆 依赖进度条BAR
#define LUAT_USE_LVGL_SPINBOX   //数字调整框 无依赖
#define LUAT_USE_LVGL_SWITCH   //开关 依赖滑杆SLIDER
#define LUAT_USE_LVGL_TEXTAREA   //文本框 依赖标签LABEL 页面PAGE
#define LUAT_USE_LVGL_TABLE   //表格 依赖标签LABEL
#define LUAT_USE_LVGL_TABVIEW   //页签 依赖页面PAGE 图片按钮IMGBTN
#define LUAT_USE_LVGL_TILEVIEW   //平铺视图 依赖页面PAGE
#define LUAT_USE_LVGL_WIN   //窗口 依赖容器CONT 按钮BTN 标签LABEL 图片IMG 页面PAGE

#define LV_HOR_RES_MAX          (160)
#define LV_VER_RES_MAX          (80)
#define LV_COLOR_DEPTH          16

#define LV_COLOR_16_SWAP   1

#define LUAT_RET int
#define LUAT_RT_RET_TYPE	void
#define LUAT_RT_CB_PARAM void *param

// 单纯为了生成文档的宏
#define LUAT_USE_PIN 1

#define LUAT_GPIO_PIN_MAX (48)

#endif
