#include "luat_base.h"
#include "luat_i2s.h"
#include "luat_malloc.h"

#include "wm_include.h"
#include "wm_i2s.h"
#include "wm_gpio_afsel.h"

#include "c_common.h"

// Function declared in SDK source but not in header
extern int wm_i2s_tranceive_dma(uint32_t i2s_mode, wm_dma_handler_type *hdma_tx, wm_dma_handler_type *hdma_rx, uint16_t *data_tx, uint16_t *data_rx, uint16_t len);

#define LUAT_LOG_TAG "i2s"
#include "luat_log.h"

static luat_i2s_conf_t prv_i2s;
static luat_i2s_save_conf_t prv_i2s_param;

// DMA handlers for async operations
static wm_dma_handler_type prv_dma_tx;
static wm_dma_handler_type prv_dma_rx;

// Full duplex mode flag
static uint8_t prv_full_duplex = 0;

// Busy state for async DMA operations (similar to ec7xx USP)
static uint8_t prv_is_busy = 0;

// RX buffer for full duplex
static uint8_t *prv_rx_buf = NULL;
static size_t prv_rx_len = 0;

#ifdef __LUATOS__
#include "luat_msgbus.h"

int l_i2s_play(lua_State *L) {
    return -1;
}

int l_i2s_pause(lua_State *L) {
    return 0;
}

int l_i2s_stop(lua_State *L) {
	luat_i2s_close(luaL_checkinteger(L, 1));
	return 0;
}
#endif

// DMA TX completion callback - called from ISR context
// Place in RAM section for faster ISR execution
// NOTE: Do NOT call luat_heap_free here - it's not ISR-safe (vPortFree is not ISR-safe)
static void __attribute__((section(".ram_run"))) prv_dma_tx_cplt_cb(wm_dma_handler_type *hdma) {
	prv_is_busy = 0;  // Clear busy state on completion
	if (prv_i2s.luat_i2s_event_callback) {
		prv_i2s.luat_i2s_event_callback(0,
			prv_full_duplex ? LUAT_I2S_EVENT_TRANSFER_DONE : LUAT_I2S_EVENT_TX_DONE,
			NULL, 0, prv_i2s.userdata);
	}
	// In full duplex mode, signal RX data availability
	// RX buffer will be freed by next transfer or close (in task context)
	if (prv_full_duplex && prv_rx_buf && prv_i2s.luat_i2s_event_callback) {
		prv_i2s.luat_i2s_event_callback(0, LUAT_I2S_EVENT_RX_DONE,
			prv_rx_buf, prv_rx_len, prv_i2s.userdata);
		// Mark buffer as "pending free" - actual free happens in task context
		// when next transfer is called or close is called
	}
}

// DMA RX completion callback - place in RAM section
static void __attribute__((section(".ram_run"))) prv_dma_rx_cplt_cb(wm_dma_handler_type *hdma) {
	prv_is_busy = 0;  // Clear busy state on completion
	if (prv_i2s.luat_i2s_event_callback) {
		// Note: actual RX data should be retrieved from wm_i2s_buf
		prv_i2s.luat_i2s_event_callback(0, LUAT_I2S_EVENT_RX_DONE,
			NULL, 0, prv_i2s.userdata);
	}
}

static int tls_i2s_io_init(void) {
	wm_i2s_ck_config(WM_IO_PB_08);
	wm_i2s_ws_config(WM_IO_PB_09);
	wm_i2s_di_config(WM_IO_PB_10);
	wm_i2s_do_config(WM_IO_PB_11);
	wm_i2s_mclk_config(WM_IO_PA_00);
	LLOGD("ck--PB08, ws--PB09, di--PB10, do--PB11, mclk--PA00");
	return WM_SUCCESS;
}

luat_i2s_conf_t *luat_i2s_get_config(uint8_t id) {
	if (id != 0) return NULL;
	return &prv_i2s;
}

