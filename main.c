#include <stdlib.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/sleep.h>
#include <hardware/rtc.h>
#include <pico/util/datetime.h>
#include <PicoTM1637.h>

#define CLK_PIN 26
#define DIO_PIN 27
#define BUZ_PIN 15

#define BUZZER_DELAY 500

// Used to set default time. Reused as a place to temporarily store times.
static datetime_t t = {
  .year = 1970,
  .month = 1,
  .day = 1,
  .dotw = 1, // 0 is Sunday
  .hour = 0,
  .min = 0,
  .sec = 0
};

static char strBuff[63];


/* Increment a datetime struct.
 *
 * datetime   Pointer to datetime struct.
 * startIndex Where to start incrementing. 0 for incrementing a second, 1 for 
 *              a minute etc. */
void increment_datetime(datetime_t *t, int startIndex) {
  int8_t *time = &(t->dotw);
  int i = 3 - startIndex;
  // Now time attributes can be found with the time pointer.
  // time[3] is seconds, 
  // time[2] is minutes, 
  // time[1] is hours and
  // time[0] is day of the week.
  // I do not care about day, month or year.
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

static void display_min_sec(void) {
  rtc_get_datetime(&t);
  TM1637_display_both(t.min, t.sec, true);
}

static void display_h_min(void) {
  rtc_get_datetime(&t);
  TM1637_display_both(t.hour, t.min, true);
}

void show_next_min() {
  rtc_get_datetime(&t);
  datetime_to_str(strBuff, 63, &t);
  printf("Current time: %s\n", strBuff);
  increment_datetime(&t, 1);
  t.sec = 0;
  uart_default_tx_wait_blocking();
  datetime_to_str(strBuff, 63, &t);
  printf(" Sleep til: %s\n", 63, strBuff);
  uart_default_tx_wait_blocking();
  
  TM1637_wait();

  sleep_goto_sleep_until(&t, &display_h_min);
}

int main() {
  // Initiate
  stdio_init_all();
  TM1637_init(CLK_PIN, DIO_PIN);
  gpio_init(BUZ_PIN);
  gpio_set_dir(BUZ_PIN, GPIO_OUT);
  
  TM1637_clear();
  printf("\nTESTING!\n");
  
  datetime_t t_alarm = {
          .year  = -1,
          .month = -1,
          .day   = -1,
          .dotw  = 1, // 0 is Sunday, so 5 is Friday
          .hour  = 0,
          .min   = 0,
          .sec   = 10
  };

  // Switch to XOSC
  uart_default_tx_wait_blocking();
  sleep_run_from_xosc();

  // Start the RTC
  rtc_init();
  rtc_set_datetime(&t);

  printf("Clock display\n");
  display_h_min();
  while (true) {
    show_next_min();
  }
  
  printf("END OF PROGRAM\n");

}
