/** 
 * song_deff.h 
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

//typedef struct Song {
//  note_t **notes;
//  int len;
//} song_t;

const int SONGS;

note_t **allNotes;

//void alloc_songs(song_t **songList);

#endif //SONG_DEFF_H_
