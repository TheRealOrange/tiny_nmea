//
// Created by Lin Yicheng on 2/1/26.
//

#ifndef TINY_NMEA_NMEA_0183_TYPE_NAMES_H
#define TINY_NMEA_NMEA_0183_TYPE_NAMES_H

#include "internal/nmea_0183_types.h"

const char *tiny_nmea_constellation_name(tiny_nmea_constellation_t t);
const char *tiny_nmea_talker_name(tiny_nmea_talker_t t);
const char *tiny_nmea_sentence_name(tiny_nmea_sentence_type_t t);
const char *tiny_nmea_fix_quality_name(tiny_nmea_fix_quality_t q);
const char *tiny_nmea_faa_mode_name(tiny_nmea_faa_mode_t m);
const char *tiny_nmea_gsa_fix_name(tiny_nmea_gsa_fix_t f);

#endif //TINY_NMEA_NMEA_0183_TYPE_NAMES_H