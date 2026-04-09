
#include "luat_base.h"
#include "luat_log.h"
#include "luat_uart.h"
#include "printf.h"
#include "luat_mem.h"
#include "luat_mcu.h"
#ifdef LUAT_USE_DBG
#include "luat_cmux.h"
extern luat_cmux_t cmux_ctx;
#endif

#include "wm_uart.h"

#ifdef LUAT_CONF_LOG_UART1
#else

static uint8_t luat_log_uart_port = 0;
static uint8_t luat_log_level_cur = LUAT_LOG_DEBUG;

extern int sendchar(int ch);

#define LOGLOG_SIZE 1024
// static char log_printf_buff[LOGLOG_SIZE]  = {0};


void soc_log_init(void);
void soc_debug_out(char *string, uint32_t size);
// void soc_info(const char *fmt, ...);
void soc_vsprintf(const char *fmt, va_list ap);
void am_make_log(int level, const char *tag, const char *fmt, va_list ap, uint8_t add_end, uint32_t out_len);

void luat_log_set_uart_port(int port) {
    luat_log_uart_port = port;
}

uint8_t luat_log_get_uart_port(void) {
    return luat_log_uart_port;
}

void luat_nprint(char *s, size_t l) {
    soc_debug_out(s, l);
}

void luat_log_write(char *s, size_t l) {
    soc_debug_out(s, l);
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
    #if 0
    // char log_printf_buff[LOGLOG_SIZE]  = {0};
    char header[128] = {0};
    uint64_t time_ms = luat_mcu_tick64_ms();
    char *tmp = (char *)luat_heap_opt_malloc(LUAT_HEAP_SRAM, LOGLOG_SIZE);
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
    #else
    va_list args;
    va_start(args, _fmt);
    soc_vsprintf(_fmt, args);
    va_end(args);
    #endif
}
void luat_log_printf(int level, const char* _fmt, ...) {
    if (luat_log_level_cur > level) return;
    #if 0
    va_list args;
    char *tmp = (char *)luat_heap_opt_malloc(LUAT_HEAP_SRAM, LOGLOG_SIZE);
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
    #else
    va_list args;
    va_start(args, _fmt);
    soc_vsprintf(_fmt, args);
    va_end(args);
    #endif
}
#endif

void soc_log_to_device(uint8_t *data, uint32_t len) {
    #if 1
    // tls_uart_write(0, (char *)data, (u16)len);
    // printf("%.*s", (int)len, data);
    
    for (uint32_t i = 0; i < len; i++) {
        sendchar(data[i]);
    }
    #else
    printf("dump %p %d\n", data, len);
    // 按128字节一行输出, 方便查看, 用buff合成, 然后再输出
    char buf[512];
    uint32_t buf_index = 0;
    for (uint32_t i = 0; i < len; i++) {
        buf_index += snprintf(buf + buf_index, sizeof(buf) - buf_index, "%02X", data[i]);
        if ((i % 128) == 127) {
            printf("%s\n", buf);
            buf_index = 0;
        }
    }
    if (buf_index > 0) {
        printf("%s\n", buf);
    }
    #endif
}

int	__wrap_printf (const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	am_make_log(0, "CAPP", fmt, args, 1, 0);
	va_end(args);
	return 1;
}
