//
// Created by linyi on 1/3/2026.
//

#include "tiny_nmea/internal/parse_sentence_fields.h"
#include "tiny_nmea/internal/data_formats.h"

#include <string.h>
#include <stdint.h>

// overflow thresholds for multiply by 10 and accumulate
// wont overflow if  val * 10 + digit if val <= THRESHOLD and
// if val == THRESHOLD then digit <= LAST_DIGIT

// UINT32_MAX
#define UINT32_MUL10_THRESHOLD   (UINT32_MAX / 10)
#define UINT32_MUL10_MAX_DIGIT   (UINT32_MAX % 10)

// helper macro for the overflow check
#define WOULD_OVERFLOW_U32(val, digit) \
((val) > UINT32_MUL10_THRESHOLD || \
((val) == UINT32_MUL10_THRESHOLD && (digit) > UINT32_MUL10_MAX_DIGIT))

bool field_empty(const field_t* f) {
  return f->ptr == NULL || f->len == 0;
}

// parse unsigned integer
bool parse_uint(const field_t* f, uint32_t* out) {
  if (field_empty(f)) return false;

  const char* p = f->ptr;
  uint8_t len = f->len;
  // check all are digits before parsing
  REQUIRE_DIGITS(p, len);

  uint32_t val = 0;
  for (uint8_t i = 0; i < len; i++) {
    uint8_t digit = p[i] - '0';
    // overflow check
    if (WOULD_OVERFLOW_U32(val, digit)) {
      return false;
    }
    val = val * 10 + digit;
  }
  *out = val;
  return true;
}

// parse signed integer
bool parse_int(const field_t* f, int32_t* out) {
  if (field_empty(f)) return false;

  const char* p = f->ptr;
  uint8_t len = f->len;
  bool negative = false;

  // account for possible '-' or '+' sign
  // and strip the sign
  if (p[0] == '-') {
    negative = true;
    p++;
    len--;
  } else if (p[0] == '+') {
    p++;
    len--;
  }

  uint32_t uval;
  field_t tmp = {.ptr = p, .len = len};
  // after stripping sign, parse as if uint
  // parse uint already handles digit verification
  // no need to check in this function
  if (!parse_uint(&tmp, &uval)) return false;

  // check for signed overflow
  if (negative) {
    if (uval > (uint32_t)INT32_MAX + 1) return false;
    *out = -(int32_t)uval;
  } else {
    if (uval > INT32_MAX) return false;
    *out = (int32_t)uval;
  }

  return true;
}

// parse single character
bool parse_char(const field_t* f, char* out) {
  if (field_empty(f)) return false;
  *out = f->ptr[0];
  return true;
}

bool parse_fixedpoint_float(const field_t* f, tiny_nmea_float_t* out) {
  out->value = 0;
  out->scale = 0;

  if (field_empty(f)) return false;

  const char* p = f->ptr;
  size_t len = f->len;
  bool negative = false;

  uint32_t integer_val = 0;
  uint32_t frac_val = 0;
  uint32_t scale = 1;
  bool have_digits = false;

  // parse the sign first so that we can
  // treat the two fields before and after the decimal point
  // as two individual uints and use the parse_uint func
  if (p[0] == '-') {
    negative = true;
    p++;
    len--;
  }
  else if (p[0] == '+') {
    p++;
    len--;
  }

  if (len == 0) return false;

  // find decimal poin tnow
  const char* decimal_pt = memchr(p, '.', len);
  if (decimal_pt) {
    size_t int_len = decimal_pt - p;
    if (int_len > 0) {
      // there is something before the decimal point
      // try to parse like a uint
      field_t tmp = {.ptr = p, .len = int_len};
      // parse uint already handles digit verification
      // no need to check in this function
      if (!parse_uint(&tmp, &integer_val)) return false;
      have_digits = true;
    }

    p = decimal_pt + 1;
    len -= int_len + 1;
  }

  if (len > 0) {
    // try to parse the remainder like another uint
    // try to parse like a uint
    field_t tmp = {.ptr = p, .len = len};
    // parse uint already handles digit verification
    // no need to check in this function
    if (!parse_uint(&tmp, &frac_val)) return false;
    have_digits = true;

    // adjust scale from number of fractional digits
    for (size_t i = 0; i < len; i++) {
      scale *= 10;
    }
  }

  // reject "." or "+" or "-" with no actual digits
  if (!have_digits) return false;

  // both parsed successfully
  if (integer_val > (uint32_t)(INT32_MAX / scale)) {
    return false; // would overflow
  }

  uint32_t combined = integer_val * scale + frac_val;
  if (combined > INT32_MAX) {
    return false; // would overflow
  }

  out->value = negative ? -(int32_t)combined : (int32_t)combined;
  out->scale = (int32_t)scale;
  return true;
}


