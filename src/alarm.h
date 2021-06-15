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

/* Add a new alarm.
 *
 * - @param *time: The datetime for when the alarm should fire. Only dotw,
 *   hour, min and sec are important.
 * - @param song: The song that should be played when the alarm fires. The
 *   argument is an index in the songList array defined in `init_alarms`. */
void add_alarm(datetime_t *time, int song);

/* Check if there is an alarm before specified time. Get that alarm.
 *
 * Returns true if *current time* < *next alarm* <= @param time.
 * If true @parmam returnNode will point to said alarm.*/
bool is_alarm_before(datetime_t *time, node_t *returnNode);

/* Remove the alarm at @param time. */
void remove_alarm(datetime_t *time);

#endif //ALARM_H_
