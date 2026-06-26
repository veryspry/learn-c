#include <stddef.h>
#include <stdint.h>

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

typedef struct {
  size_t head;
  size_t tail;
  size_t capacity;
  size_t size_mask;
  uint8_t *buffer;
} Ring_Buffer;

#endif