int luat_i2s_setup(const luat_i2s_conf_t *conf) {
	if (conf->id != 0) return -1;

	memcpy(&prv_i2s, conf, sizeof(luat_i2s_conf_t));
	if (!prv_i2s.cb_rx_len) prv_i2s.cb_rx_len = 4000;

	tls_i2s_io_init();

	// Convert to SDK I2S config
	I2S_InitDef opts = {
		I2S_MODE_MASTER, I2S_CTRL_STEREO, I2S_RIGHT_CHANNEL,
		I2S_Standard, I2S_DataFormat_16, 8000, 5000000
	};

	opts.I2S_Mode_MS = conf->mode;
	opts.I2S_Mode_SS = conf->channel_format == LUAT_I2S_CHANNEL_STEREO ? I2S_CTRL_STEREO : I2S_CTRL_MONO;
	opts.I2S_Mode_LR = conf->channel_format == 0 ? I2S_LEFT_CHANNEL : I2S_RIGHT_CHANNEL;
	opts.I2S_Trans_STD = (conf->standard * 0x1000000);
	opts.I2S_DataFormat = (conf->data_bits / 8 - 1) * 0x10;
	opts.I2S_AudioFreq = conf->sample_rate;
	opts.I2S_MclkFreq = 2 * 256 * conf->sample_rate;

	wm_i2s_port_init(&opts);
	prv_i2s.state = LUAT_I2S_STATE_STOP;

	// Setup DMA handlers
	memset(&prv_dma_tx, 0, sizeof(wm_dma_handler_type));
	memset(&prv_dma_rx, 0, sizeof(wm_dma_handler_type));
	prv_dma_tx.XferCpltCallback = prv_dma_tx_cplt_cb;
	prv_dma_rx.XferCpltCallback = prv_dma_rx_cplt_cb;

	LLOGI("setup: mode=%d, stereo=%d, channel=%d, fmt=%d, bits=%d, rate=%u, mclk=%u, full_duplex=%d",
		opts.I2S_Mode_MS, opts.I2S_Mode_SS, opts.I2S_Mode_LR,
		opts.I2S_Trans_STD >> 24, opts.I2S_DataFormat,
		opts.I2S_AudioFreq, opts.I2S_MclkFreq, conf->is_full_duplex);
	return 0;
}

int luat_i2s_set_user_data(uint8_t id, void *user_data) {
	if (id != 0) return -1;
	prv_i2s.userdata = user_data;
	return 0;
}

int luat_i2s_modify(uint8_t id, uint8_t channel_format, uint8_t data_bits, uint32_t sample_rate) {
	if (id != 0) return -1;

	if (!sample_rate) {
		if (I2S->CTRL & 0x1) {
			wm_i2s_tx_rx_stop();
		}
		prv_i2s.state = LUAT_I2S_STATE_STOP;
		LLOGD("i2s stop");
		return 0;
	}

	// Update config
	prv_i2s.channel_format = channel_format;
	prv_i2s.data_bits = data_bits;
	prv_i2s.sample_rate = sample_rate;

	wm_i2s_mono_select(channel_format == LUAT_I2S_CHANNEL_STEREO ? I2S_CTRL_STEREO : I2S_CTRL_MONO);
	wm_i2s_left_channel_sel(channel_format == 0 ? I2S_LEFT_CHANNEL : I2S_RIGHT_CHANNEL);
	wm_i2s_set_word_len((data_bits / 8 - 1) * 0x10);
	wm_i2s_set_freq(sample_rate, 2 * 256 * sample_rate);

	LLOGD("modify: rate=%u, mclk=%u, stereo=%d, bits=%d",
		sample_rate, 2 * 256 * sample_rate,
		channel_format == LUAT_I2S_CHANNEL_STEREO ? 1 : 0, data_bits);

	prv_i2s.state = LUAT_I2S_STATE_RUNING;
	return 0;
}

