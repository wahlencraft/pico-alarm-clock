#include <alarm.h>

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

static song_t *songList[SONGS];
static volatile song_state_t songState;

void sound_test(void) {
  datetime_t t1 = {
    .year = 1970,
    .month = 1,
    .day = 1,
    .dotw = 1, // 0 is Sunday
    .hour = 17,
    .min = 35,
    .sec = 2
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

  printf("Size: %d\n", sizeof(song_t));
  song_t *song1;
  song1 = songList[0];
  printf("Addr 0x%x, len %d\n", songList[0], song1->len);
  printf("Addr 0x%x, len %d\n", songList[1], songList[1]->len);
  node_add(alarms, &t1);
  node_print_all(alarms);
    
  printf("songList contents:\n");
  for (int i=0; i<SONGS; i++) {
    note_t **notes = songList[i]->notes;
    printf("  songList[%d]: 0x%x\n", i, songList[i]);
    for (int j=0; j<songList[i]->len; j++) { 
      note_t *note = *(notes+j);
      printf("    0x%x: 0x%x->(%d Hz, %d ms, %d ms)\n", notes+j, 
          note, note->freq, note->playDuration, note->waitDuration);
    }
  }
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

