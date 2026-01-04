//
// Created by dst on 12/29/25.
//

#ifndef TINY_NMEA_CONFIG_H
#define TINY_NMEA_CONFIG_H

// configs

// maximum NMEA sentence length per spec is 82 chars including $ and CRLF
// some receivers emit longer proprietary sentences so we allow override
#ifndef TINY_NMEA_MAX_SENTENCE_LEN
#define TINY_NMEA_MAX_SENTENCE_LEN 82
#endif

// working buf len must be longer than max sentence len
#ifndef TINY_NMEA_WORKING_BUF_LEN
#define TINY_NMEA_WORKING_BUF_LEN 128
#endif

// maximum satellites reported in GSV, usually 4 per message, up to 12 total
#ifndef TINY_NMEA_MAX_SATS_PER_GSV
#define TINY_NMEA_MAX_SATS_PER_GSV 4
#endif

// maximum satellites in each GSA sentence
#ifndef TINY_NMEA_MAX_SATS_GSA
#define TINY_NMEA_MAX_SATS_GSA 12
#endif

// maximum number of PRNs to store per constellation
// determines the type used (uint8_t or uint16_t for PRN)
#ifndef TINY_NMEA_MAX_PRN_PER_CONST
#define TINY_NMEA_MAX_PRN_PER_CONST 255
#endif

// satellite tracker for accumulating gsv/gsa sats data
// queryable structures for sats in view and active sats
// #define TINY_NMEA_ENABLE_SAT_TRACKER
// additional memory usage of sat tracker is num_constellations * max_prn / 8
// since it is stored as bitmask (if sat tracker is enabled, see below)

#ifdef TINY_NMEA_ENABLE_SAT_TRACKER

// maximum satellites in view to store
#ifndef TINY_NMEA_MAX_TRACKED_GSV_SATS
#define TINY_NMEA_MAX_TRACKED_GSV_SATS 64
#endif

// maximum number of active satellites to store
#ifndef TINY_NMEA_MAX_TRACKED_GSA_SATS
#define TINY_NMEA_MAX_TRACKED_GSA_SATS 128
#endif

// default time duration beyond which to consider a GSA burst complete
#ifndef DEFAULT_GSA_BURST_THRESHOLD
#define DEFAULT_GSA_BURST_THRESHOLD 1000
#endif

#endif // TINY_NMEA_ENABLE_SAT_TRACKER

#endif //TINY_NMEA_CONFIG_H