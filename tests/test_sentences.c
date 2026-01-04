//
// unit tests for standalone sentence parsing
//

#include "test.h"
#include "tiny_nmea/tiny_nmea.h"
#include "tiny_nmea/internal/sentences.h"

#include <math.h>
#include <string.h>

// RMC tests

static void test_parse_rmc(void) {
  TEST_CASE("parse RMC basic") {
    const char *sentence = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(TINY_NMEA_TALKER_GP, result.talker);
    TEST_ASSERT_EQ(TINY_NMEA_SENTENCE_RMC, result.type);

    TEST_ASSERT(result.data.rmc.time.valid);
    TEST_ASSERT_EQ(12, result.data.rmc.time.hours);
    TEST_ASSERT_EQ(35, result.data.rmc.time.minutes);
    TEST_ASSERT_EQ(19, result.data.rmc.time.seconds);

    TEST_ASSERT(result.data.rmc.status_valid);
    TEST_ASSERT_EQ('N', result.data.rmc.latitude.hemisphere);
    TEST_ASSERT_EQ('E', result.data.rmc.longitude.hemisphere);

    TEST_PASS();
  }

  TEST_CASE("parse RMC invalid status") {
    const char *sentence = "$GPRMC,123519,V,,,,,,,230394,,";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT(!result.data.rmc.status_valid);

    TEST_PASS();
  }

  TEST_CASE("parse RMC with FAA mode") {
    const char *sentence = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W,A";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(TINY_NMEA_FAA_AUTONOMOUS, result.data.rmc.faa_mode);

    TEST_PASS();
  }

  TEST_CASE("parse RMC GLONASS talker") {
    const char *sentence = "$GLRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(TINY_NMEA_TALKER_GL, result.talker);

    TEST_PASS();
  }
}

// GGA tests

static void test_parse_gga(void) {
  TEST_CASE("parse GGA basic") {
    const char *sentence = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,47.0,M,,";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(TINY_NMEA_SENTENCE_GGA, result.type);

    TEST_ASSERT(result.data.gga.time.valid);
    TEST_ASSERT_EQ(TINY_NMEA_FIX_GPS, result.data.gga.fix_quality);
    TEST_ASSERT_EQ(8, result.data.gga.satellites_used);

    TEST_PASS();
  }

  TEST_CASE("parse GGA no fix") {
    const char *sentence = "$GPGGA,123519,,,,,0,00,,,M,,M,,";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(TINY_NMEA_FIX_INVALID, result.data.gga.fix_quality);
    TEST_ASSERT_EQ(0, result.data.gga.satellites_used);

    TEST_PASS();
  }

  TEST_CASE("parse GGA DGPS fix") {
    const char *sentence = "$GPGGA,123519,4807.038,N,01131.000,E,2,08,0.9,545.4,M,47.0,M,1.0,0001";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(TINY_NMEA_FIX_DGPS, result.data.gga.fix_quality);
    TEST_ASSERT_EQ(1, result.data.gga.dgps_station_id);

    TEST_PASS();
  }

  TEST_CASE("parse GGA RTK fix") {
    const char *sentence = "$GPGGA,123519,4807.038,N,01131.000,E,4,12,0.5,100.0,M,47.0,M,,";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(TINY_NMEA_FIX_RTK, result.data.gga.fix_quality);

    TEST_PASS();
  }
}

// GSA tests

