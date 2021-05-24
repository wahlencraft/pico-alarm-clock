#ifndef HELPERS_H_
#define HELPERS_H_

extern struct GlobBinder {
  bool sleepMode;
  int buttonBuffer;
  int setting;
} globBinder;


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

#endif //HELPERS_H_
