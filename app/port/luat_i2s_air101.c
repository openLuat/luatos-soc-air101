#include "luat_base.h"
#include "luat_i2s.h"

#include "wm_include.h"
#include "wm_i2s.h"
#include "wm_gpio_afsel.h"

#include "c_common.h"

#define LUAT_LOG_TAG "i2s"
#include "luat_log.h"

#define AUDIO_BUF_SIZE		(1024 * 10)

#define PERIOD_PLAY   64
static uint8_t i2s_tx_buffer[PERIOD_PLAY];

static wm_dma_handler_type dma_tx;
static Buffer_Struct dma_tx_buf;

#define CODEC_RUNING      (1 << 0)
volatile uint8_t run_status = 0;

static int I2s_tx_send(){
	uint16_t send_bytes;
	if (PERIOD_PLAY>dma_tx_buf.Pos){
		send_bytes = dma_tx_buf.Pos;
	}else{
		send_bytes = PERIOD_PLAY;
	}
	memcpy(i2s_tx_buffer, dma_tx_buf.Data, send_bytes);
	OS_BufferRemove(&dma_tx_buf, send_bytes);
    if(send_bytes == 0)
		return -1;
	DMA_SRCADDR_REG(dma_tx.channel) = (uint32_t )i2s_tx_buffer;
	DMA_CTRL_REG(dma_tx.channel) &= ~0xFFFF00;
	DMA_CTRL_REG(dma_tx.channel) |= (send_bytes<<8);
	DMA_CHNLCTRL_REG(dma_tx.channel) = DMA_CHNL_CTRL_CHNL_ON;
	wm_i2s_tx_dma_enable(1);
	wm_i2s_tx_enable(1);
	wm_i2s_enable(1);
    return 0;
}

static void I2s_dma_tx_irq(void *p){
	int ret_len = dma_tx_buf.Pos;
	if(ret_len >= 4 ){
		I2s_tx_send();
	}else{
		run_status &= ~CODEC_RUNING;
	}
}

static int I2s_tx_dma_init(void){
	if (dma_tx_buf.Data){
		return 0;
	}
    OS_InitBuffer(&dma_tx_buf, AUDIO_BUF_SIZE);
	memset(&dma_tx, 0, sizeof(wm_dma_handler_type));
	if(dma_tx.channel){
		tls_dma_free(dma_tx.channel);
	}
	dma_tx.channel = tls_dma_request(WM_I2S_TX_DMA_CHANNEL, TLS_DMA_FLAGS_CHANNEL_SEL(TLS_DMA_SEL_I2S_TX) | TLS_DMA_FLAGS_HARD_MODE);
	if(dma_tx.channel == 0){
		return -1;
	}
	if (tls_dma_stop(dma_tx.channel)){
		return -1;
	}
	tls_dma_irq_register(dma_tx.channel, I2s_dma_tx_irq, &dma_tx, TLS_DMA_IRQ_TRANSFER_DONE);
    DMA_INTMASK_REG &= ~(0x02<<(dma_tx.channel*2));
	DMA_DESTADDR_REG(dma_tx.channel) = HR_I2S_TX;
	DMA_CTRL_REG(dma_tx.channel) = DMA_CTRL_SRC_ADDR_INC | DMA_CTRL_DATA_SIZE_WORD | DMA_CTRL_BURST_SIZE1;
	DMA_MODE_REG(dma_tx.channel) = DMA_MODE_SEL_I2S_TX | DMA_MODE_HARD_MODE;
	DMA_CTRL_REG(dma_tx.channel) &= ~0xFFFF00;
	return 0;
}

static int tls_i2s_io_init(void){
    wm_i2s_ck_config(WM_IO_PB_08);
    wm_i2s_ws_config(WM_IO_PB_09);
    wm_i2s_di_config(WM_IO_PB_10);
    wm_i2s_do_config(WM_IO_PB_11);
    wm_i2s_extclk_config(WM_IO_PA_07);
    LLOGD("ck--PB08, ws--PB09, di--PB10, do--PB11, mclk--PA07");
    return WM_SUCCESS;
}

luat_i2s_conf_t i2s_conf;
int luat_i2s_setup(luat_i2s_conf_t *conf) {
    if (I2s_tx_dma_init()){
        return -1;
    }
    memcpy(&i2s_conf, conf, sizeof(luat_i2s_conf_t));
    tls_i2s_io_init();
    // 然后转本地i2s配置
    I2S_InitDef opts = { I2S_MODE_MASTER, I2S_CTRL_STEREO, I2S_RIGHT_CHANNEL, I2S_Standard, I2S_DataFormat_16, 8000, 5000000 };

    uint8_t stereo = 1;

    opts.I2S_Mode_MS = I2S_MODE_MASTER;
    opts.I2S_Mode_SS = (stereo<<22);
    opts.I2S_Mode_LR = conf->channel_format==0?I2S_LEFT_CHANNEL : I2S_RIGHT_CHANNEL;
    opts.I2S_Trans_STD = (conf->communication_format*0x1000000);
    opts.I2S_DataFormat = (conf->bits_per_sample/8 - 1)*0x10;
    opts.I2S_AudioFreq = conf->sample_rate;
    opts.I2S_MclkFreq = conf->mclk;
    wm_i2s_port_init(&opts);
    // wm_i2s_register_callback(NULL);
    run_status = 0;
    return 0;
}

int luat_i2s_send(uint8_t id, char* buff, size_t len) {
	int ret;
    if (buff == NULL) {
        return -1;
    }
	if (len == 0) {
        return 0;
    }
	OS_BufferWrite(&dma_tx_buf, buff, len);
	if ((run_status & CODEC_RUNING) == 0) {
		run_status |= CODEC_RUNING;
	    ret = I2s_tx_send();
		if(ret == -1){
		    LLOGE("fifo empty for send\n");
			run_status &= ~CODEC_RUNING;
			return 0;
		}
	}
    return len;
}

int luat_i2s_recv(uint8_t id, char* buff, size_t len) {
    wm_i2s_rx_int((int16_t *)buff, len / 2);
    return len;
}

int luat_i2s_resume(uint8_t id) {
    wm_i2s_tx_enable(1);
    return 0;
}

int luat_i2s_stop(uint8_t id) {
    wm_i2s_tx_enable(0);
    run_status = 0;	
    return 0;
}

int luat_i2s_close(uint8_t id) {
    luat_i2s_stop(id);
	if(dma_tx.channel){
		tls_dma_free(dma_tx.channel);
	}
	if (dma_tx_buf.Data){
		OS_DeInitBuffer(&dma_tx_buf);
	}
    return 0;
}

int luat_i2s_tx_stat(uint8_t id, size_t *buffsize, size_t* remain) {
	*buffsize = AUDIO_BUF_SIZE;
	*remain = dma_tx_buf.Pos;
	return 0;
}
