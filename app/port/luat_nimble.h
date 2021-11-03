#include "luat_base.h"
#include "luat_msgbus.h"

int luat_nimble_trace_level(int level);

int luat_nimble_init(uint8_t uart_idx, char* name);
int luat_nimble_deinit();

void test_server_api_init();
void test_server_api_deinit();

