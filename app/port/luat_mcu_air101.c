#include "luat_base.h"
#include "luat_mcu.h"

#include "wm_include.h"
#include "wm_cpu.h"
#include "wm_internal_flash.h"

#include "FreeRTOS.h"
#include "task.h"

#define LUAT_LOG_TAG "mcu"
#include "luat_log.h"

/*
enum CPU_CLK{
	CPU_CLK_240M = 2,
	CPU_CLK_160M = 3,
	CPU_CLK_80M  = 6,
	CPU_CLK_40M  = 12,
	CPU_CLK_2M  = 240,
};
*/
int luat_mcu_set_clk(size_t mhz) {
    switch(mhz) {
        case 240:
            tls_sys_clk_set(CPU_CLK_240M);
            break;
        case 160:
            tls_sys_clk_set(CPU_CLK_160M);
            break;
        case 80:
            tls_sys_clk_set(CPU_CLK_80M);
            break;
        case 40:
            tls_sys_clk_set(CPU_CLK_40M);
            break;
        case 2:
            tls_sys_clk_set(CPU_CLK_2M);
            break;
        default :
            return -1;
    }
    return 0;
}

int luat_mcu_get_clk(void) {
    tls_sys_clk sysclk;
    tls_sys_clk_get(&sysclk);
    return sysclk.cpuclk;
}

static u8 unique_id[18] = {0};
static u8 unique_id_got = 0;
const char* luat_mcu_unique_id(size_t* t) {
    if (!unique_id_got && tls_fls_read_unique_id(unique_id) == 0)
        unique_id_got = 1;
    *t = unique_id_got ? unique_id[1]+2 : 0;
    return unique_id;
}

long luat_mcu_ticks(void) {
    return xTaskGetTickCount();
}

uint32_t luat_mcu_hz(void) {
    return configTICK_RATE_HZ;
}

#include "wm_include.h"
#include "wm_timer.h"
#include "wm_regs.h"
uint32_t us_timer_ticks = 0;
u8 u64_tick_timer_id = 0;

static void utimer_cb(void* arg) {
    us_timer_ticks ++;
}

void luat_mcu_tick64_init(void) {
    struct tls_timer_cfg cfg = {0};
    cfg.unit = TLS_TIMER_UNIT_US;
    cfg.timeout = (u32)(-1);
    cfg.is_repeat = 1;
    cfg.callback = utimer_cb;
    cfg.arg = NULL;
    u64_tick_timer_id = tls_timer_create(&cfg);
    tls_timer_start(u64_tick_timer_id);
}

uint64_t luat_mcu_tick64(void) {
    uint64_t ret = ((uint64_t) us_timer_ticks) << 32;
    ret += M32(HR_TIMER0_CNT + 4*u64_tick_timer_id);
    return ret;
}

int luat_mcu_us_period(void) {
    return 1;
}

uint64_t luat_mcu_tick64_ms(void) {
    return luat_mcu_tick64() / 1000;
}

void luat_mcu_set_clk_source(uint8_t source_main, uint8_t source_32k, uint32_t delay) {
    LLOGE("not support setXTAL");
}
