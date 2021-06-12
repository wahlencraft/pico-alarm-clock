#ifndef HELPERS_H_
#define HELPERS_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <pico/stdlib.h>
#include <hardware/rtc.h>
#include <pico/util/datetime.h>
#include <PicoTM1637.h>

#ifdef NDEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif

#define DATETIME_BEFORE 1
#define DATETIME_SAME 2
#define DATETIME_AFTER 3

extern struct GlobBinder {
  bool sleepMode;
  bool alarmMode;
  int buttonBuffer;
  int setting;
} globBinder;

/* Find if t1 is before (1), same as (2) or after (3) t2. 
 *
 * Does not care about year, month or day.
 *
 * Monday is the first day of the week and Sunday is last. */
int compare_datetimes(datetime_t *t1, datetime_t *t2);

void display_min_sec(void);

void display_h_min(void);

void increment_with_wrap(int *num, int wrap);

void decrement_with_wrap(int *num, int wrap);

#endif //HELPERS_H_
