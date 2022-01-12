#include "luat_base.h"
#include "time.h"
#include "luat_msgbus.h"
// #include "wm_cmdp.h"
#include "wm_include.h"
#include "wm_watchdog.h"
#include "wm_pmu.h"
#include "wm_rtc.h"
#include "wm_regs.h"
#include "wm_cpu.h"

LUAMOD_API int luaopen_gtfont( lua_State *L );
LUAMOD_API int luaopen_nimble( lua_State *L );

time_t time(time_t* _tm)
{
  struct tm tmt = {0};
  tls_get_rtc(&tmt);
  time_t t = mktime(&tmt);
  if (_tm != NULL)
    memcpy(_tm, &t, sizeof(time_t));
  return t;
}
clock_t clock(void)
{
  return (clock_t)time(NULL);
}

static const luaL_Reg loadedlibs[] = {
  {"_G", luaopen_base}, // _G
  {LUA_LOADLIBNAME, luaopen_package}, // require
  {LUA_COLIBNAME, luaopen_coroutine}, // coroutine协程库
  {LUA_TABLIBNAME, luaopen_table},    // table库,操作table类型的数据结构
  {LUA_IOLIBNAME, luaopen_io},        // io库,操作文件
  {LUA_OSLIBNAME, luaopen_os},        // os库,已精简
  {LUA_STRLIBNAME, luaopen_string},   // string库,字符串操作
  {LUA_MATHLIBNAME, luaopen_math},    // math 数值计算
//  {LUA_UTF8LIBNAME, luaopen_utf8},
  {LUA_DBLIBNAME, luaopen_debug},     // debug库,已精简
#ifdef LUAT_USE_DBG
#ifndef LUAT_USE_SHELL
#define LUAT_USE_SHELL
#endif
  {"dbg",  luaopen_dbg},               // 调试库
#endif
#if defined(LUA_COMPAT_BITLIB)
  {LUA_BITLIBNAME, luaopen_bit32},    // 不太可能启用
#endif
// 往下是LuatOS定制的库, 如需精简请仔细测试
//----------------------------------------------------------------------
// 核心支撑库, 不可禁用!!
  {"rtos",    luaopen_rtos},              // rtos底层库, 核心功能是队列和定时器
  {"log",     luaopen_log},               // 日志库
  {"timer",   luaopen_timer},             // 延时库
//-----------------------------------------------------------------------
// 设备驱动类, 可按实际情况删减. 即使最精简的固件, 也强烈建议保留uart库
#ifdef LUAT_USE_UART
  {"uart",    luaopen_uart},              // 串口操作
#endif
#ifdef LUAT_USE_GPIO
  {"gpio",    luaopen_gpio},              // GPIO脚的操作
#endif
#ifdef LUAT_USE_I2C
  {"i2c",     luaopen_i2c},               // I2C操作
#endif
#ifdef LUAT_USE_SPI
  {"spi",     luaopen_spi},               // SPI操作
#endif
#ifdef LUAT_USE_ADC
  {"adc",     luaopen_adc},               // ADC模块
#endif
#ifdef LUAT_USE_SDIO
  {"sdio",     luaopen_sdio},             // SDIO模块
#endif
#ifdef LUAT_USE_PWM
  {"pwm",     luaopen_pwm},               // PWM模块
#endif
#ifdef LUAT_USE_WDT
  {"wdt",     luaopen_wdt},               // watchdog模块
#endif
#ifdef LUAT_USE_PM
  {"pm",      luaopen_pm},                // 电源管理模块
#endif
#ifdef LUAT_USE_MCU
  {"mcu",     luaopen_mcu},               // MCU特有的一些操作
#endif
#ifdef LUAT_USE_HWTIMER
  {"hwtimer", luaopen_hwtimer},           // 硬件定时器
#endif
#ifdef LUAT_USE_RTC
  {"rtc", luaopen_rtc},                   // 实时时钟
#endif
#ifdef LUAT_USE_OTP
  {"otp", luaopen_otp},                   // OTP
#endif
  {"pin", luaopen_pin},                   // pin
//-----------------------------------------------------------------------
// 工具库, 按需选用
#ifdef LUAT_USE_CRYPTO
  {"crypto",luaopen_crypto},            // 加密和hash模块
#endif
#ifdef LUAT_USE_CJSON
  {"json",    luaopen_cjson},          // json的序列化和反序列化
#endif
#ifdef LUAT_USE_ZBUFF
  {"zbuff",   luaopen_zbuff},             // 像C语言语言操作内存块
#endif
#ifdef LUAT_USE_PACK
  {"pack",    luaopen_pack},              // pack.pack/pack.unpack
#endif
  // {"mqttcore",luaopen_mqttcore},          // MQTT 协议封装
  // {"libcoap", luaopen_libcoap},           // 处理COAP消息

#ifdef LUAT_USE_GNSS
  {"libgnss", luaopen_libgnss},           // 处理GNSS定位数据
#endif
#ifdef LUAT_USE_FS
  {"fs",      luaopen_fs},                // 文件系统库,在io库之外再提供一些方法
#endif
#ifdef LUAT_USE_SENSOR
  {"sensor",  luaopen_sensor},            // 传感器库,支持DS18B20
#endif
#ifdef LUAT_USE_SFUD
  {"sfud", luaopen_sfud},              // sfud
#endif
#ifdef LUAT_USE_DISP
  {"disp",  luaopen_disp},              // OLED显示模块,支持SSD1306
#endif
#ifdef LUAT_USE_U8G2
  {"u8g2", luaopen_u8g2},              // u8g2
#endif

#ifdef LUAT_USE_EINK
  {"eink",  luaopen_eink},              // 电子墨水屏,试验阶段
#endif

#ifdef LUAT_USE_LVGL
#ifndef LUAT_USE_LCD
#define LUAT_USE_LCD
#endif
  {"lvgl",   luaopen_lvgl},
#endif

#ifdef LUAT_USE_LCD
  {"lcd",    luaopen_lcd},
#endif
#ifdef LUAT_USE_STATEM
  {"statem",    luaopen_statem},
#endif
#ifdef LUAT_USE_GTFONT
  {"gtfont",    luaopen_gtfont},
#endif
#ifdef LUAT_USE_NIMBLE
  {"nimble",    luaopen_nimble},
#endif
#ifdef LUAT_USE_FDB
  {"fdb",       luaopen_fdb},
#endif
#ifdef LUAT_USE_LCDSEG
  {"lcdseg",       luaopen_lcdseg},
#endif
#ifdef LUAT_USE_VMX
  {"vmx",       luaopen_vmx},
#endif
#ifdef LUAT_USE_COREMARK
  {"coremark", luaopen_coremark},
#endif
#ifdef LUAT_USE_FONTS
  {"fonts", luaopen_fonts},
#endif
#ifdef LUAT_USE_ZLIB
  {"zlib", luaopen_zlib},
#endif
  {NULL, NULL}
};

