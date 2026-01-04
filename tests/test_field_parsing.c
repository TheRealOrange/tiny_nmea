//
// unit tests for field parsing functionality
//

#include "test.h"
#include "tiny_nmea/internal/parse_sentence_fields.h"
#include "tiny_nmea/internal/data_formats.h"
#include "tiny_nmea/internal/fixed_point.h"

#include <string.h>
#include <math.h>

// helper to create field from string literal
static field_t make_field(const char *str) {
  field_t f;
  f.ptr = str;
  f.len = str ? (uint8_t)strlen(str) : 0;
  return f;
}

// tokenize tests

static void test_tokenize_basic(void) {
  TEST_CASE("tokenize basic comma separation") {
    const char *data = "field1,field2,field3";
    field_t fields[8];

    uint8_t count = tokenize(data, strlen(data), fields, 8);
    TEST_ASSERT_EQ(3, count);
    TEST_ASSERT_EQ(6, fields[0].len);
    TEST_ASSERT_EQ(6, fields[1].len);
    TEST_ASSERT_EQ(6, fields[2].len);

    TEST_PASS();
  }

  TEST_CASE("tokenize empty fields") {
    const char *data = "a,,b,,c";
    field_t fields[8];

    uint8_t count = tokenize(data, strlen(data), fields, 8);
    TEST_ASSERT_EQ(5, count);
    TEST_ASSERT_EQ(1, fields[0].len);
    TEST_ASSERT_EQ(0, fields[1].len);
    TEST_ASSERT_EQ(1, fields[2].len);
    TEST_ASSERT_EQ(0, fields[3].len);
    TEST_ASSERT_EQ(1, fields[4].len);

    TEST_PASS();
  }

  TEST_CASE("tokenize max fields limit") {
    const char *data = "1,2,3,4,5,6,7,8,9,10";
    field_t fields[4];

    uint8_t count = tokenize(data, strlen(data), fields, 4);
    TEST_ASSERT_EQ(4, count);

    TEST_PASS();
  }

  TEST_CASE("tokenize single field") {
    const char *data = "onlyfield";
    field_t fields[4];

    uint8_t count = tokenize(data, strlen(data), fields, 4);
    TEST_ASSERT_EQ(1, count);
    TEST_ASSERT_EQ(9, fields[0].len);

    TEST_PASS();
  }

  TEST_CASE("tokenize trailing comma") {
    const char *data = "a,b,";
    field_t fields[8];

    uint8_t count = tokenize(data, strlen(data), fields, 8);
    TEST_ASSERT_EQ(3, count);
    TEST_ASSERT_EQ(0, fields[2].len);

    TEST_PASS();
  }
}

// field_empty tests

static void test_field_empty(void) {
  TEST_CASE("field_empty null pointer") {
    field_t f = {.ptr = NULL, .len = 5};
    TEST_ASSERT(field_empty(&f));
    TEST_PASS();
  }

  TEST_CASE("field_empty zero length") {
    field_t f = {.ptr = "test", .len = 0};
    TEST_ASSERT(field_empty(&f));
    TEST_PASS();
  }

  TEST_CASE("field_empty valid field") {
    field_t f = make_field("test");
    TEST_ASSERT(!field_empty(&f));
    TEST_PASS();
  }
}

// parse_uint tests

static void test_parse_uint(void) {
  TEST_CASE("parse_uint simple") {
    field_t f = make_field("12345");
    uint32_t val;
    TEST_ASSERT(parse_uint(&f, &val));
    TEST_ASSERT_EQ(12345, val);
    TEST_PASS();
  }

  TEST_CASE("parse_uint zero") {
    field_t f = make_field("0");
    uint32_t val;
    TEST_ASSERT(parse_uint(&f, &val));
    TEST_ASSERT_EQ(0, val);
    TEST_PASS();
  }

  TEST_CASE("parse_uint leading zeros") {
    field_t f = make_field("007");
    uint32_t val;
    TEST_ASSERT(parse_uint(&f, &val));
    TEST_ASSERT_EQ(7, val);
    TEST_PASS();
  }

  TEST_CASE("parse_uint max value") {
    field_t f = make_field("4294967295");
    uint32_t val;
    TEST_ASSERT(parse_uint(&f, &val));
    TEST_ASSERT_EQ_U(UINT32_MAX, val);
    TEST_PASS();
  }

  TEST_CASE("parse_uint overflow") {
    field_t f = make_field("4294967296");
    uint32_t val;
    TEST_ASSERT(!parse_uint(&f, &val));
    TEST_PASS();
  }

  TEST_CASE("parse_uint invalid chars") {
    field_t f = make_field("123abc");
    uint32_t val;
    TEST_ASSERT(!parse_uint(&f, &val));
    TEST_PASS();
  }

  TEST_CASE("parse_uint empty") {
    field_t f = make_field("");
    uint32_t val;
    TEST_ASSERT(!parse_uint(&f, &val));
    TEST_PASS();
  }

  TEST_CASE("parse_uint negative sign") {
    field_t f = make_field("-123");
    uint32_t val;
    TEST_ASSERT(!parse_uint(&f, &val));
    TEST_PASS();
  }
}

