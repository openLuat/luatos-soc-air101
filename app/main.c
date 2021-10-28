#include "wm_include.h"
#include "wm_gpio_afsel.h"
#include "wm_psram.h"
#include "wm_internal_flash.h"

#ifdef USE_LUATOS
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

#include "FreeRTOS.h"

#define LUAT_LOG_TAG "main"
#include "luat_log.h"

#ifndef LUAT_HEAP_SIZE
#define LUAT_HEAP_SIZE (128+48)*1024
#endif

#if (LUAT_HEAP_SIZE > 128*1024)
static uint8_t __attribute__((aligned(4))) heap_ext[LUAT_HEAP_SIZE - 128*1024] = {0};
#endif

static void luat_start(void *sdata){
	bpool((void*)0x20028000, 128*1024);
	#if (LUAT_HEAP_SIZE > 128*1024)
	bpool((void*)heap_ext, LUAT_HEAP_SIZE - 128*1024);
	#endif
	luat_main();
}

#ifdef LUAT_USE_LVGL
#include "lvgl.h"

static void _lvgl_handler(void* args) {
    while (1) {
		lv_task_handler();
        vTaskDelay(10 / (1000 / configTICK_RATE_HZ));
    };
}
#define    LVGL_TASK_SIZE      512
static OS_STK __attribute__((aligned(4)))			LVGLTaskStk[LVGL_TASK_SIZE] = {0};
#endif

#define    TASK_START_STK_SIZE         2048
static OS_STK __attribute__((aligned(4))) 			TaskStartStk[TASK_START_STK_SIZE] = {0};

#endif

int rst_sta = 0;

#ifdef USE_LUATOS
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

	// 读取开机原因
	rst_sta = tls_reg_read32(HR_CLK_RST_STA);
	tls_reg_write32(HR_CLK_RST_STA, 0xFF);


#ifdef LUAT_USE_SHELL
	luat_shell_poweron(0);
#endif

#ifdef USE_LUATOS
	tls_fls_read_unique_id(unique_id);
	if (unique_id[1] == 0x10){
		printf("I/main auth ok %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X %s\n",
			unique_id[0], unique_id[1], unique_id[2], unique_id[3], unique_id[4],
			unique_id[5], unique_id[6], unique_id[7], unique_id[8], unique_id[9],
			unique_id[10], unique_id[11], unique_id[12], unique_id[13], unique_id[14],unique_id[15],
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
#endif

#if 0
	// 首先, 初始化psram相关引脚
	printf("==============================\n");
	printf("CALL wm_psram_config(1)\n");
	wm_psram_config(1);
	// 然后初始化psram的寄存器
	printf("CALL psram_init()\n");
	psram_init(PSRAM_QPI); // 如果失败, 再试试PSRAM_SPI
	uint8_t* psram_ptr = (uint8_t*)(PSRAM_ADDR_START);
	// memset + memcheck
	memset(psram_ptr, 0x00, PSRAM_SIZE_BYTE);
	for (size_t psram_pos = 0; psram_pos < PSRAM_SIZE_BYTE; psram_pos++)
	{
		if (psram_ptr[psram_pos] != 0x00) {
			printf("PSRAM memcheck 0x00 fail at %08X %02X\n", PSRAM_ADDR_START + psram_pos, psram_ptr[psram_pos]);
			break;
		}
	}
	memset(psram_ptr, 0x3A, PSRAM_SIZE_BYTE);
	for (size_t psram_pos = 0; psram_pos < PSRAM_SIZE_BYTE; psram_pos++)
	{
		if (psram_ptr[psram_pos] != 0x3A) {
			printf("PSRAM memcheck 0x3A fail at %08X %02X\n", PSRAM_ADDR_START + psram_pos, psram_ptr[psram_pos]);
			break;
		}
	}
	printf("==============================\n");
#endif
	
	
#ifdef USE_LUATOS
#ifdef LUAT_USE_LVGL
	lv_init();
	tls_os_task_create(NULL, NULL,
				_lvgl_handler,
				NULL,
				(void *)LVGLTaskStk,          /* task's stack start address */
				LVGL_TASK_SIZE * sizeof(u32), /* task's stack size, unit:byte */
				40,
				0);
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

#ifndef USE_LUATOS
void vApplicationTickHook( void ) {}
void bpool(void *buffer, long len) {}
#endif
