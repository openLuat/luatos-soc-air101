#ifndef __SDIO_SPI_DRIVER_H__
#define __SDIO_SPI_DRIVER_H__
#include "wm_include.h"
/******************************************
*   SDIO	    W800	W801	TFT
*   sdio_ck	    PB_06	PA_09	CLK
*   sdio_cmd	PB_07	PA_10	MOSI
*******************************************/

#define LCD_SCL        	WM_IO_PA_09 //--->>TFT --SCL
#define LCD_MOSI        WM_IO_PA_10 //--->>TFT --DO
#define LCD_TE          WM_IO_PB_29	//--->>TFT --TE

void init_sdio_spi_mode(u32 clk);

void sdio_spi_put(u8 d);

void write_sdio_spi_dma(u32* data,u32 len);

void write_sdio_spi_dma_async(u32* data,u32 len);

void wait_sdio_spi_dma_ready();

void lcd_task_create(void);

#endif