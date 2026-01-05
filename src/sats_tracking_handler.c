//
// Created by linyi on 1/4/2026.
//

#include "tiny_nmea/internal/config.h"

#ifdef TINY_NMEA_ENABLE_SAT_TRACKER

#include "tiny_nmea/internal/sats_tracking_handler.h"
#include "tiny_nmea/internal/data_formats.h"

#include <stdint.h>
#include <string.h>

static const int64_t DAY_IN_MS = 86400000;

// helper to convert time struct to milliseconds from midnight
// casts to uint32_t to prevent overflow on 16-bit systems
static uint32_t time_to_ms(const tiny_nmea_time_t *t) {
  return ((uint32_t)t->hours * 3600000UL) +
         ((uint32_t)t->minutes * 60000UL) +
         ((uint32_t)t->seconds * 1000UL) +
         (t->microseconds / 1000UL);
}

#define MAX_ROLLOVER_HRS 16
// helper to calculate time difference in milliseconds
// handles midnight rollover (23:59:59 to 00:00:01)
// (assumes time is monotonically increasing)
// returns 0 if invalid
static int64_t get_time_delta_ms(const tiny_nmea_time_t *old_time,
                                 const tiny_nmea_date_t *old_date,
                                 const tiny_nmea_time_t *new_time,
                                 const tiny_nmea_date_t *new_date) {
  // if either time is invalid, we cannot compute a delta
  if (!old_time->valid || !new_time->valid) {
    return 0;
  }

  uint32_t old_ms = time_to_ms(old_time);
  uint32_t new_ms = time_to_ms(new_time);

  // if we have explicit valid dates for both, use them for exact calculation
  if (old_date->valid && new_date && new_date->valid) {
    if (new_date->day == old_date->day) {
      // same day
      return (int64_t)new_ms - (int64_t)old_ms;
    } else {
      // different day
      // assume next day (sequential updates)
      // which assumes that the messages are not more than 1 day apart
      // (and even if it is itll just mark as stale anyway unless you
      // decide to set the staleness interval greater than one day)
      // note that this calculates strictly based on 24h cycle advancement
      return ((int64_t)new_ms + DAY_IN_MS) - (int64_t)old_ms;
    }
  }

  // heuristic rollover when dates are missing/partial
  if (new_ms < old_ms) {
    // assume the time has wrapped around midnight (e.g., 23:59 to 00:00)
    // calculate the difference assuming it is the immediate next day
    int64_t rollover_delta = ((int64_t)new_ms + DAY_IN_MS) - (int64_t)old_ms;

    // assume that if the rollover delta is small (< MAX_ROLLOVER_HRS hour) its likely correct
    // this assumes that the gps data comes in an interval shorter than MAX_ROLLOVER_HRS hrs? probs ok
    if (rollover_delta < (MAX_ROLLOVER_HRS * 60 * 60 * 1000)) {
      return rollover_delta;
    }

    // large negative jump is kind of suspicious
    // return negative value so it is assumed as stale
    return (int64_t)new_ms - (int64_t)old_ms;
  }

  // Standard forward progression within same day
  return (int64_t)new_ms - (int64_t)old_ms;
}

static void reset_active_sats(tiny_nmea_sats_tracker_ctx_t *ctx) {
  memset(ctx->sats_active_bitmask, 0, sizeof(ctx->sats_active_bitmask));
  ctx->num_sats_active = 0;
}

// check if the GSA burst has timed out based on new latest time
// if the time gap between the last GSA update and the current time exceeds
// the threshold, we assume the previous burst is finished
static void check_gsa_burst_completion(tiny_nmea_sats_tracker_ctx_t *ctx,
                                       const tiny_nmea_time_t *new_time,
                                       const tiny_nmea_date_t *new_date) {
  int64_t diff = get_time_delta_ms(&ctx->sats_active_update_time,
                                   &ctx->sats_active_update_date,
                                   new_time,
                                   new_date);

  if (diff > (int64_t)ctx->gsa_burst_threshold) {
    // burst is complete (timed out)
    if (ctx->num_sats_active > 0) {
      // publish whatever we collected
      if (ctx->cb_sats_active) {
        ctx->cb_sats_active(ctx->sats_active_info,
                            ctx->num_sats_active,
                            &ctx->sats_active_update_date,
                            &ctx->sats_active_update_time);
      }
    }
    // reset to prepare for whenever the next burst arrives
    reset_active_sats(ctx);
  }
}

