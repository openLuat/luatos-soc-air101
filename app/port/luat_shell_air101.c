#include "luat_shell.h"
#include "wm_include.h"

#define LUAT_LOG_TAG "luat.shell"
#include "luat_log.h"

static int drv = -1;
#define CONSOLE_BUF_SIZE   512

typedef struct console_st
{
    size_t rx_data_len;
    char rx_buf[CONSOLE_BUF_SIZE];		/*uart rx*/
} console;

static console 	ShellConsole;

void luat_shell_write(char* buff, size_t len) {
    if (drv > -1 && len >= 0) {
        tls_uart_write(drv, buff, len);
    }
}

char* luat_shell_read(size_t *len) {
    if(ShellConsole.rx_data_len == 0)
    {
        *len = 0;
        return NULL;
    }
    if (ShellConsole.rx_data_len > CONSOLE_BUF_SIZE)
        ShellConsole.rx_data_len = CONSOLE_BUF_SIZE;
    int ret = tls_uart_read(drv, ShellConsole.rx_buf, ShellConsole.rx_data_len);
    *len = ret;
    return ShellConsole.rx_buf;
}

void luat_shell_notify_recv(void) {
    ShellConsole.rx_data_len = 0;
}

int16_t demo_console_rx(uint16_t len, void* user_data){
    ShellConsole.rx_data_len += len;
    luat_shell_notify_read();
    return 0;
}

void luat_shell_poweron(int _drv) {
    drv = _drv;
    memset(ShellConsole.rx_buf, 0, CONSOLE_BUF_SIZE + 1);
    tls_uart_rx_callback_register(drv,(int16_t(*)(uint16_t, void*))demo_console_rx, NULL);
    luat_shell_notify_recv();
}

