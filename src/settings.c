#include <settings.h>

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

typedef enum {
  C_ALM_ALM,
  C_ALM_NEW,
  C_ALM_DONE
} c_alm_t;

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
  // NOTE: 'So:' actually not needed since song has its own function. But
  // the spacer needs to be there for the others to get the right index.
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
      // The last cases (clk done, alm activate, alm done) all works
      // the same way.
      assert((settingState >= CLK_DONE) && (settingState <= ALM_DONE));
      TM1637_display_word(labels[setting][settingState], true);
  }
}

static void show_alarm(int showAlarmState, int alarmIndex, bool updateLeft) {
  if (updateLeft) {
    TM1637_display_word("AL:", true);
  }
  switch (showAlarmState) {
    case C_ALM_ALM:
      TM1637_display_right(alarmIndex, false);
      break;
    case C_ALM_NEW:
      TM1637_display_word("nEw", true);
      break;
    case C_ALM_DONE:
      TM1637_display_word("done", true);
      break;
  }
}

static void show_song(int song, bool updateLeft) {
  if (updateLeft) {
    TM1637_display_word("So:", true);
  }
  TM1637_display_right(song, false);
}

static void in_or_decrement_time_setting(
    int settingState,
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
}

static inline void increment_time_setting(int settingState, datetime_t *time) {
  in_or_decrement_time_setting(settingState, time, true);
}

static inline void decrement_time_setting(int settingState, datetime_t *time) {
  in_or_decrement_time_setting(settingState, time, false);
}

