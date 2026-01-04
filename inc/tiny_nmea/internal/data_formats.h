//
// Created by dst on 12/29/25.
//

#ifndef TINY_NMEA_DATA_FORMATS_H
#define TINY_NMEA_DATA_FORMATS_H
#include <stdbool.h>
#include <stdint.h>

#include "fixed_point.h"

typedef struct {
  uint8_t hours;        // 0-23
  uint8_t minutes;      // 0-59
  uint8_t seconds;      // 0-59
  uint32_t microseconds;// 0-999999
  bool valid;
} tiny_nmea_time_t;

typedef struct {
  uint8_t day;          // 1-31
  uint8_t month;        // 1-12
  uint16_t year;        // full year if known, 0 if century unknown (RMC before ZDA)
  uint8_t year_yy;     // 2 digit year from RMC (0 if from ZDA)
  bool valid;
} tiny_nmea_date_t;

// Internally stores raw NMEA format (DDMM.MMMM / DDDMM.MMMM) as fixed-point
// use tiny_nmea_coord_to_degrees() to convert to decimal degrees

typedef struct {
  tiny_nmea_float_t raw;   // fixed point DDMM.MMMM or DDDMM.MMMM value
  char hemisphere;         // 'N'/'S' for lat, 'E'/'W' for lon, '\0' if invalid
} tiny_nmea_coord_t;

/**
 * convert NMEA coordinate (DDMM.MMMM format) to decimal degrees
 * with S and W negative
 *
 * @param coord     NMEA coordinate
 * @return          decimal degrees as double (NaN if invalid)
 */
double tiny_nmea_coord_to_degrees(const tiny_nmea_coord_t *coord);

/**
 * convert NMEA coordinate to fixed-point decimal degrees
 *
 * @param coord     NMEA coordinate
 * @return          degrees * 10,000,000 (0 if invalid)
 */
int32_t tiny_nmea_coord_to_fixed_degrees(const tiny_nmea_coord_t *coord);

/**
 * check if a coordinate is valid
 */
static inline bool tiny_nmea_coord_valid(const tiny_nmea_coord_t *coord) {
  return coord->hemisphere != '\0' && coord->raw.scale != 0;
}

/**
 * convert speed from knots to m/s as fixed-point
 * @param knots     speed in knots (fixed-point)
 * @return          speed in m/s * 1000
 */
int32_t tiny_nmea_knots_to_mps(const tiny_nmea_float_t *knots);

/**
 * convert speed from knots to km/h as fixed-point
 * @param knots     speed in knots (fixed-point)
 * @return          speed in km/h * 1000
 */
int32_t tiny_nmea_knots_to_kph(const tiny_nmea_float_t *knots);

#endif //TINY_NMEA_DATA_FORMATS_H