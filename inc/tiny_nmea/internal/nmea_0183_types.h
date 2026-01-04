//
// Created by dst on 12/29/25.
//

#ifndef TINY_NMEA_NMEA_TYPES_H
#define TINY_NMEA_NMEA_TYPES_H

#include "config.h"
#include "data_formats.h"
#include "fixed_point.h"

#include "nmea_0183_defs.h"

// sentence / buffer constraints
_Static_assert(TINY_NMEA_MAX_SENTENCE_LEN >= 82,
               "MAX_SENTENCE_LEN must be >= 82 (NMEA spec minimum)");

_Static_assert(TINY_NMEA_WORKING_BUF_LEN > TINY_NMEA_MAX_SENTENCE_LEN,
               "WORKING_BUF_LEN must be > MAX_SENTENCE_LEN");

// GSV/GSA constraints
_Static_assert(TINY_NMEA_MAX_SATS_PER_GSV > 0 && TINY_NMEA_MAX_SATS_PER_GSV <= 8,
               "MAX_SATS_PER_GSV must be 1-8");

_Static_assert(TINY_NMEA_MAX_SATS_GSA > 0 && TINY_NMEA_MAX_SATS_GSA <= 24,
               "MAX_SATS_GSA must be 1-24");

// PRN constraints
_Static_assert(TINY_NMEA_MAX_PRN_PER_CONST > 0,
               "MAX_PRN_PER_CONST must be positive");

_Static_assert(TINY_NMEA_MAX_PRN_PER_CONST <= UINT16_MAX,
               "MAX_PRN_PER_CONST exceeds UINT16_MAX");

#if (TINY_NMEA_MAX_PRN_PER_CONST <= UINT8_MAX)
  #define TINY_NMEA_PRN_TYPE uint8_t
#elif (TINY_NMEA_MAX_PRN_PER_CONST <= UINT16_MAX)
  #define TINY_NMEA_PRN_TYPE uint16_t
#else
  #error "TINY_NMEA_MAX_PRN_PER_CONST exceeds supported range"
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L

// if we have C23 features, use typed enums
#define TINY_NMEA_DEFINE_ENUM8(name, NAME, list_macro, x_macro) \
  typedef enum : uint8_t {                                      \
    NAME##_UNKNOWN = 0,                                         \
    list_macro(x_macro)                                         \
    NAME##_COUNT                                                \
  } name##_t

// for enums with explicit values (char-based or non-sequential)
#define TINY_NMEA_DEFINE_ENUM8_VAL(name, NAME, unknown_val, list_macro, x_macro) \
  typedef enum : uint8_t {                                                       \
    NAME##_UNKNOWN = (unknown_val),                                              \
    list_macro(x_macro)                                                          \
  } name##_t


#else