tiny_nmea_res_t tiny_nmea_sat_tracking_update_datetime(tiny_nmea_sats_tracker_ctx_t *ctx, const tiny_nmea_date_t *date, const tiny_nmea_time_t *time) {
  if (!ctx || !date) {
    return TINY_NMEA_INVALID_ARGS;
  }

  // compare the current date and time with the last seen
  // date and time in the context struct
  // if there is a difference greater than the specified
  // max duration for a GSA burst, we publish whatever we have
  // and reset the GSA data
  check_gsa_burst_completion(ctx, time, date);

  // update the last seen date and time in the context struct
  // so that the next GSV/GSA update can use that as the update time
  ctx->last_seen_date = *date;
  ctx->last_seen_time = *time;

  return TINY_NMEA_OK;
}

tiny_nmea_res_t tiny_nmea_sat_tracking_update_time(tiny_nmea_sats_tracker_ctx_t *ctx, const tiny_nmea_time_t *time) {
  if (!ctx || !time) {
    return TINY_NMEA_INVALID_ARGS;
  }

  // compare the current time with the last seen time in the context struct
  // if there is a difference greater than the specified
  // max duration for a GSA burst, we publish whatever we have
  // and reset the GSA data
  check_gsa_burst_completion(ctx, time, NULL);

  // update the last seen time in the context struct
  // so that the next GSV/GSA update can use that as the update time
  // Date is preserved from previous ZDA/RMC
  ctx->last_seen_time = *time;

  return TINY_NMEA_OK;
}

tiny_nmea_res_t tiny_nmea_sat_tracking_update_gsv(tiny_nmea_sats_tracker_ctx_t *ctx, const tiny_nmea_gsv_t *gsv) {
  if (!ctx || !gsv) {
    return TINY_NMEA_INVALID_ARGS;
  }

  // when the GSV message number is 1, we know a new set of GSV data is coming in
  // and we reset and begin accumulating new GSV data
  // when the GSV message number equals the total messages, the update is complete and
  // we can publish the GSV data by invoking the callback

  // if this is the start of a new sequence (Msg 1),
  // or the total messages is no longer equal to the previous
  // we reset the accumulation buffer
  if (gsv->msg_number == 1 || gsv->total_msgs != ctx->sats_in_view_last_sentence) {
    ctx->num_sats_in_view = 0;
    ctx->sats_in_view_last_sentence = 0;
    ctx->sats_in_view_total_sentences = gsv->total_msgs;
  }

  // ensure continuity
  // if we missed a message, the array is corrupt
  if (gsv->msg_number != ctx->sats_in_view_last_sentence + 1) {
    ctx->num_sats_in_view = 0;
    ctx->sats_in_view_last_sentence = 0;
    return TINY_NMEA_OK; // ignore this broken sequence
  }

  ctx->sats_in_view_last_sentence = gsv->msg_number;

  // accumulate data
  for (uint8_t i = 0; i < gsv->sat_count; i++) {
    if (ctx->num_sats_in_view < TINY_NMEA_MAX_TRACKED_GSV_SATS) {
      ctx->sats_in_view_info[ctx->num_sats_in_view] = gsv->sats[i];
      ctx->num_sats_in_view++;
    }
  }

  // publish data
  // when the message number matches the total, the view is complete
  if (gsv->msg_number == gsv->total_msgs) {
    if (ctx->cb_sats_in_view) {
      // passing const pointer to internal buffer
      // user must handle data immediately or copy it
      ctx->cb_sats_in_view(ctx->sats_in_view_info,
                           ctx->num_sats_in_view,
                           &ctx->last_seen_date,
                           &ctx->last_seen_time);
    }
  }

  return TINY_NMEA_OK;
}

static inline void set_bit(uint8_t *bitmask, uint16_t prn) {
  if (prn == 0) return;
  uint16_t idx = prn / 8;
  uint8_t bit = prn % 8;
  bitmask[idx] |= (1 << bit);
}

static inline bool check_bit(const uint8_t *bitmask, uint16_t prn) {
  if (prn == 0) return false;
  uint16_t idx = prn / 8;
  uint8_t bit = prn % 8;
  return (bitmask[idx] & (1 << bit)) != 0;
}

