/*******************************************************************************
 * settings.h
 *
 * The settings called from main are declared here.
 *
 * Every setting has a very similar structure -- A simple state machine.
 *
 * - Fist execute some one-time code, then start a loop.
 * - The loop lasts until the user clicks some buttons to exit.
 * - In every loop the hardware buttons are checked for input. The button input
 *   is then used to determine case in a switch case expression which in turn
 *   makes the requested setting changes happen.
 * - Return the integer representing what setting should be changed next.
 *   Usually just the next one.
 *
 * For an in-depth view of the setting state machine, and the program flow,
 * please have a look at the 'Menu' section in README.md.
 ******************************************************************************/

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <stdbool.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/rtc.h>
#include <hardware/sync.h>
#include <PicoTM1637.h>

#include <pins.h>
#include <helpers.h>
#include <node.h>
#include <alarm.h>

int brightness_setting(const int setting);

int set_clock_setting(const int setting);

int set_alarm_setting(const int setting);

int done_setting(const int setting);

#endif //SETTINGS_H_
