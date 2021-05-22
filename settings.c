#include <stdbool.h>
#include <stdio.h>
//#include <stdbool.h>
#include <pico/stdlib.h>
//#include <pico/util/datetime.h>
#include <hardware/rtc.h>
#include <hardware/sync.h>
#include <PicoTM1637.h>
#include <helpers.h>

#define CLK_PIN 27
#define DIO_PIN 26
#define BUZ_PIN 15
#define RIGHT_BUTTON 22
#define MIDDLE_BUTTON 21
#define LEFT_BUTTON 20

extern struct GlobBinder *state;

/* INTERNAL HELPERS */

typedef enum {WEEKDAY, HOUR, MINUTE, SECOND, TIMES_DONE, TIMES_LEN} Times;

int fetch_button_with_irq_off(void) {
  static uint32_t irqStatus;
  int button;
  irqStatus = save_and_disable_interrupts();
  button = state->buttonBuffer;
  state->buttonBuffer = 0;
  restore_interrupts(irqStatus);
  return button;
}

void show_time_as_setting(Times clockState, datetime_t *time, bool newState) {
  printf("Show time as setting!\n");
  char *labels[] = {
    "da:", "Hr:", "mi:", "SE:", "done"
  };
  char *weekdays[] = {
    "da:Su", "da:mo", "da:tu", "da:wE", "da:tH", "da:Fr", "da:SA"
  };
  uint8_t *timeStart = &(time->dotw);
  switch (clockState) {
    case WEEKDAY:
      printf("This is weekday\n");
      TM1637_display_word(weekdays[clockState], true);
      break;
    case HOUR ... SECOND:
      if (newState) {
        // The left part of the display only needs to be updated if the
        // clockState has changed since last call (by user input).
        TM1637_display_word(labels[clockState], true);
      }
      printf("This is %d\n", timeStart[clockState]);
      TM1637_display_right(timeStart[clockState], true);
      break;
    case TIMES_DONE:
      TM1637_display_word(labels[clockState], true);
      printf("This is done\n");
  }
}

/* SETTINGS */

void brightness_setting(const int settingNum) {
  TM1637_display_word("br:", true);
  int level = TM1637_get_brightness(), button;
  state->setting = settingNum;
  TM1637_display_right(level, false);
  while (state->setting == settingNum) {
    button = fetch_button_with_irq_off();
    switch (button) {
      case 0:
        break;
      case LEFT_BUTTON:
        (state->setting)++;
        break;
      case MIDDLE_BUTTON:
        increment_with_wrap(&level, 8);
        TM1637_display_right(level, false);
        break;
      case RIGHT_BUTTON:
        TM1637_set_brightness(level);
        TM1637_display_right(level, false);
        break;
      default:
        assert(button == LEFT_BUTTON);
    }
  }
}

void set_clock_setting(const int settingNum) {
  TM1637_display_word("SEt", true);
  int button;
  while (state->setting == settingNum) {
    button = fetch_button_with_irq_off();
    switch (button) {  
      case 0:
        break;
      case LEFT_BUTTON:
        printf("E?\n");
        state->setting++;
        break;
      case RIGHT_BUTTON:
        // Go into "set clock mode"
        ;
        printf("Go into 'set clock mode'\n");
        datetime_t time;
        bool setClockMode = true;
        // initilize "set clock mode"
        Times setClockState = WEEKDAY;
        int8_t oldSec = -1;
        printf("setClockState: %d\n", setClockState);
        while (setClockMode) {
          button = fetch_button_with_irq_off();
          rtc_get_datetime(&time);
          switch (button) {
            case 0:
              // In case clock has changed (by time passing) we need to update
              // the display.
              if (oldSec != time.sec) {
                oldSec = time.sec;
                show_time_as_setting(setClockState, &time, false);
              }
              break;
            case LEFT_BUTTON:
              // go to next clock setting
              // WEEKDAY -> HOUR -> MINUTE -> SECONDS -> DONE
              //    ^                                      |
              //    |______________________________________|
              //
              setClockState = (setClockState + 1) % TIMES_LEN;
              // update display
              printf("setClockState: %d\n", setClockState);
              rtc_get_datetime(&time);
              show_time_as_setting(setClockState, &time, true);
              break;
            case MIDDLE_BUTTON:
              switch (setClockState) {
                case WEEKDAY:
                  printf("Weekday\n");
                  break;
                case HOUR:
                  printf("Hour\n");
                  break;
                default:
                  printf("Set clock state error: out of bounds\n");
              }
              break;
            case RIGHT_BUTTON:
              break;
          }
        }
        break;
      default:
        printf("ERROR SETT_CLOCK\n");
    }
  }
}

void set_alarm_setting(const int settingNum) { 
  printf("Setting = %d\n", state->setting);
  TM1637_display_word("ALAr", true);
  int button;
  while (state->setting == settingNum) {
     button = fetch_button_with_irq_off();
     switch (button) {  
       case 0:
         break;
       case LEFT_BUTTON:
         state->setting++;
         break;
       default:
         printf("ERROR SETT_ALARM\n");
     }
   }
}

void done_setting(const int settingNum) {
  printf("Setting = %d\n", state->setting);
  TM1637_display_word("done", true);
  int button;
  while (state->setting == settingNum) {
    button = fetch_button_with_irq_off();
    switch (button) {  
      case 0:
        break;
      case LEFT_BUTTON:
        state->setting = 1; // wrap aroud to first setting
        break;
      case RIGHT_BUTTON:
        display_h_min();
        state->setting = 0; // no setting, go back to sleepmode
        state->sleepMode = true;
        break;
    }
  }
}
