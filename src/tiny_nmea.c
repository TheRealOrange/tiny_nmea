//
// Created by linyi on 12/29/2025.
//

#include "tiny_nmea/tiny_nmea.h"
#include "tiny_nmea/internal/ringbuf.h"

#include <string.h>

tiny_nmea_res_t tiny_nmea_init(tiny_nmea_ctx_t *ctx,
                               uint8_t *buffer,
                               const size_t buf_size) {
  return tiny_nmea_init_callbacks(ctx, buffer, buf_size, NULL, NULL, NULL, NULL);
}

tiny_nmea_res_t tiny_nmea_init_callbacks(tiny_nmea_ctx_t *ctx,
                                         uint8_t *buffer,
                                         const size_t buf_size,
                                         const tiny_nmea_parse_callback_t parse_callback,
                                         void *parse_user_data,
                                         const tiny_nmea_error_callback_t error_callback,
                                         void *error_user_data) {
  if (!ctx || !buffer) {
    return TINY_NMEA_INVALID_ARGS; //todo: error handling before err callback registered?
  }

  ringbuf_init(&ctx->ringbuf, buffer, buf_size);

  ctx->has_checksum = false;

  ctx->parse_callback = parse_callback;
  ctx->parse_user_data = parse_user_data;
  ctx->error_callback = error_callback;
  ctx->error_user_data = error_user_data;

  ctx->zda_century = 0;

  ctx->stats.sentences_parsed = 0;
  ctx->stats.checksum_errors = 0;
  ctx->stats.parse_errors = 0;
  ctx->stats.buffer_overflows = 0;

  ctx->working_buf_len = 0;
  ctx->parse_pos = 0;
  ctx->waiting_for_data = false;

  ctx->parser_state = TINY_NMEA_PARSE_FIND_START;
  memset(ctx->working_buf, 0, TINY_NMEA_WORKING_BUF_LEN * sizeof(ctx->working_buf[0]));

  return TINY_NMEA_OK;
}

tiny_nmea_res_t tiny_nmea_set_parse_callback(tiny_nmea_ctx_t *ctx,
                                             const tiny_nmea_parse_callback_t parse_callback,
                                             void *parse_user_data) {
  if (!ctx) {
    return TINY_NMEA_INVALID_ARGS;
  }

  ctx->parse_callback = parse_callback;
  ctx->parse_user_data = parse_user_data;

  return TINY_NMEA_OK;
}


tiny_nmea_res_t tiny_nmea_set_error_callback(tiny_nmea_ctx_t *ctx,
                                             const tiny_nmea_error_callback_t error_callback,
                                             void *error_user_data) {
  if (!ctx) {
    return TINY_NMEA_INVALID_ARGS;
  }

  ctx->error_callback = error_callback;
  ctx->error_user_data = error_user_data;

  return TINY_NMEA_OK;
}


tiny_nmea_res_t tiny_nmea_feed(tiny_nmea_ctx_t *ctx, const uint8_t *data, size_t len) {
  if (!ctx || !data) {
    return TINY_NMEA_INVALID_ARGS;
  }

  size_t res = ringbuf_push(&ctx->ringbuf, data, len, RINGBUF_PUSH_DROP);
  return res == len ? TINY_NMEA_OK : TINY_NMEA_ERR_BUFFER_FULL;
}
