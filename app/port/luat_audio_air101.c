#include "luat_base.h"
#include "luat_audio.h"
#include "luat_i2s.h"
#include "wm_include.h"

#include "FreeRTOS.h"
#include "task.h"

#define LUAT_LOG_TAG "audio"
#include "luat_log.h"

luat_audio_conf_t audio_hardware = {0};

luat_audio_conf_t *luat_audio_get_config(uint8_t multimedia_id){
    if (multimedia_id == 0) return &audio_hardware;
    else return NULL;
}
