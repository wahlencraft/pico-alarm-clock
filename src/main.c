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

#include <pins.h>
#include <helpers.h>
#include <settings.h>
#include <node.h>
#include <alarm.h>
#include <timeout_timer.h>

/*******************************************************************************
 * Globals
 ******************************************************************************/

volatile struct GlobBinder *state;  // Binder for all states in the global state machine

/*******************************************************************************
 * Functions
 ******************************************************************************/

static void fire_alarm(void) {
  printf("START NEW ALARM\n");
  uint32_t hz = clock_get_hz(clk_sys);
  printf("  Running at %f MHz\n", (float) hz/1000000);
  state->alarmMode = true;
  start_alarm_timer();
}

static void gpio_callback(uint gpio, uint32_t events) {
  DEBUG_PRINT(("Interrupted by GPIO %d\n", gpio));
  DEBUG_PRINT(("  alarmMode: %d, sleepMode: %d, buttonBuffer: %d\n",
      state->alarmMode, state->sleepMode, state->buttonBuffer));
  if (state->alarmMode) {
    DEBUG_PRINT(("    Turn of alarm!\n"));
    state->alarmMode = false;
  } else {
    state->sleepMode = false;
    state->buttonBuffer = gpio;
  }
  DEBUG_PRINT(("  alarmMode: %d, sleepMode: %d, buttonBuffer: %d\n",
      state->alarmMode, state->sleepMode, state->buttonBuffer));
}

static int64_t alarm_callback(alarm_id_t id, void *user_data) {
  DEBUG_PRINT(("alarm_callback: Timer %d fired! ", (int) id));
  display_h_min();
  int64_t nextCallback = update_running_song();
  DEBUG_PRINT(("Again in %lld ms\n", nextCallback));
  // Can return a value here in us to fire in the future
  return nextCallback*1000;
}

static void setup_button(int gpio) {
  gpio_pull_up(gpio);
  gpio_set_irq_enabled_with_callback(
      gpio,
      GPIO_IRQ_EDGE_RISE,
      true,
      &gpio_callback
      );
}

/* Put cpu to sleep until next minute. Then display time. */
static void sleep_to_next_min() {
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

/* Put cpu to sleep until next alarm. Then display time and start alarmMode. */
void sleep_to_next_alarm(node_t *alarm) { 
  // Copy time
  datetime_t time;
  time.year = -1;
  time.month = -1;
  time.day = -1;
  time.dotw = alarm->time->dotw;
  time.hour = alarm->time->hour;
  time.min = alarm->time->min;
  time.sec = alarm->time->sec;
  
  // Prepare for sleep mode
  uart_default_tx_wait_blocking();
  TM1637_wait();
  
  // Go to sleep
  sleep_goto_sleep_until(&time, &fire_alarm);
}

/*******************************************************************************
 *  Main program
 *  - Initiate
 *  - Start main loop (the state machine)
 ******************************************************************************/
int main() {

  struct GlobBinder globalStruct = {.sleepMode = true, .buttonBuffer = 0};
  state = &globalStruct;
  
  stdio_init_all();
  uart_default_tx_wait_blocking();

  // Initiate
  TM1637_init(CLK_PIN, DIO_PIN);
  setup_button(LEFT_BUTTON);
  setup_button(MIDDLE_BUTTON);
  setup_button(RIGHT_BUTTON);
  DEBUG_WAIT((100));
  init_alarms();

  TM1637_clear();
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

  display_h_min();
  state->sleepMode = true;
  state->alarmMode = false;
  state->buttonBuffer = 0;

  node_t *runningAlarm = malloc(sizeof(runningAlarm));
  runningAlarm->time = malloc(sizeof(datetime_t));

  DEBUG_PRINT(("\nSTART MAIN LOOP\n"));
  // Main loop
  //
  // if alarmMode:
  //    play song (stay here until interupt)
  // else if sleepMode:
  //    if "alarm in 1 min" -> sleep_to_next_alarm
  //    else -> sleep_to_next_min
  // else: (button interupt)
  //    open settings menu
  while (true) {
    DEBUG_PRINT(("\n"));
    print_current_time();
    print_all_alarms();
    if (state->alarmMode) {
      rtc_disable_alarm();
      start_song(runningAlarm->song);
      alarm_id_t almID = add_alarm_in_us(0, alarm_callback, NULL, true);
      DEBUG_PRINT(("Alarm %d started\n", almID));
      while (state->alarmMode) {
        if (alarm_timeout()) {
          state->alarmMode = false;
          DEBUG_PRINT(("Alarm fired for too long. Turning off.\n"));
          break;
        }
      }
      cancel_alarm(almID);
      stop_song();
    } else if (state->sleepMode) {
      if (is_alarm_in_1_min(runningAlarm)) {
        DEBUG_PRINT(("Found Alarm! Fire at "));
        print_time(runningAlarm->time, 0);
        sleep_to_next_alarm(runningAlarm);
      } else {
        sleep_to_next_min();
      }
    } else {
      // Interrupted from by button. Open settings menu.
      rtc_disable_alarm();
      DEBUG_PRINT(("Woke up by interupt!\n"));
      int button;
      int setting = 1;
      state->buttonBuffer = 0;
      while (setting != 0) {
        DEBUG_PRINT(("Outer setting loop. Case is %d\n", setting));
        switch (setting) {

          case SETT_BRIGHT:
            DEBUG_PRINT((" Enter SETT_BRIGHT\n"));
            setting = brightness_setting(SETT_BRIGHT);
            break;

          case SETT_CLOCK:
            DEBUG_PRINT((" Enter SETT_CLOCK\n"));
            setting = set_clock_setting(SETT_CLOCK);
            break;

          case SETT_ALARM:
            DEBUG_PRINT((" Enter SETT_ALARM\n"));
            setting = set_alarm_setting(SETT_ALARM);
            break;

          case SETT_DONE:
            DEBUG_PRINT((" Enter SETT_DONE\n"));
            setting = done_setting(SETT_DONE);
            break;

          default:
            // Should not happen
            DEBUG_PRINT(("Runntime Error, buttonBuffer = %d\n",
                  state->buttonBuffer));
            return 1;
        }
      }
      // Exit settings menu.
      display_h_min();
      state->sleepMode = true;
    }
  }
}
