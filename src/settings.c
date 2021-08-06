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
  ALM_ACTIVE,
  ALM_DELETE,
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

static int64_t alarm_callback(alarm_id_t id, void *user_data) {
  DEBUG_PRINT(("alarm_callback: Timer %d fired! ", (int) id));
  int64_t nextCallback = update_running_song();
  DEBUG_PRINT(("Again in %lld ms\n", nextCallback));
  // Can return a value here in us to fire in the future
  return nextCallback*1000;
}

static void show_setting(
    int settingState,
    int setting,
    datetime_t *time,
    bool newState
  ){
  char *labels[2][7] = {
    "da:", "Hr:", "mi:", "SE:", "done",    "",     "",
    "da:", "Hr:", "mi:", "So:",  "Act", "dEL", "done"
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

static void show_alarm(
    int showAlarmState,
    int alarmIndex,
    node_t *alarm,
    bool updateLeft) {
  if (updateLeft) {
    TM1637_display_word("AL:", true);
  }
  switch (showAlarmState) {
    case C_ALM_ALM:
      TM1637_display_right(alarmIndex, false);
      show_if_alarm_active(alarm);
      break;
    case C_ALM_NEW:
      TM1637_display_word("nEw", true);
      led_clear();
      break;
    case C_ALM_DONE:
      TM1637_display_word("done", true);
      led_clear();
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
  DEBUG_PRINT(("Increment %d", value));
  if (increment) {
    increment_with_wrap(&value, maxForState[settingState]);
  } else {
    decrement_with_wrap(&value, maxForState[settingState]);
  }
  DEBUG_PRINT((" to %d\n", value));
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
        DEBUG_PRINT(("  setClockState: %d\n", setClockState));
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
              DEBUG_PRINT(("  setClockState: %d\n", setClockState));
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
  DEBUG_PRINT(("Setting = %d\n", setting));
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
         node_t alarm;
         alarm.time = malloc(sizeof(datetime_t));
         c_alm_t chooseAlarmState = is_alarms() ? C_ALM_ALM : C_ALM_NEW;
         bool createNewAlarm = false;
         bool editAlarm = false;
         while (true) {
           switch (chooseAlarmState) {
             case C_ALM_ALM:
               DEBUG_PRINT(("Enter choose alarm: alarm\n"));
               int alarmIndex = 0;
               if (!createNewAlarm) {
                get_next_alarm(&alarm, true);
                // Show first alarm
                show_alarm(C_ALM_ALM, alarmIndex, &alarm, true);
                DEBUG_PRINT(("Alarm %d at ", alarmIndex));
                print_time(alarm.time, 0);
               }
               while (chooseAlarmState == C_ALM_ALM) {
                 if (editAlarm || createNewAlarm) {
                   alm_t setAlarmState = ALM_DOTW;
                   if (editAlarm) {
                     // get alarm from list, remove it
                     remove_alarm(alarm.time, &alarm);
                   } else if (createNewAlarm) {
                     // make a default alarm
                     alarm.time->dotw = 1;
                     alarm.time->hour = 0;
                     alarm.time->min = 0;
                     alarm.time->sec = 0;
                     alarm.song = 0;
                     alarm.active = true;
                   }

                   // update display
                   show_setting(setAlarmState, ALM, alarm.time, true);
                   show_if_alarm_active(&alarm);
                   DEBUG_PRINT(("  setAlarmState: %d\n", setAlarmState));
                   print_all_alarms();
                   bool playSongDemo = false;
                   alarm_id_t songID;
                   while (editAlarm || createNewAlarm) {
                     button = fetch_button_with_irq_off();
                     switch (button) {
                       case 0:
                         // no button has been pressed
                         break;
                       case LEFT_BUTTON:
                         /* Go to next alarm setting
                          *
                          * dotw -> hour -> min -> song -> activate -> delete -> done
                          *   ^                                                   |
                          *   |___________________________________________________|
                          */
                         // If playing a song demo, stop it.
                         if (playSongDemo) {
                           playSongDemo = false;
                           cancel_alarm(songID);
                           stop_song();
                           show_if_alarm_active(&alarm);
                         }

                         setAlarmState = (setAlarmState + 1) % ALM_LEN;
                         DEBUG_PRINT(("  setAlarmState: %d\n", setAlarmState));
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
                          * - song: Play/stop demo
                          * - activate, delete, done: Do nothing
                          */
                         switch (setAlarmState) {
                           case ALM_DOTW ... ALM_MIN:
                             decrement_time_setting(setAlarmState, alarm.time);
                             show_setting(setAlarmState, ALM, alarm.time, false);
                             break;
                           case ALM_SONG:
                             // Play current song. Play untill pressed again,
                             // other song selected or other setting selected.
                             playSongDemo = !playSongDemo;
                             if (playSongDemo) {
                               start_song(alarm.song);
                               songID = add_alarm_in_us(0, alarm_callback, NULL, true);
                             } else {
                               cancel_alarm(songID);
                               stop_song();
                               show_if_alarm_active(&alarm);
                             }
                             break;
                           case ALM_ACTIVE:
                           case ALM_DELETE:
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
                          * - active: Toggle alarm active
                          * - delete: Exit without saving.
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
                             if (playSongDemo) {
                               // If playing demo, stop it and start a new demo.
                               cancel_alarm(songID);
                               stop_song();
                               start_song(alarm.song);
                               songID = add_alarm_in_us(0, alarm_callback, NULL, true);
                             }
                             break;
                           case ALM_ACTIVE:
                             DEBUG_PRINT(("Active before: %d\n", alarm.active));
                             toggle_alarm_active(&alarm);
                             show_if_alarm_active(&alarm);
                             DEBUG_PRINT(("Active after: %d\n", alarm.active));
                             break;
                           case ALM_DELETE:
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
                                   show_setting(setAlarmState, ALM, alarm.time, true);
                                   break;
                                 case RIGHT_BUTTON:
                                   // Remove alarm (exit without saving)
                                   DEBUG_PRINT(("    Removed alarm while editing\n"));
                                   createNewAlarm = false;
                                   editAlarm = false;
                                   removing = false;

                                   // Restart menu
                                   if (is_alarms()) {
                                     get_next_alarm(&alarm, true);
                                     alarmIndex = 0;
                                     show_alarm(C_ALM_ALM, alarmIndex, &alarm, true);
                                   } else {
                                     chooseAlarmState = C_ALM_NEW;
                                   }
                                   break;
                               }
                             }
                             break;
                           case ALM_DONE:
                             // old node already removed, just add the edited
                             // verison to the list.
                             if (!add_alarm(alarm.time, alarm.song, alarm.active)) {
                               print_all_alarms();
                               
                               createNewAlarm = false;
                               editAlarm = false;

                               // NOTE: Alarms will always come in cronological
                               // order. So the alarm might move in the list
                               // after an edit.

                               // always restart menu
                               get_next_alarm(&alarm, true);
                               alarmIndex = 0;

                               // update display
                               DEBUG_PRINT(("  setAlarmState: %d\n", setAlarmState));
                               show_alarm(C_ALM_ALM, alarmIndex, &alarm, true);
                             } else {
                               // Failed to add alarm. Flash the display to
                               // notice the user.
                               TM1637_clear();
                               busy_wait_us(300000);
                               show_setting(setAlarmState, ALM, alarm.time, false);
                             }
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
                   // In case something changed in this block, restart.
                   continue;
                 }

                 button = fetch_button_with_irq_off();
                 switch (button) {
                   case 0:
                     break;
                   case LEFT_BUTTON:
                     // Go to next alarm, or if there is no more alarms
                     // go to "choose alarm: new".
                     if (get_next_alarm(&alarm, false)) {
                       // No next alarm found
                       chooseAlarmState++;
                     } else {
                       // Next alarm found. Show it.
                       show_alarm(C_ALM_ALM, ++alarmIndex, &alarm, false);
                       DEBUG_PRINT(("Alarm %d at ", alarmIndex));
                       print_time(alarm.time, 0);
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
                           show_alarm(C_ALM_ALM, alarmIndex, &alarm, true);
                           break;
                         case RIGHT_BUTTON:
                           DEBUG_PRINT(("  Removing alarm %d at", alarmIndex));
                           print_time(alarm.time, 1);
                           print_all_alarms();
                           removing = false;
                           datetime_t timeCopy = *(alarm.time);
                           // Remove current alarm, and go to next one
                           bool restart = (alarmIndex == 0) ? true : false;
                           if (alarmIndex != 0) {
                             if (get_next_alarm(&alarm, false)) {
                               // No next alarm, go to 'choose alarm: new' instead.
                               DEBUG_PRINT(("    No next alarm, go to choose alarm: new\n"));
                               chooseAlarmState++;
                             } else {
                               show_alarm(C_ALM_ALM, ++alarmIndex, &alarm, true);
                               DEBUG_PRINT(("  Alarm %d at ", alarmIndex));
                               print_time(alarm.time, 0);
                             }
                             remove_alarm(&timeCopy, NULL);
                             print_all_alarms();
                           } else {
                             // We need to do this differently when removing
                             // the first node.
                             remove_alarm(alarm.time, NULL);
                             if (get_next_alarm(&alarm, true)) {
                               // No alarms left
                               chooseAlarmState++;
                             } else {
                               show_alarm(C_ALM_ALM, ++alarmIndex, &alarm, true);
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
               show_alarm(C_ALM_NEW, -1, NULL, false);
               while (chooseAlarmState == C_ALM_NEW) {
                 button = fetch_button_with_irq_off();
                 switch (button) {
                   case LEFT_BUTTON:
                     // Next
                     chooseAlarmState++;
                     break;
                   case RIGHT_BUTTON:
                     // Create new alarm
                     DEBUG_PRINT(("  Create new alarm\n"));
                     createNewAlarm = true;
                     chooseAlarmState = C_ALM_ALM;
                     break;
                 }
               }
               break;
             case C_ALM_DONE:
               DEBUG_PRINT(("Enter choose alarm: done\n"));
               show_alarm(C_ALM_DONE, -1, NULL, false);
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
                     free(alarm.time);
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
  DEBUG_PRINT(("Setting = %d\n", setting));
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
