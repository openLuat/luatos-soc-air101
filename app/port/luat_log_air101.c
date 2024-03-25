
#include "luat_base.h"
#include "luat_log.h"
#include "luat_uart.h"
#include "printf.h"
#include "luat_malloc.h"
#ifdef LUAT_USE_DBG
#include "luat_cmux.h"
extern luat_cmux_t cmux_ctx;
#endif

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



void luat_log_log(int level, const char* tag, const char* _fmt, ...) {
    if (luat_log_level_cur > level) return;
    // char log_printf_buff[LOGLOG_SIZE]  = {0};
    char *tmp = (char *)luat_heap_malloc(LOGLOG_SIZE);
    if (tmp == NULL) {
        return;
    }
    switch (level)
        {
        case LUAT_LOG_DEBUG:
            luat_log_write("D/", 2);
            break;
        case LUAT_LOG_INFO:
            luat_log_write("I/", 2);
            break;
        case LUAT_LOG_WARN:
            luat_log_write("W/", 2);
            break;
        case LUAT_LOG_ERROR:
            luat_log_write("E/", 2);
            break;
        default:
            luat_log_write("D/", 2);
            break;
        }
    luat_log_write((char*)tag, strlen(tag));
    luat_log_write(" ", 1);

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
