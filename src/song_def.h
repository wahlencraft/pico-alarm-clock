/** 
 * song_def.h 
**/

#ifndef SONG_DEFF_H_
#define SONG_DEFF_H_

#include <stddef.h>
#include <helpers.h>

typedef const struct Note {
  int freq;
  int playDuration;
  int waitDuration;
} note_t;

note_t **allNotes;

int get_num_of_songs();

#endif //SONG_DEFF_H_
