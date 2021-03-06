#ifndef HELPERS_H_
#define HELPERS_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <pico/stdlib.h>
#include <hardware/rtc.h>
#include <pico/util/datetime.h>
#include <PicoTM1637.h>

// #define DEBUG

#ifdef DEBUG
# define DEBUG_PRINT(x) printf x
# define DEBUG_WAIT(x) sleep_ms x
#else
# define DEBUG_PRINT(x) do {} while (0)
# define DEBUG_WAIT(x) do {} while (0)
#endif

#define DATETIME_BEFORE 1
#define DATETIME_SAME 2
#define DATETIME_AFTER 3

extern struct GlobBinder {
  bool sleepMode;
  bool alarmMode;
  int buttonBuffer;
} globBinder;

/* Find if t1 is before (1), same as (2) or after (3) t2. 
 *
 * Does not care about year, month or day.
 *
 * Monday is the first day of the week and Sunday is last. */
int compare_datetimes(datetime_t *t1, datetime_t *t2);

void deep_copy_time(datetime_t *t_source, datetime_t *t_target);

void display_min_sec(void);

void display_h_min(void);

/* Increment a datetime struct. Set year, month and day to -1.
 *
 * datetime   Pointer to datetime struct.
 * startIndex Where to start incrementing. 0 for incrementing a second, 1 for 
 *              a minute etc. */
void increment_datetime(datetime_t *t, int startIndex);

void increment_with_wrap(int *num, int wrap);

void decrement_with_wrap(int *num, int wrap);

void print_current_time();

void print_time(datetime_t *time, int indent);

#endif //HELPERS_H_
