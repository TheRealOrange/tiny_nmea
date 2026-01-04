//
// Created by linyi on 1/4/2026.
//

#ifndef TINY_NMEA_SATS_TRACKING_H
#define TINY_NMEA_SATS_TRACKING_H

#include "internal/config.h"
#include "internal/sats_tracking_types.h"

#ifdef TINY_NMEA_ENABLE_SAT_TRACKER

#include "internal/nmea_0183_types.h"

/**
 * init the sats in view and active sats tracker context
 * @param ctx       empty sat tracker ctx
 */
tiny_nmea_res_t tiny_nmea_sat_tracking_init(tiny_nmea_sats_tracker_ctx_t *ctx);

/**
 * set the time threshold to consider a burst complete for
 * accumulating active satellites data (GSA)
 * @param ctx sat tracker context
 * @param ms burst duration threshold in milliseconds, relies on gps time
 */
tiny_nmea_res_t tiny_nmea_sat_tracking_set_gsa_threshold(tiny_nmea_sats_tracker_ctx_t *ctx, uint32_t ms);

/**
 * Register callbacks for data updates and staleness notifications.
 * To ensure thread safety, do not access ctx fields directly; rely on these callbacks
 * which provide a consistent snapshot of the data.
 *
 * @param cb_view         called when GSV sequence is complete
 * @param cb_active       called when GSA cycle is complete (detected by time/conflict)
 */
tiny_nmea_res_t tiny_nmea_sat_tracking_register_callbacks(tiny_nmea_sats_tracker_ctx_t *ctx,
                                                          tiny_nmea_on_sats_view_cb_t cb_view,
                                                          tiny_nmea_on_sats_active_cb_t cb_active);


#endif

#endif //TINY_NMEA_SATS_TRACKING_H