static void test_parse_gsa(void) {
  TEST_CASE("parse GSA 3D fix") {
    const char *sentence = "$GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(TINY_NMEA_SENTENCE_GSA, result.type);

    TEST_ASSERT_EQ('A', result.data.gsa.mode_selection);
    TEST_ASSERT_EQ(3, result.data.gsa.fix_type);
    TEST_ASSERT(result.data.gsa.satellite_count >= 4);

    // check satellite IDs
    TEST_ASSERT_EQ(4, result.data.gsa.satellite_prns[0]);
    TEST_ASSERT_EQ(5, result.data.gsa.satellite_prns[1]);

    TEST_PASS();
  }

  TEST_CASE("parse GSA no fix") {
    const char *sentence = "$GPGSA,A,1,,,,,,,,,,,,,,,";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(1, result.data.gsa.fix_type);
    TEST_ASSERT_EQ(0, result.data.gsa.satellite_count);

    TEST_PASS();
  }

  TEST_CASE("parse GSA 2D fix") {
    const char *sentence = "$GPGSA,M,2,04,05,06,,,,,,,,,,3.0,2.0,2.2";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ('M', result.data.gsa.mode_selection);
    TEST_ASSERT_EQ(2, result.data.gsa.fix_type);

    TEST_PASS();
  }

  TEST_CASE("parse GSA with system ID") {
    const char *sentence = "$GNGSA,A,3,01,02,03,04,05,06,07,08,09,10,11,12,1.5,0.9,1.2,1";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(1, result.data.gsa.system_id);

    TEST_PASS();
  }
}

// GSV tests

static void test_parse_gsv(void) {
  TEST_CASE("parse GSV first message") {
    const char *sentence = "$GPGSV,3,1,11,03,03,111,00,04,15,270,00,06,01,010,00,13,06,292,00";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(TINY_NMEA_SENTENCE_GSV, result.type);

    TEST_ASSERT_EQ(3, result.data.gsv.total_msgs);
    TEST_ASSERT_EQ(1, result.data.gsv.msg_number);
    TEST_ASSERT_EQ(11, result.data.gsv.total_sats);
    TEST_ASSERT_EQ(4, result.data.gsv.sat_count);

    // check first satellite
    TEST_ASSERT_EQ(3, result.data.gsv.sats[0].prn);
    TEST_ASSERT_EQ(3, result.data.gsv.sats[0].elevation);
    TEST_ASSERT_EQ(111, result.data.gsv.sats[0].azimuth);
    TEST_ASSERT_EQ(0, result.data.gsv.sats[0].snr);

    TEST_PASS();
  }

  TEST_CASE("parse GSV partial message") {
    const char *sentence = "$GPGSV,3,3,11,30,40,120,35";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(3, result.data.gsv.msg_number);
    TEST_ASSERT_EQ(1, result.data.gsv.sat_count);

    TEST_PASS();
  }

  TEST_CASE("parse GSV with signal ID") {
    const char *sentence = "$GPGSV,2,1,08,01,40,120,42,02,30,090,38,03,60,045,45,04,15,270,30,1";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(1, result.data.gsv.signal_id);

    TEST_PASS();
  }

  TEST_CASE("parse GSV GLONASS") {
    const char *sentence = "$GLGSV,2,1,06,65,45,120,40,66,30,090,35,67,60,045,42,68,15,270,28";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(TINY_NMEA_TALKER_GL, result.talker);
    TEST_ASSERT_EQ(4, result.data.gsv.sat_count);

    TEST_PASS();
  }
}

// VTG tests

static void test_parse_vtg(void) {
  TEST_CASE("parse VTG basic") {
    const char *sentence = "$GPVTG,054.7,T,034.4,M,005.5,N,010.2,K";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(TINY_NMEA_SENTENCE_VTG, result.type);

    double course_true = tiny_nmea_to_double(&result.data.vtg.course_true_deg);
    double course_mag = tiny_nmea_to_double(&result.data.vtg.course_mag_deg);
    double speed_knots = tiny_nmea_to_double(&result.data.vtg.speed_knots);
    double speed_kph = tiny_nmea_to_double(&result.data.vtg.speed_kph);

    TEST_ASSERT_FLOAT_EQ(54.7, course_true, 0.1);
    TEST_ASSERT_FLOAT_EQ(34.4, course_mag, 0.1);
    TEST_ASSERT_FLOAT_EQ(5.5, speed_knots, 0.1);
    TEST_ASSERT_FLOAT_EQ(10.2, speed_kph, 0.1);

    TEST_PASS();
  }

  TEST_CASE("parse VTG with FAA mode") {
    const char *sentence = "$GPVTG,054.7,T,034.4,M,005.5,N,010.2,K,A";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(TINY_NMEA_FAA_AUTONOMOUS, result.data.vtg.faa_mode);

    TEST_PASS();
  }

  TEST_CASE("parse VTG empty fields") {
    const char *sentence = "$GPVTG,,T,,M,,N,,K";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(0, result.data.vtg.course_true_deg.scale);

    TEST_PASS();
  }
}

