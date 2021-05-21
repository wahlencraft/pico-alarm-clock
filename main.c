#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <pico/stdlib.h>
#include <pico/sleep.h>
#include <hardware/rtc.h>
#include <pico/util/datetime.h>
#include <PicoTM1637.h>
#include <hardware/sync.h>

/* GLOBALS */
// Global constants
#define CLK_PIN 27
#define DIO_PIN 26
#define BUZ_PIN 15
#define INTERACT_PIN 22
#define CONTINUE_PIN 21
#define NEXT_PIN 20
#define BUZZER_DELAY 500

// Global declarations
struct GlobBinder *state;  // Binder for all states in the global state machine 
static char strBuff[63];

// Include the helper files
#include <helpers.h>
#include <settings.h>


/* FUNCTIONS */

void gpio_callback(uint gpio, uint32_t events) {
  printf("Interrupted by %d\n", gpio);
  state->sleepMode = false;
  state->buttonBuffer = gpio;
  printf("  buttonBuffer = %d\n", state->buttonBuffer);
}

void setup_button(int gpio) {
  gpio_pull_up(gpio); 
  gpio_set_irq_enabled_with_callback(
      gpio, 
      GPIO_IRQ_EDGE_RISE, 
      true, 
      &gpio_callback
      );
}

void show_next_min() {
  datetime_t t;
  rtc_get_datetime(&t);
  datetime_to_str(strBuff, 63, &t);
  printf("Current time: %s\n", strBuff);
  increment_datetime(&t, 1);
  t.sec = 0;
  uart_default_tx_wait_blocking();
  datetime_to_str(strBuff, 63, &t);
  uart_default_tx_wait_blocking();
  
  TM1637_wait();

  sleep_goto_sleep_until(&t, &display_h_min);
}

/* MAIN PROGRAM */
int main() {

  struct GlobBinder globalStruct = {.sleepMode = true, .buttonBuffer = 0};
  state = &globalStruct;

  // Initiate
  stdio_init_all();
  TM1637_init(CLK_PIN, DIO_PIN);
  gpio_init(BUZ_PIN);
  gpio_set_dir(BUZ_PIN, GPIO_OUT);
  setup_button(NEXT_PIN);
  setup_button(CONTINUE_PIN);
  setup_button(INTERACT_PIN);

  TM1637_clear();
  printf("\n--NEW TEST--\n");
  
  // Start the RTC
  rtc_init();
  datetime_t t = {
    .year = 1970,
    .month = 1,
    .day = 1,
    .dotw = 1, // 0 is Sunday
    .hour = 0,
    .min = 0,
    .sec = 0
  };
  
  // Switch to XOSC
  uart_default_tx_wait_blocking();
  sleep_run_from_xosc();
  TM1637_refresh_frequency();
  
  rtc_set_datetime(&t);
  
  // Setting cases
  #define SETT_NONE 0
  #define SETT_BRIGHT 1
  #define SETT_CLOCK 2
  #define SETT_ALARM 3
  #define SETT_DONE 4

  printf("Start main loop\n");
  display_h_min();
  state->sleepMode = true;
  state->buttonBuffer = 0;

  // Main loop
  while (true) {
    if (state->sleepMode) {
      show_next_min();
    } else {
      // Interrupted from sleepmode
      rtc_disable_alarm();
      printf("Woke up by interupt!\n");
      int button;
      state->setting = 1;
      state->buttonBuffer = 0;
      while (state->setting != 0) {
        printf("Switch!\n");
        switch (state->setting) {
          
          case SETT_BRIGHT:
            brightness_setting(SETT_BRIGHT);
          
          case SETT_CLOCK:
            set_clock_setting(SETT_CLOCK);
        
          case SETT_ALARM:
            set_alarm_setting(SETT_ALARM);
          
          case SETT_DONE:
            done_setting(SETT_DONE);
            break;
          
          default:
            // Should not happen
            printf("Runntime Error, buttonBuffer = %d\n", state->buttonBuffer);
            return -1;
        }
      }
    }
  }
}
