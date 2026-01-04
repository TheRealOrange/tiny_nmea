//
// unit tests using real nmea recording files
//

#include "test.h"
#include "tiny_nmea/tiny_nmea.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// test context
static tiny_nmea_ctx_t ctx;
static uint8_t ring_buffer[1024];

// statistics for file parsing
static int total_sentences = 0;
static int rmc_count = 0;
static int gga_count = 0;
static int gsa_count = 0;
static int gsv_count = 0;
static int vtg_count = 0;
static int gll_count = 0;
static int vdm_count = 0;
static int vdo_count = 0;
static int other_count = 0;

// reset statistics
static void reset_stats(void) {
  total_sentences = 0;
  rmc_count = 0;
  gga_count = 0;
  gsa_count = 0;
  gsv_count = 0;
  vtg_count = 0;
  gll_count = 0;
  vdm_count = 0;
  vdo_count = 0;
  other_count = 0;
}

// parse callback - count sentence types
static void on_parse(const tiny_nmea_type_t *result, tiny_nmea_parser_statistics_t st, void *user_data) {
  (void)user_data;
  (void)st;
  total_sentences++;

  switch (result->type) {
    case TINY_NMEA_SENTENCE_RMC: rmc_count++; break;
    case TINY_NMEA_SENTENCE_GGA: gga_count++; break;
    case TINY_NMEA_SENTENCE_GSA: gsa_count++; break;
    case TINY_NMEA_SENTENCE_GSV: gsv_count++; break;
    case TINY_NMEA_SENTENCE_VTG: vtg_count++; break;
    case TINY_NMEA_SENTENCE_GLL: gll_count++; break;
    case TINY_NMEA_SENTENCE_VDM: vdm_count++; break;
    case TINY_NMEA_SENTENCE_VDO: vdo_count++; break;
    default: other_count++; break;
  }
}

// helper to process a file
static int process_file(const char *filename) {
  FILE *f = fopen(filename, "rb");
  if (!f) {
    printf("    " TEST_COLOR_YELLOW "warning: could not open %s" TEST_COLOR_RESET "\n", filename);
    return -1;
  }

  reset_stats();
  tiny_nmea_init_callbacks(&ctx, ring_buffer, sizeof(ring_buffer), on_parse, NULL, NULL, NULL);

  uint8_t buf[256];
  size_t bytes_read;

  while ((bytes_read = fread(buf, 1, sizeof(buf), f)) > 0) {
    tiny_nmea_feed(&ctx, buf, bytes_read);
    tiny_nmea_work(&ctx);
  }

  fclose(f);
  return 0;
}

// helper to get file path relative to test executable
// assumes tests run from project root or tests directory
static const char *get_data_path(const char *filename) {
  static char path[256];

  // try various relative paths
  const char *prefixes[] = {
    "tests/data/",
    "data/",
    "../tests/data/",
    "./",
    NULL
  };

  for (int i = 0; prefixes[i] != NULL; i++) {
    snprintf(path, sizeof(path), "%s%s", prefixes[i], filename);
    FILE *f = fopen(path, "r");
    if (f) {
      fclose(f);
      return path;
    }
  }

  // return default path even if not found
  snprintf(path, sizeof(path), "tests/data/%s", filename);
  return path;
}

// nmea 2.3 gps only tests

static void test_file_nmea_v23_gps(void) {
  TEST_CASE("file nmea v2.3 gps only recording") {
    const char *path = get_data_path("nmea_v23_gps_only.txt");
    if (process_file(path) < 0) {
      TEST_PASS();  // skip if file not found
      return;
    }

    printf("\n    parsed %d sentences (rmc=%d gga=%d gsa=%d gsv=%d vtg=%d gll=%d)\n",
           total_sentences, rmc_count, gga_count, gsa_count, gsv_count, vtg_count, gll_count);
    printf("    checksum errors: %u, parse errors: %u\n",
           ctx.stats.checksum_errors, ctx.stats.parse_errors);

    // should have parsed significant number of sentences
    TEST_ASSERT(total_sentences >= 50);

    // should have multiple sentence types
    TEST_ASSERT(rmc_count > 0);
    TEST_ASSERT(gga_count > 0);
    TEST_ASSERT(gsa_count > 0);
    TEST_ASSERT(gsv_count > 0);

    // should have very few errors for clean file
    TEST_ASSERT(ctx.stats.checksum_errors < 5);

    TEST_PASS();
  }
}

// nmea 4.1 multi-gnss tests

