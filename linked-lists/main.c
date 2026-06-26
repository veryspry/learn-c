#include "linked-list.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// goal fill up a double linked list with integers from 0 - 99

DLL *dll_new() {
  DLL *list = malloc(sizeof(DLL));

  if (list == NULL) {
    return NULL;
  }

  list->head = NULL;
  list->tail = NULL;
  list->length = 0;

  return list;
}

DLLNode *dllnode_new() {
  DLLNode *node = malloc(sizeof(DLLNode));
  if (node == NULL) {
    return NULL;
  }

  node->next = NULL;
  node->prev = NULL;

  return node;
}

void dll_free(DLL *list) {
  if (list == NULL) {
    return;
  }

  // DLLNode *node = list->head;

  while (list->head != NULL) {
    DLLNode *next = list->head->next;
    free(list->head);
    list->head = next;
  }

  free(list);
}

int main() {
  DLL *list = dll_new();

  if (list == NULL) {
    fprintf(stderr, "out of memory\n");
    return EXIT_FAILURE;
  }

  for (int i = 0; i < 100; i++) {
    DLLNode *node = malloc(sizeof(DLLNode));

    if (node == NULL) {
      dll_free(list);

      fprintf(stderr, "out of memory\n");
      return EXIT_FAILURE;
    }

    node->data = i;
    node->next = NULL;
    node->prev = list->tail;

    if (list->head == NULL) {
      list->head = node;
    } else {
      list->tail->next = node;
    }

    list->tail = node;
    list->length++;
  }

  printf("List length: %zu\n", list->length);

  DLLNode *node = list->head;
  while (node != NULL) {
    printf("%d\n", node->data);
    node = node->next;
  }

  dll_free(list);

  return 0;
}
