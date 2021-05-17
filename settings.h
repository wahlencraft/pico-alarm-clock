/* The settings called from main are declared here.
 *
 * Every setting has a very similar structure -- A simple state machine. 
 * 
 * Fist execute some one-time code, then start a loop. The loop lasts as long as
 * the global state variable stays on the setting. In every loop the hardware 
 * buttons are checked for input. The button input is then used to determine 
 * case in a switch case expression.
 */ 

#ifndef SETTINGS_H_
#define SETTINGS_H_

void brightness_setting(const int settingNum);

void set_clock_setting(const int settingNum);

void set_alarm_setting(const int settingNum); 

void done_setting(const int settingNum);

#endif //SETTINGS_H_
