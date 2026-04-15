
#include "luat_base.h"
#include "luat_log.h"
#include "luat_uart.h"
#include "luat_mem.h"
#include "luat_mcu.h"
#include "luat_pm.h"

#include "luat_soc_service.h"

uint32_t soc_get_power_on_reason(void) {
    return luat_pm_get_poweron_reason();
}
const char* soc_get_sdk_type(void) {
    return "LUATOS";
}
const char* soc_get_sdk_version(void) {
    return LUAT_BSP_VERSION;
}
const char* soc_get_chip_name(void) {
    return "AIR6208";
}

void am_print_base_info(void)
{
	uint8_t power_on_reason = soc_get_power_on_reason();
	LTIO("soc poweron: %d %s_%s_%s 0", power_on_reason, soc_get_sdk_type(), soc_get_sdk_version(), soc_get_chip_name());
	LTIO("BASEINFO: %s %s_%s_%s", "0000000000000000", soc_get_sdk_type(), soc_get_sdk_version(), soc_get_chip_name());
    LTIO("+HW: %s", soc_get_chip_name());
    #ifdef LUAT_CONF_FIRMWARE_TYPE_NUM
    #ifdef LUAT_CONF_VM_64bit
    soc_info("+FW: LuatOS %s %d 64bit", LUAT_BSP_VERSION, LUAT_CONF_FIRMWARE_TYPE_NUM);
    #else
    soc_info("+FW: LuatOS %s %d 32bit", LUAT_BSP_VERSION, LUAT_CONF_FIRMWARE_TYPE_NUM);
    #endif
    #endif
}

