//
// Created by linyi on 1/4/2026.
//

#include "tiny_nmea/sats_tracking.h"
#include "tiny_nmea/internal/sats_tracking_handler.h"

#include <string.h>

tiny_nmea_res_t tiny_nmea_sat_tracking_init(tiny_nmea_sats_tracker_ctx_t *ctx) {
  if (!ctx) return TINY_NMEA_INVALID_ARGS;

  memset(ctx, 0, sizeof(tiny_nmea_sats_tracker_ctx_t));

  ctx->gsa_burst_threshold = DEFAULT_GSA_BURST_THRESHOLD;

  // explicitly ensure callbacks are null
  ctx->cb_sats_in_view = NULL;
  ctx->cb_sats_active = NULL;

  return TINY_NMEA_OK;
}

tiny_nmea_res_t tiny_nmea_sat_tracking_set_gsa_threshold(tiny_nmea_sats_tracker_ctx_t *ctx, uint32_t ms) {
  if (!ctx) return TINY_NMEA_INVALID_ARGS;
  ctx->gsa_burst_threshold = ms;
  return TINY_NMEA_OK;
}

tiny_nmea_res_t tiny_nmea_sat_tracking_register_callbacks(tiny_nmea_sats_tracker_ctx_t *ctx,
                                                          tiny_nmea_on_sats_view_cb_t cb_view,
                                                          tiny_nmea_on_sats_active_cb_t cb_active) {
  if (!ctx) return TINY_NMEA_INVALID_ARGS;
  ctx->cb_sats_in_view = cb_view;
  ctx->cb_sats_active = cb_active;
  return TINY_NMEA_OK;
}


tiny_nmea_res_t tiny_nmea_sat_tracking_update_sentence(tiny_nmea_sats_tracker_ctx_t *ctx, tiny_nmea_type_t parse_res) {
  if (!ctx) {
    return TINY_NMEA_INVALID_ARGS;
  }

  tiny_nmea_res_t ret = TINY_NMEA_OK;
  switch (parse_res.type) {
    // for RMC and ZDA
    // full UTC time + date available
    // RMC sentences will only have valid year after
    // at least one valid ZDA sentence has been parsed
    case TINY_NMEA_SENTENCE_RMC:
      ret = tiny_nmea_sat_tracking_update_datetime(ctx, &parse_res.data.rmc.date, &parse_res.data.rmc.time);
      break;
    case TINY_NMEA_SENTENCE_ZDA:
      ret = tiny_nmea_sat_tracking_update_datetime(ctx, &parse_res.data.zda.date, &parse_res.data.zda.time);
      break;

    // only UTC time available
    case TINY_NMEA_SENTENCE_GGA:
      ret = tiny_nmea_sat_tracking_update_time(ctx, &parse_res.data.gga.time);
      break;
    case TINY_NMEA_SENTENCE_GLL:
      ret = tiny_nmea_sat_tracking_update_time(ctx, &parse_res.data.gll.time);
      break;
    case TINY_NMEA_SENTENCE_GBS:
      ret = tiny_nmea_sat_tracking_update_time(ctx, &parse_res.data.gbs.time);
      break;
    case TINY_NMEA_SENTENCE_GST:
      ret = tiny_nmea_sat_tracking_update_time(ctx, &parse_res.data.gst.time);
      break;
    case TINY_NMEA_SENTENCE_GNS:
      ret = tiny_nmea_sat_tracking_update_time(ctx, &parse_res.data.gns.time);
      break;

    // sat tracking handlers
    case TINY_NMEA_SENTENCE_GSV:
      ret = tiny_nmea_sat_tracking_update_gsv(ctx, &parse_res.data.gsv);
      break;
    case TINY_NMEA_SENTENCE_GSA:
      ret = tiny_nmea_sat_tracking_update_gsa(ctx, &parse_res.data.gsa, parse_res.talker);
      break;

    // other sentences are ignored
    default:
      break;
  }

  return ret;
}