#include "wm_include.h"
#include "wm_gpio_afsel.h"
#include "wm_psram.h"
#include "wm_internal_flash.h"
#include "wm_rtc.h"
#include "wm_osal.h"
#include "wm_watchdog.h"

#include "FreeRTOS.h"
#include "task.h"

#ifdef __LUATOS__
#include "string.h"
#include "luat_fs.h"
#include "bget.h"
#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_pm.h"
#include "luat_rtc.h"
#include "luat_pcap.h"
#include "luat_uart.h"
#include "luat_malloc.h"

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

#define LUAT_PCAP_UART_ID 2

void luat_heap_init(void);

#if defined(LUAT_USE_PCAP)
static void pcap_uart_write(void *ptr, const void* buf, size_t len) {
	luat_uart_write(LUAT_PCAP_UART_ID, buf, len);
}
#endif

static void luat_start(void *sdata){
	(void)sdata;
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
#endif


#endif

void check_stack(void* ptr) {
	(void)ptr;
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
void luat_fs_update_addr(void);
#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
void UserMain(void){
	unsigned  char unique_id [20] = {0};

	tls_uart_options_t opt = {0};
	opt.baudrate = UART_BAUDRATE_B921600;
	opt.charlength = TLS_UART_CHSIZE_8BIT;
	opt.flow_ctrl = TLS_UART_FLOW_CTRL_NONE;
	opt.paritytype = TLS_UART_PMODE_DISABLED;
	opt.stopbits = TLS_UART_ONE_STOPBITS;
	tls_uart_port_init(0, &opt, 0);

	LLOGD("poweron: %s", reason[luat_pm_get_poweron_reason()]);
	luat_fs_update_addr();
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


#ifdef LUAT_USE_WLAN
	u8 tmpmac[8] = {0};
	tls_ft_param_get(CMD_WIFI_MACAP, tmpmac, 6);
	LLOGD("AP MAC %02X:%02X:%02X:%02X:%02X:%02X", tmpmac[0], tmpmac[1], tmpmac[2], tmpmac[3], tmpmac[4], tmpmac[5]);
	tls_ft_param_get(CMD_WIFI_MAC, tmpmac, 6);
	LLOGD("STA MAC %02X:%02X:%02X:%02X:%02X:%02X", tmpmac[0], tmpmac[1], tmpmac[2], tmpmac[3], tmpmac[4], tmpmac[5]);
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

	#if defined(LUAT_USE_FOTA)
	extern void luat_fota_boot_check(void);
	luat_fota_boot_check();
	#endif

#ifdef __LUATOS__
	luat_heap_init();
// PCAP抓包
#ifdef LUAT_USE_PCAP
	// 初始化pcap
	luat_uart_t uart2 = {
		.id = LUAT_PCAP_UART_ID,
		.baud_rate = 115200,
		.data_bits = 8,
		.stop_bits = 1,
		.parity = 0
	};
	luat_uart_setup(&uart2);
	luat_pcap_init(pcap_uart_write, NULL);
	luat_pcap_write_head();
#endif
#ifdef LUAT_USE_LVGL
	lv_init();
	static tls_os_timer_t *os_timer = NULL;
	tls_os_timer_create(&os_timer, lvgl_timer_cb, NULL, 10/(1000 / configTICK_RATE_HZ), 1, NULL);
	tls_os_timer_start(os_timer);
#endif
	#define VM_SIZE (12*1024)
	#if defined(LUAT_USE_WLAN) && defined(LUAT_USE_NIMBLE) && defined(LUAT_USE_TLS)
	char *vm_task_stack = luat_heap_alloc(NULL, NULL, 0, VM_SIZE);
	// LLOGD("VM heap start %p", vm_task_stack);
	#else
	char *vm_task_stack = luat_heap_malloc(VM_SIZE);
	#endif
	tls_os_task_create(NULL, "luatos",
				luat_start,
				NULL,
				(void *)vm_task_stack,          /* task's stack start address */
				VM_SIZE, /* task's stack size, unit:byte */
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

static int is_mac_ok(u8 mac_addr[6]) {

	// 如果是默认值, 那就是不合法
	if (!memcmp(mac_addr, default_mac, 6)) {
        return 0;
    }
	// 如果任意一位是0x00或者0xFF,那就是不合法
    for (size_t i = 0; i < 6; i++)
    {
        if (mac_addr[i] == 0 || mac_addr[i] == 0xFF) {
            return 0;
        }
    }
	return 1;
}

void sys_mac_init() {
#ifdef LUAT_CONF_LOG_UART1
	luat_log_set_uart_port(1);
#endif
	u8 tmp_mac[6] = {0};
    u8 mac_addr[6] = {0};
	u8 ap_mac[6] = {0};
    unsigned  char unique_id [20] = {0};
    tls_fls_read_unique_id(unique_id);
	int ret = 0;

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
	ret = tls_ft_param_get(CMD_WIFI_MAC, mac_addr, 6);
	if (ret) {
		printf("tls_ft_param_get mac addr FALED %d !!!\n", ret);
	}
	tls_ft_param_get(CMD_WIFI_MACAP, ap_mac, 6);
	//printf("CMD_WIFI_MAC ret %d %02X%02X%02X%02X%02X%02X\n", ret, mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
	int sta_mac_ok = is_mac_ok(mac_addr);
	int ap_mac_ok = is_mac_ok(ap_mac);

	// 其中一个合法呢?
	if (sta_mac_ok && !ap_mac_ok) {
		printf("sta地址合法, ap地址不合法\n");
		memcpy(ap_mac, mac_addr, 6);
	}
	else if (!sta_mac_ok && ap_mac_ok) {
		printf("sta地址不合法, ap地址合法\n");
		memcpy(mac_addr, ap_mac, 6);
	}
	else if (!sta_mac_ok && !ap_mac_ok){ // 均不合法, 那就用uniqid
		printf("sta地址不合法, ap地址不合法\n");
		if (unique_id[1] == 0x10){
			memcpy(mac_addr, unique_id + 12, 6);
		}
		else {
			memcpy(mac_addr, unique_id + 4, 6);
		}
        mac_addr[0] = 0x0C;
        for (size_t i = 1; i < 6; i++)
        {
            if (mac_addr[i] == 0 || mac_addr[i] == 0xFF) {
                mac_addr[i] = 0x01;
            }
        }
		memcpy(ap_mac, mac_addr, 6);
	}

	if (!memcmp(mac_addr, ap_mac, 6)) {
		printf("sta与ap地址相同, 调整ap地址\n");
		// 完全相同, 那就改一下ap的地址
		if (ap_mac[5] == 0x01 || ap_mac[5] == 0xFE) {
			ap_mac[5] = 0x02;
		}
		else {
			ap_mac[5] += 1;
		}
	}

	// 全部mac都得到了, 然后重新读一下mac, 不相同的就重新写入
	
	// 先判断sta的mac
	tls_ft_param_get(CMD_WIFI_MAC, tmp_mac, 6);
	if (memcmp(tmp_mac, mac_addr, 6)) {
		// 不一致, 写入
		tls_ft_param_set(CMD_WIFI_MAC, mac_addr, 6);
		printf("更新STA mac %02X%02X%02X%02X%02X%02X\n", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
	}

	// 再判断ap的mac
	tls_ft_param_get(CMD_WIFI_MACAP, tmp_mac, 6);
	if (memcmp(tmp_mac, ap_mac, 6)) {
		// 不一致, 写入
		tls_ft_param_set(CMD_WIFI_MACAP, ap_mac, 6);
		printf("更新AP mac %02X%02X%02X%02X%02X%02X\n", ap_mac[0], ap_mac[1], ap_mac[2], ap_mac[3], ap_mac[4], ap_mac[5]);
	}
#endif
}
