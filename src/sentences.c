//
// Created by dst on 12/31/25.
//

#include "tiny_nmea/internal/sentences.h"
#include "tiny_nmea/internal/parse_sentence_fields.h"

#include <string.h>

// helper to parse status character to bool ('A' = valid)
static inline bool parse_status_valid(const field_t *f) {
  return !field_empty(f) && f->ptr[0] == 'A';
}

// helper to parse faa mode character to enum
static inline tiny_nmea_faa_mode_t parse_faa_mode_field(const field_t *f) {
  if (field_empty(f)) return TINY_NMEA_FAA_UNKNOWN;
  return parse_faa_mode(f->ptr[0]);
}

// helper to parse nav status character to enum
static inline tiny_nmea_nav_status_t parse_nav_status_field(const field_t *f) {
  if (field_empty(f)) return TINY_NMEA_NAV_STATUS_UNKNOWN;
  return parse_nav_status(f->ptr[0]);
}

// helper to parse gsa fix type character to enum
static inline tiny_nmea_gsa_fix_t parse_gsa_fix_field(const field_t *f) {
  if (field_empty(f)) return TINY_NMEA_GSA_FIX_UNKNOWN;
  return parse_gsa_fix(f->ptr[0]);
}

// rmc - recommended minimum navigation information
// format: $xxRMC,time,status,lat,ns,lon,ew,spd,cog,date,magvar,magdir[,mode[,navstatus]]*cs
//
// field  description                          required
//   0    utc time (hhmmss.ss)                 yes
//   1    status (A=valid, V=warning)          yes
//   2    latitude (ddmm.mmmm)                 yes*
//   3    n/s indicator                        yes*
//   4    longitude (dddmm.mmmm)               yes*
//   5    e/w indicator                        yes*
//   6    speed over ground (knots)            yes*
//   7    course over ground (degrees true)    yes*
//   8    date (ddmmyy)                        yes
//   9    magnetic variation (degrees)         no
//  10    magnetic variation direction (E/W)   no
//  11    faa mode indicator (nmea 2.3+)       no
//  12    nav status (nmea 4.1+)               no
//
// * may be empty if no valid fix
#define RMC_MIN_FIELDS 11
#define RMC_MAX_FIELDS 13

tiny_nmea_res_t handle_parse_rmc(const char *sentence, size_t len, tiny_nmea_rmc_t *data) {
  if (!sentence || !data) return TINY_NMEA_ERR_NULL_PTR;

  field_t f[RMC_MAX_FIELDS];
  uint8_t count = tokenize(sentence, len, f, RMC_MAX_FIELDS);

  if (count < RMC_MIN_FIELDS) return TINY_NMEA_ERR_TOO_FEW_FIELDS;

  memset(data, 0, sizeof(*data));

  // field 0: time (may be empty)
  parse_time(&f[0], &data->time);

  // field 1: status
  data->status_valid = parse_status_valid(&f[1]);

  // fields 2-3: latitude (may be empty if no fix)
  parse_latitude(&f[2], &f[3], &data->latitude);

  // fields 4-5: longitude (may be empty if no fix)
  parse_longitude(&f[4], &f[5], &data->longitude);

  // field 6: speed over ground
  parse_fixedpoint_float(&f[6], &data->speed_knots);

  // field 7: course over ground
  parse_fixedpoint_float(&f[7], &data->course_deg);

  // field 8: date (may be empty)
  parse_date(&f[8], &data->date);

  // field 9: magnetic variation (optional)
  parse_fixedpoint_float(&f[9], &data->mag_variation);

  // field 10: magnetic variation direction (optional)
  parse_char(&f[10], &data->mag_var_dir);

  // field 11: faa mode indicator (nmea 2.3+, optional)
  if (count > 11) {
    data->faa_mode = parse_faa_mode_field(&f[11]);
  }

  // field 12: navigation status (nmea 4.1+, optional)
  if (count > 12) {
    data->nav_status = parse_nav_status_field(&f[12]);
  }

  return TINY_NMEA_OK;
}

