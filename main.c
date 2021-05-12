#include <stdlib.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/sleep.h>
#include <hardware/rtc.h>
#include <pico/util/datetime.h>
#include <PicoTM1637.h>
#include <hardware/sync.h>

#include <helpers.h>

#define CLK_PIN 27
#define DIO_PIN 26
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
int buttonBuffer = 0;


// Functions

static void display_min_sec(void) {
  rtc_get_datetime(&t);
  TM1637_display_both(t.min, t.sec, true);
}

static void display_h_min(void) {
  rtc_get_datetime(&t);
  TM1637_display_both(t.hour, t.min, true);
}

void increment_with_wrap(int *num, int wrap) {
  if (*num < wrap - 1) { (*num)++;} else {*num = 0;}
}

static void gpio_callback(uint gpio, uint32_t events) {
  printf("Interrupted by %d\n", gpio);
  sleepMode = false;
  buttonBuffer = gpio;
  printf("  buttonBuffer = %d\n", buttonBuffer);
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
  
  datetime_t t_SETT_ALARM = {
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
  
  // Setting cases
  #define SETT_NONE 0
  #define SETT_BRIGHT 1
  #define SETT_CLOCK 2
  #define SETT_ALARM 3
  #define SETT_DONE 4

  printf("Start main loop\n");
  display_h_min();
  sleepMode = true;
  buttonBuffer = 0;

  // Main loop
  while (true) {
    if (sleepMode) {
      show_next_min();
    } else {
      // Interrupted from sleepmode
      rtc_disable_alarm();
      printf("Woke up by interupt!\n");
      int setting = 1, button;
      buttonBuffer = 0;
      while (setting != 0) {
        printf("Switch!\n");
        switch (setting) {
          
          case SETT_BRIGHT:
            TM1637_display_word("br:", true);
            int level = TM1637_get_brightness();
            TM1637_display_right(level, false);
            while (setting == SETT_BRIGHT) {
              button = buttonBuffer;
              buttonBuffer = 0;
              //printf("Brightnes, last clicked: %d\n", button);
              switch (button) {
                case 0:
                  break;
                case NEXT_PIN:
                  setting++;
                  break;
                case CONTINUE_PIN:
                  increment_with_wrap(&level, 8);
                  TM1637_display_right(level, false);
                  break;
                case INTERACT_PIN:
                  TM1637_set_brightness(level);
                  TM1637_display_right(level, false);
                  break;
                default:
                  assert(button == NEXT_PIN);
              }
            }
          
          case SETT_CLOCK:
            TM1637_display_word("SEt", true);
            while (setting == SETT_CLOCK) {
              button = buttonBuffer;
              buttonBuffer = 0;
              switch (button) {  
                case 0:
                  break;
                case NEXT_PIN:
                  printf("E?\n");
                  setting++;
                  break;
                default:
                  printf("ERROR SETT_CLOCK\n");
              }
            }
        
          case SETT_ALARM:
            printf("Setting = %d\n", setting);
            TM1637_display_word("ALAr", true);
            while (setting == SETT_ALARM) {
              button = buttonBuffer;
              buttonBuffer = 0;
              switch (button) {  
                case 0:
                  break;
                case NEXT_PIN:
                  setting++;
                  break;
                default:
                  printf("ERROR SETT_ALARM\n");
              }
            }
          
          case SETT_DONE:
            printf("Setting = %d\n", setting);
            TM1637_display_word("done", true);
            while (setting == SETT_DONE) {
              button = buttonBuffer;
              buttonBuffer = 0;
              switch (button) {  
                case 0:
                  break;
                case NEXT_PIN:
                  setting = 1; // wrap aroud to first setting
                  break;
                case INTERACT_PIN:
                  display_h_min();
                  setting = SETT_NONE;
                  sleepMode = true;
                  break;
              }
            }
            break;
          
          default:
            // Should not happen
            printf("Runntime Error, buttonBuffer = %d\n", buttonBuffer);
          return -1;
        }
      }
    }
  }
}
