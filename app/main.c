#include "wm_include.h"
#include "wm_gpio_afsel.h"
#include "wm_psram.h"
#include "wm_internal_flash.h"
#include "wm_rtc.h"
#include "wm_osal.h"
#include "wm_watchdog.h"

#ifdef __LUATOS__
#include "string.h"
#include "luat_fs.h"
#include "bget.h"
#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_pm.h"
#include "luat_rtc.h"

#include <string.h>
#include "wm_irq.h"
#include "tls_sys.h"
#include "wm_ram_config.h"
#include "wm_internal_flash.h"
#include "wm_psram.h"
#include "wm_efuse.h"
#include "wm_regs.h"
#include "wm_wifi.h"

#include "FreeRTOS.h"

#define LUAT_LOG_TAG "main"
#include "luat_log.h"

void luat_heap_init(void);


static void luat_start(void *sdata){
	luat_heap_init();
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

#define    TASK_START_STK_SIZE         (3*1024) // 实际*4, 即12k
static OS_STK __attribute__((aligned(4))) 			TaskStartStk[TASK_START_STK_SIZE] = {0};

#endif

void check_stack(void* ptr) {
	while (1) {
		vTaskDelay(1000);
		tls_os_disp_task_stat_info();
	}
}

static const const char* reason[] = {
	"power or reset",
	"by charge", // 不可能
	"wakeup by rtc",
	"reset by software",
	"unkown",
	"reset by key",
	"reboot by exception",
	"reboot by tool",
	"reset by watchdog",
	"reset by pad",
	"by charge" // 不可能
};

extern int luat_pm_get_poweron_reason(void);
extern int power_bk_reg;
#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
void UserMain(void){
	char unique_id [20] = {0};

	tls_uart_options_t opt = {0};
	opt.baudrate = UART_BAUDRATE_B921600;
	opt.charlength = TLS_UART_CHSIZE_8BIT;
	opt.flow_ctrl = TLS_UART_FLOW_CTRL_NONE;
	opt.paritytype = TLS_UART_PMODE_DISABLED;
	opt.stopbits = TLS_UART_ONE_STOPBITS;
	tls_uart_port_init(0, &opt, 0);

	LLOGD("poweron: %s", reason[luat_pm_get_poweron_reason()]);
	// printf("Bit 8 -- %d\n", CHECK_BIT(power_bk_reg, 8));
	// printf("Bit 5 -- %d\n", CHECK_BIT(power_bk_reg, 5));
	// printf("Bit 2 -- %d\n", CHECK_BIT(power_bk_reg, 2));
	//printf("bsp reboot_reason %d\n", tls_sys_get_reboot_reason());

	struct tm tt = {0};
	luat_rtc_get(&tt);
	// printf("main get %d-%d-%d %d:%d:%d\n", tt.tm_year + 1900, tt.tm_mon + 1, tt.tm_mday, tt.tm_hour, tt.tm_min, tt.tm_sec);
	
	// uint32_t rtc_ctrl1 = tls_reg_read32(HR_PMU_RTC_CTRL1);
	// printf("rtc_ctrl1 %ld\n", rtc_ctrl1);
	// 如果RTC计数少于1, 那肯定是第一次开机, 启动RTC并设置到1970年.
	// uint32_t rtc_ctrl2 = tls_reg_read32(HR_PMU_RTC_CTRL2);
	if (tt.tm_year == 0) {
		tt.tm_mday = 1;
		tt.tm_mon = 0;
		tt.tm_year = 71;
		luat_rtc_set(&tt);
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

#if defined(LUAT_USE_SHELL) || defined(LUAT_USE_REPL)
	luat_shell_poweron(0);
#endif

#ifdef __LUATOS__
	extern void luat_mcu_tick64_init(void);
	luat_mcu_tick64_init();
	
	tls_fls_read_unique_id(unique_id);
	if (unique_id[1] == 0x10) {
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
	tls_os_task_create(NULL, "luatos",
				luat_start,
				NULL,
				(void *)TaskStartStk,          /* task's stack start address */
				TASK_START_STK_SIZE * sizeof(u32), /* task's stack size, unit:byte */
				21,
				0);
	// tls_os_task_create(NULL, "cstack", check_stack, NULL, NULL, 2048, 10, 0);
#else
	printf("hello word\n");
	while (1);
#endif
}

#ifndef __LUATOS__
// void vApplicationTickHook( void ) {}
void bpool(void *buffer, long len) {}
#endif

extern const u8 default_mac[];
void sys_mac_init() {
    u8 mac_addr[6];
    char unique_id [20] = {0};
    tls_fls_read_unique_id(unique_id);

#ifdef LUAT_USE_NIMBLE
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

#ifdef LUAT_USE_WLAN
	tls_get_mac_addr(mac_addr);
    int mac_ok = 1;
    if (!memcmp(mac_addr, default_mac, 6)) {
        mac_ok = 0;
    }
    else {
        mac_ok = 1;
        for (size_t i = 0; i < 6; i++)
        {
            if (mac_addr[i] == 0 || mac_addr[i] == 0xFF) {
                mac_ok = 0;
                break;
            }
        }
        
    }
	if (!mac_ok) { // 看来是默认MAC, 那就改一下吧
		if (unique_id[1] == 0x10){
			memcpy(mac_addr, unique_id + 12, 6);
		}
		else {
			memcpy(mac_addr, unique_id + 4, 6);
		}
        mac_addr[0] = 0x0C;
        for (size_t i = 0; i < 6; i++)
        {
            if (mac_addr[i] == 0 || mac_addr[i] == 0xFF) {
                mac_addr[i] = 0x01;
            }
        }
        LLOGD("auto fix wifi mac addr -> %02X:%02X:%02X:%02X:%02X:%02X\n", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        
        tls_set_mac_addr(mac_addr);
	}
    // printf("WIFI %02X:%02X:%02X:%02X:%02X:%02X\n", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
#endif
}
