/**
 * node.h
 *
 * This is a ordered linked list containing datetime objects and songs (int). 
 * Every node is ordered from earliest to latest. Only one instance of the same 
 * datetime can exist. Only dotw, hour, min, sec are considered and the week 
 * starts with Monday. 
 **/

#ifndef NODE_H_
#define NODE_H_

#include <stdio.h>
#include <stdlib.h>

#include <pico/stdlib.h>
#include <hardware/clocks.h>
#include <PicoTM1637.h>

#include <helpers.h>

typedef struct Node node_t;

int node_test(void);

/* Create a new node. */
node_t *node_create();

/* Add a new datetime item to the node. It will automatically be placed
 * cronologically. */
int node_add(node_t *head, datetime_t *time, int song);

/* Return 1 if this array has no data. */
int node_is_empty(node_t *head);

void node_print_all(node_t *head);

/* Remove a datetime item based of value. */
int node_remove(node_t *head, datetime_t *time);

#endif //SOUND_H_
