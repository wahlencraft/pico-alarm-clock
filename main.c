#include <stdlib.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/sleep.h>
#include <hardware/rtc.h>
#include <pico/util/datetime.h>
#include <PicoTM1637.h>
#include <hardware/sync.h>

#include <helpers.h>

#define CLK_PIN 26
#define DIO_PIN 27
#define BUZ_PIN 15
#define INTERACT_PIN 22
#define CONTINUE_PIN 21
#define NEXT_PIN 20

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

// Gloabals
static char strBuff[63];

bool sleepMode = true;
int buttonStatus = 0;


// Functions

static void display_min_sec(void) {
  rtc_get_datetime(&t);
  TM1637_display_both(t.min, t.sec, true);
}

static void display_h_min(void) {
  rtc_get_datetime(&t);
  TM1637_display_both(t.hour, t.min, true);
}

void gpio_callback(uint gpio, uint32_t events) {
  printf("Interrupted by %d\n", gpio);
  sleepMode = false;
  buttonStatus = gpio;
}

void show_next_min() {
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

void setup_button(int gpio) {
  gpio_pull_up(gpio); 
  gpio_set_irq_enabled_with_callback(
      gpio, 
      GPIO_IRQ_EDGE_RISE, 
      true, 
      &gpio_callback
      );
}

int main() {
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

  printf("Start main loop\n");
  display_h_min();
  sleepMode = true;
  buttonStatus = 0;

  // Main loop
  while (true) {
    if (sleepMode) {
      show_next_min();
    } else {
      // Interrupted from sleepmode
      rtc_disable_alarm();
      printf("Woke up by interupt!\n");
      while (true) {
        switch (buttonStatus) {
          case 0:
            // Do nothing
            break;
          case NEXT_PIN:
            printf(" NEXT\n");
            break;
          case CONTINUE_PIN:
            printf(" CONTINUE\n");
            break;
          case INTERACT_PIN:
            printf(" INTERACT\n");
            break;
          default:
            // Should not happen
            printf("Runntime Error, buttonStatus = %d\n", buttonStatus);
            return -1;
        }
        buttonStatus = 0;
        uart_default_tx_wait_blocking();
        __wfi();
      }
    }
  }
}