// parse_int tests

static void test_parse_int(void) {
  TEST_CASE("parse_int positive") {
    field_t f = make_field("12345");
    int32_t val;
    TEST_ASSERT(parse_int(&f, &val));
    TEST_ASSERT_EQ(12345, val);
    TEST_PASS();
  }

  TEST_CASE("parse_int negative") {
    field_t f = make_field("-12345");
    int32_t val;
    TEST_ASSERT(parse_int(&f, &val));
    TEST_ASSERT_EQ(-12345, val);
    TEST_PASS();
  }

  TEST_CASE("parse_int explicit positive") {
    field_t f = make_field("+999");
    int32_t val;
    TEST_ASSERT(parse_int(&f, &val));
    TEST_ASSERT_EQ(999, val);
    TEST_PASS();
  }

  TEST_CASE("parse_int zero") {
    field_t f = make_field("0");
    int32_t val;
    TEST_ASSERT(parse_int(&f, &val));
    TEST_ASSERT_EQ(0, val);
    TEST_PASS();
  }

  TEST_CASE("parse_int negative zero") {
    field_t f = make_field("-0");
    int32_t val;
    TEST_ASSERT(parse_int(&f, &val));
    TEST_ASSERT_EQ(0, val);
    TEST_PASS();
  }

  TEST_CASE("parse_int max positive") {
    field_t f = make_field("2147483647");
    int32_t val;
    TEST_ASSERT(parse_int(&f, &val));
    TEST_ASSERT_EQ(INT32_MAX, val);
    TEST_PASS();
  }

  TEST_CASE("parse_int min negative") {
    field_t f = make_field("-2147483648");
    int32_t val;
    TEST_ASSERT(parse_int(&f, &val));
    TEST_ASSERT_EQ(INT32_MIN, val);
    TEST_PASS();
  }

  TEST_CASE("parse_int overflow positive") {
    field_t f = make_field("2147483648");
    int32_t val;
    TEST_ASSERT(!parse_int(&f, &val));
    TEST_PASS();
  }

  TEST_CASE("parse_int overflow negative") {
    field_t f = make_field("-2147483649");
    int32_t val;
    TEST_ASSERT(!parse_int(&f, &val));
    TEST_PASS();
  }
}

// parse_char tests

static void test_parse_char(void) {
  TEST_CASE("parse_char simple") {
    field_t f = make_field("A");
    char c;
    TEST_ASSERT(parse_char(&f, &c));
    TEST_ASSERT_EQ('A', c);
    TEST_PASS();
  }

  TEST_CASE("parse_char first of many") {
    field_t f = make_field("XYZ");
    char c;
    TEST_ASSERT(parse_char(&f, &c));
    TEST_ASSERT_EQ('X', c);
    TEST_PASS();
  }

  TEST_CASE("parse_char empty") {
    field_t f = make_field("");
    char c;
    TEST_ASSERT(!parse_char(&f, &c));
    TEST_PASS();
  }
}

// parse_fixedpoint_float tests

