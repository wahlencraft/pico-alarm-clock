#include <alarm.h>

/*******************************************************************************
 *  Typedefs
 ******************************************************************************/
typedef const struct Note {
  int freq;
  int playDuration;
  int waitDuration;
} note_t;

typedef struct Song {
  note_t **notes;
  int len;
} song_t;

typedef struct SongState {
  int num;    // what song is it (from songList)
  int index;  // what tone (index) should be played?
  int phase;  // 0: play phase, 1: wait phase
} song_state_t;

/*******************************************************************************
 * Globals
 ******************************************************************************/
#define SONGS 3

enum Tones {
  TONE_CM = 262,
  TONE_D = 294,
  TONE_E = 330,
  TONE_F = 349,
  TONE_G = 392,
  TONE_A = 440,
  TONE_B = 498,
  TONE_C = 523
};

node_t *alarms;
static song_t *songList[SONGS];
static volatile song_state_t songState;

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
  add_alarm(&t1, 0);
  add_alarm(&t2, 0);
  add_alarm(&t3, 1);
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
  remove_alarm(&t1);
  node_print_all(alarms);
}

void init_alarms() {
  // PWM
  gpio_set_function(BUZ_PIN, GPIO_FUNC_PWM);
  uint slice_num = pwm_gpio_to_slice_num(BUZ_PIN);
  uint channel = pwm_gpio_to_channel(BUZ_PIN);
  pwm_set_wrap(slice_num, 31);
  pwm_set_chan_level(slice_num, channel, 15);

  alarms = node_create(); // to store alarms
  
  // Create songs
  static const note_t A = {.freq=TONE_A, .playDuration=300, .waitDuration=300};
  static const note_t B = {.freq=TONE_B, .playDuration=300, .waitDuration=300};
  static const note_t C = {.freq=TONE_C, .playDuration=300, .waitDuration=200};
  static const note_t D = {.freq=TONE_D, .playDuration=300, .waitDuration=300};
  static const note_t E = {.freq=TONE_E, .playDuration=300, .waitDuration=300};
  static const note_t F = {.freq=TONE_F, .playDuration=300, .waitDuration=300};
  static const note_t G = {.freq=TONE_G, .playDuration=300, .waitDuration=200};
  static const note_t G_long = {.freq=TONE_G, .playDuration=500, .waitDuration=300};
  static const note_t C_long = {.freq=TONE_C, .playDuration=500, .waitDuration=300};

  printf(" A = %d Hz [0x%x]\n C = %d Hz [0x%x]\n D = %d Hz [0x%x]\n G = %d Hz [0x%x]\n",
      A.freq, &A, C.freq, &C, D.freq, &D, G.freq, &G);
  
  #define len0 4
  static note_t *arr0[len0] = {&A, &A, &A, &A};

  #define len1 8
  static note_t *arr1[len1] = {&C, &C, &C, &C, &G, &G, &G, &G};
  
  #define len2 8
  static note_t *arr2[len2] = {&D, &D, &D, &D, &D, &D, &D, &D};
  
  for (int i=0; i<SONGS; i++) {
    songList[i] = (song_t *) malloc(sizeof(song_t));
  }
  songList[0]->notes = arr0;
  songList[0]->len = len0;
  
  songList[1]->notes = arr1;
  songList[1]->len = len1;

  songList[2]->notes = arr2;
  songList[2]->len = len2;

  printf("songList contents:\n");
  for (int i=0; i<SONGS; i++) {
    note_t **notes = songList[i]->notes;
    printf("  songList[%d]: 0x%x\n", i, songList[i]);
    for (int j=0; j<songList[i]->len; j++) { 
      note_t *note = *(notes+j);
      printf("    0x%x: 0x%x->%d\n", notes+j, 
          note, note->freq);
    }
  }
}

/*******************************************************************************
 * Song functions
 ******************************************************************************/
void start_song(int songNum) {
  songState.num = songNum;
  songState.index = 0;
  songState.phase = 0;
}

void stop_song() {
  uint slice_num = pwm_gpio_to_slice_num(BUZ_PIN);
  pwm_set_enabled(slice_num, false);
}

int64_t update_running_song(void) {
  // For every note this function is called twice.
  // What is done depends on the contents of the global struct songState
  //  songState.num: The song to play (from songList). Is constant for a song.
  //  songState.index: What note that is playing.
  //  songState.phase: Phase for note (0 or 1).
  //    0. Play sound, return playDuration.
  //    1. Silent, return waitDuration.
  //note_t **notes = songList[songState.num]->notes;
  note_t *note = *(songList[songState.num]->notes + songState.index);
  printf("songState: %d\n", songState.index);
  int64_t retval;
  uint32_t sysFreq = clock_get_hz(clk_sys);
  uint slice_num = pwm_gpio_to_slice_num(BUZ_PIN);
  if (songState.phase == 0) {
    pwm_set_clkdiv(slice_num, sysFreq/note->freq);
    pwm_set_enabled(slice_num, true);
    printf("Play %d Hz for %d ms\n", note->freq, note->playDuration);
    songState.phase = 1;
    retval = note->playDuration;
  } else {
    // phase 1
    pwm_set_enabled(slice_num, false);
    printf("Silent for %d ms\n", note->waitDuration);
    songState.index = (songState.index == songList[songState.num]->len - 1) ? 
      0 : songState.index + 1;
    songState.phase = 0;
    retval = note->waitDuration;
  }
  return retval;
}

/*******************************************************************************
 * Alarm functions
 ******************************************************************************/
void add_alarm(datetime_t *time, int song) {
  if (node_add(alarms, time, song)) {
    // Failed to add. Datetime occupied.
    // TODO
    printf("Failed to add new alarm. There is already an alarm at this time.\n");
  }
}

bool is_alarm_in_1_min(node_t *nextNode) {
  datetime_t t_now;
  rtc_get_datetime(&t_now);
  printf("node_find_next (from D%d %d:%d)\n", t_now.dotw, t_now.hour, t_now.min);
  if (node_find_next(&t_now, alarms, nextNode)) {
    // There is no next node
    DEBUG_PRINT(("There is no next alarm.\n"));
    return false;
  }
  increment_datetime(&t_now, 1);
  datetime_t *t_nextMin = &t_now;
  t_nextMin->sec = 0;  // increment datetime did not touch seconds.
  printf("Next min is D%d %d:%d:%d\n", 
      t_nextMin->dotw, t_nextMin->hour,
      t_nextMin->min, t_nextMin->sec);
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

void remove_alarm(datetime_t *time) {
  if (node_remove(alarms, time)) {
    // Failed to remove alarm. Alarm did not exist. TODO.
    DEBUG_PRINT(("Tried to remove alarm, but alarm did not exist."));
  }
}
