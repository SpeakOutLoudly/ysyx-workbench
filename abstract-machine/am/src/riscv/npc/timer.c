#include <am.h>

void __am_timer_init() {
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  volatile uint32_t *const timer_lo = (volatile uint32_t *)0x20000000u;
  volatile uint32_t *const timer_hi = (volatile uint32_t *)0x20000004u;
  uint32_t hi_before, lo, hi_after;

  do {
    hi_before = *timer_hi;
    lo = *timer_lo;
    hi_after = *timer_hi;
  } while (hi_before != hi_after);

  uptime->us = ((uint64_t)hi_after << 32) | lo;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}
