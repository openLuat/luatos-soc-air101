
#include "wm_oneshot_esp.h"



#define ESP_GUIDE_DATUM			512
#define ESP_DATA_OFFSET			40
#define ESP_CRC_POLYNOM 		0x8C
#define ESP_CRC_INITIAL 		0x00

struct esp_data_t{
	u8 data;
	u8 used;
};

esp_printf_fn esp_printf = NULL;
static u8 *esp_scan_bss;


const u8 esp_dst_addr[3] = {0x01,0x00,0x5e};
u8 esp_last_num[2] = {0,0};
u16 esp_head[2][4] = {{0,0,0,0},{0,0,0,0}};
u16 esp_byte[2][3] = {{0,0,0},{0,0,0}};
u8 esp_state = 0;
u16	esp_data_datum = 0; 
u8 esp_head_cnt[2] = {0,0};
u8 esp_byte_cnt[2] = {0,0};
u8 esp_sync_cnt = 0;
u8 esp_src_mac[6] = {0};
u8 esp_crc_table[256];
u8 esp_crc_value = 0;
u8 esp_data_cnt = 0;
u16 esp_last_seq[2] = {0,0};
u16 esp_last_len = 0;
u8 esp_temp_lock = 0;



struct esp_data_t esp_data[120];
struct esp_param_t esp_param;

extern u8 gucssidData[];
extern u8 gucpwdData[];

void esp_crc8_table_init(void)
{
	int i;
	u8 bit;
	u8 remainder;
	
	for(i=0; i<256; i++)
	{
		remainder = i;
		for(bit=0; bit<8; bit++)
		{
			if((remainder&0x01) != 0)
				remainder = (remainder >> 1) ^ ESP_CRC_POLYNOM;
			else
				remainder >>= 1;
		}
		esp_crc_table[i] = remainder;
	}
}

void esp_crc8_init(void)
{
	esp_crc_value = ESP_CRC_INITIAL;
}

void esp_crc8_update(u8 data)
{
	u8 val;
	val = data ^ esp_crc_value;
	esp_crc_value = esp_crc_table[val] ^ (esp_crc_value<<8);
}

u8 esp_crc8_get(void)
{
	return esp_crc_value;
}

u8 esp_crc8_calc(u8 *buf, u16 len)
{
	u16 i;
	
	esp_crc8_init();
	for(i=0; i<len; i++)
	{
		esp_crc8_update(buf[i]);
	}

	return esp_crc8_get();
}

static int esp_ssid_bssid_crc_match(u8 ssidCrc, u8 bssidCrc, u8 ssidLen, u8 *ssid,  u8 *bssid)
{
	struct tls_scan_bss_t *bss = NULL;
	int i = 0;

	bss = (struct tls_scan_bss_t *)esp_scan_bss;

	if(bss == NULL)
	{
		return -1;
	}
	
	for (i = 0; i < bss->count; i++)
	{
		if ((ssidCrc == esp_crc8_calc(bss->bss[i].ssid, bss->bss[i].ssid_len)) && (ssidLen ==  bss->bss[i].ssid_len)
			&& (bssidCrc == esp_crc8_calc(bss->bss[i].bssid, 6)))
		{
			memcpy(ssid, bss->bss[i].ssid, bss->bss[i].ssid_len);
			memcpy(bssid, bss->bss[i].bssid, 6);
			return 0;
		}
	}

	return -1;
}

