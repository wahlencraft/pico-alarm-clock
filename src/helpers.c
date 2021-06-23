#include <helpers.h>

#define DATETIME_BEFORE 1
#define DATETIME_SAME 2
#define DATETIME_AFTER 3

static char strBuff[63];

int compare_datetimes(datetime_t *time1_input, datetime_t *time2_input) {
  // find out if t1 is (before | same | after) t2
  int8_t *time1 = &(time1_input->dotw);
  int8_t *time2 = &(time2_input->dotw);
  // Now time attributes can be found with the time pointer.
  // timex[0] is day of the week.
  // timex[1] is hours and
  // timex[2] is minutes,
  // timex[3] is seconds 
  for (int i = 0; i < 4; i++) {
    int t1, t2;
    // I want Monday to be the first day of the week. Simply replace 0 with 7 if 
    // i = 0 and the days of the week will range from 1 to 7.
    t1 = (time1[i] == 0 & i == 0) ? 7 : time1[i];
    t2 = (time2[i] == 0 & i == 0) ? 7 : time2[i];
    if (t1 < t2) {
      return DATETIME_BEFORE;
    } else if (t1 > t2) {
      return DATETIME_AFTER;
    }
  }
  // We made it trough the entire loop, this means the times are identical
  return DATETIME_SAME;
}

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
void increment_with_wrap(int *num, int wrap) {
  *num = (*num < wrap - 1) ? *num + 1 : 0;
}

void decrement_with_wrap(int *num, int wrap) {
  *num = (*num > 0) ? *num - 1 : wrap - 1;
}

void print_current_time() {
  datetime_t t;
  rtc_get_datetime(&t);
  printf("Current ");
  print_time(&t, 0);
}

void print_time(datetime_t *time, int indent) {
  char *weekdays[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
  };
  while (indent-- > 0) {
    printf(" ");
  }
  printf("time: %s %d:%02d:%02d\n",
      weekdays[time->dotw], time->hour, time->min, time->sec
      );
}

