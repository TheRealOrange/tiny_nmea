//
// Created by linyi on 12/29/2025.
//

// a (hopefully) lock-free ring-buffer implementation?
// adhering to single-producer single-consumer logic

#ifndef RINGBUF_H
#define RINGBUF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ringbuf_type.h"

typedef enum {
  RINGBUF_PUSH_DROP,   // drop bytes that don't fit
  RINGBUF_PUSH_WRAP,   // overwrite oldest data to fit entire write, NOT SPSC-safe, needs mutex or single-threaded
  RINGBUF_PUSH_ATOMIC  // all or nothing
} ringbuf_push_mode_t;

/**
 * init ringbuf with user-provided storage
 */
void ringbuf_init(ringbuf_t *rb, uint8_t *buf, size_t size);

/**
 * clear ringbuf to empty
 * NOTE: not thread-safe, should only be called when no concurrent access
 */
void ringbuf_clear(ringbuf_t *rb);

/**
 * current num of bytes in ringbuf
 */
size_t ringbuf_len(const ringbuf_t *rb);

/**
 * bytes remaining in ringbuf
 */
size_t ringbuf_free(const ringbuf_t *rb);

/**
 * check if ringbuf empty
 */
bool ringbuf_empty(const ringbuf_t *rb);

/**
 * check if ringbuf full
 */
bool ringbuf_full(const ringbuf_t *rb);

/**
 * push data into ringbuf (producer operation)
 * @param rb    ringbuf
 * @param data  data to push
 * @param len   num of bytes
 * @param mode  behavior when data does not fit
 *              PUSH_WRAP breaks SPSC thread-safety (needs mutex or single-threaded)
 * @return      bytes actually written
 */
size_t ringbuf_push(ringbuf_t *rb, const uint8_t *data, size_t len, ringbuf_push_mode_t mode);

/**
 * pop data from ringbuf (removes from buffer) (consumer operation)
 * @param rb    ringbuf
 * @param data  dest buffer (NULL to just discard)
 * @param len   max bytes to pop
 * @return      bytes actually popped
 */
size_t ringbuf_pop(ringbuf_t *rb, uint8_t *data, size_t len);

/**
 * peek at data without removing
 * @param rb      ringbuf
 * @param data    dest buffer
 * @param len     max bytes to peek
 * @param offset  offset from tail (0 = oldest byte)
 * @return        bytes actually copied
 */
size_t ringbuf_peek(const ringbuf_t *rb, uint8_t *data, size_t len, size_t offset);

/**
 * peek single byte at offset
 * @param rb      ringbuf
 * @param offset  offset from tail
 * @param out     output byte
 * @return        true if valid offset
 */
bool ringbuf_peek_byte(const ringbuf_t *rb, size_t offset, uint8_t *out);

/**
 * discard bytes from front of ringbuf (consumer operation)
 * @param rb    ringbuf
 * @param len   bytes to discard
 * @return      bytes actually discarded
 */
size_t ringbuf_discard(ringbuf_t *rb, size_t len);

#endif //RINGBUF_H