// Auto-start I2S if not running
static void luat_i2s_check_start(void) {
	if (!prv_i2s.state) {
		LLOGD("i2s auto start: rate=%u, format=%d, full_duplex=%d",
			prv_i2s.sample_rate, prv_i2s.channel_format, prv_i2s.is_full_duplex);
		wm_i2s_enable(1);
		prv_i2s.state = LUAT_I2S_STATE_RUNING;
	}
}

int luat_i2s_send(uint8_t id, uint8_t *buff, size_t len) {
	if (id != 0) return -1;
	if (buff == NULL || len == 0) return -1;
	if (prv_is_busy) {
		LLOGW("i2s send: busy, previous transfer not completed");
		return -1;
	}

	luat_i2s_check_start();
	prv_full_duplex = 0;
	prv_is_busy = 1;  // Set busy state before starting DMA

	LLOGD("send: len=%d, data=%p", len / 2, buff);

	// Use async DMA transmit
	int ret = wm_i2s_transmit_dma(&prv_dma_tx, (uint16_t *)buff, len / 2);
	if (ret != WM_SUCCESS) {
		LLOGE("send failed: ret=%d, len=%d", ret, len / 2);
		prv_is_busy = 0;  // Clear busy on error
		return -1;
	}

	return len;
}

int luat_i2s_recv(uint8_t id, uint8_t *buff, size_t len) {
	if (id != 0) return -1;
	if (buff == NULL || len == 0) return -1;
	if (prv_is_busy) {
		LLOGW("i2s recv: busy, previous transfer not completed");
		return -1;
	}

	if (prv_i2s.is_full_duplex) {
		// Full duplex mode should use transfer, not recv
		LLOGW("full duplex mode, use transfer instead of recv");
		return -1;
	}

	luat_i2s_check_start();
	prv_full_duplex = 0;
	prv_is_busy = 1;  // Set busy state before starting DMA

	LLOGD("recv: len=%d, data=%p", len / 2, buff);

	// Use async DMA receive
	int ret = wm_i2s_receive_dma(&prv_dma_rx, (uint16_t *)buff, len / 2);
	if (ret != WM_SUCCESS) {
		LLOGE("recv failed: ret=%d, len=%d", ret, len / 2);
		prv_is_busy = 0;  // Clear busy on error
		return -1;
	}

	return len;
}

int luat_i2s_transfer(uint8_t id, uint8_t *txbuff, size_t len) {
	if (id != 0) return -1;
	if (txbuff == NULL || len == 0) return -1;
	if (prv_is_busy) {
		LLOGW("i2s transfer: busy, previous transfer not completed");
		return -1;
	}

	if (!prv_i2s.is_full_duplex) {
		// Non-full-duplex mode: fallback to send
		return luat_i2s_send(id, txbuff, len);
	}

	luat_i2s_check_start();
	prv_full_duplex = 1;
	prv_is_busy = 1;  // Set busy state before starting DMA

	// Allocate RX buffer for full duplex using LuatOS heap
	if (prv_rx_buf) {
		luat_heap_free(prv_rx_buf);
	}
	prv_rx_buf = (uint8_t *)luat_heap_malloc(len);
	if (prv_rx_buf == NULL) {
		LLOGE("i2s transfer: rx buffer alloc failed");
		prv_is_busy = 0;  // Clear busy on error
		return -1;
	}
	prv_rx_len = len;

	// Determine I2S mode (master or slave)
	uint32_t i2s_mode = (prv_i2s.mode == LUAT_I2S_MODE_SLAVE) ? I2S_MODE_SLAVE : I2S_MODE_MASTER;

	LLOGD("transfer: mode=%d, len=%d, tx=%p, rx=%p", i2s_mode, len / 2, txbuff, prv_rx_buf);

	// Use async tranceive DMA
	int ret = wm_i2s_tranceive_dma(i2s_mode,
		&prv_dma_tx, &prv_dma_rx,
		(uint16_t *)txbuff, (uint16_t *)prv_rx_buf, len / 2);

	if (ret != WM_SUCCESS) {
		LLOGE("transfer failed: ret=%d, len=%d", ret, len / 2);
		luat_heap_free(prv_rx_buf);
		prv_rx_buf = NULL;
		prv_is_busy = 0;  // Clear busy on error
		return -1;
	}

	return len;
}

