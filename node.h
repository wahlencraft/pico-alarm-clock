#ifndef NODE_H_
#define NODE_H_

/* Node
 *
 * This is a ordered linked list containing datetime objects. Every node
 * is ordered from earliest to latest. Only one instance of the same date can
 * exist. Only dotw, hour, min, sec are considered and the week starts with 
 * Monday. */

typedef struct Node node_t;

void node_test(void);

/* Create a new node. */
node_t *node_create();

/* Add a new datetime item to the node. It will automatically be placed
 * cronologically. */
int node_add(node_t *head, datetime_t *time);

/* Return 1 if this array has no data. */
int node_is_empty(node_t *head);

void node_print_all(node_t *head);

/* Remove a datetime item based of value. */
int node_remove(node_t *head, datetime_t *time);

#endif //SOUND_H_