static void test_parse_fixedpoint_float(void) {
  TEST_CASE("parse float integer") {
    // note: implementation treats integer-only input specially
    // "123" becomes value=123, scale=1000 (as if ".123" was parsed)
    // this is intentional for nmea data which typically has decimals
    field_t f = make_field("123");
    tiny_nmea_float_t val;
    TEST_ASSERT(parse_fixedpoint_float(&f, &val));
    TEST_ASSERT_EQ(123, val.value);
    // scale depends on digit count of input
    TEST_ASSERT_EQ(1000, val.scale);
    TEST_PASS();
  }

  TEST_CASE("parse float decimal") {
    field_t f = make_field("123.456");
    tiny_nmea_float_t val;
    TEST_ASSERT(parse_fixedpoint_float(&f, &val));
    TEST_ASSERT_EQ(123456, val.value);
    TEST_ASSERT_EQ(1000, val.scale);
    TEST_PASS();
  }

  TEST_CASE("parse float negative") {
    field_t f = make_field("-45.5");
    tiny_nmea_float_t val;
    TEST_ASSERT(parse_fixedpoint_float(&f, &val));
    TEST_ASSERT_EQ(-455, val.value);
    TEST_ASSERT_EQ(10, val.scale);
    TEST_PASS();
  }

  TEST_CASE("parse float leading decimal") {
    field_t f = make_field(".5");
    tiny_nmea_float_t val;
    TEST_ASSERT(parse_fixedpoint_float(&f, &val));
    TEST_ASSERT_EQ(5, val.value);
    TEST_ASSERT_EQ(10, val.scale);
    TEST_PASS();
  }

  TEST_CASE("parse float trailing decimal") {
    field_t f = make_field("42.");
    tiny_nmea_float_t val;
    TEST_ASSERT(parse_fixedpoint_float(&f, &val));
    TEST_ASSERT_EQ(42, val.value);
    TEST_ASSERT_EQ(1, val.scale);
    TEST_PASS();
  }

  TEST_CASE("parse float precision") {
    field_t f = make_field("3855.4487");
    tiny_nmea_float_t val;
    TEST_ASSERT(parse_fixedpoint_float(&f, &val));
    TEST_ASSERT_EQ(38554487, val.value);
    TEST_ASSERT_EQ(10000, val.scale);
    TEST_PASS();
  }

  TEST_CASE("parse float empty") {
    field_t f = make_field("");
    tiny_nmea_float_t val;
    TEST_ASSERT(!parse_fixedpoint_float(&f, &val));
    TEST_PASS();
  }

  TEST_CASE("parse float just decimal") {
    field_t f = make_field(".");
    tiny_nmea_float_t val;
    TEST_ASSERT(!parse_fixedpoint_float(&f, &val));
    TEST_PASS();
  }

  TEST_CASE("parse float just sign") {
    field_t f = make_field("-");
    tiny_nmea_float_t val;
    TEST_ASSERT(!parse_fixedpoint_float(&f, &val));
    TEST_PASS();
  }
}

// parse_time tests

static void test_parse_time(void) {
  TEST_CASE("parse time basic") {
    field_t f = make_field("123456");
    tiny_nmea_time_t t;
    TEST_ASSERT(parse_time(&f, &t));
    TEST_ASSERT(t.valid);
    TEST_ASSERT_EQ(12, t.hours);
    TEST_ASSERT_EQ(34, t.minutes);
    TEST_ASSERT_EQ(56, t.seconds);
    TEST_ASSERT_EQ(0, t.microseconds);
    TEST_PASS();
  }

  TEST_CASE("parse time with milliseconds") {
    field_t f = make_field("093045.123");
    tiny_nmea_time_t t;
    TEST_ASSERT(parse_time(&f, &t));
    TEST_ASSERT(t.valid);
    TEST_ASSERT_EQ(9, t.hours);
    TEST_ASSERT_EQ(30, t.minutes);
    TEST_ASSERT_EQ(45, t.seconds);
    TEST_ASSERT_EQ(123000, t.microseconds);
    TEST_PASS();
  }

  TEST_CASE("parse time with microseconds") {
    field_t f = make_field("235959.999999");
    tiny_nmea_time_t t;
    TEST_ASSERT(parse_time(&f, &t));
    TEST_ASSERT(t.valid);
    TEST_ASSERT_EQ(23, t.hours);
    TEST_ASSERT_EQ(59, t.minutes);
    TEST_ASSERT_EQ(59, t.seconds);
    TEST_ASSERT_EQ(999999, t.microseconds);
    TEST_PASS();
  }

  TEST_CASE("parse time midnight") {
    field_t f = make_field("000000.000");
    tiny_nmea_time_t t;
    TEST_ASSERT(parse_time(&f, &t));
    TEST_ASSERT(t.valid);
    TEST_ASSERT_EQ(0, t.hours);
    TEST_ASSERT_EQ(0, t.minutes);
    TEST_ASSERT_EQ(0, t.seconds);
    TEST_PASS();
  }

  TEST_CASE("parse time leap second") {
    field_t f = make_field("235960");
    tiny_nmea_time_t t;
    TEST_ASSERT(parse_time(&f, &t));
    TEST_ASSERT(t.valid);
    TEST_ASSERT_EQ(60, t.seconds);
    TEST_PASS();
  }

  TEST_CASE("parse time invalid hours") {
    field_t f = make_field("250000");
    tiny_nmea_time_t t;
    TEST_ASSERT(!parse_time(&f, &t));
    TEST_PASS();
  }

  TEST_CASE("parse time invalid minutes") {
    field_t f = make_field("126100");
    tiny_nmea_time_t t;
    TEST_ASSERT(!parse_time(&f, &t));
    TEST_PASS();
  }

  TEST_CASE("parse time too short") {
    field_t f = make_field("12345");
    tiny_nmea_time_t t;
    TEST_ASSERT(!parse_time(&f, &t));
    TEST_PASS();
  }

  TEST_CASE("parse time empty") {
    field_t f = make_field("");
    tiny_nmea_time_t t;
    TEST_ASSERT(!parse_time(&f, &t));
    TEST_PASS();
  }
}

