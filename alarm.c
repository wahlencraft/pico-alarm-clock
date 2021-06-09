#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <pico/stdlib.h>
#include <hardware/clocks.h>
#include <hardware/pwm.h>

#include <PicoTM1637.h>
#include <node.h>

#define PIN 18
#define TONE_END -1
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
  const int waitDuration;
} note_t;

typedef struct Song {
  note_t **notes;
  int len;
} song_t;

static song_t *songList[SONGS];

void sound_test(void) {
  uint32_t sysFreq = clock_get_hz(clk_sys);
  printf("System clock is at %d kHz\n", sysFreq/1000);

  gpio_set_function(PIN, GPIO_FUNC_PWM);
  gpio_set_function(PIN+1, GPIO_FUNC_PWM);

  uint slice_num = pwm_gpio_to_slice_num(PIN);

  pwm_set_wrap(slice_num, 15);
  pwm_set_chan_level(slice_num, PWM_CHAN_A, 8);  

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
  init_alarm();

  printf("Size: %d\n", sizeof(song_t));
  song_t *song1;
  song1 = songList[0];
  printf("Addr 0x%x, len %d\n", songList[0], song1->len);
  printf("Addr 0x%x, len %d\n", songList[1], songList[1]->len);
  //play(songList[0]);
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

void init_alarm() {
  alarms = node_create();

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

void update_alarm(void) {
}

