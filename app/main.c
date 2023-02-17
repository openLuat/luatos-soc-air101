#include "wm_include.h"
#include "wm_gpio_afsel.h"
#include "wm_psram.h"
#include "wm_internal_flash.h"
#include "wm_rtc.h"

#ifdef __LUATOS__
#include "string.h"
#include "luat_fs.h"
#include "bget.h"
#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_pm.h"

#include <string.h>
#include "wm_irq.h"
#include "tls_sys.h"
#include "wm_ram_config.h"
#include "wm_internal_flash.h"
#include "wm_psram.h"
#include "wm_efuse.h"
#include "wm_regs.h"

#include "FreeRTOS.h"

#define LUAT_LOG_TAG "main"
#include "luat_log.h"

#ifndef LUAT_HEAP_SIZE
#ifdef LUAT_USE_NIMBLE
#define LUAT_HEAP_SIZE (128+16)*1024
#else
#define LUAT_HEAP_SIZE (128+48)*1024
#endif
#endif

#if (LUAT_HEAP_SIZE > 128*1024)
 __attribute__((aligned(8))) static uint64_t heap_ext[(LUAT_HEAP_SIZE - 128*1024) / 8];
#endif

#ifdef LUAT_USE_TLSF
#include "tlsf.h"
tlsf_t luavm_tlsf;
pool_t luavm_tlsf_ext;
#endif

static void luat_start(void *sdata){
	// 毕竟sram还是快很多的, 优先sram吧
#ifndef LUAT_USE_TLSF
	bpool((void*)0x20028000, 128*1024);
#else
	luavm_tlsf = tlsf_create_with_pool((void*)0x20028000, 128*1024);
#endif

#ifdef LUAT_USE_PSRAM
	char test[] = {0xAA, 0xBB, 0xCC, 0xDD};
	char* psram_ptr = (void*)0x30010000;
	LLOGD("check psram ...");
	memcpy(psram_ptr, test, 4);
	if (memcmp(psram_ptr, test, 4)) {
		LLOGE("psram is enable, but can't access!!");
	}
	else {
		LLOGD("psram is ok");
		memset(psram_ptr, 0, 1024);
		// 存在psram, 加入到内存次, 就不使用系统额外的内存了.
		#ifdef LUAT_USE_TLSF
		luavm_tlsf_ext = tlsf_add_pool(luavm_tlsf, psram_ptr, 4*1024*1024);
		#else
		bpool(psram_ptr, 4*1024*1024); // 如果是8M内存, 改成 8也可以.
		#endif
		luat_main();
		return;
	}
#else
	// 暂时还是放在这里吧, 考虑改到0x20028000之前
	#if (LUAT_HEAP_SIZE > 128*1024)
#ifndef LUAT_USE_TLSF
	bpool((void*)heap_ext, LUAT_HEAP_SIZE - 128*1024);
#else
	luavm_tlsf_ext = tlsf_add_pool(luavm_tlsf, heap_ext, LUAT_HEAP_SIZE - 128*1024);
#endif
	#endif
#endif
	luat_main();
}

#ifdef LUAT_USE_LVGL
#include "lvgl.h"

// static uint8_t lvgl_called = 0;
static uint32_t lvgl_tick_cnt;
static int luat_lvgl_cb(lua_State *L, void* ptr) {
	if (lvgl_tick_cnt) lvgl_tick_cnt--;
	lv_task_handler();
	// lvgl_called = 0;
	return 0;
}

static void lvgl_timer_cb(void *ptmr, void *parg) {
	// if (lvgl_called)
		// return;
	if (lvgl_tick_cnt < 10)
	{
		lvgl_tick_cnt++;
		rtos_msg_t msg = {0};
		msg.handler = luat_lvgl_cb;
		luat_msgbus_put(&msg, 0);
	}
	// lvgl_called = 1;
}
// #define    LVGL_TASK_SIZE      512
// static OS_STK __attribute__((aligned(4)))			LVGLTaskStk[LVGL_TASK_SIZE] = {0};
#endif

#define    TASK_START_STK_SIZE         4096
static OS_STK __attribute__((aligned(4))) 			TaskStartStk[TASK_START_STK_SIZE] = {0};

#endif

// uint32_t rst_sta = 0;

#ifdef __LUATOS__
extern unsigned int  TLS_FLASH_PARAM_DEFAULT        ;
extern unsigned int  TLS_FLASH_PARAM1_ADDR          ;
extern unsigned int  TLS_FLASH_PARAM2_ADDR          ;
extern unsigned int  TLS_FLASH_PARAM_RESTORE_ADDR   ;
extern unsigned int  TLS_FLASH_OTA_FLAG_ADDR        ;
extern unsigned int  TLS_FLASH_END_ADDR             ;
#endif

