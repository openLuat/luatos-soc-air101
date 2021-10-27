#include "luat_base.h"
#include "luat_mcu.h"

#include "wm_include.h"
#include "wm_cpu.h"

#include "FreeRTOS.h"
#include "task.h"

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
