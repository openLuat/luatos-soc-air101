#include "luat_base.h"
#include "luat_audio.h"
#include "luat_i2s.h"
#include "wm_include.h"
#include "luat_audio_air101.h"
#include "es8311.h"

extern volatile uint8_t run_status;

audio_codec_conf_t audio_hardware = {
    .pa_pin = -1
};

int luat_audio_play_multi_files(uint8_t multimedia_id, uData_t *info, uint32_t files_num, uint8_t error_stop){

}

int luat_audio_play_file(uint8_t multimedia_id, const char *path){

}

uint8_t luat_audio_is_finish(uint8_t multimedia_id){
    return run_status;
}

int luat_audio_play_stop(uint8_t multimedia_id){
    luat_i2s_close(0);
}

int luat_audio_play_get_last_error(uint8_t multimedia_id){

}

int luat_audio_start_raw(uint8_t multimedia_id, uint8_t audio_format, uint8_t num_channels, uint32_t sample_rate, uint8_t bits_per_sample, uint8_t is_signed){
    if(sample_rate == 8000 && bits_per_sample == 8){
        wm_printf("not support 8K 8Bit record!\n");
        return -1;
    }
	
    luat_i2s_conf_t conf = {
        .channel_format = 0,
        .communication_format = 0,
        .bits_per_sample = bits_per_sample,
        .sample_rate = sample_rate,
        .mclk = 8000000
    };
    luat_i2s_setup(&conf);

	audio_hardware.codec_opts->control(&audio_hardware,CODEC_CTL_RATE,sample_rate);
	audio_hardware.codec_opts->control(&audio_hardware,CODEC_CTL_MODE,CODEC_MODE_SLAVE);
	audio_hardware.codec_opts->control(&audio_hardware,CODEC_CTL_PA,CODEC_PA_ON);

}

int luat_audio_write_raw(uint8_t multimedia_id, uint8_t *data, uint32_t len){
    int send_bytes = 0;
    while (send_bytes < len) {
        int length = luat_i2s_send(0,data + send_bytes, len - send_bytes);
        if (length > 0) {
            send_bytes += length;
        }
        vTaskDelay(1);
    }
    return 0;
}

int luat_audio_stop_raw(uint8_t multimedia_id){
    luat_i2s_close(0);
}

int luat_audio_pause_raw(uint8_t multimedia_id, uint8_t is_pause){
    if (is_pause){
        audio_hardware.codec_opts->control(&audio_hardware,CODEC_CTL_PA,CODEC_PA_OFF);
        luat_i2s_stop(0);
    }else{
        audio_hardware.codec_opts->control(&audio_hardware,CODEC_CTL_PA,CODEC_PA_ON);
        luat_i2s_resume(0);
    }
}

void luat_audio_config_pa(uint8_t multimedia_id, uint32_t pin, int level, uint32_t dummy_time_len, uint32_t pa_delay_time){
	if (pin <= WM_IO_PB_31){
		audio_hardware.pa_pin = pin;
		audio_hardware.pa_on_level = level;
        tls_gpio_cfg(pin, !level, WM_GPIO_ATTR_FLOATING);
        tls_gpio_write(pin, !level);
	}else{
		audio_hardware.pa_pin = -1;
	}
    audio_hardware.dummy_time_len = dummy_time_len;
	audio_hardware.pa_delay_time = pa_delay_time;
}

void luat_audio_config_dac(uint8_t multimedia_id, int pin, int level, uint32_t dac_off_delay_time){}

uint16_t luat_audio_vol(uint8_t multimedia_id, uint16_t vol){
    if(vol < 0 || vol > 100){
		return -1;
    }
	audio_hardware.vol = vol;
    audio_hardware.codec_opts->control(&audio_hardware,CODEC_CTL_VOLUME,vol);
	return audio_hardware.vol;
}

void luat_audio_set_bus_type(uint8_t bus_type){
    if (bus_type == 1){
        audio_hardware.codec_opts = &codec_opts_es8311;
        audio_hardware.codec_opts->init(&audio_hardware);
        audio_hardware.codec_opts->control(&audio_hardware,CODEC_CTL_MODE,CODEC_MODE_SLAVE);
    }
}


