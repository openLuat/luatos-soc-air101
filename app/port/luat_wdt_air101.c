
#include "luat_base.h"
#include "luat_wdt.h"

#include "wm_include.h"
#include "wm_watchdog.h"

int luat_wdt_init(size_t timeout) {
    if (timeout < 1000)
        timeout = 1000;
    tls_watchdog_init(timeout * 1000);
    return 0;
}

int luat_wdt_set_timeout(size_t timeout) {
    return luat_wdt_init(timeout);
}

int luat_wdt_feed(void) {
    tls_watchdog_clr();
    return 0;
}

int luat_wdt_close(void) {
    tls_watchdog_deinit();
    return 0;
}
