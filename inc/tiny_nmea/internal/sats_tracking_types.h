//
// Created by linyi on 1/4/2026.
//

#ifndef TINY_NMEA_SATS_TRACKING_TYPES_H
#define TINY_NMEA_SATS_TRACKING_TYPES_H

#include "config.h"

#ifdef TINY_NMEA_ENABLE_SAT_TRACKER

#include "nmea_0183_types.h"

_Static_assert(TINY_NMEA_MAX_TRACKED_GSV_SATS > 0,
               "TINY_NMEA_MAX_TRACKED_GSV_SATS must be positive");

_Static_assert(TINY_NMEA_MAX_TRACKED_GSV_SATS <= UINT8_MAX,
               "TINY_NMEA_MAX_TRACKED_GSV_SATS must be less than UINT8_MAX");

_Static_assert(TINY_NMEA_MAX_TRACKED_GSA_SATS > 0,
               "TINY_NMEA_MAX_TRACKED_GSA_SATS must be positive");

_Static_assert(TINY_NMEA_MAX_TRACKED_GSA_SATS <= UINT8_MAX,
               "TINY_NMEA_MAX_TRACKED_GSA_SATS must be less than UINT8_MAX");

#define TINY_NMEA_TRACK_PRN_PER_CONST_ARR_SIZE (TINY_NMEA_MAX_PRN_PER_CONST / 8)

typedef struct {
  TINY_NMEA_PRN_TYPE prn;  // satellite ID/PRN
  tiny_nmea_constellation_t constellation;
} tiny_nmea_gsa_sat_info_t;

/**
 * Called when a full sequence of GSV (Satellites in View) messages is complete.
 * @param sats      Array of satellites in view
 * @param count     Number of satellites in the array
 * @param timestamp Time of this update
 */
typedef void (*tiny_nmea_on_sats_view_cb_t)(const tiny_nmea_sat_info_t *sats,
                                            uint8_t count,
                                            const tiny_nmea_date_t *date,
                                            const tiny_nmea_time_t *time);

/**
 * Called when a full cycle of GSA (Active Satellites) messages is complete.
 * Triggered when Time advances (New Epoch) or a Conflict is detected (New Cycle).
 * @param sats      Array of active satellites
 * @param count     Number of satellites in the array
 * @param timestamp Time of the *system* when this set was finalized
 */
typedef void (*tiny_nmea_on_sats_active_cb_t)(const tiny_nmea_gsa_sat_info_t *sats,
                                              uint8_t count,
                                              const tiny_nmea_date_t *date,
                                              const tiny_nmea_time_t *time);



typedef struct {
  // INTERNAL DATA DO NOT ACCESS DIRECTLY
  // Use callbacks to receive thread-safe snapshots

  // GSA satellites in view tracking info
  // bitmask for deduplication/realising it is stale data
  // (if a PRN is seen which is already in the sats_active_bitmask, then
  // we treat the entire GSA update as a new one and overwrite the previous one)
  uint8_t sats_active_bitmask[TINY_NMEA_CONSTELLATION_COUNT][TINY_NMEA_TRACK_PRN_PER_CONST_ARR_SIZE];
  tiny_nmea_gsa_sat_info_t sats_active_info[TINY_NMEA_MAX_TRACKED_GSA_SATS];
  uint8_t num_sats_active;
  // timestamp of the data currently in the GSA buffer
  tiny_nmea_time_t sats_active_update_time;
  tiny_nmea_date_t sats_active_update_date;

  // GSV satellites in view tracking data
  tiny_nmea_sat_info_t sats_in_view_info[TINY_NMEA_MAX_TRACKED_GSV_SATS];
  uint8_t sats_in_view_total_sentences;
  uint8_t sats_in_view_last_sentence;
  uint8_t num_sats_in_view;

  // time configuration for considering a gsa burst complete
  uint32_t gsa_burst_threshold;

  // current running latest time and date
  tiny_nmea_time_t last_seen_time;
  tiny_nmea_date_t last_seen_date;

  // callbacks
  tiny_nmea_on_sats_view_cb_t cb_sats_in_view;
  tiny_nmea_on_sats_active_cb_t cb_sats_active;

} tiny_nmea_sats_tracker_ctx_t;

#endif

#endif //TINY_NMEA_SATS_TRACKING_TYPES_H