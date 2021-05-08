#include <stdlib.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/rtc.h>
#include <pico/util/datetime.h>
#include <PicoTM1637.h>

/* Increment a datetime struct. Set year, month and day to -1.
 *
 * datetime   Pointer to datetime struct.
 * startIndex Where to start incrementing. 0 for incrementing a second, 1 for 
 *              a minute etc. */
void increment_datetime(datetime_t *t, int startIndex) {
  // Set year, month and day to -1
  t->year = -1;
  t->month = -1;
  t->day = -1;
  int8_t *time = &(t->dotw);
  int i = 3 - startIndex;
  // Now time attributes can be found with the time pointer.
  // time[3] is seconds, 
  // time[2] is minutes, 
  // time[1] is hours and
  // time[0] is day of the week.
  const int maxValues[] = {6, 23, 59, 59};
  back:
  switch (maxValues[i] - time[i]) {
    case 0:
      // Overflow detected
      time[i--] = 0;
      if (i >= 0) goto back;
      break;
    default:
      time[i]++;
      break;
  }
}

