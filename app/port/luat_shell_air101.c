#include "luat_shell.h"
#include "wm_include.h"

#define LUAT_LOG_TAG "luat.shell"
#include "luat_log.h"
#include "wm_uart.h"

#include "luat_uart.h"

typedef struct {
	size_t len;
	char buff[0];
}uart_data;

void luat_shell_write(char* buff, size_t len) {
    int i=0;
    while (i < len){
		while(tls_reg_read32(HR_UART0_FIFO_STATUS) & 0x3F);
	    tls_reg_write32(HR_UART0_TX_WIN, (unsigned int)buff[i++]);
    }
}

void luat_shell_notify_recv(void) {
}

static tls_os_queue_t *shell_queue = NULL;

static int16_t luat_shell_uart_cb(uint16_t len, void* user_data){
	int uartid = (int)user_data;
	char buff[512] = {0};
	if(uartid >= 100)
	{
		int l = 1;
		while (l > 0) {
			l = luat_uart_read(0, buff, 512);
			//printf("uart read buff %d %s\n", l, buff);
			if (l > 0){
				// luat_shell_push(buff, l);
				uart_data* send_buf = (uart_data*)luat_heap_malloc(sizeof(uart_data)+l);
				send_buf->len = l;
    			memcpy(send_buf->buff, buff, l);
				tls_os_queue_send(shell_queue, (void *)send_buf, sizeof(uart_data)+l);
			}
		}
	}
    return 0;
}

static void luat_shell(void *sdata){
	uart_data* receive_buf;
	while (1) {
		tls_os_queue_receive(shell_queue, (void **) &receive_buf, 0, 0);
		// printf("uart read buff %s\n", (char*)msg);
		luat_shell_push(receive_buf->buff, receive_buf->len);
		luat_heap_free(receive_buf);
	}
}

#define    TASK_START_STK_SIZE         2048
static OS_STK __attribute__((aligned(4))) 			TaskStartStk[TASK_START_STK_SIZE] = {0};
void luat_shell_poweron(int _drv) {
    tls_uart_rx_callback_register(0, luat_shell_uart_cb, NULL);
	tls_os_queue_create(&shell_queue, 2048);
	tls_os_task_create(NULL, NULL,
				luat_shell,
				NULL,
				(void *)TaskStartStk,          /* task's stack start address */
				TASK_START_STK_SIZE * sizeof(u32), /* task's stack size, unit:byte */
				10,
				0);
}

