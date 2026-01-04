//
// unit tests for ringbuffer functionality
//

#include "test.h"
#include "tiny_nmea/internal/ringbuf.h"

#include <string.h>

// test ringbuf initialization
static void test_ringbuf_init(void) {
  TEST_CASE("ringbuf init") {
    uint8_t buf[64];
    ringbuf_t rb;

    ringbuf_init(&rb, buf, sizeof(buf));

    TEST_ASSERT(ringbuf_empty(&rb));
    TEST_ASSERT(!ringbuf_full(&rb));
    TEST_ASSERT_EQ(0, ringbuf_len(&rb));
    // one slot reserved for full/empty distinction
    TEST_ASSERT_EQ(63, ringbuf_free(&rb));

    TEST_PASS();
  }
}

// test basic push and pop
static void test_ringbuf_push_pop(void) {
  TEST_CASE("ringbuf push and pop basic") {
    uint8_t buf[16];
    ringbuf_t rb;
    ringbuf_init(&rb, buf, sizeof(buf));

    const uint8_t data[] = "hello";
    size_t written = ringbuf_push(&rb, data, 5, RINGBUF_PUSH_DROP);
    TEST_ASSERT_EQ(5, written);
    TEST_ASSERT_EQ(5, ringbuf_len(&rb));
    TEST_ASSERT(!ringbuf_empty(&rb));

    uint8_t out[8] = {0};
    size_t read = ringbuf_pop(&rb, out, sizeof(out));
    TEST_ASSERT_EQ(5, read);
    TEST_ASSERT_MEM_EQ("hello", out, 5);
    TEST_ASSERT(ringbuf_empty(&rb));

    TEST_PASS();
  }
}

// test wraparound behavior
static void test_ringbuf_wraparound(void) {
  TEST_CASE("ringbuf wraparound") {
    uint8_t buf[8];
    ringbuf_t rb;
    ringbuf_init(&rb, buf, sizeof(buf));

    // fill partially
    const uint8_t data1[] = "abcde";
    ringbuf_push(&rb, data1, 5, RINGBUF_PUSH_DROP);

    // pop some to move tail
    uint8_t tmp[3];
    ringbuf_pop(&rb, tmp, 3);
    TEST_ASSERT_MEM_EQ("abc", tmp, 3);

    // push more to cause wraparound
    const uint8_t data2[] = "12345";
    size_t written = ringbuf_push(&rb, data2, 5, RINGBUF_PUSH_DROP);
    TEST_ASSERT_EQ(5, written);

    // verify data integrity across wrap
    uint8_t out[8] = {0};
    size_t read = ringbuf_pop(&rb, out, 7);
    TEST_ASSERT_EQ(7, read);
    TEST_ASSERT_MEM_EQ("de12345", out, 7);

    TEST_PASS();
  }
}

