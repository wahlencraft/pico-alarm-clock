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

typedef struct Node {
  datetime_t *time;
  int song;
  bool active;
  struct Node *next;
} node_t;

/* Add a new node to the list. It will be placed cronologically. */
int node_add(node_t **head, node_t *newNode);

/* Find the first node with a time after `time`.
 *
 * Returns 1 if list ends early.
 * - @param time is the time from which the next node should be found.
 * - @param head is the head of the list to look in.
 * - @param nextNode is a pointer in which the found node will be put. Will be
 *   NONE if list ends early.*/
int node_get_next_from_time(datetime_t *time, node_t *head, node_t *nextNode);

/* Return true if node (alarm) is activated. */
inline bool node_is_active(node_t *node) {
  return node->active;
}

/* Return 1 if this array has no items. */
int node_is_empty(node_t *head);

/* Print all nodes.*/
void node_print_all(node_t *head);

/* Print one node. Indent is spaces before print. */
void node_print(node_t *node, int indent);

/* Remove the node with datetime @param time. 
 *
 * Fails if node not found.*/
int node_remove(node_t **head, datetime_t *time, node_t *copy);

#endif //SOUND_H_
