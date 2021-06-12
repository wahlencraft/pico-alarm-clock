#include <stdbool.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/rtc.h>
#include <hardware/sync.h>
#include <PicoTM1637.h>

#include <pins.h>
#include <helpers.h>

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
  char *labels[] = {
    "da:", "Hr:", "mi:", "SE:", "done"
  };
  char *weekdays[] = {
    "da:Su", "da:mo", "da:tu", "da:wE", "da:tH", "da:Fr", "da:SA"
  };
  uint8_t *timeStart = &(time->dotw);
  switch (clockState) {
    case WEEKDAY:
      TM1637_display_word(weekdays[time->dotw], true);
      break;
    case HOUR ... SECOND:
      if (newState) {
        // The left part of the display only needs to be updated if the
        // clockState has changed since last call (by user input).
        TM1637_display_word(labels[clockState], true);
      }
      TM1637_display_right(timeStart[clockState], true);
      break;
    case TIMES_DONE:
      TM1637_display_word(labels[clockState], true);
  }
}

void in_or_decrement_time_setting(Times clockState, datetime_t *time, bool increment) {
  int maxForClockState[] = {7, 24, 60};
  int8_t *timeStart = &(time->dotw);
  int value = (int) timeStart[clockState];
  printf("Increment %d", value);
  if (increment) {
    increment_with_wrap(&value, maxForClockState[clockState]);
  } else {
    decrement_with_wrap(&value, maxForClockState[clockState]);
  }
  printf(" to %d\n", value);
  timeStart[clockState] = (int8_t) value;
  rtc_set_datetime(time);
  show_time_as_setting(clockState, time, false);
}

inline void increment_time_setting(Times clockState, datetime_t *time) {
  in_or_decrement_time_setting(clockState, time, true);
}

inline void decrement_time_setting(Times clockState, datetime_t *time) {
  in_or_decrement_time_setting(clockState, time, false);
}

void zero_seconds(Times clockState, datetime_t *time) {
  time->sec = 0;
  rtc_set_datetime(time);
  show_time_as_setting(clockState, time, false);
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
  bool stay = true;
  while (stay) {
    button = fetch_button_with_irq_off();
    switch (button) {  
      case 0:
        break;
      case LEFT_BUTTON:
        // Increment setting state, exit
        stay = false;
        state->setting++;
        break;
      case RIGHT_BUTTON:
        // Go into "set clock mode"
        ;
        datetime_t time;
        bool setClockMode = true;
        // initilize "set clock mode"
        Times setClockState = WEEKDAY;
        int8_t oldSec = -1;
        printf("  setClockState: %d\n", setClockState);
        while (setClockMode) {
          button = fetch_button_with_irq_off();
          rtc_get_datetime(&time);
          switch (button) {
            case 0: // no button has been pressed
              // In case clock has changed (by time passing) we need to update
              // the display.
              if (oldSec != time.sec) {
                oldSec = time.sec;
                show_time_as_setting(setClockState, &time, false);
              }
              break;
            case LEFT_BUTTON:
              /* Go to next clock setting
               *
               * WEEKDAY -> HOUR -> MINUTE -> SECONDS -> DONE
               *    ^                                      |
               *    |______________________________________|
               */
              setClockState = (setClockState + 1) % TIMES_LEN;
              // update display
              printf("  setClockState: %d\n", setClockState);
              rtc_get_datetime(&time);
              show_time_as_setting(setClockState, &time, true);
              break;
            case MIDDLE_BUTTON:
              /* Action
               * - WEEKDAY, hour, minute: Decrement
               * - SECONDS: Zero
               * - DONE: Do nothing
               */
              switch (setClockState) {
                case WEEKDAY ... MINUTE:
                  decrement_time_setting(setClockState, &time);
                  break;
                case SECOND:
                  zero_seconds(setClockState, &time);
                  break;
                default:
                  printf("Set clock state error: out of bounds\n");
              }
              break;
            case RIGHT_BUTTON:
              /* Action
               * - WEEKDAY, hour, minute: Increment
               * - SECONDS: Zero
               * - DONE: Exit set clock mode
               */
              switch (setClockState) {
                case WEEKDAY ... MINUTE:
                  increment_time_setting(setClockState, &time);
                  break;
                case SECOND:
                  zero_seconds(setClockState, &time);
                  break;
                case TIMES_DONE:
                  setClockMode = false;
                  stay = false;
                  break;
                default:
                  printf("Error: 'set clock state' -> 'right button' : out of bounds.\n");
              }
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
         DEBUG_PRINT(("ERROR in set_alarm_setting, unknown state."));
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
