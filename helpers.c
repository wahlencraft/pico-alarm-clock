#include <stdlib.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/rtc.h>
#include <pico/util/datetime.h>
#include <PicoTM1637.h>

static char strBuff[63];

void display_min_sec(void) {
  datetime_t t;
  rtc_get_datetime(&t);
  TM1637_display_both(t.min, t.sec, true);
}

void display_h_min(void) {
  datetime_t t;
  rtc_get_datetime(&t);
  TM1637_display_both(t.hour, t.min, true);
}

void increment_with_wrap(int *num, int wrap) {
  *num = (*num < wrap - 1) ? *num + 1 : 0;
}

void decrement_with_wrap(int *num, int wrap) {
  *num = (*num > 0) ? *num - 1 : wrap - 1;
}