// GLL tests

static void test_parse_gll(void) {
  TEST_CASE("parse GLL basic") {
    const char *sentence = "$GPGLL,4916.45,N,12311.12,W,225444,A";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(TINY_NMEA_SENTENCE_GLL, result.type);

    TEST_ASSERT_EQ('N', result.data.gll.latitude.hemisphere);
    TEST_ASSERT_EQ('W', result.data.gll.longitude.hemisphere);
    TEST_ASSERT(result.data.gll.status_valid);
    TEST_ASSERT(result.data.gll.time.valid);
    TEST_ASSERT_EQ(22, result.data.gll.time.hours);

    TEST_PASS();
  }

  TEST_CASE("parse GLL invalid") {
    const char *sentence = "$GPGLL,4916.45,N,12311.12,W,225444,V";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT(!result.data.gll.status_valid);

    TEST_PASS();
  }

  TEST_CASE("parse GLL with FAA mode") {
    const char *sentence = "$GPGLL,4916.45,N,12311.12,W,225444,A,D";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(TINY_NMEA_FAA_DIFFERENTIAL, result.data.gll.faa_mode);

    TEST_PASS();
  }
}

// ZDA tests

static void test_parse_zda(void) {
  TEST_CASE("parse ZDA basic") {
    const char *sentence = "$GPZDA,160012.71,11,03,2004,-1,00";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(TINY_NMEA_SENTENCE_ZDA, result.type);

    TEST_ASSERT(result.data.zda.time.valid);
    TEST_ASSERT_EQ(16, result.data.zda.time.hours);
    TEST_ASSERT_EQ(0, result.data.zda.time.minutes);
    TEST_ASSERT_EQ(12, result.data.zda.time.seconds);

    TEST_ASSERT(result.data.zda.date.valid);
    TEST_ASSERT_EQ(11, result.data.zda.date.day);
    TEST_ASSERT_EQ(3, result.data.zda.date.month);
    TEST_ASSERT_EQ(2004, result.data.zda.date.year);

    TEST_ASSERT_EQ(-1, result.data.zda.tz_hours);
    TEST_ASSERT_EQ(0, result.data.zda.tz_minutes);

    TEST_PASS();
  }

  TEST_CASE("parse ZDA UTC") {
    const char *sentence = "$GPZDA,120000.00,01,01,2025,00,00";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(0, result.data.zda.tz_hours);
    TEST_ASSERT_EQ(0, result.data.zda.tz_minutes);

    TEST_PASS();
  }
}

// GBS tests

static void test_parse_gbs(void) {
  TEST_CASE("parse GBS basic") {
    const char *sentence = "$GPGBS,235503.00,1.6,1.4,3.2,03,,-21.4,3.8";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(TINY_NMEA_SENTENCE_GBS, result.type);

    TEST_ASSERT(result.data.gbs.time.valid);
    TEST_ASSERT_EQ(3, result.data.gbs.failed_sat_id);

    double err_lat = tiny_nmea_to_double(&result.data.gbs.err_lat_m);
    TEST_ASSERT_FLOAT_EQ(1.6, err_lat, 0.1);

    TEST_PASS();
  }
}

// GST tests

static void test_parse_gst(void) {
  TEST_CASE("parse GST basic") {
    const char *sentence = "$GPGST,172814.0,0.006,0.023,0.020,273.6,0.023,0.020,0.031";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(TINY_NMEA_SENTENCE_GST, result.type);

    TEST_ASSERT(result.data.gst.time.valid);
    TEST_ASSERT_EQ(17, result.data.gst.time.hours);

    double rms = tiny_nmea_to_double(&result.data.gst.rms_range);
    TEST_ASSERT_FLOAT_EQ(0.006, rms, 0.001);

    TEST_PASS();
  }
}

