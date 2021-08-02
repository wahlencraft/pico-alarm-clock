#include <alarm.h>

/*******************************************************************************
 *  Typedefs
 ******************************************************************************/
typedef struct SongState {
  note_t **start;  // Pointer to pointer of start of song
  int index;  // what tone (index) should be played?
  int phase;  // 0: play phase, 1: wait phase
} song_state_t;

/*******************************************************************************
 * Globals
 ******************************************************************************/
node_t *alarms;
static note_t ***songs;
static volatile song_state_t songState;
int SONGS;

/*******************************************************************************
 * Tests and initialization
 ******************************************************************************/
void sound_test(void) {
  datetime_t t1 = {
    .year = 1970,
    .month = 1,
    .day = 1,
    .dotw = 1, // 0 is Sunday
    .hour = 0,
    .min = 1,
    .sec = -1
  };
  datetime_t t2 = {
    .year = -1,
    .month = -1,
    .day = -1,
    .dotw = 1, // 0 is Sunday
    .hour = 17,
    .min = -1,
    .sec = 0
  };
  datetime_t t3 = {
    .year = -1,
    .month = -1,
    .day = -1,
    .dotw = 1, // 0 is Sunday
    .hour = 17,
    .min = 20,
    .sec = 0
  };
  init_alarms();

  node_print_all(alarms);
  add_alarm(&t1, 0, true);
  add_alarm(&t2, 0, true);
  add_alarm(&t3, 1, true);
  node_print_all(alarms);
  node_t *nextAlarm = malloc(sizeof(nextAlarm));
  if (is_alarm_in_1_min(nextAlarm)) {
    printf("True\n");
    printf("Found node ");
    node_print(nextAlarm);
  } else {
    printf("False\n");
  }
  free(nextAlarm);
  remove_alarm(&t1, NULL);
  node_print_all(alarms);
}

void init_alarms() {
  DEBUG_PRINT(("INIT ALARMS\n"));
  // LED
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  
  // PWM
  gpio_init(BUZ_PIN);
  gpio_set_dir(BUZ_PIN, GPIO_OUT);
  gpio_set_function(BUZ_PIN, GPIO_FUNC_PWM);
  uint slice_num = pwm_gpio_to_slice_num(BUZ_PIN);
  uint channel = pwm_gpio_to_channel(BUZ_PIN);
  pwm_set_wrap(slice_num, 31);
  pwm_set_chan_level(slice_num, channel, 15);
  
  printf("Print notes\n");
  for (int i=0; i<11; i++) {
    printf("0x%x ", allNotes[i]);
  }
  printf("\n");

  int index = 0;
  
  // Allocate
  SONGS = get_num_of_songs();
  DEBUG_PRINT(("Found %d songs\n", SONGS);)
  songs = malloc(sizeof(note_t**)*SONGS);
  printf("songs: 0x%x\n  ", songs);
  for (int song=0; song<SONGS; song++) {
    printf("songs[%d]: 0x%x  ", song, songs[song]);
  }
  printf("\n");

  for (int song=0; song<SONGS; song++) {
    songs[song] = &allNotes[index];
    while (allNotes[++index] != NULL) {}
    index++;
  }
  printf("songs: 0x%x\n  ", songs);
  for (int song=0; song<SONGS; song++) {
    printf("songs[%d]: 0x%x  ", song, songs[song]);
  }
  printf("\n");

  printf("Print songs (%d)\n", SONGS);
  for (int song=0; song<SONGS; song++) {
    note_t *note = *songs[song];
    int i = 1;
    do {
      printf(" |0x%x %d %d/%d|", note, note->freq, note->playDuration, note->waitDuration);
      note = *(songs[song] + i++);
    } while (note != NULL);
    printf("\n");
  }
}


/*******************************************************************************
 * LED functions
 ******************************************************************************/
static void drive_led_high(bool high) {
  gpio_put(LED_PIN, high);
}

void show_if_alarm_active(node_t *alarm) {
  gpio_put(LED_PIN, alarm->active);
}

void led_clear() {
  gpio_put(LED_PIN, false);
}

/*******************************************************************************
 * Song functions
 ******************************************************************************/
int get_number_of_songs() {
  return SONGS;
}

void start_song(int songNum) {
  songState.start = songs[songNum];
  songState.index = 0;
  songState.phase = 0;
}

