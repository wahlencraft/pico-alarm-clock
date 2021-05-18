#include <stdbool.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/sync.h>
#include <PicoTM1637.h>
#include <helpers.h>

#define CLK_PIN 27
#define DIO_PIN 26
#define BUZ_PIN 15
#define INTERACT_PIN 22
#define CONTINUE_PIN 21
#define NEXT_PIN 20

extern struct GlobBinder *state;

/* INTERNAL HELPER */
int fetch_button_with_irq_off(void) {
  static uint32_t irqStatus;
  int button;
  irqStatus = save_and_disable_interrupts();
  button = state->buttonBuffer;
  state->buttonBuffer = 0;
  restore_interrupts(irqStatus);
  return button;
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
      case NEXT_PIN:
        (state->setting)++;
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
}

void set_clock_setting(const int settingNum) {
  TM1637_display_word("SEt", true);
  int button;
  while (state->setting == settingNum) {
    button = fetch_button_with_irq_off();
    switch (button) {  
      case 0:
        break;
      case NEXT_PIN:
        printf("E?\n");
        state->setting++;
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
       case NEXT_PIN:
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
      case NEXT_PIN:
        state->setting = 1; // wrap aroud to first setting
        break;
      case INTERACT_PIN:
        display_h_min();
        state->setting = 0; // no setting, go back to sleepmode
        state->sleepMode = true;
        break;
    }
  }
}
