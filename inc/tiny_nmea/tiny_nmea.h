//
// Created by linyi on 12/29/2025.
//

#ifndef TINY_NMEA_TINY_NMEA_H
#define TINY_NMEA_TINY_NMEA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "internal/ringbuf_type.h"
#include "internal/nmea_0183_types.h"

typedef struct {
  uint32_t sentences_parsed;
  uint32_t checksum_errors;
  uint32_t parse_errors;
  uint32_t buffer_overflows;
} tiny_nmea_parser_statistics_t;

// sentenced parsed callback
// use result->type to determine which union member to access
typedef void (*tiny_nmea_parse_callback_t)(const tiny_nmea_type_t *result, tiny_nmea_parser_statistics_t stats, void *parse_user_data);
typedef void (*tiny_nmea_error_callback_t)(const tiny_nmea_type_t *result, tiny_nmea_parser_statistics_t stats, void *error_user_data);

typedef enum {
  TINY_NMEA_PARSE_FIND_START = 0,
  TINY_NMEA_PARSE_FIND_TALKER_AND_TYPE,
  TINY_NMEA_PARSE_FIND_CHECKSUM_OR_END,
  TINY_NMEA_PARSE_FIND_END,
  TINY_NMEA_SENTENCE_COMPLETE
} tiny_nmea_parser_fsm_state_t;

// parser context
typedef struct {
  // INTERNAL DATA DO NOT ACCESS DIRECTLY
  // use callbacks to get thread safe snapshots

  ringbuf_t ringbuf; // ingest ring buffer
  tiny_nmea_parse_callback_t parse_callback; // parse complete callback
  tiny_nmea_error_callback_t error_callback; // if got sentence, but error in parsing, this will be invoked

  // data for callback
  void *parse_user_data;
  void *error_user_data;

  // working buffer
  uint8_t working_buf[TINY_NMEA_WORKING_BUF_LEN];
  size_t working_buf_len;
  size_t parse_pos;                          // current parse position in working_buf
  // check for overrun length while waiting for data
  bool waiting_for_data;

  // current sentence info
  char start_char;
  bool has_checksum;
  uint8_t computed_checksum;                 // computed xor checksum for debugging
  uint8_t received_checksum;                 // actual read checksum for debugging
  uint8_t *data_end;                         // sentence end, right at the last char '*' if checksum or 'CR' if 'CRLF'
  uint8_t *line_end;                         // at the first char 'CR' of 'CRLF' etc
  tiny_nmea_talker_t current_talker;
  tiny_nmea_sentence_type_t current_type;
  tiny_nmea_parser_fsm_state_t parser_state;

  // century tracking from ZDA
  uint8_t zda_century;    // 0 if unknown

  // parse statistics
  tiny_nmea_parser_statistics_t stats;
} tiny_nmea_ctx_t;

/**
 * init the parser context
 * @param ctx       empty parser context to init
 * @param buffer    user supplied working buffer for ringbuf
 * @param buf_size  size of the provided buffer
 * @param parse_callback  callback func when a sentence is parsed (NULL for none)
 * @param parse_user_data data passed to parse callback
 * @param error_callback  callback func when sentence found but error parsing (NULL for none)
 * @param error_user_data data passed to error callback
 */
tiny_nmea_res_t tiny_nmea_init_callbacks(tiny_nmea_ctx_t *ctx,
                                         uint8_t *buffer,
                                         size_t buf_size,
                                         tiny_nmea_parse_callback_t parse_callback,
                                         void *parse_user_data,
                                         tiny_nmea_error_callback_t error_callback,
                                         void *error_user_data);

/**
 * init the parser context without setting callbacks
 * @param ctx       empty parser context to init
 * @param buffer    user supplied working buffer for ringbuf
 * @param buf_size  size of the provided buffer
 */
tiny_nmea_res_t tiny_nmea_init(tiny_nmea_ctx_t *ctx,
                               uint8_t *buffer,
                               size_t buf_size);

/**
 * set the callback func to call when a sentence is parsed
 * @param parse_callback  callback func when a sentence is parsed (NULL for none)
 * @param parse_user_data data passed to parse callback
 */
tiny_nmea_res_t tiny_nmea_set_parse_callback(tiny_nmea_ctx_t *ctx,
                                             tiny_nmea_parse_callback_t parse_callback,
                                             void *parse_user_data);


/**
 * set the callback func to call when an error occurs during parsing
 * @param error_callback  callback func when sentence found but error parsing (NULL for none)
 * @param error_user_data data passed to error callback
 */
tiny_nmea_res_t tiny_nmea_set_error_callback(tiny_nmea_ctx_t *ctx,
                                             tiny_nmea_error_callback_t error_callback,
                                             void *error_user_data);

/**
 * process available data in the ring buffer
 * call this periodically to parse complete
 * sentences and invoke callbacks
 *
 * @param ctx       parser context
 * @return          number of sentences processed
 */
tiny_nmea_res_t tiny_nmea_work(tiny_nmea_ctx_t *ctx);

/**
 * ingest data into the ring buffer, call this when you
 * have UART data in a linear buffer already
 *
 * @param ctx       Parser context
 * @param data      Raw NMEA data
 * @param len       Length of data
 * @return          Number of sentences processed
 */
tiny_nmea_res_t tiny_nmea_feed(tiny_nmea_ctx_t *ctx, const uint8_t *data, size_t len);

/**
 * parse a single NMEA sentence
 * the sentence should just contain the data, starting from '$' and ending
 * at the data end which is either before '*' for sentences with checksum
 * or 'CRLF' for sentences without checksum
 *
 * @param sentence  null-terminated sentence string
 * @param result    output result structure
 * @return          TINY_NMEA_OK if parsed successfully
 */
tiny_nmea_res_t tiny_nmea_parse(const char *sentence, tiny_nmea_type_t *result);

/**
 * get sentence type name as string
 */
const char *tiny_nmea_sentence_name(tiny_nmea_sentence_type_t type);

/**
 * get talker ID name as string
 */
const char *tiny_nmea_talker_name(tiny_nmea_talker_t talker);

/**
 * reset parser statistics
 */
void tiny_nmea_reset_stats(tiny_nmea_ctx_t *ctx);

#endif //TINY_NMEA_TINY_NMEA_H