// gga - global positioning system fix data
// format: $xxGGA,time,lat,ns,lon,ew,qual,numsv,hdop,alt,M,sep,M,age,stnid*cs
//
// field  description                          required
//   0    utc time (hhmmss.ss)                 yes
//   1    latitude (ddmm.mmmm)                 yes*
//   2    n/s indicator                        yes*
//   3    longitude (dddmm.mmmm)               yes*
//   4    e/w indicator                        yes*
//   5    fix quality (0-8)                    yes
//   6    number of satellites used            yes
//   7    hdop                                 yes*
//   8    altitude above msl (meters)          yes*
//   9    altitude units (always 'M')          yes
//  10    geoidal separation (meters)          yes*
//  11    separation units (always 'M')        yes
//  12    dgps data age (seconds)              no
//  13    dgps station id                      no
//
// * may be empty if no valid fix
#define GGA_MIN_FIELDS 14
#define GGA_MAX_FIELDS 15

tiny_nmea_res_t handle_parse_gga(const char *sentence, size_t len, tiny_nmea_gga_t *data) {
  if (!sentence || !data) return TINY_NMEA_ERR_NULL_PTR;

  field_t f[GGA_MAX_FIELDS];
  uint8_t count = tokenize(sentence, len, f, GGA_MAX_FIELDS);

  if (count < GGA_MIN_FIELDS) return TINY_NMEA_ERR_TOO_FEW_FIELDS;

  memset(data, 0, sizeof(*data));

  // field 0: time
  parse_time(&f[0], &data->time);

  // fields 1-2: latitude
  parse_latitude(&f[1], &f[2], &data->latitude);

  // fields 3-4: longitude
  parse_longitude(&f[3], &f[4], &data->longitude);

  // field 5: fix quality
  uint32_t tmp;
  if (parse_uint(&f[5], &tmp)) {
    data->fix_quality = (tiny_nmea_fix_quality_t)tmp;
  }

  // field 6: number of satellites used
  if (parse_uint(&f[6], &tmp)) {
    data->satellites_used = (uint8_t)tmp;
  }

  // field 7: hdop
  parse_fixedpoint_float(&f[7], &data->hdop);

  // field 8: altitude (field 9 is units, always 'M')
  parse_fixedpoint_float(&f[8], &data->altitude_m);

  // field 10: geoidal separation (field 11 is units, always 'M')
  parse_fixedpoint_float(&f[10], &data->geoid_sep_m);

  // field 12: dgps age (optional, may be empty)
  parse_fixedpoint_float(&f[12], &data->dgps_age_sec);

  // field 13: dgps station id (optional, may be empty)
  if (parse_uint(&f[13], &tmp)) {
    data->dgps_station_id = (uint16_t)tmp;
  }

  return TINY_NMEA_OK;
}

// gns - gnss fix data (nmea 3.0+)
// format: $xxGNS,time,lat,ns,lon,ew,mode,numsv,hdop,alt,sep,age,stnid[,navstatus]*cs
//
// field  description                          required
//   0    utc time (hhmmss.ss)                 yes
//   1    latitude (ddmm.mmmm)                 yes*
//   2    n/s indicator                        yes*
//   3    longitude (dddmm.mmmm)               yes*
//   4    e/w indicator                        yes*
//   5    mode indicators (1 char per const)   yes
//   6    number of satellites used            yes
//   7    hdop                                 yes*
//   8    altitude above msl (meters)          yes*
//   9    geoidal separation (meters)          yes*
//  10    dgps data age (seconds)              no
//  11    dgps station id                      no
//  12    nav status (nmea 4.1+)               no
//
// * may be empty if no valid fix
// mode chars: N=none, A=auto, D=diff, P=precise, R=rtk, F=float, E=est, M=manual, S=sim
#define GNS_MIN_FIELDS 12
#define GNS_MAX_FIELDS 14