uint8_t tokenize(const char* sentence, size_t len,
                 field_t* fields, uint8_t max_fields) {
  if (!sentence || !fields || len == 0) {
    // argument null checks
    return 0;
  }

  const char* ptr = sentence;
  const char* end = sentence + len;
  uint8_t count = 0;

  while (ptr <= end && count < max_fields) {
    // find the position of the next comma if any
    const char* comma = memchr(ptr, ',', end - ptr);
    const char* field_end = comma ? comma : end;

    fields[count].ptr = ptr;
    fields[count].len = (uint8_t)(field_end - ptr);
    count++;

    // if we found a comma, there is more fields
    if (!comma) break;

    // continue to the position after the comma
    ptr = comma + 1;
  }

  return count;
}

// field parsers

// parse time, expecting hhmmss or hhmmss.s{1-6}
bool parse_time(const field_t* f, tiny_nmea_time_t* out) {
  out->valid = false;
  out->hours = 0;
  out->minutes = 0;
  out->seconds = 0;
  out->microseconds = 0;

  if (field_empty(f) || f->len < 6) return false;

  const char* p = f->ptr;
  // ensure at least 6 digits before we parse into
  // hours, minutes and seconds
  REQUIRE_DIGITS(p, 6);
  out->hours = (p[0] - '0') * 10 + (p[1] - '0');
  out->minutes = (p[2] - '0') * 10 + (p[3] - '0');
  out->seconds = (p[4] - '0') * 10 + (p[5] - '0');

  // validate that this is a real time value
  if (out->hours > 23 || out->minutes > 59 || out->seconds > 60) {
    return false; // seconds can be 60 for leap second
  }

  // parse fractional seconds
  // the number of digits can vary depending on
  // the precision provided by the receiver
  // we simplify parsing by scaling everything to
  // microseconds
  if (f->len > 7 && p[6] == '.') {
    uint32_t frac = 0;
    uint8_t digits = 0;

    for (uint8_t i = 7; i < f->len && digits < 6; i++) {
      char c = p[i];
      if (!IS_DIGIT(c)) break;
      frac = frac * 10 + (c - '0');
      digits++;
    }

    // scale to microseconds
    while (digits < 6) {
      frac *= 10;
      digits++;
    }

    out->microseconds = frac;
  }

  out->valid = true;
  return true;
}

// parse date expecting ddmmyy
bool parse_date(const field_t* f, tiny_nmea_date_t* out) {
  out->valid = false;
  out->day = 0;
  out->month = 0;
  out->year = 0;
  out->year_yy = 0;

  if (field_empty(f) || f->len < 6) return false;

  const char* p = f->ptr;

  // ensure at least 6 digits before we parse into
  // the date fields
  REQUIRE_DIGITS(p, 6);
  out->day = (p[0] - '0') * 10 + (p[1] - '0');
  out->month = (p[2] - '0') * 10 + (p[3] - '0');
  out->year_yy = (p[4] - '0') * 10 + (p[5] - '0');

  // check the date numbers make sense
  if (out->day < 1 || out->day > 31 ||
        out->month < 1 || out->month > 12) {
    return false;
        }

  // the full year is left as zero unlees
  // we know the century from the context and can
  // update it with the actual year

  out->valid = true;
  return true;
}

// parse latitude: ddmm.mmmm -> decimal degrees as fixed-point
// Result: degrees + minutes/60
bool parse_latitude(const field_t* f, const field_t* dir, tiny_nmea_coord_t* out) {
  out->raw.value = 0;
  out->raw.scale = 0;
  out->hemisphere = '\0';

  if (field_empty(f)) return false;

  // parse the raw value as fixedpoint
  if (!parse_fixedpoint_float(f, &out->raw)) return false;

  // parse hemisphere
  if (!field_empty(dir)) {
    char h = dir->ptr[0];
    if (h == 'N' || h == 'S') {
      out->hemisphere = h;
    }
    else {
      return false; // Invalid hemisphere
    }
  }

  return true;
}

// parse longitude: dddmm.mmmm -> decimal degrees as fixed-point
bool parse_longitude(const field_t* f, const field_t* dir, tiny_nmea_coord_t* out) {
  out->raw.value = 0;
  out->raw.scale = 0;
  out->hemisphere = '\0';

  if (field_empty(f)) return false;

  // parse the raw value as fixedpoint
  if (!parse_fixedpoint_float(f, &out->raw)) return false;

  // parse the hemisphere
  if (!field_empty(dir)) {
    char h = dir->ptr[0];
    if (h == 'E' || h == 'W') {
      out->hemisphere = h;
    }
    else {
      return false; // Invalid hemisphere
    }
  }

  return true;
}