tiny_nmea_res_t tiny_nmea_sat_tracking_update_gsa(tiny_nmea_sats_tracker_ctx_t *ctx, const tiny_nmea_gsa_t *gsa, tiny_nmea_talker_t talker)  {
  if (!ctx || !gsa) {
    return TINY_NMEA_INVALID_ARGS;
  }

  // when the GSA sentence comes in, (handling GSA bursts is tricky!)
  // if at any point the GSA sentence conflicts with the already seen bitmask, we know it is a new set.
  // so we must discard the old set
  // but how do we know if the new one is done building?
  // we have to assume the GSA data comes in a short burst, and
  // if its past the burst duration, we have a complete set and can
  // publish it by invoking the callback

  // check reset conditions
  // check if the current active buffer is "old" compared to system time
  // handles the case where update_time() hasn't been called yet, or
  // if the threshold is tight
  int64_t diff = get_time_delta_ms(&ctx->sats_active_update_time,
                                   &ctx->sats_active_update_date,
                                   &ctx->last_seen_time,
                                   &ctx->last_seen_date);

  if (diff > (int64_t)ctx->gsa_burst_threshold) {
    if (ctx->num_sats_active > 0 && ctx->cb_sats_active) {
      ctx->cb_sats_active(ctx->sats_active_info,
                          ctx->num_sats_active,
                          &ctx->sats_active_update_date,
                          &ctx->sats_active_update_time);
    }
    reset_active_sats(ctx);
  }

  // determine constellation
  // use NMEA 4.11 system ID if available, otherwise fallback to talker ID
  tiny_nmea_constellation_t constellation = TINY_NMEA_CONSTELLATION_GP; // default to GPS

  if (gsa->system_id > 0) {
    switch(gsa->system_id) {
    case 1: constellation = TINY_NMEA_CONSTELLATION_GP; break; // GPS
    case 2: constellation = TINY_NMEA_CONSTELLATION_GL; break; // GLONASS
    case 3: constellation = TINY_NMEA_CONSTELLATION_GA; break; // Galileo
    case 4: constellation = TINY_NMEA_CONSTELLATION_GB; break; // BeiDou
    default: constellation = TINY_NMEA_CONSTELLATION_GN; break;
    }
  } else {
    // fallback to talker ID
    tiny_nmea_constellation_t derived = tiny_nmea_const_from_talker(talker);
    if (derived != TINY_NMEA_CONSTELLATION_UNKNOWN) {
      constellation = derived;
    }
  }

  // duplicate detection
  // check for conflicts in the bitmask, if a PRN is already present in the active mask,
  // it implies the previous cycle has finished and a new cycle has started
  // assuming GNSS active sets have high persistence, an overlap SHOULD occur
  // if this sentence belongs to a new cycle of the same constellation
  bool conflict = false;
  for (uint8_t i = 0; i < gsa->satellite_count; i++) {
    TINY_NMEA_PRN_TYPE prn = gsa->satellite_prns[i];
    if (prn == 0 || prn >= TINY_NMEA_MAX_PRN_PER_CONST) continue;

    if (check_bit(ctx->sats_active_bitmask[constellation], prn)) {
      conflict = true;
      break;
    }
  }

  if (conflict) {
    // conflict detected, indicates the CURRENT buffer is a complete set from the previous cycle
    // publish it now before we reset
    if (ctx->cb_sats_active) {
      ctx->cb_sats_active(ctx->sats_active_info,
                          ctx->num_sats_active,
                          &ctx->sats_active_update_date,
                          &ctx->sats_active_update_time);
    }

    // callback has terminated, user should have done whatever they need
    // with the data already, so we reset the view
    reset_active_sats(ctx);
  }

  // accumulate data
  for (uint8_t i = 0; i < gsa->satellite_count; i++) {
    TINY_NMEA_PRN_TYPE prn = gsa->satellite_prns[i];
    if (prn == 0 || prn >= TINY_NMEA_MAX_PRN_PER_CONST) continue;

    // mark as seen
    set_bit(ctx->sats_active_bitmask[constellation], prn);

    // add to active info list if capacity allows
    if (ctx->num_sats_active < TINY_NMEA_MAX_TRACKED_GSA_SATS) {
      ctx->sats_active_info[ctx->num_sats_active].prn = prn;
      ctx->sats_active_info[ctx->num_sats_active].constellation = constellation;
      ctx->num_sats_active++;
    }
  }

  // sync buffer timestamps to prevent immediate timeout on next call
  ctx->sats_active_update_time = ctx->last_seen_time;
  ctx->sats_active_update_date = ctx->last_seen_date;

  return TINY_NMEA_OK;
}

#endif