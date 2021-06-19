#include <stdbool.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/rtc.h>
#include <hardware/sync.h>
#include <PicoTM1637.h>

#include <pins.h>
#include <helpers.h>

extern struct GlobBinder *state;

/*******************************************************************************
 *  Internal helper functions
 ******************************************************************************/

typedef enum {
  CLK_DOTW,
  CLK_HOUR,
  CLK_MIN,
  CLK_SEC,
  CLK_DONE,
  CLK_LEN
} clk_t;

typedef enum {
  ALM_DOTW,
  ALM_HOUR,
  ALM_MIN,
  ALM_SONG,
  ALM_CONF,
  ALM_DONE,
  ALM_LEN
} alm_t;

#define CLK 0
#define ALM 1

static int fetch_button_with_irq_off(void) {
  static uint32_t irqStatus;
  int button;
  irqStatus = save_and_disable_interrupts();
  button = state->buttonBuffer;
  state->buttonBuffer = 0;
  restore_interrupts(irqStatus);
  return button;
}

static void show_setting(
    int settingState,
    int setting,
    datetime_t *time,
    bool newState
  ){
  char *labels[2][6] = {
    "da:", "Hr:", "mi:", "SE:", "done",     "",
    "da:", "Hr:", "mi:", "So:",  "Act", "done"
  };
  //switch (setting) {
  //  case CLK:
  //    ;
  //    char *l = {
  //      "da:", "Hr:", "mi:", "SE:", "done"
  //    };
  //    labels = l;
  //    break;
  //  case ALM:
  //    ;
  //    char *l = {
  //      "da:", "Hr", "mi:", "So:", "Act", "done"
  //    };
  //    labels = l;
  //    break;
  //}
  char *weekdays[] = {
    "da:Su", "da:mo", "da:tu", "da:wE", "da:tH", "da:Fr", "da:SA"
  };
  uint8_t *timeStart = &(time->dotw);
  switch (settingState) {
    // NOTE: Both the CLK and ALM enumeratins starts at 0, so they overlap
    case CLK_DOTW:
      // For ALM this corresponds to ALM_DOTW
      TM1637_display_word(weekdays[time->dotw], true);
      break;
    case CLK_HOUR ... CLK_SEC:
      // For ALM this corresponds to ALM_HOUR ... ALM_SONG
      if (newState) {
        // The left part of the display only needs to be updated if the
        // settingState has changed since last call (by user input).
        TM1637_display_word(labels[setting][settingState], true);
      }
      TM1637_display_right(timeStart[settingState], true);
      break;
    default:
      // The last cases (clk done, alm song, alm activate, alm done) all works
      // the same way.
      assert((settingState >= CLK_DONE) && (settingState <= ALM_DONE));
      TM1637_display_word(labels[setting][settingState], true);
  }
}

static void in_or_decrement_time_setting(
    int settingState,
    int setting,
    datetime_t *time,
    bool increment
  ){
  int maxForState[] = {7, 24, 60};
  int8_t *timeStart = &(time->dotw);
  int value = (int) timeStart[settingState];
  printf("Increment %d", value);
  if (increment) {
    increment_with_wrap(&value, maxForState[settingState]);
  } else {
    decrement_with_wrap(&value, maxForState[settingState]);
  }
  printf(" to %d\n", value);
  timeStart[settingState] = (int8_t) value;
  rtc_set_datetime(time);
  show_setting(settingState, setting, time, false);
}

static inline void increment_time_setting(
    int settingState, 
    int setting, 
    datetime_t *time
    ){
  in_or_decrement_time_setting(settingState, setting, time, true);
}

static inline void decrement_time_setting(
    int settingState, 
    int setting,
    datetime_t *time
    ){
  in_or_decrement_time_setting(settingState, setting, time, false);
}

/* Set time->sec to 0 and show. Only needs to work for CLK */
void zero_seconds(int settingState, datetime_t *time) {
  time->sec = 0;
  rtc_set_datetime(time);
  show_setting(settingState, CLK, time, false);
}

