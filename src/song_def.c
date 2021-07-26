#include <song_def.h>


// Definition of tones. These are choosen because they are loud on my buzzer.
// What works well with a different one might vary.
enum Tones {
  T_A4 = 440,  // Loudest
  T_B5 = 988,
  T_C5 = 523,
  T_D4 = 294,
  T_E6 = 1319,  // Also loud
  T_F4 = 349,  // Annoying sound
  T_F6 = 1397,
  T_G6 = 1568  // Nice but faint
};

#define LONG 500
#define MEDIUM 300
#define SHORT 150

// Definition of notes.
static const note_t A = {.freq=T_A4, .playDuration=MEDIUM, .waitDuration=MEDIUM};
static const note_t B = {.freq=T_B5, .playDuration=MEDIUM, .waitDuration=MEDIUM};
static const note_t C = {.freq=T_C5, .playDuration=MEDIUM, .waitDuration=MEDIUM};
static const note_t D = {.freq=T_D4, .playDuration=MEDIUM, .waitDuration=MEDIUM};
static const note_t E = {.freq=T_E6, .playDuration=MEDIUM, .waitDuration=MEDIUM};
static const note_t F = {.freq=T_F6, .playDuration=MEDIUM, .waitDuration=MEDIUM};
static const note_t G = {.freq=T_G6, .playDuration=MEDIUM, .waitDuration=MEDIUM};

static const note_t As = {.freq=T_A4, .playDuration=SHORT, .waitDuration=SHORT};
static const note_t Cs = {.freq=T_C5, .playDuration=SHORT, .waitDuration=MEDIUM};
static const note_t Es = {.freq=T_E6, .playDuration=SHORT, .waitDuration=SHORT};
static const note_t Fs = {.freq=T_F6, .playDuration=SHORT, .waitDuration=SHORT};
static const note_t Gs = {.freq=T_G6, .playDuration=SHORT, .waitDuration=SHORT};

static const note_t Al = {.freq=T_A4, .playDuration=LONG, .waitDuration=LONG};
static const note_t Cl = {.freq=T_C5, .playDuration=LONG, .waitDuration=LONG};
static const note_t Dl = {.freq=T_D4, .playDuration=LONG, .waitDuration=LONG};
static const note_t Gl = {.freq=T_G6, .playDuration=LONG, .waitDuration=LONG};

// Definition of songs. Every song can have an arbitrary amount of notes. Songs
// are seperated by NULL values.
note_t *songDeff[] = {
  &A, &E, NULL,
  &Al, NULL,
  &Gs, &Cs, &Gs, &Cs, &Gs, &Cs, &E, NULL,
  &C, &C, &G, &G, &A, &A, &Gl, &F, &F, &E, &E, &D, &D, &Cl, // twinkles
  &G, &G, &F, &F, &E, &E, &Dl, &G, &G, &F, &F, &E, &E, &Dl, 
  &C, &C, &G, &G, &A, &A, &Gl, &F, &F, &E, &E, &D, &D, &Cl, NULL
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
