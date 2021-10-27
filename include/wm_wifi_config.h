/**
 * @file    wm_wifi_config.h
 *
 * @brief   WM wifi model configure
 *
 * @author  winnermicro
 *
 * Copyright (c) 2015 Winner Microelectronics Co., Ltd.
 */
#ifndef __WM_WIFI_CONFIG_H__
#define __WM_WIFI_CONFIG_H__

#define	CFG_WIFI_ON								1
#define CFG_WIFI_OFF								0

/*******************WIFI INFO**************************
  			Below Switch Only for Reference!!!
********************************************************/
#define  TLS_CONFIG_AP        				CFG_WIFI_OFF
#define  TLS_CONFIG_11N                     CFG_WIFI_OFF

#define  TLS_CONFIG_USE_VENDOR_IE           CFG_WIFI_OFF

#define  TLS_CONFIG_SOFTAP_11N              (CFG_WIFI_OFF&& TLS_CONFIG_AP)

#define  LINK_RETRY_METHOD_MACRO            CFG_WIFI_OFF

#define  TLS_CONFIG_AP_BEACON_SOFT         (CFG_OFF && TLS_CONFIG_AP)
#define  TLS_CONFIG_AP_OPT_PS              (CFG_WIFI_OFF && TLS_CONFIG_AP)/* SOFTAP POWER SAVE */
#define  TLS_CONFIG_AP_OPT_FWD             (CFG_WIFI_OFF && TLS_CONFIG_AP)/* IP PACKET FORWARD */

#define  TLS_CONFIG_WPS       				CFG_WIFI_OFF /* WPS&EAPOL should be enabled together */
#define  TLS_IEEE8021X_EAPOL   				CFG_WIFI_OFF

#define  TLS_CONFIG_1SSID_MULTI_PWD          CFG_WIFI_OFF

#define TLS_CONFIG_FAST_JOIN_NET			CFG_WIFI_OFF

#define TLS_CONFIG_LOG_PRINT			    CFG_WIFI_OFF

#define TLS_PS_MODE_NEW_FTR                CFG_WIFI_OFF

#endif /*__WM_WIFI_CONFIG_H__*/

