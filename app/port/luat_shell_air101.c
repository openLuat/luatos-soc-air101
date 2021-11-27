#include "luat_shell.h"
#include "wm_include.h"

#define LUAT_LOG_TAG "luat.shell"
#include "luat_log.h"
#include "wm_uart.h"

static int drv = -1;
#define CONSOLE_BUF_SIZE   512

extern struct tls_uart_port uart_port[TLS_UART_MAX];

typedef struct console_st
{
    size_t rx_data_len;
    char rx_buf[CONSOLE_BUF_SIZE];		/*uart rx*/
} console;

static console 	ShellConsole;

int luat_sendchar(int port,int ch){
	if (port == 0){
		if(ch == '\n'){
			while (tls_reg_read32(HR_UART0_FIFO_STATUS) & 0x3F);
			tls_reg_write32(HR_UART0_TX_WIN, '\r');
	    }
	    while(tls_reg_read32(HR_UART0_FIFO_STATUS) & 0x3F);
	    tls_reg_write32(HR_UART0_TX_WIN, (char)ch);
	}else if (port == 1){
		if(ch == '\n'){
			while (tls_reg_read32(HR_UART1_FIFO_STATUS) & 0x3F);
			tls_reg_write32(HR_UART1_TX_WIN, '\r');
	    }
		while(tls_reg_read32(HR_UART1_FIFO_STATUS) & 0x3F);
		tls_reg_write32(HR_UART1_TX_WIN, (char)ch);
	}else if (port == 2){
		if(ch == '\n'){
			while (tls_reg_read32(HR_UART2_FIFO_STATUS) & 0x3F);
			tls_reg_write32(HR_UART2_TX_WIN, '\r');
	    }
		while(tls_reg_read32(HR_UART2_FIFO_STATUS) & 0x3F);
		tls_reg_write32(HR_UART2_TX_WIN, (char)ch);
	}else if (port == 3){
		if(ch == '\n'){
			while (tls_reg_read32(HR_UART3_FIFO_STATUS) & 0x3F);
			tls_reg_write32(HR_UART3_TX_WIN, '\r');
	    }
		while(tls_reg_read32(HR_UART3_FIFO_STATUS) & 0x3F);
		tls_reg_write32(HR_UART3_TX_WIN, (char)ch);
	}else if (port == 4){
		if(ch == '\n'){
			while (tls_reg_read32(HR_UART4_FIFO_STATUS) & 0x3F);
			tls_reg_write32(HR_UART4_TX_WIN, '\r');
	    }
		while(tls_reg_read32(HR_UART4_FIFO_STATUS) & 0x3F);
		tls_reg_write32(HR_UART4_TX_WIN, (char)ch);
	}
    return ch;
}

void luat_shell_write(char* buff, size_t len) {
    int i=0;
    if (drv > -1 && len >= 0) {
        while (i<len){
            luat_sendchar(drv,buff[i++]);
        };
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

static int16_t luat_shell_uart_cb(uint16_t len, void* user_data){
	int uartid = (int)user_data;
	if(uartid >= 100)
	{
		ShellConsole.rx_data_len = CIRC_CNT(uart_port[0].recv.head, uart_port[0].recv.tail, TLS_UART_RX_BUF_SIZE);
    	luat_shell_notify_read();
	}
    return 0;
}

void luat_shell_poweron(int _drv) {
    drv = _drv;
    memset(ShellConsole.rx_buf, 0, CONSOLE_BUF_SIZE + 1);
    tls_uart_rx_callback_register(drv,(int16_t(*)(uint16_t, void*))luat_shell_uart_cb, NULL);
    luat_shell_notify_recv();
}

