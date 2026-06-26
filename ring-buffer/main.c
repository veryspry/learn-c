#include "ring-buffer.h"
#include <stdio.h>
#include <stdlib.h>

#define STATIC_BUFFER_SIZE = 512

static uint8_t data[STATIC_BUFFER_SIZE];

int main() {
  Ring_Buffer *buf = malloc(sizeof(Ring_Buffer));

  buf->tail = 0;
  buf->head = 0;
  buf->capacity;


  return 0;
}
