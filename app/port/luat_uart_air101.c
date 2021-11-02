#include "luat_base.h"
#include "luat_malloc.h"
#include "luat_msgbus.h"
#include "luat_uart.h"
#define LUAT_LOG_TAG "luat.uart.101"
#include "luat_log.h"

#include "wm_include.h"
#include "wm_uart.h"
#include "wm_gpio_afsel.h"
#include "stdio.h"
//串口数量，编号从0开始
#define MAX_DEVICE_COUNT TLS_UART_MAX
//存放串口设备句柄
static uint8_t serials_buff_len[MAX_DEVICE_COUNT] ={TLS_UART_RX_BUF_SIZE};
extern struct tls_uart_port uart_port[TLS_UART_MAX];

int luat_uart_exist(int uartid)
{
    if (uartid < 0 || uartid >= MAX_DEVICE_COUNT)
    {
        return 0;
    }
    return 1;
}

static s16 uart_input_cb(u16 len, void* user_data)
{
    int uartid = (int)user_data;
    //不是fifo超时回调
    if(uartid <= MAX_DEVICE_COUNT)
    {
        //未读取长度够不够？
        if(CIRC_CNT(uart_port[uartid].recv.head, uart_port[uartid].recv.tail, TLS_UART_RX_BUF_SIZE)
            < (serials_buff_len[uartid] - 200))
            return 0;
    }
    else//是fifo超时回调
    {
        uartid /= 10;
    }

    rtos_msg_t msg;
    msg.handler = l_uart_handler;
    msg.ptr = NULL;
    msg.arg1 = uartid;
    msg.arg2 = CIRC_CNT(uart_port[uartid].recv.head, uart_port[uartid].recv.tail, TLS_UART_RX_BUF_SIZE);
    luat_msgbus_put(&msg, 1);
    return 0;
}

//串口发送完成事件回调
static s16 uart_sent_cb(struct tls_uart_port *port)
{
    tls_uart_free_tx_sent_data(port);
    rtos_msg_t msg;
    msg.handler = l_uart_handler;
    msg.arg1 = port->uart_no;
    msg.arg2 = 0;
    msg.ptr = NULL;
    luat_msgbus_put(&msg, 1);
    return 0;
}

int luat_uart_setup(luat_uart_t *uart)
{
    int ret;
    tls_uart_options_t opt = {0};
    if (!luat_uart_exist(uart->id))
    {
        return -1;
    }

    opt.baudrate = uart->baud_rate;
    opt.charlength = (uart->data_bits)-5;
    opt.paritytype = uart->parity;
    opt.flow_ctrl = TLS_UART_FLOW_CTRL_NONE;
    opt.stopbits = (uart->stop_bits)-1;

    switch (uart->id)
    {
    case TLS_UART_0:
        wm_uart0_rx_config(WM_IO_PB_20);
        wm_uart0_tx_config(WM_IO_PB_19);
        break;
    case TLS_UART_1:
        wm_uart1_rx_config(WM_IO_PB_07);
        wm_uart1_tx_config(WM_IO_PB_06);
        break;
    case TLS_UART_2:
        wm_uart2_rx_config(WM_IO_PB_03);
        wm_uart2_tx_scio_config(WM_IO_PB_02);
        break;
    case TLS_UART_3:
        wm_uart3_rx_config(WM_IO_PB_01);
        wm_uart3_tx_config(WM_IO_PB_00);
        break;
    case TLS_UART_4:
        wm_uart4_rx_config(WM_IO_PB_05);
        wm_uart4_tx_config(WM_IO_PB_04);
        break;
#ifdef AIR103
    case TLS_UART_5:
        wm_uart5_rx_config(WM_IO_PA_13);
        wm_uart5_tx_config(WM_IO_PA_12);
        break;
#endif
    default:
        break;
    }

    ret = tls_uart_port_init(uart->id, &opt, 0);
    if (ret !=  WM_SUCCESS)
    {
       return ret; //初始化失败
    }
    if(uart->bufsz > TLS_UART_RX_BUF_SIZE)
        uart->bufsz = TLS_UART_RX_BUF_SIZE;
    if(uart->bufsz < 1024)
        uart->bufsz = 1024;
    serials_buff_len[uart->id] = uart->bufsz;

    return ret;

}


int luat_uart_write(int uartid, void *data, size_t length)
{
    int ret = 0;
    //printf("uid:%d,data:%s,length = %d\r\n",uartid, (char *)data, length);
    if (!luat_uart_exist(uartid))
    {
        return 0;
    }
    ret = tls_uart_write(uartid, data,length);

    return ret;
}

int luat_uart_read(int uartid, void *buffer, size_t length)
{
    int ret = 0;
    if (!luat_uart_exist(uartid))
    {
        return 0;
    }
    ret =  tls_uart_read(uartid,(u8 *) buffer,(u16)length);
    return ret;
}

int luat_uart_close(int uartid)
{
    if (!luat_uart_exist(uartid))
    {
        return 0;
    }
    // tls_uart_port_init(uartid,NULL,0);
    return 0;
}

int luat_setup_cb(int uartid, int received, int sent)
{
    if (!luat_uart_exist(uartid))
    {
        return -1;
    }
    if (received)
    {
        tls_uart_rx_callback_register(uartid,(s16(*)(u16, void*))uart_input_cb, (void*)uartid);
    }

    if (sent)
    {
        tls_uart_tx_callback_register(uartid, (s16(*)(struct tls_uart_port *))uart_sent_cb);
    }

    return 0;
}
