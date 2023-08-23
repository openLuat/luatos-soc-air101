#include "luat_base.h"
#include "luat_rtc.h"
#include "luat_msgbus.h"
#include "wm_rtc.h"
#include "time.h"

#define LUAT_LOG_TAG "rtc"
#include "luat_log.h"

int rtc_timezone = 0;

static int luat_rtc_handler(lua_State *L, void* ptr) {
    lua_getglobal(L, "sys_pub");
    if (lua_isfunction(L, -1)) {
        lua_pushstring(L, "RTC_IRQ");
        lua_call(L, 1, 0);
    }
    return 0;
}

static void luat_rtc_cb(void *arg) {
    //LLOGI("rtc irq");
    rtos_msg_t msg;
    msg.handler = luat_rtc_handler;
    luat_msgbus_put(&msg, 0);
}

int luat_rtc_set(struct tm *tblock) {
    if (tblock == NULL)
        return -1;
    // TODO 防御非法的RTC值
    tls_set_rtc(tblock);
    struct tm tt;
    tls_get_rtc(&tt);
    // printf("set %d-%d-%d %d:%d:%d\n", tblock->tm_year + 1900, tblock->tm_mon + 1, tblock->tm_mday, tblock->tm_hour, tblock->tm_min, tblock->tm_sec);
    // printf("get %d-%d-%d %d:%d:%d\n", tt.tm_year + 1900, tt.tm_mon + 1, tt.tm_mday, tt.tm_hour, tt.tm_min, tt.tm_sec);
    return 0;
}

int luat_rtc_get(struct tm *tblock) {
    if (tblock == NULL)
        return -1;
    tls_get_rtc(tblock);
    return 0;
}

int luat_rtc_timer_start(int id, struct tm *tblock) {
    if (id || tblock == NULL)
        return -1;
    tls_rtc_isr_register(luat_rtc_cb, NULL);
    tls_rtc_timer_start(tblock);
    return 0;
}

int luat_rtc_timer_stop(int id) {
    if (id)
        return -1;
    tls_rtc_timer_stop();
    return 0;
}

int luat_rtc_timezone(int* timezone) {
    if (timezone)
        rtc_timezone = *timezone;
    return rtc_timezone;
}

void luat_rtc_set_tamp32(uint32_t tamp) {
    time_t t;
    t = tamp;
    struct tm *tblock = gmtime(&t);
    luat_rtc_set(tblock);
}

time_t time(time_t* _tm)
{
  struct tm tt = {0};
  tls_get_rtc(&tt);
  time_t t = mktime(&tt);
//   printf("time %d\n", t);
  if (_tm != NULL)
    memcpy(_tm, &t, sizeof(time_t));
  return t;
}

clock_t clock(void)
{
  return (clock_t)time(NULL);
}

//---------------------------------------
#include "c_common.h"
void RTC_GetDateTime(Date_UserDataStruct *Date, Time_UserDataStruct *Time);

time_t __wrap_mktime(struct tm* tt) {
    Time_UserDataStruct Time = {0};
	Date_UserDataStruct Date = {0};

    Time.Hour = tt->tm_hour;
    Time.Min = tt->tm_min;
    Time.Sec = tt->tm_sec;
    Time.Week = tt->tm_wday;

    Date.Day = tt->tm_mday;
    Date.Mon = tt->tm_mon + 1;
    Date.Year = tt->tm_year + 1900;
    // printf("????\n");
    uint64_t t = UTC2Tamp(&Date, &Time);
    // printf("????2\n");
    time_t tmp = (time_t) t & 0xFFFFFFFF;
    return tmp;
}

void RTC_GetDateTime(Date_UserDataStruct *Date, Time_UserDataStruct *Time) {
    struct tm tt = {0};
    tls_get_rtc(&tt);
    Time->Hour = tt.tm_hour;
    Time->Min = tt.tm_min;
    Time->Sec = tt.tm_sec;
    Time->Week = tt.tm_wday;

    Date->Day = tt.tm_mday;
    Date->Mon = tt.tm_mon + 1;
    Date->Year = tt.tm_year + 1900;
}

static struct tm prvTM;
extern const uint32_t DayTable[2][12];

struct tm *__wrap_localtime (const time_t *_timer)
{
	Time_UserDataStruct Time;
	Date_UserDataStruct Date;
	uint64_t Sec;
	if (_timer)
	{
		Sec = *_timer;
		Tamp2UTC(Sec, &Date, &Time, 0);
	}
	else
	{
		RTC_GetDateTime(&Date, &Time);
	}
	prvTM.tm_year = Date.Year - 1900;
	prvTM.tm_mon = Date.Mon - 1;
	prvTM.tm_mday = Date.Day;
	prvTM.tm_hour = Time.Hour;
	prvTM.tm_min = Time.Min;
	prvTM.tm_sec = Time.Sec;
	prvTM.tm_wday = Time.Week;
	prvTM.tm_yday = Date.Day - 1;
	prvTM.tm_yday += DayTable[IsLeapYear(Date.Year)][Date.Mon - 1];
	return &prvTM;
}

struct tm *__wrap_gmtime (const time_t *_timer)
{
	Time_UserDataStruct Time;
	Date_UserDataStruct Date;
	uint64_t Sec;
	if (_timer)
	{
		Sec = *_timer;
		Tamp2UTC(Sec, &Date, &Time, 0);
	}
	else
	{
		RTC_GetDateTime(&Date, &Time);
	}
	prvTM.tm_year = Date.Year - 1900;
	prvTM.tm_mon = Date.Mon - 1;
	prvTM.tm_mday = Date.Day;
	prvTM.tm_hour = Time.Hour;
	prvTM.tm_min = Time.Min;
	prvTM.tm_sec = Time.Sec;
	prvTM.tm_wday = Time.Week;
	prvTM.tm_yday = Date.Day - 1;
	prvTM.tm_yday += DayTable[IsLeapYear(Date.Year)][Date.Mon - 1];
	return &prvTM;
}