/* Set time->sec to 0 and show. Only needs to work for CLK */
void zero_seconds(int settingState, datetime_t *time) {
  time->sec = 0;
  rtc_set_datetime(time);
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
        rtc_get_datetime(&time);
        show_setting(setClockState, CLK, &time, true);
        while (setClockMode) {
          button = fetch_button_with_irq_off();
          rtc_get_datetime(&time);
          switch (button) {
            case 0: // no button has been pressed
              // In case clock has changed (by time passing) we need to update
              // the display.
              if ((oldSec != time.sec) && setClockState == CLK_SEC) {
                oldSec = time.sec;
                show_setting(setClockState, CLK, &time, false);
              }
              break;
            case LEFT_BUTTON:
              /* Go to next clock setting
               *
               * dotw -> hour -> min -> sec -> done
               *   ^                             |
               *   |_____________________________|
               */
              setClockState = (setClockState + 1) % CLK_LEN;
              // update display
              printf("  setClockState: %d\n", setClockState);
              rtc_get_datetime(&time);
              show_setting(setClockState, CLK, &time, true);
              break;
            case MIDDLE_BUTTON:
              /* Action
               * - dotw, hour, min: Decrement
               * - sec: Zero
               * - done: Do nothing
               */
              switch (setClockState) {
                case CLK_DOTW ... CLK_MIN:
                  decrement_time_setting(setClockState, &time);
                  show_setting(setClockState, CLK, &time, false);
                  rtc_set_datetime(&time);
                  break;
                case CLK_SEC:
                  time.sec = 0;
                  show_setting(setClockState, CLK, &time, false);
                  rtc_set_datetime(&time);
                  break;
                case CLK_DONE:
                  break;
                default:
                  printf("ERROR in function %s: Left button unknown state\n",
                       __func__);
              }
              break;
            case RIGHT_BUTTON:
              /* Action
               * - dotw, hour, min: Increment
               * - sec: Zero
               * - done: Exit set clock mode
               */
              switch (setClockState) {
                case CLK_DOTW ... CLK_MIN:
                  increment_time_setting(setClockState, &time);
                  show_setting(setClockState, CLK, &time, false);
                  rtc_set_datetime(&time);
                  break;
                case CLK_SEC:
                  time.sec = 0;
                  show_setting(setClockState, CLK, &time, false);
                  rtc_set_datetime(&time);
                  break;
                case CLK_DONE:
                  // Exit, but come back to the same setting.
                  setClockMode = false;
                  return setting;
                default:
                  printf("ERROR in function %s: Right button unknown state\n",
                       __func__);
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
       case RIGHT_BUTTON:
         // Go into "choose alarm mode"
         ;
         datetime_t alarmTime;
         c_alm_t chooseAlarmState = is_alarms() ? C_ALM_ALM : C_ALM_NEW;
         bool createNewAlarm = false;
         bool editAlarm = false;
         while (true) {
           switch (chooseAlarmState) {
             case C_ALM_ALM:
               DEBUG_PRINT(("Enter choose alarm: alarm\n"));
               int alarmIndex = 0;
               if (!createNewAlarm) {
                get_next_alarm_time(&alarmTime, true);
                // Show first alarm
                show_alarm(C_ALM_ALM, alarmIndex, true);
                printf("Alarm %d at ", alarmIndex);
                print_time(&alarmTime, 0);
               }
               while (chooseAlarmState == C_ALM_ALM) {
                 if (editAlarm || createNewAlarm) {
                   node_t alarm;
                   alm_t setAlarmState = ALM_DOTW;
                   if (editAlarm) {
                     // get alarm from list, remove it
                     remove_alarm(&alarmTime, &alarm);
                   } else if (createNewAlarm) {
                     // make a default alarm
                     alarmTime.dotw = 1;
                     alarmTime.hour = 0;
                     alarmTime.min = 0;
                     alarmTime.sec = 0;
                     alarm.time = &alarmTime;
                     alarm.song = 0;
                   }
                   
                   // update display
                   printf("  setAlarmState: %d\n", setAlarmState);
                   show_setting(setAlarmState, ALM, alarm.time, true);
                   print_all_alarms();
                   
                   while (editAlarm || createNewAlarm) {
                     button = fetch_button_with_irq_off();
                     switch (button) {
                       case 0:
                         // no button has been pressed
                         break;
                       case LEFT_BUTTON:
                         /* Go to next alarm setting
                          *
                          * dotw -> hour -> min -> song -> confirm -> done
                          *   ^                                         |
                          *   |_________________________________________|
                          */
                         setAlarmState = (setAlarmState + 1) % ALM_LEN;
                         printf("  setAlarmState: %d\n", setAlarmState);
                         // update display
                         switch (setAlarmState) {
                           case ALM_SONG:
                             show_song(alarm.song, true);
                             break;
                           default:
                             show_setting(setAlarmState, ALM, alarm.time, true);
                         }
                         break;
                       case MIDDLE_BUTTON:
                         /* Action
                          * - dotw, hour, min: Decrement
                          * - song: Play TODO
                          * - done: Do nothing
                          */
                         switch (setAlarmState) {
                           case ALM_DOTW ... ALM_MIN:
                             decrement_time_setting(setAlarmState, alarm.time);
                             show_setting(setAlarmState, ALM, alarm.time, false);
                             break;
                           case ALM_SONG:
                             break;
                           case ALM_DONE:
                             break;
                           default:
                             printf("ERROR in function %s: Left button unknown state\n",
                                 __func__);
                         }
                         break;
                       case RIGHT_BUTTON:
                         /* Action
                          * - dotw, hour, min: Increment
                          * - song: Increment
                          * - done: Save and exit edit mode.
                          */
                         switch (setAlarmState) {
                           case ALM_DOTW ... ALM_MIN:
                             increment_time_setting(setAlarmState, alarm.time);
                             show_setting(setAlarmState, ALM, alarm.time, false);
                             break;
                           case ALM_SONG:
                             increment_with_wrap(
                                 &(alarm.song),
                                 get_number_of_songs()
                                 );
                             show_song(alarm.song, false);
                             break;
                           case ALM_DONE:
                             createNewAlarm = false;
                             editAlarm = false;
                             
                             // old node already removed, just add the edited
                             // verison to the list.
                             add_alarm(alarm.time, alarm.song);
                             print_all_alarms();

                             // NOTE: Alarms will always come in cronological
                             // order. So the alarm might move in the list
                             // after an edit.

                             // always restart menu
                             get_next_alarm_time(&alarmTime, true);
                             alarmIndex = 0;

                             // update display
                             printf("  setAlarmState: %d\n", setAlarmState);
                             show_alarm(C_ALM_ALM, alarmIndex, true);
                             break;
                           default:
                             printf("ERROR in function %s: Right button unknown state\n",
                                 __func__);
                         }
                         break;
                       default:
                         printf("ERROR: SET_ALARM\n");
                     }
                   }
                 }
                 
                 button = fetch_button_with_irq_off();
                 switch (button) {
                   case 0:
                     break;
                   case LEFT_BUTTON:
                     // Go to next alarm, or if there is no more alarms
                     // go to "choose alarm: new".
                     if (get_next_alarm_time(&alarmTime, false)) {
                       // No next alarm found
                       chooseAlarmState++;
                     } else {
                       // Next alarm found. Show it.
                       show_alarm(C_ALM_ALM, ++alarmIndex, false);
                       printf("Alarm %d at ", alarmIndex);
                       print_time(&alarmTime, 0);
                     }
                     break;
                   case MIDDLE_BUTTON:
                     // Remove current alarm
                     TM1637_display_word("Conf", true);
                     DEBUG_PRINT(("  Remove alarm?\n"));
                     bool removing = true;
                     while (removing) {
                       button = fetch_button_with_irq_off();
                       switch (button) {
                         case 0:
                           break;
                         case LEFT_BUTTON:
                         case MIDDLE_BUTTON:
                           // cancel
                           DEBUG_PRINT(("    abort\n"));
                           removing = false;
                           show_alarm(C_ALM_ALM, alarmIndex, true);
                           break;
                         case RIGHT_BUTTON:
#                          ifdef NDEBUG
                             printf("  Removing alarm %d at", alarmIndex);
                             print_time(&alarmTime, 1);
#                          endif
                           print_all_alarms();
                           removing = false;
                           datetime_t timeCopy = alarmTime;
                           // Remove current alarm, and go to next one
                           bool restart = (alarmIndex == 0) ? true : false;
                           if (alarmIndex != 0) {
                             if (get_next_alarm_time(&alarmTime, false)) {
                               // No next alarm, go to 'choose alarm: new' instead.
                               DEBUG_PRINT(("    No next alarm, go to choose alarm: new\n"));
                               chooseAlarmState++;
                             } else {
                               show_alarm(C_ALM_ALM, ++alarmIndex, true);
                               printf("  Alarm %d at ", alarmIndex);
                               print_time(&alarmTime, 0);
                             }
                             remove_alarm(&timeCopy, NULL);
                             print_all_alarms();
                           } else {
                             // We need to do this differently when removing
                             // the first node.
                             remove_alarm(&alarmTime, NULL);
                             if (get_next_alarm_time(&alarmTime, true)) {
                               // No alarms left
                               chooseAlarmState++;
                             } else {
                               show_alarm(C_ALM_ALM, ++alarmIndex, true);
                             }
                           }
                           break;
                       }
                     }
                     break;
                   case RIGHT_BUTTON:
                     // Edit current alarm
                     editAlarm = true;
                     break;
                   default:
                     printf("ERROR in %s. Unknown button for 'choose alarm: alarm'\n");
                     return EXIT_FAILURE;
                 }
               }
               break;
             case C_ALM_NEW:
               DEBUG_PRINT(("Enter choose alarm: new\n"));
               show_alarm(C_ALM_NEW, -1, false);
               while (chooseAlarmState == C_ALM_NEW) {
                 button = fetch_button_with_irq_off();
                 switch (button) {
                   case LEFT_BUTTON:
                     // Next
                     chooseAlarmState++;
                     break;
                   case RIGHT_BUTTON:
                     // Create new alarm
                     printf("  Create new alarm\n");
                     createNewAlarm = true; 
                     chooseAlarmState = C_ALM_ALM;
                     break;
                 }
               }
               break;
             case C_ALM_DONE:
               printf(("Enter choose alarm: done\n"));
               show_alarm(C_ALM_DONE, -1, false);
               while (chooseAlarmState == C_ALM_DONE) {
                 button = fetch_button_with_irq_off();
                 switch (button) {
                   case 0:
                     break;
                   case LEFT_BUTTON:
                     // Wrap around
                     if (is_alarms()) {
                       chooseAlarmState = C_ALM_ALM;
                     } else {
                       chooseAlarmState = C_ALM_NEW;
                     }
                     break;
                   case RIGHT_BUTTON:
                     // Exit but stay in alarm setting
                     return setting;
                 }
               }
               break;
           }
         }
       default:
         DEBUG_PRINT(("ERROR in set_alarm_setting, unknown state."));
     }
   }
}

int done_setting(const int setting) {
  printf("Setting = %d\n", setting);
  TM1637_display_word("done", true);
  int button;
  while (true) {
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