// parse_date tests

static void test_parse_date(void) {
  TEST_CASE("parse date basic") {
    field_t f = make_field("150125");
    tiny_nmea_date_t d;
    TEST_ASSERT(parse_date(&f, &d));
    TEST_ASSERT(d.valid);
    TEST_ASSERT_EQ(15, d.day);
    TEST_ASSERT_EQ(1, d.month);
    TEST_ASSERT_EQ(25, d.year_yy);
    TEST_PASS();
  }

  TEST_CASE("parse date end of year") {
    field_t f = make_field("311299");
    tiny_nmea_date_t d;
    TEST_ASSERT(parse_date(&f, &d));
    TEST_ASSERT(d.valid);
    TEST_ASSERT_EQ(31, d.day);
    TEST_ASSERT_EQ(12, d.month);
    TEST_ASSERT_EQ(99, d.year_yy);
    TEST_PASS();
  }

  TEST_CASE("parse date first of month") {
    field_t f = make_field("010100");
    tiny_nmea_date_t d;
    TEST_ASSERT(parse_date(&f, &d));
    TEST_ASSERT(d.valid);
    TEST_ASSERT_EQ(1, d.day);
    TEST_ASSERT_EQ(1, d.month);
    TEST_PASS();
  }

  TEST_CASE("parse date invalid day zero") {
    field_t f = make_field("000125");
    tiny_nmea_date_t d;
    TEST_ASSERT(!parse_date(&f, &d));
    TEST_PASS();
  }

  TEST_CASE("parse date invalid day too high") {
    field_t f = make_field("320125");
    tiny_nmea_date_t d;
    TEST_ASSERT(!parse_date(&f, &d));
    TEST_PASS();
  }

  TEST_CASE("parse date invalid month zero") {
    field_t f = make_field("150025");
    tiny_nmea_date_t d;
    TEST_ASSERT(!parse_date(&f, &d));
    TEST_PASS();
  }

  TEST_CASE("parse date invalid month 13") {
    field_t f = make_field("151325");
    tiny_nmea_date_t d;
    TEST_ASSERT(!parse_date(&f, &d));
    TEST_PASS();
  }

  TEST_CASE("parse date too short") {
    field_t f = make_field("15012");
    tiny_nmea_date_t d;
    TEST_ASSERT(!parse_date(&f, &d));
    TEST_PASS();
  }
}

// parse_latitude tests