int luat_i2s_transfer_loop(uint8_t id, uint8_t *buff, uint32_t one_truck_byte_len, uint32_t total_trunk_cnt, uint8_t need_callback) {
	if (id != 0) return -1;
	if (buff == NULL || one_truck_byte_len == 0 || total_trunk_cnt == 0) return -1;

	// xt804 SDK supports DMA chain mode for loop transfer
	// But this requires complex buffer management
	// Return not supported for now, matching EC7xx behavior
	LLOGW("xt804 i2s loop transfer not implemented");
	return -1;
}

int luat_i2s_pause(uint8_t id) {
	if (id != 0) return -1;
	wm_i2s_tx_enable(0);
	wm_i2s_rx_enable(0);
	prv_i2s.state = LUAT_I2S_STATE_STOP;
	prv_is_busy = 0;  // Clear busy state on pause
	return 0;
}

int luat_i2s_resume(uint8_t id) {
	if (id != 0) return -1;
	wm_i2s_tx_enable(1);
	wm_i2s_rx_enable(1);
	wm_i2s_enable(1);
	prv_i2s.state = LUAT_I2S_STATE_RUNING;
	return 0;
}

int luat_i2s_close(uint8_t id) {
	if (id != 0) return -1;

	wm_i2s_tx_rx_stop();
	prv_i2s.state = LUAT_I2S_STATE_STOP;
	prv_full_duplex = 0;
	prv_is_busy = 0;  // Clear busy state on close

	if (prv_rx_buf) {
		luat_heap_free(prv_rx_buf);
		prv_rx_buf = NULL;
	}

	LLOGD("i2s closed");
	return 0;
}

int luat_i2s_save_old_config(uint8_t id) {
	if (id != 0) return -1;
	if (prv_i2s_param.is_saved) {
		LLOGW("i2s old param already saved");
		return -2;
	}

	prv_i2s_param.sample_rate = prv_i2s.sample_rate;
	prv_i2s_param.cb_rx_len = prv_i2s.cb_rx_len;
	prv_i2s_param.luat_i2s_event_callback = prv_i2s.luat_i2s_event_callback;
	prv_i2s_param.is_full_duplex = prv_i2s.is_full_duplex;
	prv_i2s_param.is_saved = 1;

	LLOGD("i2s config saved");
	return 0;
}

int luat_i2s_load_old_config(uint8_t id) {
	if (id != 0) return -1;
	if (!prv_i2s_param.is_saved) {
		LLOGW("i2s old param not saved");
		return -2;
	}

	prv_i2s.sample_rate = prv_i2s_param.sample_rate;
	prv_i2s.cb_rx_len = prv_i2s_param.cb_rx_len;
	prv_i2s.luat_i2s_event_callback = prv_i2s_param.luat_i2s_event_callback;
	prv_i2s.is_full_duplex = prv_i2s_param.is_full_duplex;
	prv_i2s_param.is_saved = 0;

	LLOGD("i2s config loaded");
	return 0;
}

int luat_i2s_txbuff_info(uint8_t id, size_t *buffsize, size_t *remain) {
	// xt804 SDK doesn't expose internal buffer info
	return -1;
}

int luat_i2s_rxbuff_info(uint8_t id, size_t *buffsize, size_t *remain) {
	// xt804 SDK doesn't expose internal buffer info
	return -1;
}

uint8_t luat_i2s_is_busy(uint8_t id) {
	if (id != 0) return 0;
	return prv_is_busy;
}