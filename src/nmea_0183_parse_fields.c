//
// Created by Lin Yicheng on 2/1/26.
//

#include "tiny_nmea/internal/nmea_0183_types.h"

tiny_nmea_constellation_t parse_constellation(const char *s) {
  switch (NMEA_HASH2(s[0], s[1])) {
#define X_CASE(name, c1, c2, desc) \
case NMEA_HASH2(c1, c2): return TINY_NMEA_CONSTELLATION_##name;
    TINY_NMEA_CONSTELLATION_LIST(X_CASE)
#undef X_CASE
    default: return TINY_NMEA_CONSTELLATION_UNKNOWN;
  }
}

tiny_nmea_talker_t parse_talker_id(const char *s) {
  switch (NMEA_HASH2(s[0], s[1])) {
#define X_CASE(name, c1, c2, desc) \
case NMEA_HASH2(c1, c2): return TINY_NMEA_TALKER_##name;
    TINY_NMEA_TALKER_LIST(X_CASE)
#undef X_CASE
    default: return TINY_NMEA_TALKER_UNKNOWN;
  }
}

tiny_nmea_sentence_type_t parse_sentence_type(const char *s) {
  switch (NMEA_HASH3(s[0], s[1], s[2])) {
#define X_CASE(name, c1, c2, c3) \
case NMEA_HASH3(c1, c2, c3): return TINY_NMEA_SENTENCE_##name;
    TINY_NMEA_SENTENCE_LIST(X_CASE)
#undef X_CASE
    default: return TINY_NMEA_SENTENCE_UNKNOWN;
  }
}

tiny_nmea_fix_quality_t parse_fix_quality(char c) {
  switch (c) {
#define X_CASE(name, val, desc) \
case ('0' + (val)): return TINY_NMEA_FIX_##name;
    TINY_NMEA_FIX_QUALITY_LIST(X_CASE)
#undef X_CASE
    default: return TINY_NMEA_FIX_INVALID;
  }
}

tiny_nmea_faa_mode_t parse_faa_mode(char c) {
  switch (c) {
#define X_CASE(name, ch, desc) \
case (ch): return TINY_NMEA_FAA_##name;
    TINY_NMEA_FAA_MODE_LIST(X_CASE)
#undef X_CASE
    default: return TINY_NMEA_FAA_UNKNOWN;
  }
}

tiny_nmea_gsa_fix_t parse_gsa_fix(char c) {
  switch (c) {
#define X_CASE(name, val, desc) \
case ('0' + (val)): return TINY_NMEA_GSA_##name;
    TINY_NMEA_GSA_FIX_LIST(X_CASE)
#undef X_CASE
    default: return TINY_NMEA_GSA_FIX_UNKNOWN;
  }
}

const char *tiny_nmea_talker_str(tiny_nmea_talker_t t) {
  switch (t) {
#define X_STR(name, c1, c2, desc) \
case TINY_NMEA_TALKER_##name: return #name;
    TINY_NMEA_TALKER_LIST(X_STR)
#undef X_STR
    default: return "??";
  }
}

const char *tiny_nmea_sentence_str(tiny_nmea_sentence_type_t t) {
  switch (t) {
#define X_STR(name, c1, c2, c3) \
case TINY_NMEA_SENTENCE_##name: return #name;
    TINY_NMEA_SENTENCE_LIST(X_STR)
#undef X_STR
    default: return "Unknown";
  }
}

tiny_nmea_nav_status_t parse_nav_status(char c) {
  switch (c) {
#define X_CASE(name, ch, desc) \
case (ch): return TINY_NMEA_NAV_STATUS_##name;
    TINY_NMEA_NAV_STATUS_LIST(X_CASE)
#undef X_CASE
    default: return TINY_NMEA_NAV_STATUS_UNKNOWN;
  }
}