static void test_parse_latitude(void) {
  TEST_CASE("parse latitude north") {
    field_t val = make_field("4807.038");
    field_t dir = make_field("N");
    tiny_nmea_coord_t coord;
    TEST_ASSERT(parse_latitude(&val, &dir, &coord));
    TEST_ASSERT_EQ('N', coord.hemisphere);
    TEST_ASSERT_EQ(4807038, coord.raw.value);
    TEST_ASSERT_EQ(1000, coord.raw.scale);
    TEST_PASS();
  }

  TEST_CASE("parse latitude south") {
    field_t val = make_field("3355.1234");
    field_t dir = make_field("S");
    tiny_nmea_coord_t coord;
    TEST_ASSERT(parse_latitude(&val, &dir, &coord));
    TEST_ASSERT_EQ('S', coord.hemisphere);
    TEST_PASS();
  }

  TEST_CASE("parse latitude empty direction") {
    field_t val = make_field("4807.038");
    field_t dir = make_field("");
    tiny_nmea_coord_t coord;
    TEST_ASSERT(parse_latitude(&val, &dir, &coord));
    TEST_ASSERT_EQ('\0', coord.hemisphere);
    TEST_PASS();
  }

  TEST_CASE("parse latitude invalid direction") {
    field_t val = make_field("4807.038");
    field_t dir = make_field("E");
    tiny_nmea_coord_t coord;
    TEST_ASSERT(!parse_latitude(&val, &dir, &coord));
    TEST_PASS();
  }

  TEST_CASE("parse latitude empty value") {
    field_t val = make_field("");
    field_t dir = make_field("N");
    tiny_nmea_coord_t coord;
    TEST_ASSERT(!parse_latitude(&val, &dir, &coord));
    TEST_PASS();
  }
}

// parse_longitude tests

static void test_parse_longitude(void) {
  TEST_CASE("parse longitude east") {
    field_t val = make_field("01131.000");
    field_t dir = make_field("E");
    tiny_nmea_coord_t coord;
    TEST_ASSERT(parse_longitude(&val, &dir, &coord));
    TEST_ASSERT_EQ('E', coord.hemisphere);
    TEST_ASSERT_EQ(1131000, coord.raw.value);
    TEST_ASSERT_EQ(1000, coord.raw.scale);
    TEST_PASS();
  }

  TEST_CASE("parse longitude west") {
    field_t val = make_field("12200.5678");
    field_t dir = make_field("W");
    tiny_nmea_coord_t coord;
    TEST_ASSERT(parse_longitude(&val, &dir, &coord));
    TEST_ASSERT_EQ('W', coord.hemisphere);
    TEST_PASS();
  }

  TEST_CASE("parse longitude invalid direction") {
    field_t val = make_field("01131.000");
    field_t dir = make_field("N");
    tiny_nmea_coord_t coord;
    TEST_ASSERT(!parse_longitude(&val, &dir, &coord));
    TEST_PASS();
  }
}

// conversion tests

static void test_conversions(void) {
  TEST_CASE("tiny_nmea_to_float") {
    tiny_nmea_float_t f = {.value = 12345, .scale = 100};
    float result = tiny_nmea_to_float(&f);
    TEST_ASSERT_FLOAT_EQ(123.45f, result, 0.001f);
    TEST_PASS();
  }

  TEST_CASE("tiny_nmea_to_double") {
    tiny_nmea_float_t f = {.value = -99999, .scale = 1000};
    double result = tiny_nmea_to_double(&f);
    TEST_ASSERT_FLOAT_EQ(-99.999, result, 0.0001);
    TEST_PASS();
  }

  TEST_CASE("coord to degrees north") {
    // 48 degrees, 7.038 minutes north
    // = 48 + 7.038/60 = 48.1173 degrees
    tiny_nmea_coord_t coord = {
      .raw = {.value = 4807038, .scale = 1000},
      .hemisphere = 'N'
    };
    double deg = tiny_nmea_coord_to_degrees(&coord);
    TEST_ASSERT_FLOAT_EQ(48.1173, deg, 0.0001);
    TEST_PASS();
  }

  TEST_CASE("coord to degrees south") {
    tiny_nmea_coord_t coord = {
      .raw = {.value = 3355123, .scale = 1000},
      .hemisphere = 'S'
    };
    double deg = tiny_nmea_coord_to_degrees(&coord);
    TEST_ASSERT(deg < 0);
    TEST_PASS();
  }

  TEST_CASE("coord to degrees invalid") {
    tiny_nmea_coord_t coord = {
      .raw = {.value = 0, .scale = 0},
      .hemisphere = '\0'
    };
    double deg = tiny_nmea_coord_to_degrees(&coord);
    TEST_ASSERT(isnan(deg));
    TEST_PASS();
  }
}

int main(void) {
  TEST_TITLE("field parsing tests");

  test_tokenize_basic();
  test_field_empty();
  test_parse_uint();
  test_parse_int();
  test_parse_char();
  test_parse_fixedpoint_float();
  test_parse_time();
  test_parse_date();
  test_parse_latitude();
  test_parse_longitude();
  test_conversions();

  TEST_SUMMARY();
}
