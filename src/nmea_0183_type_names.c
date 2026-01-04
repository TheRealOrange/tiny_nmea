//
// Created by Lin Yicheng on 2/1/26.
//

#include "tiny_nmea/nmea_0183_type_names.h"
#include "../inc/tiny_nmea/internal/nmea_0183_types.h"

// _name are human-readable functions
// maybe wrap with a ifdef to only include for debug builds?
const char *tiny_nmea_constellation_name(tiny_nmea_constellation_t t) {
  switch (t) {
#define X_STR(name, c1, c2, desc) \
case TINY_NMEA_CONSTELLATION_##name: return desc;
    TINY_NMEA_CONSTELLATION_LIST(X_STR)
#undef X_STR
    default: return "Unknown";
  }
}

const char *tiny_nmea_talker_name(tiny_nmea_talker_t t) {
  switch (t) {
#define X_STR(name, c1, c2, desc) \
case TINY_NMEA_TALKER_##name: return desc;
    TINY_NMEA_TALKER_LIST(X_STR)
#undef X_STR
    default: return "Unknown";
  }
}

const char *tiny_nmea_sentence_name(tiny_nmea_sentence_type_t t) {
  // same as the _str function because it just returns
  // the acronym representing the sentence type
  return tiny_nmea_sentence_str(t);
}

const char *tiny_nmea_fix_quality_name(tiny_nmea_fix_quality_t q) {
  switch (q) {
#define X_STR(name, val, desc) \
case TINY_NMEA_FIX_##name: return desc;
    TINY_NMEA_FIX_QUALITY_LIST(X_STR)
#undef X_STR
    default: return "Unknown";
  }
}

const char *tiny_nmea_faa_mode_name(tiny_nmea_faa_mode_t m) {
  switch (m) {
    case TINY_NMEA_FAA_UNKNOWN: return "None";
#define X_STR(name, ch, desc) \
case TINY_NMEA_FAA_##name: return desc;
    TINY_NMEA_FAA_MODE_LIST(X_STR)
#undef X_STR
    default: return "Unknown";
  }
}

const char *tiny_nmea_gsa_fix_name(tiny_nmea_gsa_fix_t f) {
  switch (f) {
  case TINY_NMEA_GSA_FIX_UNKNOWN: return "Unknown";
#define X_STR(name, val, desc) \
case TINY_NMEA_GSA_##name: return desc;
    TINY_NMEA_GSA_FIX_LIST(X_STR)
#undef X_STR
    default: return "Unknown";
  }
}

const char *tiny_nmea_nav_status_name(tiny_nmea_nav_status_t s) {
  switch (s) {
  case TINY_NMEA_NAV_STATUS_UNKNOWN: return "Unknown";
#define X_STR(name, ch, desc) \
case TINY_NMEA_NAV_STATUS_##name: return desc;
    TINY_NMEA_NAV_STATUS_LIST(X_STR)
#undef X_STR
    default: return "Unknown";
  }
}