void UserMain(void){
	char unique_id [18] = {0};

	tls_uart_options_t opt;
	opt.baudrate = UART_BAUDRATE_B921600;
	opt.charlength = TLS_UART_CHSIZE_8BIT;
	opt.flow_ctrl = TLS_UART_FLOW_CTRL_NONE;
	opt.paritytype = TLS_UART_PMODE_DISABLED;
	opt.stopbits = TLS_UART_ONE_STOPBITS;
	tls_uart_port_init(0, &opt, 0);

	struct tm tblock = {0};
	
	uint32_t rtc_ctrl1 = tls_reg_read32(HR_PMU_RTC_CTRL1);
	// 如果RTC计数少于1, 那肯定是第一次开机, 启动RTC并设置到1970年.
	// uint32_t rtc_ctrl2 = tls_reg_read32(HR_PMU_RTC_CTRL2);
	if (0x2 > rtc_ctrl1) {
		tblock.tm_mday = 1;
		tblock.tm_mon = 0;
		tblock.tm_year = 70;
		tls_set_rtc(&tblock);
	}
	else {
		// 只需要确保RTC启用
		int ctrl2  = tls_reg_read32(HR_PMU_RTC_CTRL2);		/* enable */
		ctrl2 |= (1 << 16);
		tls_reg_write32(HR_PMU_RTC_CTRL2, ctrl2);
	}

	// 完全禁用jtag
	//u32 value = tls_reg_read32(HR_CLK_SEL_CTL);
	// printf("HR_CLK_SEL_CTL %08X\n", value);
	//value = value & 0x7FFF;
	// value = value & 0x7F00;
	//tls_reg_write32(HR_CLK_SEL_CTL, value);
	// tls_reg_read32(HR_CLK_SEL_CTL);
	// printf("HR_CLK_SEL_CTL %08X\n", value);

	// 读取开机原因
	// rst_sta = tls_reg_read32(HR_CLK_RST_STA);
	// tls_reg_write32(HR_CLK_RST_STA, 0xFF);

#ifdef LUAT_USE_SHELL
	luat_shell_poweron(0);
#endif

#ifdef __LUATOS__
	extern void luat_mcu_tick64_init(void);
	luat_mcu_tick64_init();
	
	tls_fls_read_unique_id(unique_id);
	if (unique_id[1] == 0x10){
		printf("I/main auth ok %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X %s\n",
			unique_id[0], unique_id[1], unique_id[2], unique_id[3], unique_id[4],
			unique_id[5], unique_id[6], unique_id[7], unique_id[8], unique_id[9],
			unique_id[10], unique_id[11], unique_id[12], unique_id[13], unique_id[14],
			unique_id[15],unique_id[16],unique_id[17],
			luat_os_bsp());
	}else{
		printf("I/main auth ok %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X %s\n",
			unique_id[0], unique_id[1], unique_id[2], unique_id[3], unique_id[4],
			unique_id[5], unique_id[6], unique_id[7], unique_id[8], unique_id[9],
			luat_os_bsp());
	}
#endif

#ifdef AIR103
TLS_FLASH_PARAM_DEFAULT        =		  (0x80FB000UL);
TLS_FLASH_PARAM1_ADDR          =		  (0x80FC000UL);
TLS_FLASH_PARAM2_ADDR          =		  (0x80FD000UL);
TLS_FLASH_PARAM_RESTORE_ADDR   =	      (0x80FE000UL);
TLS_FLASH_OTA_FLAG_ADDR        =	      (0x80FF000UL);
TLS_FLASH_END_ADDR             =		  (0x80FFFFFUL);
#endif

#ifdef LUAT_USE_NIMBLE
	// TODO 注意, 除了启用LUAT_USE_NIMBTE外
	// 1. 修改FreeRTOSConfig.h的configTICK_RATE_HZ为500, 并重新make lib
	// 2. 若修改libblehost.a相关代码,需要手工复制bin目录下的文件,拷贝到lib目录. 
    tls_ft_param_init();
    tls_param_load_factory_default();
    tls_param_init(); /*add param to init sysparam_lock sem*/

	// 读蓝牙mac, 如果是默认值,就根据unique_id读取
	uint8_t bt_mac[6];
	// 缺省mac C0:25:08:09:01:10
	uint8_t bt_default_mac[] = {0xC0,0x25,0x08,0x09,0x01,0x10};
	tls_get_bt_mac_addr(bt_mac);
	if (!memcmp(bt_mac, bt_default_mac, 6)) { // 看来是默认MAC, 那就改一下吧
		if (unique_id[1] == 0x10){
			memcpy(bt_mac, unique_id + 10, 6);
		}
		else {
			memcpy(bt_mac, unique_id + 2, 6);
		}
		tls_set_bt_mac_addr(bt_mac);
	}
	LLOGD("BLE_4.2 %02X:%02X:%02X:%02X:%02X:%02X", bt_mac[0], bt_mac[1], bt_mac[2], bt_mac[3], bt_mac[4], bt_mac[5]);
#endif

// 如要使用psram,启用以下代码,并重新编译sdk
#ifdef LUAT_USE_PSRAM
	// 首先, 初始化psram相关引脚
#ifndef LUAT_USE_PSRAM_PORT
#ifdef AIR101
	// air101只能是0, 与SPI和UART3冲突, PB0~PB5
#define LUAT_USE_PSRAM_PORT 0
#else
    // air103可以是0或1
	// 1的话, PB2~PB5, PA15, PB27, 依然占用SPI0,但改用SPI1
#define LUAT_USE_PSRAM_PORT 1
#endif
#endif
	printf("psram init\n");
	wm_psram_config(LUAT_USE_PSRAM_PORT);
	// 然后初始化psram的寄存器
	psram_init(PSRAM_QPI);
	//uint8_t* psram_ptr = (uint8_t*)(PSRAM_ADDR_START);
#endif
	
#ifdef __LUATOS__
#ifdef LUAT_USE_LVGL
	lv_init();
	static tls_os_timer_t *os_timer = NULL;
	tls_os_timer_create(&os_timer, lvgl_timer_cb, NULL, 10/(1000 / configTICK_RATE_HZ), 1, NULL);
	tls_os_timer_start(os_timer);
#endif
	tls_os_task_create(NULL, NULL,
				luat_start,
				NULL,
				(void *)TaskStartStk,          /* task's stack start address */
				TASK_START_STK_SIZE * sizeof(u32), /* task's stack size, unit:byte */
				31,
				0);
#else
	printf("hello word\n");
	while (1);
#endif
}

#ifndef __LUATOS__
// void vApplicationTickHook( void ) {}
void bpool(void *buffer, long len) {}
#endif
