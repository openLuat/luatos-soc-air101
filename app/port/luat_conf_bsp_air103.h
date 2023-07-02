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

#define LUAT_BSP_VERSION "V0017"

// Air101 与 Air103 的Flash大小有差异,需要区分
#define AIR103

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
#define LUAT_USE_SDIO 1
// 段码屏/段式屏, 按需启用
#define LUAT_USE_LCDSEG 1
// OTP
#define LUAT_USE_OTP 1
#define LUAT_USE_TOUCHKEY 1


#define LUAT_USE_IOTAUTH 1

//----------------------------
// 常用工具库, 按需启用, cjson和pack是强烈推荐启用的
#define LUAT_USE_CRYPTO  1
#define LUAT_USE_CJSON  1
#define LUAT_USE_ZBUFF  1
#define LUAT_USE_PACK  1
#define LUAT_USE_LIBGNSS  1
#define LUAT_USE_FS  1
#define LUAT_USE_SENSOR  1
#define LUAT_USE_SFUD  1
#define LUAT_USE_COREMARK 1
#define LUAT_USE_IR 1
#define LUAT_USE_FSKV 1
#define LUAT_USE_OTA 1
#define LUAT_USE_I2CTOOLS 1
#define LUAT_USE_LORA 1
#define LUAT_USE_MLX90640 1
#define LUAT_USE_MAX30102 1
#define LUAT_USE_MINIZ 1

// RSA 加解密,加签验签
#define LUAT_USE_RSA 1
#define LUAT_USE_GMSSL 1
#define LUAT_USE_FATFS

//----------------------------
// 高级功能, 其中shell是推荐启用, 除非你打算uart0也读数据
#define LUAT_USE_REPL
#define LUAT_USE_PROTOBUF 1


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

#endif
