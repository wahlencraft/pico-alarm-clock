#include <node.h>

typedef struct Node {
  datetime_t *time;
  int song;
  struct Node *next;
} node_t;

void node_print_all(node_t *head) {
    if (head->next == head) {
      // list is empty
      printf("time: NULL song: NULL, addr: 0x%x, next: 0x%x\n",
          head, head->next);
      return;
    }
    node_t *current = head;
    while (current != NULL) {
      printf("time: D%d %02d:%02d:%02d song: %d, addr: 0x%x, next: 0x%x\n", 
          current->time->dotw, current->time->hour, current->time->min, 
          current->time->sec, current->song, current, current->next);
      current = current->next;
    }
}

node_t *node_create() {
  node_t *head = NULL;
  head = (node_t *) malloc(sizeof(node_t));
  if (head == NULL) {
    return 1;
  }
  head->time = NULL;
  head->song = -1;  // nonsense value
  head->next = head;
  return head;
}

int node_add(node_t *head, datetime_t *time, int song) {
  DEBUG_PRINT(("Adding to list: "));
  if (node_is_empty(head)) {
    // Singular case. This is an empty list
    DEBUG_PRINT(("First item in list\n"));
    head->time = time;
    head->song = song;
    head->next = NULL;
    return 0;
    }
  node_t *current = head;
  while (true) {
    if (compare_datetimes(current->time, time) == DATETIME_AFTER) {
      // Insert in list
      DEBUG_PRINT(("inserting\n"));
      
      node_t *next_node = NULL;
      next_node = (node_t *) malloc(sizeof(node_t));
      next_node->time = current->time;
      next_node->song = current->song;
      next_node->next = current->next;

      current->time = time;
      current->song = song;
      current->next = next_node;
      return 0;
    } else if (compare_datetimes(current->time, time) == DATETIME_SAME) {
      // This date is already in the list. There can only be one alarm
      // per datetime, so this should not happen. Pass error to caller.
      DEBUG_PRINT(("WARNING! Can't add alarm, datetime already exists.\n"));
      return 1;
    } else if (current->next == NULL) {
      // Append to end of list
      DEBUG_PRINT(("appending\n"));
      current->next = (node_t *) malloc(sizeof(node_t));
      current->next->time = time;
      current->next->song = song;
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
    DEBUG_PRINT(("Removing head\n"));
    if (head->next == NULL) {
      // List will become empty
      head->next = head;
      return EXIT_SUCCESS;
    } else {
      // Change the content of head to be that of it's child, then remove child.
      node_t *child = head->next;
      head->time = child->time;
      head->song = child->song;
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
        DEBUG_PRINT((" Wasn't [0x%x]\n", current));
        if (current->next != NULL) {
          last = current;
          current = current->next;
          break;
        } else {
          DEBUG_PRINT(("Item not found. List ended early\n"));
          return EXIT_FAILURE;
        }
      case DATETIME_SAME:
        DEBUG_PRINT((" Found item [0x%x], deleting\n", current));
        last->next = current->next;
        free(current);
        return EXIT_SUCCESS;
        case DATETIME_AFTER:
        // Time is later than requested. The wanted node does not exist.
        DEBUG_PRINT(("Item not found!\n"));
        return EXIT_FAILURE;
      default:
        // Should not happen
        DEBUG_PRINT(("Compare datetimes error. Unknown case."));
        return EXIT_FAILURE;
    }
  }
}

int node_is_empty(node_t *head) {
  return (head->next == head);
}

int node_test(void) {
  DEBUG_PRINT(("-NODE TEST-\n"));
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
  node_t *head = node_create();
  node_print_all(head);
  if (node_add(head, &t2, 0)) {
    printf("Failed to add t2\n");
  }
  if (node_add(head, &t3, 1)) {
    printf("Failed to add t3\n");
  }
  if (node_add(head, &t3, 4)) {
    printf("Failed to add t3\n");
  }
  node_print_all(head);
  node_remove(head, &t2);
  node_remove(head, &t3);
  node_print_all(head);
  node_add(head, &t1, -1);
  node_print_all(head);

  int status = compare_datetimes(&t1, &t2);
  printf("Compare datetimes returned: %d\n", status);
}


