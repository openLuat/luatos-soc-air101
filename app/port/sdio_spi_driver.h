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

#define LCD_ASYNC_SEND  0  // 0 : sync    1 : async

void init_sdio_spi_mode(u32 clk);

void sdio_spi_put(u8 d);

void write_sdio_spi_dma(u32* data,u32 len);

void write_sdio_spi_dma_async(u32* data,u32 len);

void sdio_dma_sem_acquire();

void sdio_spi_dma_write_async(u32* data, u32 len);

void wait_for_prev_frame_completion();

void wait_sdio_prev_data_completion();

void lcd_task_create(void* arg);

int lcd_dma_queue_write(u16 x_start, u16 y_start, u16 x_end, u16 y_end, u32* data, u32 len);

#endif