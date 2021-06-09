#ifndef ALARM_H_
#define ALARM_H_

/** 
 * alarm.h
 * 
 * Start, maintain and stop the alarm.
**/

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