// else use the typedef type to save memory
// enum for constants only (compiler chooses size)
// typedef actual storage type to save memory in arrays
// and enforce that it fits
#define TINY_NMEA_DEFINE_ENUM8(name, NAME, list_macro, x_macro) \
  enum name##_e {                                               \
    NAME##_UNKNOWN = 0,                                         \
    list_macro(x_macro)                                         \
    NAME##_COUNT                                                \
  };                                                            \
  typedef uint8_t name##_t;                                     \
  _Static_assert(NAME##_COUNT <= UINT8_MAX, #NAME "_COUNT exceeds uint8_t")

// for enums with explicit values (char-based or non-sequential)
#define TINY_NMEA_DEFINE_ENUM8_VAL(name, NAME, unknown_val, list_macro, x_macro) \
  enum name##_e {                                                                \
    NAME##_UNKNOWN = (unknown_val),                                              \
    list_macro(x_macro)                                                          \
    NAME##_COUNT                                                                 \
  };                                                                             \
  typedef uint8_t name##_t;                                                      \
  _Static_assert(NAME##_COUNT <= UINT8_MAX, #NAME "_COUNT exceeds uint8_t")

#endif

// constellation (GP, GL, GA, etc.)
#define X_CONST(name, c1, c2, desc) TINY_NMEA_CONSTELLATION_##name,
TINY_NMEA_DEFINE_ENUM8(tiny_nmea_constellation, TINY_NMEA_CONSTELLATION, TINY_NMEA_CONSTELLATION_LIST, X_CONST);
#undef X_CONST

// talker ID (extends constellation)
#define X_TALKER(name, c1, c2, desc) TINY_NMEA_TALKER_##name,
TINY_NMEA_DEFINE_ENUM8(tiny_nmea_talker, TINY_NMEA_TALKER, TINY_NMEA_TALKER_LIST, X_TALKER);
#undef X_TALKER

// sentence type (RMC, GGA, etc.)
#define X_SENT(name, c1, c2, c3) TINY_NMEA_SENTENCE_##name,
TINY_NMEA_DEFINE_ENUM8(tiny_nmea_sentence_type, TINY_NMEA_SENTENCE, TINY_NMEA_SENTENCE_LIST, X_SENT);
#undef X_SENT

// fix quality (GGA: 0-8)
#define X_FIX(name, val, desc) TINY_NMEA_FIX_##name = (val),
TINY_NMEA_DEFINE_ENUM8_VAL(tiny_nmea_fix_quality, TINY_NMEA_FIX, 0, TINY_NMEA_FIX_QUALITY_LIST, X_FIX);
#undef X_FIX

// FAA mode / GNS mode indicator (char-based: A/D/E/F/M/N/P/R/S)
#define X_FAA(name, ch, desc) TINY_NMEA_FAA_##name = (ch),
TINY_NMEA_DEFINE_ENUM8_VAL(tiny_nmea_faa_mode, TINY_NMEA_FAA, 0, TINY_NMEA_FAA_MODE_LIST, X_FAA);
#undef X_FAA

// GSA fix type (1-3)
#define X_GSA(name, val, desc) TINY_NMEA_GSA_##name = (val),
TINY_NMEA_DEFINE_ENUM8_VAL(tiny_nmea_gsa_fix, TINY_NMEA_GSA_FIX, 0, TINY_NMEA_GSA_FIX_LIST, X_GSA);
#undef X_GSA

// navigation status (NMEA 4.1+: S/C/U/V)
#define X_NAV(name, ch, desc) TINY_NMEA_NAV_STATUS_##name = (ch),
TINY_NMEA_DEFINE_ENUM8_VAL(tiny_nmea_nav_status, TINY_NMEA_NAV_STATUS, 0, TINY_NMEA_NAV_STATUS_LIST, X_NAV);
#undef X_NAV

// RMC
// recommended minimum nav information
// position, velocity, time, date
typedef struct {
  tiny_nmea_time_t time;
  tiny_nmea_date_t date;
  bool status_valid;               // A=valid, V=warning
  tiny_nmea_coord_t latitude;
  tiny_nmea_coord_t longitude;
  tiny_nmea_float_t speed_knots;   // Speed over ground
  tiny_nmea_float_t course_deg;    // Track made good (degrees true)
  tiny_nmea_float_t mag_variation; // Magnetic variation
  char mag_var_dir;                // 'E' or 'W'
  tiny_nmea_faa_mode_t faa_mode;   // NMEA 2.3+
  tiny_nmea_nav_status_t nav_status; // NMEA 4.1+
} tiny_nmea_rmc_t;

// GGA
// gps fix information
// fix quality, altitude, HDOP, satellite count
typedef struct {
  tiny_nmea_time_t time;
  tiny_nmea_coord_t latitude;
  tiny_nmea_coord_t longitude;
  tiny_nmea_fix_quality_t fix_quality;
  uint8_t satellites_used;
  tiny_nmea_float_t hdop;
  tiny_nmea_float_t altitude_m;   // Altitude above mean sea level
  tiny_nmea_float_t geoid_sep_m;  // Geoidal separation
  tiny_nmea_float_t dgps_age_sec; // Age of differential data
  uint16_t dgps_station_id;
} tiny_nmea_gga_t;

// GNS - gnss fix data (NMEA 3.0+)
// multi-constellation replacement for GGA
typedef struct {
  tiny_nmea_time_t time;
  tiny_nmea_coord_t latitude;
  tiny_nmea_coord_t longitude;
  tiny_nmea_faa_mode_t mode[TINY_NMEA_CONSTELLATION_COUNT]; // mode per constellation
  uint8_t mode_count;                                       // number of valid modes
  uint8_t satellites_used;
  tiny_nmea_float_t hdop;
  tiny_nmea_float_t altitude_m;
  tiny_nmea_float_t geoid_sep_m;
  tiny_nmea_float_t dgps_age_sec;
  uint16_t dgps_station_id;
  tiny_nmea_nav_status_t nav_status; // NMEA 4.1+
} tiny_nmea_gns_t;

// GSA
// DOP and Active Satellites
typedef struct {
  char mode_selection;     // 'M'=manual, 'A'=automatic
  tiny_nmea_gsa_fix_t fix_type; // 1=no fix, 2=2D, 3=3D
  TINY_NMEA_PRN_TYPE satellite_prns[TINY_NMEA_MAX_SATS_GSA];
  uint8_t satellite_count; // how many valid satellite IDs
  tiny_nmea_float_t pdop;
  tiny_nmea_float_t hdop;
  tiny_nmea_float_t vdop;
  uint8_t system_id;       // NMEA 4.11+
} tiny_nmea_gsa_t;

// GSV
// satellites in view satellite info
typedef struct {
  TINY_NMEA_PRN_TYPE prn;  // satellite ID/PRN
  int8_t elevation;        // elevation in degrees (-90 to 90), -128 if invalid
  int16_t azimuth;         // azimuth in degrees (0-359), -1 if invalid
  int8_t snr;              // signal-to-noise ratio in dB (0-99), -1 if invalid
} tiny_nmea_sat_info_t;

typedef struct {
  uint8_t total_msgs; // total number of GSV messages
  uint8_t msg_number; // this message number (1-based)
  uint8_t total_sats; // total satellites in view
  tiny_nmea_sat_info_t sats[TINY_NMEA_MAX_SATS_PER_GSV];
  uint8_t sat_count;  // valid satellites in this message
  uint8_t signal_id;  // NMEA 4.11+
} tiny_nmea_gsv_t;

// VTG
// track made good and ground speed
typedef struct {
  tiny_nmea_float_t course_true_deg;
  tiny_nmea_float_t course_mag_deg;
  tiny_nmea_float_t speed_knots;
  tiny_nmea_float_t speed_kph;
  tiny_nmea_faa_mode_t faa_mode;
} tiny_nmea_vtg_t;

// GLL
// geographic position (lat/lon)
typedef struct {
  tiny_nmea_coord_t latitude;
  tiny_nmea_coord_t longitude;
  tiny_nmea_time_t time;
  bool status_valid;
  tiny_nmea_faa_mode_t faa_mode;
} tiny_nmea_gll_t;

// ZDA
// time and date info
typedef struct {
  tiny_nmea_time_t time;
  tiny_nmea_date_t date;
  int8_t tz_hours;    // local timezone hours (-13 to +13)
  uint8_t tz_minutes; // local timezone minutes (0-59)
} tiny_nmea_zda_t;

// GBS
// satellite fault detection
typedef struct {
  tiny_nmea_time_t time;
  tiny_nmea_float_t err_lat_m;      // Expected error in latitude
  tiny_nmea_float_t err_lon_m;      // Expected error in longitude
  tiny_nmea_float_t err_alt_m;      // Expected error in altitude
  TINY_NMEA_PRN_TYPE failed_sat_id; // Most likely failed satellite
  tiny_nmea_float_t prob_missed;    // Probability of missed detection
  tiny_nmea_float_t bias_m;         // Bias estimate
  tiny_nmea_float_t bias_stddev_m;  // Standard deviation of bias
} tiny_nmea_gbs_t;

// GST
// pseudorange noise information
typedef struct {
  tiny_nmea_time_t time;
  tiny_nmea_float_t rms_range;   // RMS of ranges
  tiny_nmea_float_t std_major_m; // Std dev of semi-major axis
  tiny_nmea_float_t std_minor_m; // Std dev of semi-minor axis
  tiny_nmea_float_t orient_deg;  // Orientation of semi-major axis
  tiny_nmea_float_t std_lat_m;   // Std dev of latitude error
  tiny_nmea_float_t std_lon_m;   // Std dev of longitude error
  tiny_nmea_float_t std_alt_m;   // Std dev of altitude error
} tiny_nmea_gst_t;

// AIS
// ais packet fragment information
typedef struct {
  uint8_t fragment_count;    // Total sentences in message (1-9)
  uint8_t fragment_number;   // This sentence number (1-based)
  uint8_t sequential_id;     // Links multi-sentence messages (0 if single/empty)
  char channel;              // 'A', 'B', '1', '2', or '\0' if empty
  char payload[64];          // Armored 6-bit ASCII payload
  uint8_t payload_len;       // Length of payload
  uint8_t fill_bits;         // Bits to ignore in last character (0-5)
} tiny_nmea_ais_t;

typedef struct {
  tiny_nmea_sentence_type_t type;
  tiny_nmea_talker_t talker;

  // only one will be valid based on type
  union {
    tiny_nmea_rmc_t rmc;
    tiny_nmea_gga_t gga;
    tiny_nmea_gns_t gns;
    tiny_nmea_gsa_t gsa;
    tiny_nmea_gsv_t gsv;
    tiny_nmea_vtg_t vtg;
    tiny_nmea_gll_t gll;
    tiny_nmea_zda_t zda;
    tiny_nmea_gbs_t gbs;
    tiny_nmea_gst_t gst;
    tiny_nmea_ais_t ais;
  } data;
} tiny_nmea_type_t;

// return types
typedef enum {
  TINY_NMEA_OK,
  TINY_NMEA_INVALID_ARGS,
  TINY_NMEA_MALFORMED_SENTENCE,
  TINY_NMEA_ERR_NULL_PTR,
  TINY_NMEA_ERR_EMPTY_FIELD,
  TINY_NMEA_ERR_TOO_FEW_FIELDS,
  TINY_NMEA_ERR_INVALID_FORMAT,
  TINY_NMEA_ERR_INVALID_TIME,
  TINY_NMEA_ERR_INVALID_DATE,
  TINY_NMEA_ERR_INVALID_COORD,
  TINY_NMEA_ERR_INVALID_NUMBER,
  TINY_NMEA_ERR_OVERFLOW,
  TINY_NMEA_ERR_BUFFER_FULL,
  TINY_NMEA_ERR_CHECKSUM,
  TINY_NMEA_ERR_UNSUPPORTED,
} tiny_nmea_res_t;

tiny_nmea_constellation_t parse_constellation(const char *s);
tiny_nmea_talker_t parse_talker_id(const char *s);
tiny_nmea_sentence_type_t parse_sentence_type(const char *s);
tiny_nmea_fix_quality_t parse_fix_quality(char c);
tiny_nmea_faa_mode_t parse_faa_mode(char c);
tiny_nmea_gsa_fix_t parse_gsa_fix(char c);
tiny_nmea_nav_status_t parse_nav_status(char c);

const char *tiny_nmea_talker_str(tiny_nmea_talker_t t);
const char *tiny_nmea_sentence_str(tiny_nmea_sentence_type_t t);

// use the 2 letters packed to a uint16_t number as a hash-lookup
#define NMEA_HASH2(a, b)                     \
    (((uint16_t)(unsigned char)(a) << 8) |   \
    (uint16_t)(unsigned char)(b))

// for the 3 letters packed as a 24-bit, so packed to an uint32_t hash-lookup
#define NMEA_HASH3(a, b, c)                  \
    (((uint32_t)(unsigned char)(a) << 16) |  \
    ((uint32_t)(unsigned char)(b) << 8)  |  \
    (uint32_t)(unsigned char)(c))

static inline bool tiny_nmea_constellation_valid(const tiny_nmea_constellation_t t) {
  return t > TINY_NMEA_CONSTELLATION_UNKNOWN && t < TINY_NMEA_CONSTELLATION_COUNT;
}

static inline bool tiny_nmea_talker_valid(const tiny_nmea_talker_t t) {
  return t > TINY_NMEA_TALKER_UNKNOWN && t < TINY_NMEA_TALKER_COUNT;
}

static inline bool tiny_nmea_sentence_valid(const tiny_nmea_sentence_type_t t) {
  return t > TINY_NMEA_SENTENCE_UNKNOWN && t < TINY_NMEA_SENTENCE_COUNT;
}

static inline tiny_nmea_constellation_t tiny_nmea_const_from_talker(const tiny_nmea_talker_t t) {
  return tiny_nmea_constellation_valid((const tiny_nmea_constellation_t)t) ? (tiny_nmea_constellation_t)t : TINY_NMEA_CONSTELLATION_UNKNOWN;
}

static inline bool tiny_nmea_is_lat_hemisphere(char c) {
  return c == 'N' || c == 'S';
}

static inline bool tiny_nmea_is_lon_hemisphere(char c) {
  return c == 'E' || c == 'W';
}

static inline int tiny_nmea_hemisphere_sign(char c) {
  return (c == 'S' || c == 'W') ? -1 : 1;
}

#endif //TINY_NMEA_NMEA_TYPES_H
