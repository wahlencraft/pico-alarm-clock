#ifndef HELPERS_H_
#define HELPERS_H_

/* Increment a datetime struct. Set year, month and day to -1.
 *
 * datetime   Pointer to datetime struct.
 * startIndex Where to start incrementing. 0 for incrementing a second, 1 for 
 *              a minute etc. */
void increment_datetime(datetime_t *t, int startIndex);

#endif //HELPERS_H_