// AIS tests

static void test_parse_ais(void) {
  TEST_CASE("parse VDM basic") {
    const char *sentence = "!AIVDM,1,1,,B,177KQJ5000G?tO`K>RA1wUbN0TKH,0";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(TINY_NMEA_SENTENCE_VDM, result.type);
    TEST_ASSERT_EQ(TINY_NMEA_TALKER_AI, result.talker);

    TEST_ASSERT_EQ(1, result.data.ais.fragment_count);
    TEST_ASSERT_EQ(1, result.data.ais.fragment_number);
    TEST_ASSERT_EQ('B', result.data.ais.channel);
    TEST_ASSERT_EQ(0, result.data.ais.fill_bits);
    TEST_ASSERT(result.data.ais.payload_len > 0);

    TEST_PASS();
  }

  TEST_CASE("parse VDM multipart") {
    const char *sentence = "!AIVDM,2,1,3,B,55?MbV02>H97ac<H4eEK6@T4@Dn2222220j1p>1240Ht50,0";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(2, result.data.ais.fragment_count);
    TEST_ASSERT_EQ(1, result.data.ais.fragment_number);
    TEST_ASSERT_EQ(3, result.data.ais.sequential_id);

    TEST_PASS();
  }

  TEST_CASE("parse VDO") {
    const char *sentence = "!AIVDO,1,1,,A,1P000000000000000000000,0";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(TINY_NMEA_SENTENCE_VDO, result.type);

    TEST_PASS();
  }
}

// error handling tests

static void test_parse_errors(void) {
  TEST_CASE("parse unknown talker") {
    const char *sentence = "$XXRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_MALFORMED_SENTENCE, tiny_nmea_parse(sentence, &result));

    TEST_PASS();
  }

  TEST_CASE("parse unknown sentence type") {
    const char *sentence = "$GPXXX,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_MALFORMED_SENTENCE, tiny_nmea_parse(sentence, &result));

    TEST_PASS();
  }

  TEST_CASE("parse too few fields RMC") {
    const char *sentence = "$GPRMC,123519,A,4807.038,N";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_ERR_TOO_FEW_FIELDS, tiny_nmea_parse(sentence, &result));

    TEST_PASS();
  }

  TEST_CASE("parse too few fields GGA") {
    const char *sentence = "$GPGGA,123519,4807.038,N";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_ERR_TOO_FEW_FIELDS, tiny_nmea_parse(sentence, &result));

    TEST_PASS();
  }
}

// multi-constellation tests

static void test_multi_constellation(void) {
  TEST_CASE("parse GN combined talker") {
    const char *sentence = "$GNRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(TINY_NMEA_TALKER_GN, result.talker);

    TEST_PASS();
  }

  TEST_CASE("parse GA Galileo") {
    const char *sentence = "$GAGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,47.0,M,,";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(TINY_NMEA_TALKER_GA, result.talker);

    TEST_PASS();
  }

  TEST_CASE("parse GB BeiDou") {
    const char *sentence = "$GBGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,47.0,M,,";
    tiny_nmea_type_t result = {0};

    TEST_ASSERT_EQ(TINY_NMEA_OK, tiny_nmea_parse(sentence, &result));
    TEST_ASSERT_EQ(TINY_NMEA_TALKER_GB, result.talker);

    TEST_PASS();
  }
}

int main(void) {
  TEST_TITLE("sentence parsing tests");

  test_parse_rmc();
  test_parse_gga();
  test_parse_gsa();
  test_parse_gsv();
  test_parse_vtg();
  test_parse_gll();
  test_parse_zda();
  test_parse_gbs();
  test_parse_gst();
  test_parse_ais();
  test_parse_errors();
  test_multi_constellation();

  TEST_SUMMARY();
}