// test push modes
static void test_ringbuf_push_modes(void) {
  TEST_CASE("ringbuf push mode atomic") {
    uint8_t buf[8];
    ringbuf_t rb;
    ringbuf_init(&rb, buf, sizeof(buf));

    // fill most of buffer
    ringbuf_push(&rb, (uint8_t *)"abcdef", 6, RINGBUF_PUSH_DROP);
    TEST_ASSERT_EQ(6, ringbuf_len(&rb));

    // try to push more than fits with atomic mode - should fail
    size_t written = ringbuf_push(&rb, (uint8_t *)"xyz", 3, RINGBUF_PUSH_ATOMIC);
    TEST_ASSERT_EQ(0, written);
    TEST_ASSERT_EQ(6, ringbuf_len(&rb));

    TEST_PASS();
  }

  TEST_CASE("ringbuf push mode drop") {
    uint8_t buf[8];
    ringbuf_t rb;
    ringbuf_init(&rb, buf, sizeof(buf));

    // fill most of buffer
    ringbuf_push(&rb, (uint8_t *)"abcdef", 6, RINGBUF_PUSH_DROP);

    // push more than fits with drop mode - should write partial
    size_t written = ringbuf_push(&rb, (uint8_t *)"xyz", 3, RINGBUF_PUSH_DROP);
    TEST_ASSERT_EQ(1, written);
    TEST_ASSERT_EQ(7, ringbuf_len(&rb));

    TEST_PASS();
  }

  TEST_CASE("ringbuf push mode wrap") {
    uint8_t buf[8];
    ringbuf_t rb;
    ringbuf_init(&rb, buf, sizeof(buf));

    // fill buffer
    ringbuf_push(&rb, (uint8_t *)"abcdefg", 7, RINGBUF_PUSH_DROP);
    TEST_ASSERT_EQ(7, ringbuf_len(&rb));

    // push with wrap mode - should overwrite old data
    size_t written = ringbuf_push(&rb, (uint8_t *)"XYZ", 3, RINGBUF_PUSH_WRAP);
    TEST_ASSERT_EQ(3, written);
    TEST_ASSERT_EQ(7, ringbuf_len(&rb));

    // verify old data was overwritten
    uint8_t out[8] = {0};
    ringbuf_pop(&rb, out, 7);
    TEST_ASSERT_MEM_EQ("defgXYZ", out, 7);

    TEST_PASS();
  }
}

// test peek operations
static void test_ringbuf_peek(void) {
  TEST_CASE("ringbuf peek") {
    uint8_t buf[16];
    ringbuf_t rb;
    ringbuf_init(&rb, buf, sizeof(buf));

    ringbuf_push(&rb, (uint8_t *)"hello world", 11, RINGBUF_PUSH_DROP);

    // peek without removing
    uint8_t out[8] = {0};
    size_t peeked = ringbuf_peek(&rb, out, 5, 0);
    TEST_ASSERT_EQ(5, peeked);
    TEST_ASSERT_MEM_EQ("hello", out, 5);

    // data should still be there
    TEST_ASSERT_EQ(11, ringbuf_len(&rb));

    // peek with offset
    memset(out, 0, sizeof(out));
    peeked = ringbuf_peek(&rb, out, 5, 6);
    TEST_ASSERT_EQ(5, peeked);
    TEST_ASSERT_MEM_EQ("world", out, 5);

    TEST_PASS();
  }

  TEST_CASE("ringbuf peek byte") {
    uint8_t buf[16];
    ringbuf_t rb;
    ringbuf_init(&rb, buf, sizeof(buf));

    ringbuf_push(&rb, (uint8_t *)"abcdef", 6, RINGBUF_PUSH_DROP);

    uint8_t byte;
    TEST_ASSERT(ringbuf_peek_byte(&rb, 0, &byte));
    TEST_ASSERT_EQ('a', byte);

    TEST_ASSERT(ringbuf_peek_byte(&rb, 3, &byte));
    TEST_ASSERT_EQ('d', byte);

    TEST_ASSERT(ringbuf_peek_byte(&rb, 5, &byte));
    TEST_ASSERT_EQ('f', byte);

    // out of range
    TEST_ASSERT(!ringbuf_peek_byte(&rb, 6, &byte));

    TEST_PASS();
  }
}

// test discard
static void test_ringbuf_discard(void) {
  TEST_CASE("ringbuf discard") {
    uint8_t buf[16];
    ringbuf_t rb;
    ringbuf_init(&rb, buf, sizeof(buf));

    ringbuf_push(&rb, (uint8_t *)"hello world", 11, RINGBUF_PUSH_DROP);

    // discard first 6 bytes
    size_t discarded = ringbuf_discard(&rb, 6);
    TEST_ASSERT_EQ(6, discarded);
    TEST_ASSERT_EQ(5, ringbuf_len(&rb));

    // verify remaining data
    uint8_t out[8] = {0};
    ringbuf_pop(&rb, out, 5);
    TEST_ASSERT_MEM_EQ("world", out, 5);

    TEST_PASS();
  }
}

