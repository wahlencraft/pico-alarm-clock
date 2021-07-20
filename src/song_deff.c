#include <song_deff.h>

enum Tones {
  TONE_D = 294,
  TONE_E = 330,
  TONE_F = 349,
  TONE_A4 = 440,
  TONE_C = 523,
  TONE_A7 = 3520,
  TONE_B5 = 988,
  TONE_C6 = 1047,
  TONE_D5 = 587,
  TONE_E5 = 659,
  TONE_F5 = 698,
  TONE_G5 = 784
};

// Definition of notes.
static const note_t A4 = {.freq=440, .playDuration=300, .waitDuration=300};
static const note_t B5 = {.freq=988, .playDuration=300, .waitDuration=300};
static const note_t C5 = {.freq=523, .playDuration=300, .waitDuration=300};
static const note_t D4 = {.freq=294, .playDuration=300, .waitDuration=300};
static const note_t E6 = {.freq=1319, .playDuration=300, .waitDuration=300};
static const note_t F4 = {.freq=349, .playDuration=300, .waitDuration=300};
static const note_t F6 = {.freq=1397, .playDuration=300, .waitDuration=300};
static const note_t G6 = {.freq=1568, .playDuration=300, .waitDuration=300};

// Devinition of songs. Every song can have an arbitrary amount of notes. Songs
// are seperated by NULL values.
note_t *songDeff[] = {
  &A4, NULL,
  &B5, NULL,
  &C5, NULL,
  &D4, NULL,
  &E6, NULL,
  &F4, NULL,
  &F6, NULL,
  &G6, NULL
};

int get_num_of_songs() {
  int count = 0;
  for (int i=0; i<sizeof(songDeff)/sizeof(note_t*); i++) {
    if (songDeff[i] == NULL) {
      count++;
    }
  }
  return count;
}

note_t **allNotes = songDeff;
