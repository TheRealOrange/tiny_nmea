//
// simple test framework for tiny_nmea
// no external dependencies
//

#ifndef TINY_NMEA_TEST_H
#define TINY_NMEA_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// ansi color codes
#define TEST_COLOR_RED     "\033[31m"
#define TEST_COLOR_GREEN   "\033[32m"
#define TEST_COLOR_YELLOW  "\033[33m"
#define TEST_COLOR_BLUE    "\033[34m"
#define TEST_COLOR_RESET   "\033[0m"

// test statistics
static int test_total = 0;
static int test_passed = 0;
static int test_failed = 0;
static const char *test_current_suite = NULL;

// start a test
#define TEST_TITLE(name) do { \
  test_current_suite = (name); \
  printf(TEST_COLOR_BLUE "[TEST: %s]" TEST_COLOR_RESET "\n", name); \
} while (0)

// run a single test case
#define TEST_CASE(name) \
  for (int _tc_done = (printf("  %-50s ", name), test_total++, 0); \
       _tc_done == 0; \
       _tc_done = 1)

// mark test as passed (call at end of successful test)
#define TEST_PASS() do { \
  test_passed++; \
  printf(TEST_COLOR_GREEN "[PASS]" TEST_COLOR_RESET "\n"); \
} while (0)

// mark test as failed with message
#define TEST_FAIL(fmt, ...) do { \
  test_failed++; \
  printf(TEST_COLOR_RED "[FAIL]" TEST_COLOR_RESET "\n"); \
  printf("    " fmt "\n", ##__VA_ARGS__); \
  return; \
} while (0)

// assert condition, fail with message if false
#define TEST_ASSERT(cond) do { \
  if (!(cond)) { \
    TEST_FAIL("assertion failed: %s (line %d)", #cond, __LINE__); \
  } \
} while (0)

// assert equal integers
#define TEST_ASSERT_EQ(expected, actual) do { \
  long long _exp = (long long)(expected); \
  long long _act = (long long)(actual); \
  if (_exp != _act) { \
    TEST_FAIL("expected %lld, got %lld (line %d)", _exp, _act, __LINE__); \
  } \
} while (0)

// assert equal unsigned integers
#define TEST_ASSERT_EQ_U(expected, actual) do { \
  unsigned long long _exp = (unsigned long long)(expected); \
  unsigned long long _act = (unsigned long long)(actual); \
  if (_exp != _act) { \
    TEST_FAIL("expected %llu, got %llu (line %d)", _exp, _act, __LINE__); \
  } \
} while (0)

// assert strings equal
#define TEST_ASSERT_STR_EQ(expected, actual) do { \
  const char *_exp = (expected); \
  const char *_act = (actual); \
  if (_exp == NULL || _act == NULL || strcmp(_exp, _act) != 0) { \
    TEST_FAIL("expected \"%s\", got \"%s\" (line %d)", \
              _exp ? _exp : "(null)", _act ? _act : "(null)", __LINE__); \
  } \
} while (0)

// assert memory equal
#define TEST_ASSERT_MEM_EQ(expected, actual, len) do { \
  if (memcmp((expected), (actual), (len)) != 0) { \
    TEST_FAIL("memory mismatch (line %d)", __LINE__); \
  } \
} while (0)

// assert floating point approximately equal
#define TEST_ASSERT_FLOAT_EQ(expected, actual, epsilon) do { \
  double _exp = (double)(expected); \
  double _act = (double)(actual); \
  double _diff = _exp - _act; \
  if (_diff < 0) _diff = -_diff; \
  if (_diff > (epsilon)) { \
    TEST_FAIL("expected %f, got %f (line %d)", _exp, _act, __LINE__); \
  } \
} while (0)

// print test summary and return exit code
#define TEST_SUMMARY() do { \
  printf("\n" TEST_COLOR_BLUE "[TEST SUMMARY]" TEST_COLOR_RESET "\n"); \
  printf("  total:  %d\n", test_total); \
  printf("  " TEST_COLOR_GREEN "passed: %d" TEST_COLOR_RESET "\n", test_passed); \
  if (test_failed > 0) { \
    printf("  " TEST_COLOR_RED "failed: %d" TEST_COLOR_RESET "\n", test_failed); \
  } else { \
    printf("  failed: %d\n", test_failed); \
  } \
  return test_failed > 0 ? 1 : 0; \
} while (0)

// helper to reset test counters (for running multiple test files)
#define TEST_RESET() do { \
  test_total = 0; \
  test_passed = 0; \
  test_failed = 0; \
} while (0)

#endif // TINY_NMEA_TEST_H
