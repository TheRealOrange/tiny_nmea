//
// Created by linyi on 1/4/2026.
//

#ifndef TINY_NMEA_SATS_TRACKING_HANDLER_H
#define TINY_NMEA_SATS_TRACKING_HANDLER_H

#include "config.h"

#ifdef TINY_NMEA_ENABLE_SAT_TRACKER

#include "sats_tracking_types.h"
#include "nmea_0183_types.h"

tiny_nmea_res_t tiny_nmea_sat_tracking_update_datetime(tiny_nmea_sats_tracker_ctx_t *ctx, const tiny_nmea_date_t *date, const tiny_nmea_time_t *time);
tiny_nmea_res_t tiny_nmea_sat_tracking_update_time(tiny_nmea_sats_tracker_ctx_t *ctx, const tiny_nmea_time_t *time);

tiny_nmea_res_t tiny_nmea_sat_tracking_update_gsv(tiny_nmea_sats_tracker_ctx_t *ctx, const tiny_nmea_gsv_t *gsv);
tiny_nmea_res_t tiny_nmea_sat_tracking_update_gsa(tiny_nmea_sats_tracker_ctx_t *ctx, const tiny_nmea_gsa_t *gsa, tiny_nmea_talker_t talker);

#endif

#endif //TINY_NMEA_SATS_TRACKING_HANDLER_H