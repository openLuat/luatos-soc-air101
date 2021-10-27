/**
 * @file    wm_ram_config.h
 *
 * @brief   WM ram model configure
 *
 * @author  winnermicro
 *
 * Copyright (c) 2015 Winner Microelectronics Co., Ltd.
 */
#ifndef __WM_RAM_CONFIG_H__
#define __WM_RAM_CONFIG_H__
#include "wm_config.h"

extern unsigned int __heap_end;
extern unsigned int __heap_start;

/*High speed SPI or SDIO buffer to exchange data*/
#define SLAVE_HSPI_SDIO_ADDR        ((unsigned int)(&__heap_end))

#if TLS_CONFIG_HS_SPI
#define SLAVE_HSPI_MAX_SIZE         (0x2000)
#else
#define SLAVE_HSPI_MAX_SIZE         (0x0)
#endif

/*Wi-Fi use buffer to exchange data*/
#define WIFI_MEM_START_ADDR		(SLAVE_HSPI_SDIO_ADDR + SLAVE_HSPI_MAX_SIZE)


#endif /*__WM_RAM_CONFIG_H__*/

