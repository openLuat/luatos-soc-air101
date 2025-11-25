
#include "luat_base.h"
#include "luat_log.h"
#include "luat_uart.h"
#include "printf.h"
#include "luat_malloc.h"
#include "luat_mcu.h"
#ifdef LUAT_USE_DBG
#include "luat_cmux.h"
extern luat_cmux_t cmux_ctx;
#endif

#ifdef LUAT_CONF_LOG_UART1
#else

static uint8_t luat_log_uart_port = 0;
static uint8_t luat_log_level_cur = LUAT_LOG_DEBUG;

#define LOGLOG_SIZE 1024
// static char log_printf_buff[LOGLOG_SIZE]  = {0};

void luat_log_set_uart_port(int port) {
    luat_log_uart_port = port;
}

uint8_t luat_log_get_uart_port(void) {
    return luat_log_uart_port;
}

void luat_nprint(char *s, size_t l) {
#ifdef LUAT_USE_SHELL
    if (cmux_ctx.state == 1 && cmux_ctx.log_state ==1){
        luat_cmux_write(LUAT_CMUX_CH_LOG,  CMUX_FRAME_UIH & ~ CMUX_CONTROL_PF,s, l);
    }else
#endif
    // luat_uart_write(luat_log_uart_port, s, l);
    printf("%.*s", l, s);
}

void luat_log_write(char *s, size_t l) {
#ifdef LUAT_USE_SHELL
    if (cmux_ctx.state == 1 && cmux_ctx.log_state ==1){
        luat_cmux_write(LUAT_CMUX_CH_LOG,  CMUX_FRAME_UIH & ~ CMUX_CONTROL_PF,s, l);
    }else
#endif
    // luat_uart_write(luat_log_uart_port, s, l);
    printf("%.*s", l, s);
}

void luat_log_set_level(int level) {
    luat_log_level_cur = level;
}


int luat_log_get_level() {
    return luat_log_level_cur;
}

static const char lstr[] = {'D', 'I', 'W', 'E'};

void luat_log_log(int level, const char* tag, const char* _fmt, ...) {
    if (luat_log_level_cur > level) return;
    // char log_printf_buff[LOGLOG_SIZE]  = {0};
    char header[128] = {0};
    uint64_t time_ms = luat_mcu_tick64_ms();
    char *tmp = (char *)luat_heap_malloc(LOGLOG_SIZE);
    if (tmp == NULL) {
        return;
    }
    if (level > LUAT_LOG_ERROR) {
        level = LUAT_LOG_ERROR;
    }
    else if (level < LUAT_LOG_DEBUG) {
        level = LUAT_LOG_DEBUG;
    }
    snprintf(header, sizeof(header), "[%08llu.%03llu]%c/%s ", time_ms / 1000, time_ms % 1000, lstr[level - 1], tag);
    luat_log_write(header, strlen(header));

    va_list args;
    va_start(args, _fmt);
    int len = vsnprintf_(tmp, LOGLOG_SIZE - 2, _fmt, args);
    va_end(args);
    if (len > 0) {
        tmp[len] = '\n';
        luat_log_write(tmp, len+1);
    }
    luat_heap_free(tmp);
}
void luat_log_printf(int level, const char* _fmt, ...) {
    va_list args;
    if (luat_log_level_cur > level) return;
    char *tmp = (char *)luat_heap_malloc(LOGLOG_SIZE);
    if (tmp == NULL) {
        return;
    }
    va_start(args, _fmt);
    int len = vsnprintf_(tmp, LOGLOG_SIZE - 2, _fmt, args);
    va_end(args);
    if (len > 0) {
        tmp[len] = '\n';
        luat_log_write(tmp, len);
    }
    luat_heap_free(tmp);
}
#endif
