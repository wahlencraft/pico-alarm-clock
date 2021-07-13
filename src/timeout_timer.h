/**
 * timeout_timer.h
 *
 * Start and check status of a timeout timer. This can be used to give
 * a max duration for the alarms so that they won't fire forever.
 **/

#ifndef TIMER_H_
#define TIMER_H_

#include <stdio.h>
#include <stdlib.h>

#include <pico/stdlib.h>

#include <helpers.h>
#include <pins.h>

#define ALARM_TIMEOUT 300  // How long an alarm should fire until alarm_timeout
                           // returns true.

/* Start the timeout timer. */
void start_alarm_timer();

/* Check if the alarm has fired for to long. */
bool alarm_timeout();

#endif //TIMER_H_
