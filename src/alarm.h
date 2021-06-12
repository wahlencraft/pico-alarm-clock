/** 
 * alarm.h
 * 
 * Start, maintain and stop the alarm.
 *
 * - Alarm: A time based event stored as a node.
 * - Song: The sound associated with the alarm.
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

/* Start a new song with song 'songNum'. */
void start_song(int songNum);

/* Turn of the running song. */
void stop_song();

/* Update state in the running song. 
 *
 * Returns the time in ms for when it shuld be run again. Warning, if not run
 * at thease intervals, sound might glich out. */
int64_t update_running_song();

#endif //ALARM_H_
