//
// Created by dst on 12/29/25.
//

#include "tiny_nmea/internal/data_formats.h"

double tiny_nmea_coord_to_degrees(const tiny_nmea_coord_t *coord) {
    if (coord->hemisphere == '\0' || coord->raw.scale == 0) {
        return 0.0 / 0.0;  // NaN
    }

    // raw value is DDDMM.MMMM * scale
    // extract degrees and minutes
    double raw = (double)coord->raw.value / (double)coord->raw.scale;

    // integer degrees = raw / 100 (floor)
    int degrees = (int)(raw / 100.0);

    // minutes = raw - degrees * 100
    double minutes = raw - (degrees * 100.0);

    // convert to decimal degrees
    double result = degrees + (minutes / 60.0);

    // account for hemisphere
    if (coord->hemisphere == 'S' || coord->hemisphere == 'W') {
        result = -result;
    }

    return result;
}

int32_t tiny_nmea_coord_to_fixed_degrees(const tiny_nmea_coord_t *coord) {
    if (coord->hemisphere == '\0' || coord->raw.scale == 0) {
        return 0;
    }

    // result in degrees * 10^7
    // Raw is DDDMM.MMMM (or DDMM.MMMM)
    //
    // get raw value scaled: raw_int = value
    // degrees_int = raw_int / (100 * scale)
    // minutes_frac = raw_int - degrees_int * 100 * scale
    // result = degrees_int * 10^7 + (minutes_frac * 10^7) / (60 * scale)

    int64_t raw = coord->raw.value;
    int64_t scale = coord->raw.scale;

    // get integer degrees
    int64_t degrees = raw / (100 * scale);

    // remaining scaled minutes
    int64_t minutes_scaled = raw - (degrees * 100 * scale);

    // convert to degrees * 10^7
    // degrees * 10^7 + (minutes_scaled * 10^7) / (60 * scale)
    const int64_t SCALE_OUT = 10000000;  // 10^7

    int64_t result = degrees * SCALE_OUT + (minutes_scaled * SCALE_OUT) / (60 * scale);

    // account for hemisphere
    if (coord->hemisphere == 'S' || coord->hemisphere == 'W') {
        result = -result;
    }

    return (int32_t)result;
}

// knots to m/s: 1 knot = 0.514444 m/s
// ret value is m/s * 1000
int32_t tiny_nmea_knots_to_mps(const tiny_nmea_float_t *knots) {
    if (knots->scale == 0) return 0;
    // knots * 514444 / (scale * 1000)
    // Simplify: knots * 514 / scale (approximately)
    return (int32_t)(((int64_t)knots->value * 514444) / (knots->scale * 1000));
}

// knots to km/h: 1 knot = 1.852 km/h
// ret value is km/h * 1000
int32_t tiny_nmea_knots_to_kph(const tiny_nmea_float_t *knots) {
    if (knots->scale == 0) return 0;
    // knots * 1852 / scale
    return (int32_t)(((int64_t)knots->value * 1852) / knots->scale);
}

