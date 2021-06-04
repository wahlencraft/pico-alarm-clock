/**
 * main.c
 *
 * Some important functions and the upper level state-machine.
 **/

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <pico/stdlib.h>
#include <pico/sleep.h>
#include <hardware/rtc.h>
#include <pico/util/datetime.h>
#include <PicoTM1637.h>
#include <hardware/sync.h>

#include <helpers.h>
#include <settings.h>
#include <node.h>

/*******************************************************************************
 * Globals
 ******************************************************************************/
// Global constants
#define CLK_PIN 27
#define DIO_PIN 26
#define BUZ_PIN 15
#define RIGHT_BUTTON 22
#define MIDDLE_BUTTON 21
#define LEFT_BUTTON 20
#define BUZZER_DELAY 500

// Global declarations
struct GlobBinder *state;  // Binder for all states in the global state machine 
static char strBuff[63];

/*******************************************************************************
 * Functions 
 ******************************************************************************/

void gpio_callback(uint gpio, uint32_t events) {
  printf("    { Interrupted by %d\n", gpio);
  state->sleepMode = false;
  state->buttonBuffer = gpio;
  printf("      buttonBuffer = %d }\n", state->buttonBuffer);
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
  // Print current time
  datetime_t t;
  rtc_get_datetime(&t);
  datetime_to_str(strBuff, 63, &t);
  printf("Current time: %s\n", strBuff);
  // Set next wakeup
  datetime_t time = {
    .year = -1,
    .month = -1,
    .day = -1,
    .dotw = -1,
    .hour = -1,
    .min = -1,
    .sec = 0
  };
  uart_default_tx_wait_blocking(); 
  TM1637_wait();
  sleep_goto_sleep_until(&time, &display_h_min);
}

/*******************************************************************************
 *  Main program
 *  - Initiate
 *  - Start main loop (the state machine)
 ******************************************************************************/
int main() {

  struct GlobBinder globalStruct = {.sleepMode = true, .buttonBuffer = 0};
  state = &globalStruct;

  // Initiate
  stdio_init_all();
  TM1637_init(CLK_PIN, DIO_PIN);
  gpio_init(BUZ_PIN);
  gpio_set_dir(BUZ_PIN, GPIO_OUT);
  setup_button(LEFT_BUTTON);
  setup_button(MIDDLE_BUTTON);
  setup_button(RIGHT_BUTTON);

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

  node_test();

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
        printf("Outer setting loop. Case is %d\n", state->setting);
        switch (state->setting) {
          
          case SETT_BRIGHT:
            printf(" Enter SETT_BRIGHT\n");
            brightness_setting(SETT_BRIGHT);
            break;
          
          case SETT_CLOCK:
            printf(" Enter SETT_CLOCK\n");
            set_clock_setting(SETT_CLOCK);
            break;
        
          case SETT_ALARM:
            printf(" Enter SETT_ALARM\n");
            set_alarm_setting(SETT_ALARM);
            break;
          
          case SETT_DONE:
            printf(" Enter SETT_DONE\n");
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