void stop_song() {
  uint slice_num = pwm_gpio_to_slice_num(BUZ_PIN);
  pwm_set_enabled(slice_num, false);
  drive_led_high(false);
}

int64_t update_running_song(void) {
  // For every note this function is called twice.
  // What is done depends on the contents of the global struct songState
  //  songState.start: Pointer to pointer of first note in song.
  //  songState.index: What note that is playing.
  //  songState.phase: Phase for note (0 or 1).
  //    0. Play sound, return playDuration.
  //    1. Silent, return waitDuration.
  //note_t **notes = songList[songState.num]->notes;
  note_t *note = *(songState.start + songState.index);
  printf("songState: %d\n", songState.index);
  int64_t retval;
  uint32_t sysFreq = clock_get_hz(clk_sys);
  uint slice_num = pwm_gpio_to_slice_num(BUZ_PIN);
  if (songState.phase == 0) {
    pwm_set_clkdiv(slice_num, sysFreq/note->freq);
    pwm_set_enabled(slice_num, true);
    printf("  Play %d Hz for %d ms\n", note->freq, note->playDuration);
    songState.phase = 1;
    retval = note->playDuration;
    drive_led_high(true);
  } else {
    // phase 1
    pwm_set_enabled(slice_num, false);
    printf("  Silent for %d ms\n", note->waitDuration);
    songState.index = (*(songState.start+songState.index+1) == NULL) ? 
      0 : songState.index + 1;
    songState.phase = 0;
    retval = note->waitDuration;
    drive_led_high(false);
  }
  return retval;
}
/*******************************************************************************
 * Alarm functions
 ******************************************************************************/
int add_alarm(datetime_t *time, int song, bool active) {
  node_t *newAlarm = malloc(sizeof(node_t));
  newAlarm->time = malloc(sizeof(datetime_t));
  deep_copy_time(time, newAlarm->time);
  newAlarm->song = song;
  newAlarm->active = active;
# ifdef NDEBUG
  printf("Adding alarm:");
  node_print(newAlarm);
# endif
  if (node_add(&alarms, newAlarm)) {
    // Failed to add. Datetime occupied.
    DEBUG_PRINT(("Failed to add new alarm. There is already an alarm at this time.\n"));
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int get_next_alarm(node_t *alarm, bool restart) {
  static node_t *node = NULL;
  if (!is_alarms()) {
    return EXIT_FAILURE;
  }
  if (restart) {
    node = alarms;
  } else if (node == NULL) {
    printf("ERROR in function %s on line &d, never restarted\n", 
        __func__, __LINE__);
    return EXIT_FAILURE;
  } else if (node->next != NULL) {
    node = node->next;
  } else {
    return EXIT_FAILURE;
  }
  alarm->song = node->song;
  alarm->active = node->active;
  deep_copy_time(node->time, alarm->time);
  return EXIT_SUCCESS;
}

bool is_alarms() {
  if (node_is_empty(alarms)) {
    return false;
  } else {
    return true;
  }
}

bool is_alarm_in_1_min(node_t *nextNode) {
  datetime_t t_now;
  rtc_get_datetime(&t_now);
  if (!is_alarms()) {
    return false;
  }
  if (node_get_next_from_time(&t_now, alarms, nextNode)) {
    // There is no next node
    DEBUG_PRINT(("There is no next alarm.\n"));
    return false;
  }
  if (!(nextNode->active)) {
    DEBUG_PRINT(("Next alarm disabled\n"));
    return false;
  }
  increment_datetime(&t_now, 1);
  datetime_t *t_nextMin = &t_now;
  t_nextMin->sec = 0;  // increment datetime did not touch seconds.
  switch (compare_datetimes(nextNode->time, t_nextMin)) {
    case DATETIME_BEFORE:
    case DATETIME_SAME:
      nextNode = nextNode;
      return true;
    case DATETIME_AFTER:
      DEBUG_PRINT(("Next alarm is in more than 1 minute.\n"));
      nextNode = NULL;
      return false;
  }
}

void print_all_alarms() {
  printf("Printing all alarms.\n");
  node_print_all(alarms);
}

void remove_alarm(datetime_t *time, node_t *copy) {
  if (node_remove(&alarms, time, copy)) {
    // Failed to remove alarm. Alarm did not exist. TODO.
#   ifdef NDEBUG
      printf("  Tried to remove alarm, but alarm did not exist.\n");
      print_all_alarms();
#   endif
  }
}