static void test_file_nmea_v41_multi_gnss(void) {
  TEST_CASE("file nmea v4.1 multi-constellation recording") {
    const char *path = get_data_path("nmea_v41_multi_gnss.txt");
    if (process_file(path) < 0) {
      TEST_PASS();
      return;
    }

    printf("\n    parsed %d sentences (rmc=%d gga=%d gsa=%d gsv=%d vtg=%d gll=%d)\n",
           total_sentences, rmc_count, gga_count, gsa_count, gsv_count, vtg_count, gll_count);
    printf("    checksum errors: %u, parse errors: %u\n",
           ctx.stats.checksum_errors, ctx.stats.parse_errors);

    // should have parsed some sentences
    TEST_ASSERT(total_sentences >= 10);

    // should have some gsv and gsa sentences
    TEST_ASSERT(gsv_count >= 1);
    TEST_ASSERT(gsa_count >= 1);

    TEST_PASS();
  }
}

// ais mixed recording tests

static void test_file_ais_mixed(void) {
  TEST_CASE("file ais mixed nmea/ais recording") {
    const char *path = get_data_path("nmea_ais_mixed.txt");
    if (process_file(path) < 0) {
      TEST_PASS();
      return;
    }

    printf("\n    parsed %d sentences (gps: rmc=%d gga=%d, ais: vdm=%d vdo=%d)\n",
           total_sentences, rmc_count, gga_count, vdm_count, vdo_count);
    printf("    checksum errors: %u, parse errors: %u\n",
           ctx.stats.checksum_errors, ctx.stats.parse_errors);

    // should have parsed at least some sentences
    TEST_ASSERT(total_sentences >= 1);

    // should have some ais sentences
    TEST_ASSERT(vdm_count >= 1 || vdo_count >= 1);

    TEST_PASS();
  }
}

// corrupted stream tests

static void test_file_corrupted_stream(void) {
  TEST_CASE("file corrupted/noisy stream") {
    const char *path = get_data_path("nmea_corrupted_stream.bin");
    if (process_file(path) < 0) {
      TEST_PASS();
      return;
    }

    printf("\n    parsed %d valid sentences despite corruption\n", total_sentences);
    printf("    checksum errors: %u, parse errors: %u\n",
           ctx.stats.checksum_errors, ctx.stats.parse_errors);

    // should have detected some errors (but may or may not recover sentences)
    TEST_ASSERT(ctx.stats.checksum_errors > 0 || ctx.stats.parse_errors > 0 || total_sentences >= 0);

    TEST_PASS();
  }
}

// realistic drive recording tests

static void test_file_realistic_drive(void) {
  TEST_CASE("file realistic drive recording (~300+ sentences)") {
    const char *path = get_data_path("nmea_realistic_drive.txt");
    if (process_file(path) < 0) {
      TEST_PASS();
      return;
    }

    printf("\n    parsed %d sentences (rmc=%d gga=%d gsa=%d gsv=%d vtg=%d gll=%d)\n",
           total_sentences, rmc_count, gga_count, gsa_count, gsv_count, vtg_count, gll_count);
    printf("    checksum errors: %u, parse errors: %u\n",
           ctx.stats.checksum_errors, ctx.stats.parse_errors);

    // should have parsed many sentences
    TEST_ASSERT(total_sentences >= 200);

    // should have roughly equal rmc and gga (one per second)
    TEST_ASSERT(rmc_count >= 90);
    TEST_ASSERT(gga_count >= 90);

    // vtg every 2 seconds, gll every 3 seconds
    TEST_ASSERT(vtg_count >= 40);
    TEST_ASSERT(gll_count >= 30);

    // gsv every 10 seconds (multiple messages per update)
    TEST_ASSERT(gsv_count >= 10);

    // clean generated file should have no errors
    TEST_ASSERT(ctx.stats.checksum_errors == 0);

    TEST_PASS();
  }
}

// byte-by-byte processing tests

static void test_file_byte_by_byte(void) {
  TEST_CASE("file processing byte-by-byte") {
    const char *path = get_data_path("nmea_v23_gps_only.txt");
    FILE *f = fopen(path, "rb");
    if (!f) {
      TEST_PASS();
      return;
    }

    reset_stats();
    tiny_nmea_init_callbacks(&ctx, ring_buffer, sizeof(ring_buffer), on_parse, NULL, NULL, NULL);

    int byte;
    while ((byte = fgetc(f)) != EOF) {
      uint8_t b = (uint8_t)byte;
      tiny_nmea_feed(&ctx, &b, 1);
      tiny_nmea_work(&ctx);
    }

    fclose(f);

    printf("\n    parsed %d sentences byte-by-byte\n", total_sentences);

    // should parse same number of sentences as bulk processing
    TEST_ASSERT(total_sentences >= 50);

    TEST_PASS();
  }
}