tiny_nmea_res_t handle_parse_gns(const char *sentence, size_t len, tiny_nmea_gns_t *data) {
  if (!sentence || !data) return TINY_NMEA_ERR_NULL_PTR;

  field_t f[GNS_MAX_FIELDS];
  uint8_t count = tokenize(sentence, len, f, GNS_MAX_FIELDS);

  if (count < GNS_MIN_FIELDS) return TINY_NMEA_ERR_TOO_FEW_FIELDS;

  memset(data, 0, sizeof(*data));

  // field 0: time
  parse_time(&f[0], &data->time);

  // fields 1-2: latitude
  parse_latitude(&f[1], &f[2], &data->latitude);

  // fields 3-4: longitude
  parse_longitude(&f[3], &f[4], &data->longitude);

  // field 5: mode indicators (one char per constellation)
  if (!field_empty(&f[5])) {
    uint8_t mode_len = f[5].len;
    if (mode_len > TINY_NMEA_CONSTELLATION_COUNT) {
      mode_len = TINY_NMEA_CONSTELLATION_COUNT;
    }
    for (uint8_t i = 0; i < mode_len; i++) {
      data->mode[i] = parse_faa_mode(f[5].ptr[i]);
    }
    data->mode_count = mode_len;
  }

  // field 6: number of satellites used
  uint32_t tmp;
  if (parse_uint(&f[6], &tmp)) {
    data->satellites_used = (uint8_t)tmp;
  }

  // field 7: hdop
  parse_fixedpoint_float(&f[7], &data->hdop);

  // field 8: altitude
  parse_fixedpoint_float(&f[8], &data->altitude_m);

  // field 9: geoidal separation
  parse_fixedpoint_float(&f[9], &data->geoid_sep_m);

  // field 10: dgps age (optional)
  parse_fixedpoint_float(&f[10], &data->dgps_age_sec);

  // field 11: dgps station id (optional)
  if (parse_uint(&f[11], &tmp)) {
    data->dgps_station_id = (uint16_t)tmp;
  }

  // field 12: navigation status (nmea 4.1+, optional)
  if (count > 12) {
    data->nav_status = parse_nav_status_field(&f[12]);
  }

  return TINY_NMEA_OK;
}

// gsa - gnss dop and active satellites
// format: $xxGSA,mode,fix,sv1,sv2,...,sv12,pdop,hdop,vdop[,sysid]*cs
//
// field  description                          required
//   0    mode selection (M=manual, A=auto)    yes
//   1    fix type (1=none, 2=2d, 3=3d)        yes
//  2-13  satellite prns (up to 12)            no (empty if unused)
//  14    pdop                                 yes*
//  15    hdop                                 yes*
//  16    vdop                                 yes*
//  17    system id (nmea 4.11+)               no
//
// * may be empty if no fix
#define GSA_MIN_FIELDS 17
#define GSA_MAX_FIELDS 18

tiny_nmea_res_t handle_parse_gsa(const char *sentence, size_t len, tiny_nmea_gsa_t *data) {
  if (!sentence || !data) return TINY_NMEA_ERR_NULL_PTR;

  field_t f[GSA_MAX_FIELDS];
  uint8_t count = tokenize(sentence, len, f, GSA_MAX_FIELDS);

  if (count < GSA_MIN_FIELDS) return TINY_NMEA_ERR_TOO_FEW_FIELDS;

  memset(data, 0, sizeof(*data));

  // field 0: mode selection
  parse_char(&f[0], &data->mode_selection);

  // field 1: fix type
  data->fix_type = parse_gsa_fix_field(&f[1]);

  // fields 2-13: satellite prns (may be empty)
  uint32_t tmp;
  data->satellite_count = 0;
  for (uint8_t i = 0; i < 12 && data->satellite_count < TINY_NMEA_MAX_SATS_GSA; i++) {
    if (!field_empty(&f[2 + i]) && parse_uint(&f[2 + i], &tmp)) {
      data->satellite_prns[data->satellite_count++] = (TINY_NMEA_PRN_TYPE)tmp;
    }
  }

  // field 14: pdop
  parse_fixedpoint_float(&f[14], &data->pdop);

  // field 15: hdop
  parse_fixedpoint_float(&f[15], &data->hdop);

  // field 16: vdop
  parse_fixedpoint_float(&f[16], &data->vdop);

  // field 17: system id (nmea 4.11+, optional)
  if (count > 17 && parse_uint(&f[17], &tmp)) {
    data->system_id = (uint8_t)tmp;
  }

  return TINY_NMEA_OK;
}

