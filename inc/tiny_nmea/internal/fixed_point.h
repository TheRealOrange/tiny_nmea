//
// Created by dst on 12/29/25.
//

#ifndef TINY_NMEA_FIXED_POINT_H
#define TINY_NMEA_FIXED_POINT_H
#include <stdbool.h>
#include <stdint.h>

// store NMEA numbers as {value, scale} pairs to preserve precision
// without floating point. e.g.:
//   "-123.456" -> {-123456, 1000}
//   "45.5"     -> {455, 10}
//   "3855.4487" (lat DDMM.MMMM) -> {38554487, 10000}
//
// no loss of precision, no floating point operation possible,
// lazy conversion to float possible
// actual = value / scale
//
// use int32_t which gives us:
//   - latitude:  ±DDMM.MMMMM with 5 decimal places
//   - longitude: ±DDDMM.MMMMM with 5 decimal places

typedef struct {
  int32_t value;    // scaled value
  int32_t scale;    // divisor
} tiny_nmea_float_t;

/**
 * convert fixed-point to float
 * @param f         Fixed-point value
 * @return          Floating point equivalent
 */
float tiny_nmea_to_float(const tiny_nmea_float_t *f);

/**
 * convert fixed-point to double
 */
double tiny_nmea_to_double(const tiny_nmea_float_t *f);

/**
 * rescale a fixed-point value to a new scale
 * for fixed-point math
 *
 * @param f         Fixed-point value
 * @param new_scale Target scale
 * @return          Rescaled integer value
 */
int32_t tiny_nmea_rescale(const tiny_nmea_float_t *f, int32_t new_scale);

bool tiny_nmea_float_valid(const tiny_nmea_float_t *f);

tiny_nmea_float_t tiny_nmea_fp_mul_int(const tiny_nmea_float_t *f, int32_t n);
tiny_nmea_float_t tiny_nmea_fp_add(const tiny_nmea_float_t *a,
                                   const tiny_nmea_float_t *b);
tiny_nmea_float_t tiny_nmea_fp_div_int(const tiny_nmea_float_t *f, int32_t n);

#endif //TINY_NMEA_FIXED_POINT_H