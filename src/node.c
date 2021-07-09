#include <node.h>

void node_print_all(node_t *head) {
  printf("Printing nodes:\n");
  if (head == NULL) {
    // list is empty
    printf("  List is empty\n");
    return;
  }
  node_t *current = head;
  while (current != NULL) {
    node_print(current);
    current = current->next;
  }
}

void node_print(node_t *node) {
  if (node == NULL) {
    // list is empty
    printf("  List is empty\n");
    return;
  }
  printf("  time [0x%x]: D%d %02d:%02d:%02d song: %d, active: %d, addr: 0x%x, next: 0x%x\n",
      node->time,
      node->time->dotw, node->time->hour, node->time->min, node->time->sec,
      node->song, node->active, node, node->next
      );
}

int node_add(node_t **head, node_t *newNode) {
  DEBUG_PRINT(("Adding to list: "));
  node_print(newNode);
  if (node_is_empty(*head)) {
    // Singular case. This is an empty list
    DEBUG_PRINT(("List empty, adding first item\n"));
    *head = newNode;
    newNode->next = NULL;
    return 0;
    }
  int compCode = compare_datetimes((*head)->time, newNode->time);
  switch (compCode) {
    case DATETIME_AFTER:
      // Singular case. The new node is first
      newNode->next = *head;
      *head = newNode;
      return 0;
    case DATETIME_SAME:
      DEBUG_PRINT(("WARNING: New node at same time as head\n"));
      DEBUG_PRINT(("  New:"));
      node_print(newNode);
      DEBUG_PRINT(("  Head:"));
      node_print(*head);
      return 1;
  }
  node_t *last = *head;
  node_t *current = (*head)->next;
  while (current != NULL) {
    compCode = compare_datetimes(current->time, newNode->time);
    switch (compCode) {
      case DATETIME_BEFORE:
        // Continue forward in the list
        last = current;
        current = current->next;
        break;
      case DATETIME_SAME:
        DEBUG_PRINT(("WARNING: New node at same time as other ("));
        print_time(newNode->time, 1);
        DEBUG_PRINT((") \n"));
        return 1;
      case DATETIME_AFTER:
        // Insert in list
        DEBUG_PRINT(("inserting\n"));
        last->next = newNode;
        newNode->next = current;
        return 0;
      default:
        printf("ERROR in function %s at line %d: impossible compCode\n",
            __func__, __LINE__);
        return 1;
    }
  }
  // Run out of list. Append at end
  DEBUG_PRINT(("Appending\n"));
  last->next = newNode;
  newNode->next = NULL;
}

int node_get_next_from_time(datetime_t *time, node_t *head, node_t *foundNode) {
  node_t *current = head;
  while (current != NULL) {
    int dateStatus = compare_datetimes(current->time, time);
    switch (dateStatus) {
      case DATETIME_BEFORE:
      case DATETIME_SAME:
        current = current->next;
        break;
      case DATETIME_AFTER:
        *foundNode = *current;
        return EXIT_SUCCESS;
      default:
        DEBUG_PRINT(("ERROR in node_get_next_from_time, impossible state."));
        return EXIT_FAILURE;
      }
  }
  // We run out of nodes. There is no node after the specified time.
  printf(" Run out of nodes.\n");
  foundNode = NULL;
  return EXIT_FAILURE;
}

// TODO. This needs to be rewritten.
int node_remove(node_t *head, datetime_t *time, node_t *copy) {
  if (compare_datetimes(head->time, time) == DATETIME_SAME) {
    // Special case, removing head.
    DEBUG_PRINT(("Removing head\n"));
    if (copy != NULL) {
      copy->time = head->time;
      copy->song = head->song;
      copy->active = head->active;
      copy->next = NULL; // The copy is standalone and not in the linked list.
    }
    if (head->next == NULL) {
      // List will become empty
      head->next = head;
      return EXIT_SUCCESS;
    } else {
      // Change the content of head to be that of it's child, then remove child.
      node_t *child = head->next;
      head->time = child->time;
      head->song = child->song;
      head->active = child->active;
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
        if (copy != NULL) {
          copy->time = current->time;
          copy->song = current->song;
          copy->active = current->active;
          copy->next = NULL; // The copy is standalone and not in the linked list.
        }
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
  return (head == NULL);
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
}

