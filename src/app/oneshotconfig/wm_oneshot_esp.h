
#ifndef WM_ONESHOT_ESP_H
#define WM_ONESHOT_ESP_H


#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "wm_type_def.h"

#include "wm_wifi.h"
#include "tls_common.h"
#include "wm_ieee80211_gcc.h"



#define ESP_ONESHOT_DEBUG 		0

#define ESP_REPLY_PORT			18266
#define ESP_REPLY_MAX_CNT		20


typedef enum
{

	ESP_ONESHOT_CONTINUE = 0,

	ESP_ONESHOT_CHAN_TEMP_LOCKED = 1,

	ESP_ONESHOT_CHAN_LOCKED = 2,

	ESP_ONESHOT_COMPLETE = 3

} esp_oneshot_status_t;

struct esp_param_t{
	u8 ssid[33];
	u8 pwd[65];
	u8 ip[4];
	u8 bssid[6];
	u8 ssid_len;
	u8 pwd_len;
	u8 total_len;
};

typedef int (*esp_printf_fn) (const char* format, ...);

extern struct esp_param_t esp_param;
extern esp_printf_fn esp_printf;
extern u8 esp_reply;



int tls_esp_recv(u8 *buf, u16 data_len);
void tls_esp_init(u8 *scanBss);


#endif