// 按不同的rtconfig加载不同的库函数
void luat_openlibs(lua_State *L) {
    // 初始化队列服务
    luat_msgbus_init();
    // print_list_mem("done>luat_msgbus_init");
    // 加载系统库
    const luaL_Reg *lib;
    /* "require" functions from 'loadedlibs' and set results to global table */
    for (lib = loadedlibs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);  /* remove lib */
        //extern void print_list_mem(const char* name);
        //print_list_mem(lib->name);
    }
}

#include "FreeRTOS.h"
#include "task.h"

#if configUSE_HEAP3
extern size_t xTotalHeapSize;
extern size_t xFreeBytesRemaining;
extern size_t xFreeBytesMin;
#endif

void luat_meminfo_sys(size_t* total, size_t* used, size_t* max_used)
{
#if configUSE_HEAP3
  *used = xTotalHeapSize - xFreeBytesRemaining;
  *max_used = xTotalHeapSize - xFreeBytesMin;
  *total = xTotalHeapSize;
#else
  *used = configTOTAL_HEAP_SIZE - xPortGetFreeHeapSize();
	*max_used = *used;
  *total = configTOTAL_HEAP_SIZE;
#endif
}


const char* luat_os_bsp(void)
{
#ifdef AIR103
    return "air103";
#else
    return "air101";
#endif
}

void luat_os_reboot(int code)
{
   tls_sys_reset();
}

static void pmu_timer0_irq(u8 *arg){}
void luat_os_standy(int timeout)
{
  tls_pmu_timer0_isr_register((tls_pmu_irq_callback)pmu_timer0_irq, NULL);
  tls_pmu_timer0_start(timeout);
  tls_pmu_standby_start();
  return;
}

//void delay_1us(unsigned int time);

void luat_timer_us_delay(size_t time)
{
  if(time<=0)
    return;
	volatile unsigned int value=1;
  clk_div_reg clk_div;
  clk_div.w = tls_reg_read32(HR_CLK_DIV_CTL);
  switch (clk_div.b.CPU)
  {
    case CPU_CLK_240M:
      value = 341*time/10 - 30;
      break;
    case CPU_CLK_160M:
      value = 227*time/10 - 22;
      break;
    case CPU_CLK_80M:
      value = 113*time/10;
      if(value>=22) value -= 22;
      break;
    case CPU_CLK_40M:
      value = 562*time/100;
      if(value>=31) value -= 31;
      break;
    default:
      value = time/2;
      break;
  }
  if(value<=1)
    return;
	while(value--)
	{
		__NOP();
	}
}

u32 cpu_sr = 0;
void luat_os_entry_cri(void) {
  cpu_sr = tls_os_set_critical();
}

void luat_os_exit_cri(void) {
  tls_os_release_critical(cpu_sr);
}

void luat_os_irq_disable(uint8_t IRQ_Type) {
  tls_irq_disable(IRQ_Type);
}

void luat_os_irq_enable(uint8_t IRQ_Type) {
  tls_irq_enable(IRQ_Type);
}

#ifdef LUAT_USE_LVGL
#include "lvgl.h"
#endif

void vApplicationTickHook( void ) {
	#ifdef LUAT_USE_LVGL
	lv_tick_inc(1000 / configTICK_RATE_HZ);
	#endif
}


//-------------- cjson 需要这个函数
int  strncasecmp ( const char* s1, const char* s2, size_t len )
{
	register unsigned int  x2;
	register unsigned int  x1;
	register const char*   end = s1 + len;

	while (1)
	{
		if ((s1 >= end) )
			return 0;

		x2 = *s2 - 'A'; if ((x2 < 26u)) x2 += 32;
		x1 = *s1 - 'A'; if ((x1 < 26u)) x1 += 32;
		s1++; s2++;

		if (x2 != x1)
			break;

		if (x1 == (unsigned int)-'A')
			break;
	}

	return x1 - x2;
}
//--------------
