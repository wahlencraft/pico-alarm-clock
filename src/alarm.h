/** 
 * alarm.h
 * 
 * Start, maintain and stop the alarm.
**/

#ifndef ALARM_H_
#define ALARM_H_

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>

#include <pico/stdlib.h>
#include <hardware/clocks.h>
#include <hardware/pwm.h>
#include <PicoTM1637.h>

#include <pins.h>
#include <node.h>

void sound_test();

void init_alarms();

/* Start a new alarm with song 'songNum'. */
void start_alarm(int songNum);

/* Turn of the running alarm. */
void stop_alarm();

/* Update state in the running alarm. 
 *
 * Returns the time in ms for when it shuld be run again. Warning, if not run
 * at thease intervals, sound might glich out. */
int64_t update_running_alarm();

#endif //ALARM_H_