// gsv - gnss satellites in view
// format: $xxGSV,total,msgnum,numsv[,prn,elev,az,snr]...[,sigid]*cs
//
// field  description                          required
//   0    total number of messages             yes
//   1    message number (1-based)             yes
//   2    total satellites in view             yes
// 3-6    satellite 1 (prn,elev,az,snr)        no
// 7-10   satellite 2 (prn,elev,az,snr)        no
// 11-14  satellite 3 (prn,elev,az,snr)        no
// 15-18  satellite 4 (prn,elev,az,snr)        no
//  19    signal id (nmea 4.11+)               no
//
// up to 4 satellites per message
// snr may be empty if satellite not tracked
#define GSV_MIN_FIELDS 3
#define GSV_MAX_FIELDS 20

tiny_nmea_res_t handle_parse_gsv(const char *sentence, size_t len, tiny_nmea_gsv_t *data) {
  if (!sentence || !data) return TINY_NMEA_ERR_NULL_PTR;

  field_t f[GSV_MAX_FIELDS];
  uint8_t count = tokenize(sentence, len, f, GSV_MAX_FIELDS);

  if (count < GSV_MIN_FIELDS) return TINY_NMEA_ERR_TOO_FEW_FIELDS;

  memset(data, 0, sizeof(*data));

  uint32_t tmp;

  // field 0: total messages
  if (parse_uint(&f[0], &tmp)) data->total_msgs = (uint8_t)tmp;

  // field 1: message number
  if (parse_uint(&f[1], &tmp)) data->msg_number = (uint8_t)tmp;

  // field 2: total satellites in view
  if (parse_uint(&f[2], &tmp)) data->total_sats = (uint8_t)tmp;

  // satellite blocks (up to 4 per message)
  data->sat_count = 0;
  for (uint8_t i = 0; i < TINY_NMEA_MAX_SATS_PER_GSV; i++) {
    uint8_t base = 3 + (i * 4);
    if (base >= count) break;
    if (field_empty(&f[base])) continue;

    tiny_nmea_sat_info_t *sat = &data->sats[data->sat_count];

    // init with invalid markers
    sat->prn = 0;
    sat->elevation = -128;
    sat->azimuth = -1;
    sat->snr = -1;

    // prn is required for a valid satellite entry
    if (!parse_uint(&f[base], &tmp)) continue;
    sat->prn = (TINY_NMEA_PRN_TYPE)tmp;

    // elevation (optional)
    int32_t stmp;
    if (base + 1 < count && parse_int(&f[base + 1], &stmp)) {
      sat->elevation = (int8_t)stmp;
    }

    // azimuth (optional)
    if (base + 2 < count && parse_uint(&f[base + 2], &tmp)) {
      sat->azimuth = (int16_t)tmp;
    }

    // snr (optional, may be empty if not tracking)
    if (base + 3 < count && parse_int(&f[base + 3], &stmp)) {
      sat->snr = (int8_t)stmp;
    }

    data->sat_count++;
  }

  // signal id (nmea 4.11+, optional)
  // located after the last satellite block
  uint8_t sig_idx = 3 + (data->sat_count * 4);
  if (sig_idx < count && parse_uint(&f[sig_idx], &tmp)) {
    data->signal_id = (uint8_t)tmp;
  }

  return TINY_NMEA_OK;
}

