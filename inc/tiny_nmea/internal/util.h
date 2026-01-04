//
// Created by Lin Yicheng on 1/1/26.
//

#ifndef TINY_NMEA_UTIL_H
#define TINY_NMEA_UTIL_H

#define DEFINE_MIN(name, type) \
  static inline type min_##name(type a, type b) { return a < b ? a : b; }

#define DEFINE_MAX(name, type) \
  static inline type max_##name(type a, type b) { return a > b ? a : b; }

#define DEFINE_CLAMP(name, type) \
  static inline type clamp_##name(type x, type lo, type hi) { \
    return x < lo ? lo : x > hi ? hi : x; \
  }

#define DEFINE_SWAP(name, type) \
  static inline void swap_##name(type *a, type *b) { \
    type tmp = *a; *a = *b; *b = tmp; \
  }

DEFINE_MIN(ptr,    void*)
DEFINE_MIN(size,   size_t)

static inline int parse_hex_char(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

static inline int parse_hex_byte(const char *s, uint8_t *out) {
  int hi = parse_hex_char(s[0]);
  int lo = parse_hex_char(s[1]);
  if (hi < 0 || lo < 0) return 0;
  *out = (uint8_t)((hi << 4) | lo);
  return 2;
}

#endif //TINY_NMEA_UTIL_H
