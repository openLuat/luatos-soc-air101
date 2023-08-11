#ifndef _LUAT_AUDIOAIR101_H_
#define _LUAT_AUDIOAIR101_H_

struct audio_codec_opts;

typedef struct audio_codec_conf {
    int samplerate;         //16k
    int bits;               //16
    int channels;           //1ch/2ch
    int pa_pin;
	uint8_t vol;
	uint8_t pa_on_level;
    uint32_t dummy_time_len;
    uint32_t pa_delay_time;
    const struct audio_codec_opts* codec_opts;
} audio_codec_conf_t;

typedef struct audio_codec_opts{
    const char* name;
    int (*init)(audio_codec_conf_t* conf);
    int (*deinit)(audio_codec_conf_t* conf);
    int (*control)(audio_codec_conf_t* conf,uint8_t cmd,int data);
    int (*start)(audio_codec_conf_t* conf);
    int (*stop)(audio_codec_conf_t* conf);
} audio_codec_opts_t;

#define CODEC_MODE_MASTER               0x00
#define CODEC_MODE_SLAVE                0x01

#define CODEC_PA_OFF                    0x00
#define CODEC_PA_ON                     0x01

#define CODEC_CTL_MODE                  0x00
#define CODEC_CTL_VOLUME                0x01
#define CODEC_CTL_RATE                  0x02
#define CODEC_CTL_BITS                  0x03
#define CODEC_CTL_CHANNEL               0x04
#define CODEC_CTL_PA                    0x05

#endif