// vtg - course over ground and ground speed
// format: $xxVTG,cogt,T,cogm,M,sog,N,sokph,K[,mode]*cs
//
// field  description                          required
//   0    course over ground true (degrees)    yes*
//   1    'T' indicator                        yes
//   2    course over ground magnetic (deg)    no
//   3    'M' indicator                        yes
//   4    speed over ground (knots)            yes*
//   5    'N' indicator                        yes
//   6    speed over ground (km/h)             yes*
//   7    'K' indicator                        yes
//   8    faa mode indicator (nmea 2.3+)       no
//
// * may be empty if no valid fix
#define VTG_MIN_FIELDS 8
#define VTG_MAX_FIELDS 10

tiny_nmea_res_t handle_parse_vtg(const char *sentence, size_t len, tiny_nmea_vtg_t *data) {
  if (!sentence || !data) return TINY_NMEA_ERR_NULL_PTR;

  field_t f[VTG_MAX_FIELDS];
  uint8_t count = tokenize(sentence, len, f, VTG_MAX_FIELDS);

  if (count < VTG_MIN_FIELDS) return TINY_NMEA_ERR_TOO_FEW_FIELDS;

  memset(data, 0, sizeof(*data));

  // field 0: course true (field 1 is 'T')
  parse_fixedpoint_float(&f[0], &data->course_true_deg);

  // field 2: course magnetic (field 3 is 'M')
  parse_fixedpoint_float(&f[2], &data->course_mag_deg);

  // field 4: speed knots (field 5 is 'N')
  parse_fixedpoint_float(&f[4], &data->speed_knots);

  // field 6: speed km/h (field 7 is 'K')
  parse_fixedpoint_float(&f[6], &data->speed_kph);

  // field 8: faa mode (nmea 2.3+, optional)
  if (count > 8) {
    data->faa_mode = parse_faa_mode_field(&f[8]);
  }

  return TINY_NMEA_OK;
}

// gll - geographic position (latitude/longitude)
// format: $xxGLL,lat,ns,lon,ew,time,status[,mode]*cs
//
// field  description                          required
//   0    latitude (ddmm.mmmm)                 yes*
//   1    n/s indicator                        yes*
//   2    longitude (dddmm.mmmm)               yes*
//   3    e/w indicator                        yes*
//   4    utc time (hhmmss.ss)                 yes
//   5    status (A=valid, V=warning)          yes
//   6    faa mode indicator (nmea 2.3+)       no
//
// * may be empty if no valid fix
#define GLL_MIN_FIELDS 6
#define GLL_MAX_FIELDS 8

tiny_nmea_res_t handle_parse_gll(const char *sentence, size_t len, tiny_nmea_gll_t *data) {
  if (!sentence || !data) return TINY_NMEA_ERR_NULL_PTR;

  field_t f[GLL_MAX_FIELDS];
  uint8_t count = tokenize(sentence, len, f, GLL_MAX_FIELDS);

  if (count < GLL_MIN_FIELDS) return TINY_NMEA_ERR_TOO_FEW_FIELDS;

  memset(data, 0, sizeof(*data));

  // fields 0-1: latitude
  parse_latitude(&f[0], &f[1], &data->latitude);

  // fields 2-3: longitude
  parse_longitude(&f[2], &f[3], &data->longitude);

  // field 4: time
  parse_time(&f[4], &data->time);

  // field 5: status
  data->status_valid = parse_status_valid(&f[5]);

  // field 6: faa mode (nmea 2.3+, optional)
  if (count > 6) {
    data->faa_mode = parse_faa_mode_field(&f[6]);
  }

  return TINY_NMEA_OK;
}

// zda - time and date
// format: $xxZDA,time,day,month,year,ltzh,ltzn*cs
//
// field  description                          required
//   0    utc time (hhmmss.ss)                 yes
//   1    day (01-31)                          yes
//   2    month (01-12)                        yes
//   3    year (4 digits)                      yes
//   4    local zone hours (-13 to +13)        yes*
//   5    local zone minutes (00-59)           yes*
//
// * may be empty (00 if not known)
#define ZDA_MIN_FIELDS 6
#define ZDA_MAX_FIELDS 7

