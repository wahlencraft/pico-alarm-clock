#ifndef HELPERS_H_
#define HELPERS_H_

extern struct GlobBinder {
  bool sleepMode;
  int buttonBuffer;
  int setting;
} globBinder;


void display_min_sec(void);

void display_h_min(void);

void increment_with_wrap(int *num, int wrap);

void decrement_with_wrap(int *num, int wrap);

#endif //HELPERS_H_