// test clear
static void test_ringbuf_clear(void) {
  TEST_CASE("ringbuf clear") {
    uint8_t buf[16];
    ringbuf_t rb;
    ringbuf_init(&rb, buf, sizeof(buf));

    ringbuf_push(&rb, (uint8_t *)"test data", 9, RINGBUF_PUSH_DROP);
    TEST_ASSERT_EQ(9, ringbuf_len(&rb));

    ringbuf_clear(&rb);

    TEST_ASSERT(ringbuf_empty(&rb));
    TEST_ASSERT_EQ(0, ringbuf_len(&rb));

    TEST_PASS();
  }
}

// test full condition
static void test_ringbuf_full(void) {
  TEST_CASE("ringbuf full detection") {
    uint8_t buf[8];
    ringbuf_t rb;
    ringbuf_init(&rb, buf, sizeof(buf));

    TEST_ASSERT(!ringbuf_full(&rb));

    // fill to capacity (size - 1 due to sentinel)
    ringbuf_push(&rb, (uint8_t *)"1234567", 7, RINGBUF_PUSH_DROP);

    TEST_ASSERT(ringbuf_full(&rb));
    TEST_ASSERT_EQ(0, ringbuf_free(&rb));
    TEST_ASSERT_EQ(7, ringbuf_len(&rb));

    TEST_PASS();
  }
}

// test edge cases
static void test_ringbuf_edge_cases(void) {
  TEST_CASE("ringbuf empty pop") {
    uint8_t buf[8];
    ringbuf_t rb;
    ringbuf_init(&rb, buf, sizeof(buf));

    uint8_t out[4];
    size_t read = ringbuf_pop(&rb, out, sizeof(out));
    TEST_ASSERT_EQ(0, read);

    TEST_PASS();
  }

  TEST_CASE("ringbuf zero length push") {
    uint8_t buf[8];
    ringbuf_t rb;
    ringbuf_init(&rb, buf, sizeof(buf));

    size_t written = ringbuf_push(&rb, (uint8_t *)"test", 0, RINGBUF_PUSH_DROP);
    TEST_ASSERT_EQ(0, written);
    TEST_ASSERT(ringbuf_empty(&rb));

    TEST_PASS();
  }

  TEST_CASE("ringbuf pop with null output") {
    uint8_t buf[8];
    ringbuf_t rb;
    ringbuf_init(&rb, buf, sizeof(buf));

    ringbuf_push(&rb, (uint8_t *)"test", 4, RINGBUF_PUSH_DROP);

    // pop with null output should just discard
    size_t read = ringbuf_pop(&rb, NULL, 2);
    TEST_ASSERT_EQ(2, read);
    TEST_ASSERT_EQ(2, ringbuf_len(&rb));

    TEST_PASS();
  }
}

// test large data that requires multiple chunks
static void test_ringbuf_large_data(void) {
  TEST_CASE("ringbuf large data handling") {
    uint8_t buf[256];
    ringbuf_t rb;
    ringbuf_init(&rb, buf, sizeof(buf));

    // create pattern data
    uint8_t pattern[200];
    for (int i = 0; i < 200; i++) {
      pattern[i] = (uint8_t)(i & 0xFF);
    }

    size_t written = ringbuf_push(&rb, pattern, 200, RINGBUF_PUSH_DROP);
    TEST_ASSERT_EQ(200, written);

    uint8_t out[200] = {0};
    size_t read = ringbuf_pop(&rb, out, 200);
    TEST_ASSERT_EQ(200, read);
    TEST_ASSERT_MEM_EQ(pattern, out, 200);

    TEST_PASS();
  }
}

int main(void) {
  TEST_TITLE("ringbuffer tests");

  test_ringbuf_init();
  test_ringbuf_push_pop();
  test_ringbuf_wraparound();
  test_ringbuf_push_modes();
  test_ringbuf_peek();
  test_ringbuf_discard();
  test_ringbuf_clear();
  test_ringbuf_full();
  test_ringbuf_edge_cases();
  test_ringbuf_large_data();

  TEST_SUMMARY();
}
