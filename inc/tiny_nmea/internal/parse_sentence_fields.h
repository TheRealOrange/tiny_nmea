//
// Created by linyi on 1/3/2026.
//

#ifndef TINY_NMEA_PARSE_DATA_TYPES_H
#define TINY_NMEA_PARSE_DATA_TYPES_H

#include <stdint.h>

#include "data_formats.h"
#include "fixed_point.h"

#define IS_DIGIT(c)     ((uint8_t)((c) - '0') <= 9)
#define IS_UPPER(c)     ((uint8_t)((c) - 'A') <= 25)
#define IS_LOWER(c)     ((uint8_t)((c) - 'a') <= 25)
#define IS_ALPHA(c)     (IS_UPPER(c) | IS_LOWER(c))
#define IS_ALNUM(c)     (IS_DIGIT(c) | IS_ALPHA(c))
#define IS_XDIGIT(c)    (IS_DIGIT(c) | ((uint8_t)((c) - 'A') <= 5) | ((uint8_t)((c) - 'a') <= 5))

// n consecutive
#define DEFINE_IS_N(name, predicate)                      \
  static inline bool name(const char* p, int n) {         \
    uint32_t valid = 1;                                   \
    for (int i = 0; i < n; i++) valid &= predicate(p[i]); \
    return valid;                                         \
  }

DEFINE_IS_N(is_ndigits,  IS_DIGIT)
DEFINE_IS_N(is_nxdigits, IS_XDIGIT)
DEFINE_IS_N(is_nupper,   IS_UPPER)
DEFINE_IS_N(is_nlower,   IS_LOWER)
DEFINE_IS_N(is_nalpha,   IS_ALPHA)

// early return
#define REQUIRE(pred)           do { if (!(pred)) return false; } while(0)
#define REQUIRE_DIGIT(c)        REQUIRE(IS_DIGIT(c))
#define REQUIRE_DIGITS(p, n)    REQUIRE(is_ndigits(p, n))
#define REQUIRE_UPPER(c)        REQUIRE(IS_UPPER(c))
#define REQUIRE_XDIGIT(c)       REQUIRE(IS_XDIGIT(c))

typedef struct {
  const char* ptr;
  uint8_t len;
} field_t;

bool field_empty(const field_t* f);

/**
 * Tokenize sentence body into field array
 * @param sentence  Start of first field (after "$GPRMC,")
 * @param len       Length to parse (excluding "*XX" checksum)
 * @param fields    Output array
 * @param max_fields Size of output array
 * @return Number of fields found
 */
uint8_t tokenize(const char* sentence, size_t len, field_t* fields, uint8_t max_fields);

// parse unsigned integer
bool parse_uint(const field_t* f, uint32_t* out);
bool parse_int(const field_t* f, int32_t* out);
bool parse_char(const field_t* f, char* out);
bool parse_fixedpoint_float(const field_t* f, tiny_nmea_float_t* out);

bool parse_time(const field_t* f, tiny_nmea_time_t* out);
bool parse_date(const field_t* f, tiny_nmea_date_t* out);

bool parse_latitude(const field_t* f, const field_t* dir, tiny_nmea_coord_t* out);
bool parse_longitude(const field_t* f, const field_t* dir, tiny_nmea_coord_t* out);

#endif //TINY_NMEA_PARSE_DATA_TYPES_H