tiny_nmea_res_t handle_parse_zda(const char *sentence, size_t len, tiny_nmea_zda_t *data) {
  if (!sentence || !data) return TINY_NMEA_ERR_NULL_PTR;

  field_t f[ZDA_MAX_FIELDS];
  uint8_t count = tokenize(sentence, len, f, ZDA_MAX_FIELDS);

  if (count < ZDA_MIN_FIELDS) return TINY_NMEA_ERR_TOO_FEW_FIELDS;

  memset(data, 0, sizeof(*data));

  // field 0: time
  if (!parse_time(&f[0], &data->time)) {
    return TINY_NMEA_ERR_INVALID_TIME;
  }

  // fields 1-3: date (day, month, year as separate fields)
  uint32_t tmp;
  if (parse_uint(&f[1], &tmp)) data->date.day = (uint8_t)tmp;
  if (parse_uint(&f[2], &tmp)) data->date.month = (uint8_t)tmp;
  if (parse_uint(&f[3], &tmp)) data->date.year = (uint16_t)tmp;

  // validate date
  if (data->date.day < 1 || data->date.day > 31 ||
      data->date.month < 1 || data->date.month > 12) {
    return TINY_NMEA_ERR_INVALID_DATE;
  }
  data->date.valid = true;

  // fields 4-5: local timezone offset (optional, may be empty)
  int32_t stmp;
  if (parse_int(&f[4], &stmp)) data->tz_hours = (int8_t)stmp;
  if (parse_int(&f[5], &stmp)) data->tz_minutes = (uint8_t)stmp;

  return TINY_NMEA_OK;
}

// gbs - gnss satellite fault detection
// format: $xxGBS,time,errlat,errlon,erralt,prn,prob,bias,stddev*cs
//
// field  description                          required
//   0    utc time (hhmmss.ss)                 yes
//   1    expected error in latitude (m)       yes*
//   2    expected error in longitude (m)      yes*
//   3    expected error in altitude (m)       yes*
//   4    failed satellite prn                 no
//   5    probability of missed detection      no
//   6    bias estimate (m)                    no
//   7    standard deviation of bias (m)       no
//
// * may be empty if not computed
#define GBS_MIN_FIELDS 8
#define GBS_MAX_FIELDS 9

tiny_nmea_res_t handle_parse_gbs(const char *sentence, size_t len, tiny_nmea_gbs_t *data) {
  if (!sentence || !data) return TINY_NMEA_ERR_NULL_PTR;

  field_t f[GBS_MAX_FIELDS];
  uint8_t count = tokenize(sentence, len, f, GBS_MAX_FIELDS);

  if (count < GBS_MIN_FIELDS) return TINY_NMEA_ERR_TOO_FEW_FIELDS;

  memset(data, 0, sizeof(*data));

  // field 0: time
  parse_time(&f[0], &data->time);

  // fields 1-3: expected errors
  parse_fixedpoint_float(&f[1], &data->err_lat_m);
  parse_fixedpoint_float(&f[2], &data->err_lon_m);
  parse_fixedpoint_float(&f[3], &data->err_alt_m);

  // field 4: failed satellite prn (optional)
  uint32_t tmp;
  if (parse_uint(&f[4], &tmp)) {
    data->failed_sat_id = (uint8_t)tmp;
  }

  // fields 5-7: fault detection data (optional)
  parse_fixedpoint_float(&f[5], &data->prob_missed);
  parse_fixedpoint_float(&f[6], &data->bias_m);
  parse_fixedpoint_float(&f[7], &data->bias_stddev_m);

  return TINY_NMEA_OK;
}

