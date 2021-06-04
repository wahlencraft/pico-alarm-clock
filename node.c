#include <stdio.h>
#include <stdlib.h>

#include <pico/stdlib.h>
#include <hardware/clocks.h>

#include <PicoTM1637.h>
#include <helpers.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define EMPTY_LIST -1

typedef struct Node {
  datetime_t *time;
  struct Node *next;
} node_t;

void print_list(node_t *head) {
    node_t *current = head;
    while (current != NULL) {
      printf("time: D%d %02d:%02d:%02d, addr: 0x%x, next: 0x%x\n", 
          current->time->dotw, current->time->hour, current->time->min, 
          current->time->sec, current, current->next);
      current = current->next;
    }
}

node_t *node_create(datetime_t *time) {
  node_t *head = NULL;
  head = (node_t *) malloc(sizeof(node_t));
  if (head == NULL) {
    return 1;
  }
  head->time = time;
  head->next = NULL;
  return head;
}

int node_add(node_t *head, datetime_t *time) {
  printf("Adding to list: ");
  node_t *current = head;
  while (true) {
    if (compare_datetimes(current->time, time) == DATETIME_AFTER) {
      // Insert in list
      printf("inserting\n");
      
      node_t *next_node = NULL;
      next_node = (node_t *) malloc(sizeof(node_t));
      next_node->time = current->time;
      next_node->next = current->next;

      current->time = time;
      current->next = next_node;
      return 0;
    } else if (current->next == NULL) {
      // Append to end of list
      printf("appending\n");
      current->next = (node_t *) malloc(sizeof(node_t));
      current->next->time = time;
      current->next->next = NULL;
      return 0;
    } else {
      current = current->next;
    }
  }
}

int node_remove(node_t *head, datetime_t *time) {
  if (compare_datetimes(head->time, time) == DATETIME_SAME) {
    // Special case, removing head.
    printf("Removing head\n");
    if (head->next == NULL) {
      // List will become empty, this must be passed on to the caller.
      free(head);
      return EMPTY_LIST;
    } else {
      // Change the content of head to be that of it's child, then remove child.
      node_t *child = head->next;
      head->time = child->time;
      head->next = child->next;
      free(child);
      return EXIT_SUCCESS;
    }
  }
  node_t *last = head;
  node_t *current = head->next;
  while (true) {
    // Iterate over every node until the one with matching time is found. Then
    // delete it. The list is ordered, so if a later time is found, exit.
    switch (compare_datetimes(current->time, time)) {
      case DATETIME_BEFORE:
        printf(" Wasn't [0x%x]\n", current);
        if (current->next != NULL) {
          last = current;
          current = current->next;
          break;
        } else {
          printf("Item not found. List ended early\n");
          return EXIT_FAILURE;
        }
      case DATETIME_SAME:
        printf(" Found item [0x%x], deleting\n", current);
        last->next = current->next;
        free(current);
        return EXIT_SUCCESS;
        case DATETIME_AFTER:
        // Time is later than requested. The wanted node does not exist.
        printf("Item not found!\n");
        return EXIT_FAILURE;
      default:
        // Should not happen
        printf("Compare datetimes error. Unknown case.");
        return EXIT_FAILURE;
    }
  }
}

int node_test(void) {
  printf("Node test\n");
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
  node_t *head = node_create(&t1);
  print_list(head);
  node_add(head, &t2);
  node_add(head, &t3);
  print_list(head);
  node_remove(head, &t1);
  print_list(head);

  int status = compare_datetimes(&t1, &t2);
  printf("Compare datetimes returned: %d\n", status);
}


