//
// Created by linyi on 1/4/2026.
//

#ifndef TINY_NMEA_RINGBUF_TYPE_H
#define TINY_NMEA_RINGBUF_TYPE_H

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint8_t *buf;
  size_t size;           // total buffer size
  _Atomic size_t head;   // write index (modified by producer)
  _Atomic size_t tail;   // read index (modified by consumer)
} ringbuf_t;


#endif //TINY_NMEA_RINGBUF_TYPE_H