/*******************************************************************************
 * The setting functions.
 *
 * These are called directly from the main loop based on pressed buttons.
 ******************************************************************************/

int brightness_setting(const int setting) {
  TM1637_display_word("br:", true);
  int level = TM1637_get_brightness(), button;
  TM1637_display_right(level, false);
  while (true) {
    button = fetch_button_with_irq_off();
    switch (button) {
      case 0:
        break;
      case LEFT_BUTTON:
        return setting + 1;
      case MIDDLE_BUTTON:
        decrement_with_wrap(&level, 8);
        TM1637_set_brightness(level);
        TM1637_display_right(level, false);
        break;
      case RIGHT_BUTTON:
        increment_with_wrap(&level, 8);
        TM1637_set_brightness(level);
        TM1637_display_right(level, false);
        break;
      default:
        DEBUG_PRINT(("ERROR in function %s on line %d\n Impossible state\n",
              __func__, __LINE__));
        return -1;
    }
  }
}

int set_clock_setting(const int setting) {
  TM1637_display_word("SEt", true);
  int button;
  while (true) {
    button = fetch_button_with_irq_off();
    switch (button) {
      case 0:
        break;
      case LEFT_BUTTON:
        // Exit go to next setting
        return setting + 1;
      case RIGHT_BUTTON:
        // Go into "set clock mode"
        ;
        datetime_t time;
        bool setClockMode = true;
        // initilize "set clock mode"
        clk_t setClockState = CLK_DOTW;
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
                show_setting(setClockState, CLK, &time, false);
              }
              break;
            case LEFT_BUTTON:
              /* Go to next clock setting
               *
               * CLK_DOTW -> CLK_HOUR -> CLK_MIN -> CLK_SECS -> DONE
               *    ^                                      |
               *    |______________________________________|
               */
              setClockState = (setClockState + 1) % CLK_LEN;
              // update display
              printf("  setClockState: %d\n", setClockState);
              rtc_get_datetime(&time);
              show_setting(setClockState, CLK, &time, true);
              break;
            case MIDDLE_BUTTON:
              /* Action
               * - CLK_DOTW, hour, minute: Decrement
               * - CLK_SECS: Zero
               * - DONE: Do nothing
               */
              switch (setClockState) {
                case CLK_DOTW ... CLK_MIN:
                  decrement_time_setting(setClockState, CLK, &time);
                  break;
                case CLK_SEC:
                  zero_seconds(setClockState, &time);
                  break;
                default:
                  printf("Set clock state error: out of bounds\n");
              }
              break;
            case RIGHT_BUTTON:
              /* Action
               * - CLK_DOTW, hour, minute: Increment
               * - CLK_SECS: Zero
               * - DONE: Exit set clock mode
               */
              switch (setClockState) {
                case CLK_DOTW ... CLK_MIN:
                  increment_time_setting(setClockState, CLK, &time);
                  break;
                case CLK_SEC:
                  zero_seconds(setClockState, &time);
                  break;
                case CLK_DONE:
                  // Exit, but come back to the same setting.
                  setClockMode = false;
                  return setting;
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

int set_alarm_setting(const int setting) {
  printf("Setting = %d\n", setting);
  TM1637_display_word("ALAr", true);
  int button;
  while (true) {
     button = fetch_button_with_irq_off();
     switch (button) {
       case 0:
         break;
       case LEFT_BUTTON:
         // Exit, go to next setting
         return setting + 1;
       default:
         DEBUG_PRINT(("ERROR in set_alarm_setting, unknown state."));
     }
   }
}

int done_setting(const int setting) {
  printf("Setting = %d\n", setting);
  TM1637_display_word("done", true);
  int button;
  while (setting == setting) {
    button = fetch_button_with_irq_off();
    switch (button) {
      case 0:
        break;
      case LEFT_BUTTON:
        // Exit, wrap around to first setting.
        return 1;
      case RIGHT_BUTTON:
        // Exit, then exit settings menu as well.
        return 0;
    }
  }
}