// gst - gnss pseudorange error statistics
// format: $xxGST,time,rms,smaj,smin,orient,errlat,errlon,erralt*cs
//
// field  description                          required
//   0    utc time (hhmmss.ss)                 yes
//   1    rms of ranges (m)                    yes*
//   2    std dev of semi-major axis (m)       yes*
//   3    std dev of semi-minor axis (m)       yes*
//   4    orientation of semi-major (deg)      yes*
//   5    std dev of latitude error (m)        yes*
//   6    std dev of longitude error (m)       yes*
//   7    std dev of altitude error (m)        yes*
//
// * may be empty if not computed
#define GST_MIN_FIELDS 8
#define GST_MAX_FIELDS 9

tiny_nmea_res_t handle_parse_gst(const char *sentence, size_t len, tiny_nmea_gst_t *data) {
  if (!sentence || !data) return TINY_NMEA_ERR_NULL_PTR;

  field_t f[GST_MAX_FIELDS];
  uint8_t count = tokenize(sentence, len, f, GST_MAX_FIELDS);

  if (count < GST_MIN_FIELDS) return TINY_NMEA_ERR_TOO_FEW_FIELDS;

  memset(data, 0, sizeof(*data));

  // field 0: time
  parse_time(&f[0], &data->time);

  // fields 1-7: error statistics
  parse_fixedpoint_float(&f[1], &data->rms_range);
  parse_fixedpoint_float(&f[2], &data->std_major_m);
  parse_fixedpoint_float(&f[3], &data->std_minor_m);
  parse_fixedpoint_float(&f[4], &data->orient_deg);
  parse_fixedpoint_float(&f[5], &data->std_lat_m);
  parse_fixedpoint_float(&f[6], &data->std_lon_m);
  parse_fixedpoint_float(&f[7], &data->std_alt_m);

  return TINY_NMEA_OK;
}

// ais - vdm/vdo sentences (automatic identification system)
// format: !xxVDM,fragcnt,fragnum,seqid,channel,payload,fillbits*cs
//         !xxVDO,fragcnt,fragnum,seqid,channel,payload,fillbits*cs
//
// field  description                          required
//   0    fragment count (1-9)                 yes
//   1    fragment number (1-based)            yes
//   2    sequential message id                no (empty for single-sentence)
//   3    channel (A/B/1/2)                    yes*
//   4    payload (6-bit armored ascii)        yes
//   5    fill bits (0-5)                      yes
//
// * may be empty
#define AIS_FIELDS 6

tiny_nmea_res_t handle_parse_ais(const char *sentence, size_t len, tiny_nmea_ais_t *data) {
  if (!sentence || !data) return TINY_NMEA_ERR_NULL_PTR;

  field_t f[AIS_FIELDS];
  uint8_t count = tokenize(sentence, len, f, AIS_FIELDS);

  if (count < AIS_FIELDS) return TINY_NMEA_ERR_TOO_FEW_FIELDS;

  memset(data, 0, sizeof(*data));

  uint32_t tmp;

  // field 0: fragment count
  if (parse_uint(&f[0], &tmp)) {
    data->fragment_count = (uint8_t)tmp;
  }

  // field 1: fragment number
  if (parse_uint(&f[1], &tmp)) {
    data->fragment_number = (uint8_t)tmp;
  }

  // field 2: sequential id (optional, empty for single-sentence messages)
  if (!field_empty(&f[2]) && parse_uint(&f[2], &tmp)) {
    data->sequential_id = (uint8_t)tmp;
  }

  // field 3: channel (optional)
  parse_char(&f[3], &data->channel);

  // field 4: payload
  if (!field_empty(&f[4])) {
    uint8_t payload_len = f[4].len;
    if (payload_len > sizeof(data->payload) - 1) {
      payload_len = sizeof(data->payload) - 1;
    }
    memcpy(data->payload, f[4].ptr, payload_len);
    data->payload[payload_len] = '\0';
    data->payload_len = payload_len;
  }

  // field 5: fill bits
  if (parse_uint(&f[5], &tmp)) {
    data->fill_bits = (uint8_t)tmp;
  }

  return TINY_NMEA_OK;
}