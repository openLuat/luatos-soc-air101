#include "luat_shell.h"
#include "wm_include.h"

#define LUAT_LOG_TAG "luat.shell"
#include "luat_log.h"
#include "wm_uart.h"

#include "luat_uart.h"

void luat_shell_write(char* buff, size_t len) {
    int i=0;
    while (i < len){
		while(tls_reg_read32(HR_UART0_FIFO_STATUS) & 0x3F);
	    tls_reg_write32(HR_UART0_TX_WIN, (unsigned int)buff[i++]);
    }
}

void luat_shell_notify_recv(void) {
}

static int16_t luat_shell_uart_cb(uint16_t len, void* user_data){
	int uartid = (int)user_data;
	char buff[512] = {0};
	if(uartid >= 100)
	{
		int l = 1;
		while (l > 0) {
			l = luat_uart_read(0, buff, 512);
			//printf("uart read buff %d %s\n", l, buff);
			if (l > 0)
				luat_shell_push(buff, l);
		}
	}
    return 0;
}

void luat_shell_poweron(int _drv) {
    tls_uart_rx_callback_register(0, luat_shell_uart_cb, NULL);
}

