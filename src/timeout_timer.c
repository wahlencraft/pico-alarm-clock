#include <timeout_timer.h>

static absolute_time_t waitTime;

void start_alarm_timer() {
  absolute_time_t currentTime = get_absolute_time();
  waitTime = delayed_by_ms(currentTime, (int32_t) ALARM_TIMEOUT*1000);
}

bool alarm_timeout() {
  absolute_time_t currentTime = get_absolute_time();
  return (currentTime >= waitTime) ? true : false;
}
