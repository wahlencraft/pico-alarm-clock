#include <song_deff.h>

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

// Definition of notes.
static const note_t A = {.freq=TONE_A, .playDuration=300, .waitDuration=300};
static const note_t B = {.freq=TONE_B, .playDuration=300, .waitDuration=300};
static const note_t C = {.freq=TONE_C, .playDuration=300, .waitDuration=200};
static const note_t D = {.freq=TONE_D, .playDuration=300, .waitDuration=300};
static const note_t E = {.freq=TONE_E, .playDuration=300, .waitDuration=300};
static const note_t F = {.freq=TONE_F, .playDuration=300, .waitDuration=300};
static const note_t G = {.freq=TONE_G, .playDuration=300, .waitDuration=200};
static const note_t G_long = {.freq=TONE_G, .playDuration=500, .waitDuration=300};
static const note_t C_long = {.freq=TONE_C, .playDuration=500, .waitDuration=300};

// Number of songs. It's important to update this if songDeff is changed.
const int SONGS = 4;

// Devinition of songs. Every song can have an arbitrary amount of notes. Songs
// are seperated by NULL values.
note_t *songDeff[] = {  
  &A, &A, &A, NULL,     
  &B, NULL,
  &C, &C, &C, &C, NULL,
  &A, &B, &C, NULL
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
