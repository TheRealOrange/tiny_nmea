//
// Created by dst on 12/29/25.
//

#include "tiny_nmea/internal/fixed_point.h"

float tiny_nmea_to_float(const tiny_nmea_float_t *f) {
  if (!tiny_nmea_float_valid(f)) return 0.0f;
  return (float)f->value / (float)f->scale;
}

double tiny_nmea_to_double(const tiny_nmea_float_t *f) {
  if (!tiny_nmea_float_valid(f)) return 0.0;
  return (double)f->value / (double)f->scale;
}

int32_t tiny_nmea_rescale(const tiny_nmea_float_t *f, int32_t new_scale) {
  if (!tiny_nmea_float_valid(f) || new_scale == 0) {
    return 0;
  }

  if (f->scale == new_scale) {
    return f->value;
  }

  // int64_t for intermediate to avoid overflow
  return (int32_t)(((int64_t)f->value * new_scale) / f->scale);
}

bool tiny_nmea_float_valid(const tiny_nmea_float_t *f) { return f->scale != 0; }

tiny_nmea_float_t tiny_nmea_fp_mul_int(const tiny_nmea_float_t *f, int32_t n) {
  return (tiny_nmea_float_t){.value = f->value * n, .scale = f->scale};
}

// Helper: add two fixed-point numbers (rescales to common scale)
tiny_nmea_float_t tiny_nmea_fp_add(const tiny_nmea_float_t *a,
                                        const tiny_nmea_float_t *b) {
  // Use larger scale as common scale
  if (a->scale >= b->scale) {
    int32_t b_rescaled = tiny_nmea_rescale(b, a->scale);
    return (tiny_nmea_float_t){.value = a->value + b_rescaled, .scale = a->scale};
  } else {
    int32_t a_rescaled = tiny_nmea_rescale(a, b->scale);
    return (tiny_nmea_float_t){.value = a_rescaled + b->value, .scale = b->scale};
  }
}

// Helper: divide fixed-point by integer (increases scale to maintain precision)
tiny_nmea_float_t fp_div_int(const tiny_nmea_float_t *f, int32_t n) {
  // To divide by n without losing precision, multiply scale by n
  // value/scale ÷ n = value/(scale*n)
  return (tiny_nmea_float_t){.value = f->value, .scale = f->scale * n};
}