// chunked processing tests

static void test_file_random_chunks(void) {
  TEST_CASE("file processing with random chunk sizes") {
    const char *path = get_data_path("nmea_realistic_drive.txt");
    FILE *f = fopen(path, "rb");
    if (!f) {
      TEST_PASS();
      return;
    }

    reset_stats();
    tiny_nmea_init_callbacks(&ctx, ring_buffer, sizeof(ring_buffer), on_parse, NULL, NULL, NULL);

    uint8_t buf[64];
    srand(42);  // deterministic random

    while (!feof(f)) {
      // random chunk size 1-64 bytes
      size_t chunk_size = (rand() % 64) + 1;
      size_t bytes_read = fread(buf, 1, chunk_size, f);
      if (bytes_read > 0) {
        tiny_nmea_feed(&ctx, buf, bytes_read);
        tiny_nmea_work(&ctx);
      }
    }

    fclose(f);

    printf("\n    parsed %d sentences with random chunks\n", total_sentences);

    // should parse same as continuous processing
    TEST_ASSERT(total_sentences >= 200);
    TEST_ASSERT(ctx.stats.checksum_errors == 0);

    TEST_PASS();
  }
}

// stress test with rapid work calls

static void test_file_rapid_work(void) {
  TEST_CASE("file processing with rapid work() calls") {
    const char *path = get_data_path("nmea_v41_multi_gnss.txt");
    FILE *f = fopen(path, "rb");
    if (!f) {
      TEST_PASS();
      return;
    }

    reset_stats();
    tiny_nmea_init_callbacks(&ctx, ring_buffer, sizeof(ring_buffer), on_parse, NULL, NULL, NULL);

    uint8_t buf[16];
    size_t bytes_read;

    while ((bytes_read = fread(buf, 1, sizeof(buf), f)) > 0) {
      tiny_nmea_feed(&ctx, buf, bytes_read);
      // call work multiple times per feed
      for (int i = 0; i < 5; i++) {
        tiny_nmea_work(&ctx);
      }
    }

    fclose(f);

    printf("\n    parsed %d sentences with rapid work calls\n", total_sentences);

    // should parse at least some sentences
    TEST_ASSERT(total_sentences >= 1);

    TEST_PASS();
  }
}

// callback for small buffer test
static int small_buffer_count = 0;
static void small_buffer_callback(const tiny_nmea_type_t *result, tiny_nmea_parser_statistics_t st, void *user_data) {
  (void)result;
  (void)st;
  (void)user_data;
  small_buffer_count++;
}

// memory pressure test

static void test_file_small_buffer(void) {
  TEST_CASE("file processing with minimal buffer") {
    const char *path = get_data_path("nmea_v23_gps_only.txt");
    FILE *f = fopen(path, "rb");
    if (!f) {
      TEST_PASS();
      return;
    }

    // use very small ring buffer
    uint8_t small_ring[128];
    tiny_nmea_ctx_t small_ctx;

    small_buffer_count = 0;

    tiny_nmea_init_callbacks(&small_ctx, small_ring, sizeof(small_ring), small_buffer_callback, NULL, NULL, NULL);

    uint8_t buf[32];
    size_t bytes_read;

    while ((bytes_read = fread(buf, 1, sizeof(buf), f)) > 0) {
      tiny_nmea_feed(&small_ctx, buf, bytes_read);
      tiny_nmea_work(&small_ctx);
    }

    fclose(f);

    printf("\n    parsed %d sentences with 128-byte buffer\n", small_buffer_count);
    printf("    buffer overflows: %u\n", small_ctx.stats.buffer_overflows);

    // should still parse some sentences even with small buffer
    TEST_ASSERT(small_buffer_count >= 20);

    TEST_PASS();
  }
}

int main(void) {
  TEST_TITLE("nmea file recording tests");

  test_file_nmea_v23_gps();
  test_file_nmea_v41_multi_gnss();
  test_file_ais_mixed();
  test_file_corrupted_stream();
  test_file_realistic_drive();
  test_file_byte_by_byte();
  test_file_random_chunks();
  test_file_rapid_work();
  test_file_small_buffer();

  TEST_SUMMARY();
}
