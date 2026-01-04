//
// Created by linyi on 12/29/2025.
//

#include "tiny_nmea/internal/ringbuf.h"

#include <stdatomic.h>
#include <string.h>

// macro to atomically load pointers
// and make the vars available in scope

// for observer functions that need a consistent snapshot
#define RINGBUF_LOAD(rb, head_order, tail_order) \
size_t head = atomic_load_explicit(&(rb)->head, head_order); \
size_t tail = atomic_load_explicit(&(rb)->tail, tail_order)

#define RINGBUF_LOAD_PRODUCER(rb) RINGBUF_LOAD(rb, memory_order_relaxed, memory_order_acquire)
#define RINGBUF_LOAD_CONSUMER(rb) RINGBUF_LOAD(rb, memory_order_acquire, memory_order_relaxed)
#define RINGBUF_LOAD_OBSERVER(rb) RINGBUF_LOAD(rb, memory_order_acquire, memory_order_acquire)

// implements the simple length logic to reduce repeated blocks
#define RINGBUF_COMPUTE_LENGTH(hd, tl) \
  (((hd) >= (tl)) ? (hd) - (tl) : rb->size - (tl) + (hd))

// implements get free space logic to reduce repeats
#define RINGBUF_COMPUTE_FREE_SPACE(hd, tl) \
  (rb->size - RINGBUF_COMPUTE_LENGTH(hd, tl) - 1)


void ringbuf_init(ringbuf_t *rb, uint8_t *buf, size_t size) {
  rb->buf = buf;
  rb->size = size;
  atomic_init(&rb->head, 0);
  atomic_init(&rb->tail, 0);
}

void ringbuf_clear(ringbuf_t *rb) {
  // clearing writes the head pointer to match tail
  // this is NOT thread-safe and should only be called
  // when you can guarantee no concurrent access
  const size_t tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
  atomic_store_explicit(&rb->head, tail, memory_order_relaxed);
}

size_t ringbuf_len(const ringbuf_t *rb) {
  // get snapshot of both pointers
  RINGBUF_LOAD_OBSERVER(rb);
  return RINGBUF_COMPUTE_LENGTH(head, tail);
}


size_t ringbuf_free(const ringbuf_t *rb) {
  // one slot kept empty to distinguish full from empty
  RINGBUF_LOAD_OBSERVER(rb);
  return RINGBUF_COMPUTE_FREE_SPACE(head, tail);
}


bool ringbuf_empty(const ringbuf_t *rb) {
  RINGBUF_LOAD_OBSERVER(rb);
  return head == tail;
}

bool ringbuf_full(const ringbuf_t *rb) {
  // consider ringbuf full if the head pointer will equal
  // the tail pointer if we advance by one (size-1 items)
  // because we cannot distinguish head == tail due to empty
  // or full otherwise
  RINGBUF_LOAD_OBSERVER(rb);
  return ((head + 1) % rb->size) == tail;
}

size_t ringbuf_push(ringbuf_t *rb, const uint8_t *data, size_t len, ringbuf_push_mode_t mode) {
  RINGBUF_LOAD_PRODUCER(rb);
  const size_t free_space = RINGBUF_COMPUTE_FREE_SPACE(head, tail);

  if (len > free_space) {
    switch (mode) {
      case RINGBUF_PUSH_ATOMIC:
        return 0;

      case RINGBUF_PUSH_DROP:
        len = free_space;
        break;

      case RINGBUF_PUSH_WRAP:
        // this mode is NOT SPSC-safe!
        // this modifies tail pointer, no lock-free guarantee
        // use with exclusive access

        if (len >= rb->size) {
          // incoming data larger than buffer
          // keep only the last (size-1) bytes
          data += len - (rb->size - 1);
          len = rb->size - 1;
          // reset buffer to empty state
          atomic_store_explicit(&rb->tail, 0, memory_order_relaxed);
          atomic_store_explicit(&rb->head, 0, memory_order_relaxed);
          // update local variables to match
          head = 0;
          tail = 0;
        } else {
          // advance tail to make room for new data
          // discard old data from the buffer
          ringbuf_discard(rb, len - free_space);
          // tail has changed, reload it
          tail = atomic_load_explicit(&rb->tail, memory_order_acquire);
        }
        break;
    }
  }

  if (len == 0) {
    return 0;
  }

  // copy in one or two chunks to handle wraparound
  size_t to_end = rb->size - head;
  if (to_end >= len) {
    memcpy(rb->buf + head, data, len);
  } else {
    memcpy(rb->buf + head, data, to_end);
    memcpy(rb->buf, data + to_end, len - to_end);
  }

  // publish the new data with release semantics
  // this ensures all data writes are visible before head update
  atomic_store_explicit(&rb->head, (head + len) % rb->size, memory_order_release);
  return len;
}

size_t ringbuf_pop(ringbuf_t *rb, uint8_t *data, size_t len) {
  RINGBUF_LOAD_CONSUMER(rb);
  const size_t avail = RINGBUF_COMPUTE_LENGTH(head, tail);
  if (len > avail) {
    len = avail;
  }

  if (len == 0) {
    return 0;
  }

  size_t to_end = rb->size - tail;

  if (data != NULL) {
    // check the user actually supplied a buffer
    if (to_end >= len) {
      memcpy(data, rb->buf + tail, len);
    } else {
      memcpy(data, rb->buf + tail, to_end);
      memcpy(data + to_end, rb->buf, len - to_end);
    }
  }

  // publish the freed space with release semantics
  // this ensures all data reads are complete before tail update
  atomic_store_explicit(&rb->tail, (tail + len) % rb->size, memory_order_release);
  return len;
}

size_t ringbuf_peek(const ringbuf_t *rb, uint8_t *data, size_t len, size_t offset) {
  RINGBUF_LOAD_OBSERVER(rb);
  const size_t avail = RINGBUF_COMPUTE_LENGTH(head, tail);

  if (offset >= avail) {
    return 0;
  }

  if (len > avail - offset) {
    len = avail - offset;
  }

  size_t start = (tail + offset) % rb->size;
  size_t to_end = rb->size - start;
  if (to_end >= len) {
    memcpy(data, rb->buf + start, len);
  } else {
    memcpy(data, rb->buf + start, to_end);
    memcpy(data + to_end, rb->buf, len - to_end);
  }

  return len;
}

bool ringbuf_peek_byte(const ringbuf_t *rb, size_t offset, uint8_t *out) {
  RINGBUF_LOAD_OBSERVER(rb);
  const size_t length = RINGBUF_COMPUTE_LENGTH(head, tail);
  if (offset >= length) {
    return false;
  }

  *out = rb->buf[(tail + offset) % rb->size];
  return true;
}

size_t ringbuf_discard(ringbuf_t *rb, size_t len) {
  RINGBUF_LOAD_CONSUMER(rb);
  const size_t avail = RINGBUF_COMPUTE_LENGTH(head, tail);
  if (len > avail) {
    len = avail;
  }

  // publish the freed space with release semantics
  atomic_store_explicit(&rb->tail, (tail + len) % rb->size, memory_order_release);
  return len;
}