int tls_esp_recv(u8 *buf, u16 data_len)
{
    struct ieee80211_hdr *hdr = (struct ieee80211_hdr*)buf;
    u8 *multicast = NULL;
    u8 *SrcMac = NULL;
	u8 crc, num, data;
	u16 i;
	u8 totalXor, totalLen, apPwdLen, apSsidCrc, apSsidLen, apBssidCrc;
	int ret;
	u16 frm_len;
	u16 guide_len;
	u8 tods = 0;

	multicast = ieee80211_get_DA(hdr);

	if(hdr->duration_id & 0x02)		//normal mode stbc不处理
	{
		return ESP_ONESHOT_CONTINUE;
	}

    if (ieee80211_is_data_qos(hdr->frame_control))
    {
        frm_len = data_len - 2;
    }
	else
	{
		frm_len = data_len;
	}
	
	tods = ieee80211_has_tods(hdr->frame_control);
	SrcMac = ieee80211_get_SA(hdr);
	
	if(memcmp(multicast, esp_dst_addr, 3) &&  (0 == is_broadcast_ether_addr(multicast)))
	{
		return ESP_ONESHOT_CONTINUE;
	}

	switch(esp_state)
	{
		case 0:
			if(frm_len < 512)
			{
				return ESP_ONESHOT_CONTINUE;
			}
					
			if(is_zero_ether_addr(esp_src_mac))
			{
				memcpy(esp_src_mac, SrcMac, 6);
				esp_head_cnt[0] = esp_head_cnt[1] = 0;
				esp_sync_cnt = 0;
				esp_temp_lock = 0;
				esp_last_seq[0] = esp_last_seq[1] = 0;
				memset(esp_head, 0, sizeof(esp_head));
			}
			else
			{
				if(memcmp(esp_src_mac, SrcMac, 6))
				{
					memcpy(esp_src_mac, SrcMac, 6);
					esp_head_cnt[0] = esp_head_cnt[1] = 0;
					esp_sync_cnt = 0;
					esp_temp_lock = 0;
					esp_last_seq[0] = esp_last_seq[1] = 0;
					memset(esp_head, 0, sizeof(esp_head));
				}else{
					if(esp_printf)
						esp_printf("tods:%d,%d,"MACSTR"\n", tods, frm_len, MAC2STR(SrcMac));
				}
			}

			if (ieee80211_has_retry(hdr->frame_control) && (esp_last_seq[tods] == hdr->seq_ctrl))
			{
				return ESP_ONESHOT_CONTINUE;
			}
			esp_last_seq[tods] = hdr->seq_ctrl;

			esp_head[tods][esp_head_cnt[tods]] = frm_len;

			if(esp_head_cnt[tods] > 0)
			{
				if(((esp_head[tods][esp_head_cnt[tods]]+1) != esp_head[tods][esp_head_cnt[tods]-1])
					&& ((esp_head[tods][esp_head_cnt[tods]]-3) != esp_head[tods][esp_head_cnt[tods]-1]))
				{
					esp_temp_lock = 0;
					esp_head_cnt[tods] = 0;
					esp_head[tods][0] = frm_len;
				}else{
					esp_temp_lock = 1;
				}

			}
			esp_head_cnt[tods] ++;

			if(esp_head_cnt[tods] >= 4)
			{
				esp_sync_cnt ++;
				esp_head_cnt[tods] = 0;
			}
	
			if(esp_sync_cnt >= 1)
			{
				guide_len = esp_head[tods][0];		
				for(i=1; i<=3; i++)
				{
					if(guide_len > esp_head[tods][i])
						guide_len = esp_head[tods][i];								//取出同步头中最小值					
				}
				esp_state = 1;														//同步完成, 锁定源MAC和信道
				esp_data_datum = guide_len - ESP_GUIDE_DATUM + ESP_DATA_OFFSET;		//获取到基准长度
				if(esp_printf)
					esp_printf("esp lock:%d\n", esp_data_datum);	
				return ESP_ONESHOT_CHAN_LOCKED;
			}	
			if(esp_temp_lock == 1)
			{
				return ESP_ONESHOT_CHAN_TEMP_LOCKED;
			}
			break;

		case 1:
			if((frm_len >= 512) || (frm_len < esp_data_datum))
			{
				return ESP_ONESHOT_CONTINUE;
			}

			if(memcmp(esp_src_mac, SrcMac, 6))
			{
				return ESP_ONESHOT_CONTINUE;
			}
				
			if (ieee80211_has_retry(hdr->frame_control) && (esp_last_seq[tods] == hdr->seq_ctrl))
			{
				return ESP_ONESHOT_CONTINUE;
			}
			esp_last_seq[tods] = hdr->seq_ctrl;

			if(esp_last_num[tods] != multicast[5])
			{
				memset((u8 *)&esp_byte[tods][0], 0, 3);
				esp_byte_cnt[tods] = 0;
				esp_last_num[tods] = multicast[5];
			}

			esp_byte[tods][esp_byte_cnt[tods]] = frm_len - esp_data_datum;
			if((esp_byte_cnt[tods]==0) && (esp_byte[tods][0]>=256))
			{
				esp_byte_cnt[tods] = 0;
			}
			else if((esp_byte_cnt[tods]==1) && (esp_byte[tods][1]<256))
			{
				esp_byte[tods][0] = frm_len - esp_data_datum;
				esp_byte_cnt[tods] = 1;
			}
			else if((esp_byte_cnt[tods]==2) && (esp_byte[tods][2]>=256))
			{
				esp_byte_cnt[tods] = 0;
			}
			else
			{
				esp_byte_cnt[tods] ++;
			}

			if(esp_byte_cnt[tods] >= 3)
			{	
				crc = (esp_byte[tods][0]&0xF0) + ((esp_byte[tods][2]&0xF0)>>4);
				num = esp_byte[tods][1]&0xFF;
				data = ((esp_byte[tods][0]&0x0F)<<4) + (esp_byte[tods][2]&0x0F);
				if(esp_data[num].used == 0)
				{
					esp_crc8_init();
					esp_crc8_update(data);
					esp_crc8_update(num);
					if(crc == esp_crc8_get())
					{
						if(esp_printf)
							esp_printf("%d\n", num);
						esp_data[num].data = data;
						esp_data[num].used = 1;
						esp_data_cnt ++;
					}
				}
				esp_byte_cnt[tods] = 0;
			}
			if(esp_data[0].used)
			{
				if(esp_data_cnt >= esp_data[0].data + 6)		//计算异或校验，CRC校验等
				{
					if(esp_printf)
						esp_printf("here1\n");
					totalLen = esp_data[0].data;
					totalXor = 0;
					for(i=0; i<totalLen; i++)
					{
						totalXor ^= esp_data[i].data;
					}
					if(totalXor != 0)							//异或校验错误
					{
						if(esp_printf)
							esp_printf("totalXor err\n");
						memset((u8 *)&esp_data, 0, sizeof(esp_data));
						return ESP_ONESHOT_CONTINUE;
					}
					apPwdLen = esp_data[1].data;
					apSsidCrc = esp_data[2].data;
					apBssidCrc = esp_data[3].data;
					apSsidLen = totalLen - 9 - apPwdLen;
					esp_crc8_init();
					for(i=0; i<apSsidLen; i++)
					{
						esp_crc8_update(esp_data[9+apPwdLen+i].data);
						esp_param.ssid[i] = esp_data[9+apPwdLen+i].data;
					}
					if(apSsidCrc != esp_crc8_get())
					{
						if(esp_printf)
							esp_printf("apSsidCrc err\n");
						memset((u8 *)&esp_data, 0, sizeof(esp_data));
						return ESP_ONESHOT_CONTINUE;
					}
					esp_param.ssid_len = apSsidLen;
					esp_param.pwd_len = apPwdLen;
					esp_param.total_len = totalLen;
					for(i=0; i<apPwdLen; i++)
					{
						esp_param.pwd[i] = esp_data[9+i].data;
					}
					for(i=0; i<4; i++)
					{
						esp_param.ip[i] = esp_data[5+i].data;
					}
					esp_crc8_init();
					for(i=0; i<6; i++)
					{
						esp_crc8_update(esp_data[totalLen+i].data);
						esp_param.bssid[i] = esp_data[totalLen+i].data;
					}
					if(apBssidCrc != esp_crc8_get())
					{
						if(esp_printf)
							esp_printf("apBssidCrc err\n");
						memset(esp_param.bssid, 0, 6);
					}					
					return ESP_ONESHOT_COMPLETE;

				}
				else
				{
					if(esp_data[1].used && esp_data[2].used && esp_data[3].used && esp_data[4].used)
					{
						for(i=0; i<4; i++)
						{
							if(esp_data[5+i].used)
							{
								esp_param.ip[i] = esp_data[5+i].data;
							}
							else
							{
								break;
							}
						}
						if(i != 4)
						{
							return ESP_ONESHOT_CONTINUE;
						}
						totalLen = esp_data[0].data;
						apPwdLen = esp_data[1].data;
						apSsidCrc = esp_data[2].data;
						apBssidCrc = esp_data[3].data;
						apSsidLen = totalLen - 9 - apPwdLen;						
						for(i=0; i<apPwdLen; i++)
						{
							if(esp_data[9+i].used)
							{
								esp_param.pwd[i] = esp_data[9+i].data;
							}
							else
							{
								break;
							}
						}
						if(i == apPwdLen)
						{
							if(esp_printf)
								esp_printf("here2\n");
							ret = esp_ssid_bssid_crc_match(apSsidCrc, apBssidCrc, apSsidLen, esp_param.ssid,  esp_param.bssid);
							if(ret == 0)
							{
								if(esp_printf)
									esp_printf("esp_ssid_bssid_crc_match sucess\n");
								totalXor = 0;
								for(i=0; i<totalLen-apSsidLen; i++)
								{
									totalXor ^= esp_data[i].data;	
								}
								for(i=0; i<apSsidLen; i++)
								{
									totalXor ^= esp_param.ssid[i];
								}
								
								if(totalXor != 0)							//异或校验错误
								{
									if(esp_printf)
										esp_printf("totalXor err\n");
									memset((u8 *)&esp_data, 0, sizeof(esp_data));
									return ESP_ONESHOT_CONTINUE;
								}			
								if(esp_printf)
									esp_printf("esp totalXor sucess\n");
								esp_param.ssid_len = apSsidLen;
								esp_param.pwd_len = apPwdLen;
								esp_param.total_len = totalLen;
								return ESP_ONESHOT_COMPLETE;
							}
						}
					}
				}
			}
			break;
	}
	return ESP_ONESHOT_CONTINUE;			
}


void tls_esp_init(u8 *scanBss)
{
	esp_crc8_table_init();
	memset((u8 *)&esp_data[0], 0, sizeof(esp_data));
	memset(esp_head, 0, sizeof(esp_head));
	memset(esp_byte, 0, sizeof(esp_byte));
	memset(esp_src_mac, 0, 6);
	memset(&esp_param, 0, sizeof(struct esp_param_t));
	memset(esp_last_num, 0, sizeof(esp_last_num));
	esp_temp_lock = 0;
	esp_state = 0;
	esp_data_datum = 0; 
	memset(esp_head_cnt, 0, sizeof(esp_head_cnt));
	memset(esp_byte_cnt, 0, sizeof(esp_byte_cnt));
	esp_sync_cnt = 0;
	esp_crc_value = 0;
	esp_data_cnt = 0;
	memset(esp_last_seq, 0, sizeof(esp_last_seq));
	esp_scan_bss = scanBss;
	
	if(esp_printf)
		esp_printf("tls_esp_init\n");
}
