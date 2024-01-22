#include <string.h>
#include "wm_irq.h"
#include "tls_sys.h"

#include "wm_regs.h"
#include "wm_type_def.h"
#include "wm_timer.h"
#include "wm_irq.h"
#include "wm_params.h"
#include "wm_hostspi.h"
#include "wm_flash.h"
#include "wm_fls_gd25qxx.h"
#include "wm_internal_flash.h"
#include "wm_efuse.h"
#include "wm_debug.h"
#include "wm_netif.h"
#include "wm_at_ri_init.h"
#include "wm_config.h"
#include "wm_osal.h"
#include "wm_http_client.h"
#include "wm_cpu.h"
#include "wm_webserver.h"
#include "wm_io.h"
#include "wm_mem.h"
#include "wm_wl_task.h"
#include "wm_wl_timers.h"
#ifdef TLS_CONFIG_HARD_CRYPTO
#include "wm_crypto_hard.h"
#endif
#include "wm_gpio_afsel.h"
#include "wm_pmu.h"
#include "wm_ram_config.h"
#include "wm_uart.h"
#include "luat_conf_bsp.h"
#include "wm_watchdog.h"
#include "wm_wifi.h"
#if TLS_CONFIG_ONLY_FACTORY_ATCMD
#include "../../src/app/factorycmd/factory_atcmd.h"
#endif

#include "wm_flash_map.h"
#include "wm_wifi.h"


#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_timer.h"
#include "luat_gpio.h"
#include "luat_uart.h"
#include "luat_spi.h"
#include "luat_malloc.h"
#include "luat_uart.h"

#include "luat_zlink.h"
#include "crc.h"

#define LUAT_LOG_TAG "zlink.wlan"
#include "luat_log.h"

uint32_t zlink_pkgid = 0;
extern u8* tls_wifi_buffer_acquire(int total_len);
extern void tls_wifi_buffer_release(bool is_apsta, u8* buffer);


static void zlink_wifi_status_change(uint8_t status) {
    LLOGD("wifi status change to %d", status);
}

static int zlink_net_rx_data_cb(const u8 *bssid, u8 *buf, u32 buf_len) {
    // LLOGD("net recv %d bytes", buf_len);
    // if (buf_len == 60)
    //     return 0;
    printf("net recv %d bytes\n", buf_len);
    
    // u8 mac_source[6];
    // u8 mac_dest[6];
    // memcpy(mac_source, buf + 6, 6);
    // memcpy(mac_dest, buf, 6);
    // printf("OUT %02x:%02x:%02x:%02x:%02x:%02x -> %02x:%02x:%02x:%02x:%02x:%02x %u len %u\n", 
    //     mac_source[0], mac_source[1], mac_source[2], mac_source[3], mac_source[4], mac_source[5],  
    //     mac_dest[0], mac_dest[1], mac_dest[2], mac_dest[3], mac_dest[4], mac_dest[5],
    //     buf[12], buf_len);
    uint32_t checksum = calcCRC32(buf, buf_len);
    // printf("CRC %08X\n", checksum);
    luat_zlink_pkg_t zd = {0};
    memcpy(zd.magic, "ZLNK", 4);
    zd.pkgid[0] = zlink_pkgid >> 24;
    zd.pkgid[1] = zlink_pkgid >> 16;
    zd.pkgid[2] = zlink_pkgid >> 8;
    zd.pkgid[3] = zlink_pkgid & 0xFF;
    zlink_pkgid ++;

    zd.cmd0 = 2; // mac包
    zd.cmd1 = 1; // 发送

    u16 len = buf_len + 4;
    zd.len[0] = len >> 8;
    zd.len[1] = len & 0xFF;
    luat_uart_write(1, &zd, sizeof(luat_zlink_pkg_t));
    luat_uart_write(1, "\0\0", 2);

    luat_uart_write(1, buf, buf_len);
    return 0;
}

//串口数量，编号从0开始
#define MAX_DEVICE_COUNT TLS_UART_MAX

//存放串口设备句柄
static uint8_t serials_buff_len[MAX_DEVICE_COUNT] ={TLS_UART_RX_BUF_SIZE};
extern struct tls_uart_port uart_port[TLS_UART_MAX];

#define BUFF_SIZE (4096)
static u8 uart_buff[BUFF_SIZE];
static uint16_t buff_offset;

static s16 uart_input_cb(u16 len, void* userdata) {
    // printf("uart_input_cb %d\n", len);
    // if(CIRC_CNT(uart_port[1].recv.head, uart_port[1].recv.tail, TLS_UART_RX_BUF_SIZE)
    //         < (serials_buff_len[1] - 200))
    //     return 0;
    // if (len == 0)
    //     return 0;
    // if (buff_offset + len > BUFF_SIZE) {
    //     buff_offset = 0;
    // }
    len = luat_uart_read(1, uart_buff + buff_offset, BUFF_SIZE - buff_offset);
    if (len < 0) {
        printf("luat_uart_read %d\n", len);
        return 0;
    }
    if (len == 0) {
        return 0;
    }
    buff_offset += len;
    if (buff_offset < 12) {
        // printf("数据还不够 %d\n", len);
        return 0;
    }
    luat_zlink_pkg_t* zd = (luat_zlink_pkg_t*)uart_buff;
    if (memcmp(zd->magic, "ZLNK", 4) != 0) {
        printf("数据包开头不是ZINK\n");
        buff_offset = 0;
        return 0;
    }
    u16 zlen = (zd->len[0] << 8) | zd->len[1];
    if (buff_offset >= zlen + 12) {
        printf("完整的zlink数据包来了\n");
        if (zd->cmd0 == 2 && zd->cmd1 == 1) {
            // wifi mac包
            printf("mac包 %d\n", zlen - 4);
            u8* ptr = tls_wifi_buffer_acquire(zlen - 4);
            if (ptr) {
                printf("发送到wifi\n");
                memcpy(ptr, uart_buff + 16, zlen - 4);
                tls_wifi_buffer_release(false, ptr);
            }
            else {
                printf("wifi buffer full?\n");
            }
        }
        buff_offset -= zlen + 12;
    }
    else {
        // printf("数据包不完整,等待数据\n");
    }
    return 0;
}

void luat_zlink_wlan_init(void) {
    // tls_uart_options_t opt = {0};
	// opt.baudrate = UART_BAUDRATE_B115200;
	// opt.charlength = TLS_UART_CHSIZE_8BIT;
	// opt.flow_ctrl = TLS_UART_FLOW_CTRL_NONE;
	// opt.paritytype = TLS_UART_PMODE_DISABLED;
	// opt.stopbits = TLS_UART_ONE_STOPBITS;
	// tls_uart_port_init(1, &opt, 0);

    luat_uart_t conf = {0};
    conf.id = 1;
    conf.baud_rate = 115200;
    conf.data_bits = 8;
    conf.stop_bits = 1;
    conf.parity = LUAT_PARITY_NONE;
    luat_uart_setup(&conf);

    tls_uart_rx_callback_register(1, uart_input_cb, NULL);

    tls_wifi_status_change_cb_register(zlink_wifi_status_change);
    tls_ethernet_data_rx_callback(zlink_net_rx_data_cb);
}
