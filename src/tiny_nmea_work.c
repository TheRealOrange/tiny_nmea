//
// Created by Lin Yicheng on 2/1/26.
//

#include "tiny_nmea/tiny_nmea.h"
#include "tiny_nmea/internal/ringbuf.h"
#include "tiny_nmea/internal/util.h"

#include <string.h>

static uint8_t nmea_checksum_helper(const char *start, const char *end) {
  uint8_t cs = 0;
  while (start < end) {
    cs ^= *start++;
  }
  return cs;
}

static inline uint8_t date_get_century_helper(const tiny_nmea_date_t *d) {
  return (d->valid && d->year >= 100) ? (uint8_t)(d->year / 100) : 0;
}

void parse_post_process(tiny_nmea_ctx_t *ctx, tiny_nmea_type_t *result) {
  switch (result->type) {
  case TINY_NMEA_SENTENCE_ZDA:
    // ZDA gives us the century so we can keep track
    if (result->data.zda.date.valid) {
      ctx->zda_century = date_get_century_helper(&result->data.zda.date);
    }
    break;

  case TINY_NMEA_SENTENCE_RMC:
    // compute correct full year from 2 digit year if century known
    if (ctx->zda_century > 0) {
      result->data.rmc.date.year = (uint16_t)ctx->zda_century * 100 + result->data.rmc.date.year_yy;
    }
    break;

  default:
    break;
  }
}

#define RESET_WORKBUF_BYTES(c, len) {               \
  (c)->working_buf_len = (len);                     \
  (c)->parse_pos = 0;                               \
  (c)->parser_state = TINY_NMEA_PARSE_FIND_START;   \
}

static inline void discard_bytes(tiny_nmea_ctx_t *ctx, size_t amt) {
  if (amt >= ctx->working_buf_len) {
    ctx->working_buf_len = 0;
    return;
  }
  memmove(ctx->working_buf, ctx->working_buf + amt, ctx->working_buf_len - amt);
  ctx->working_buf_len -= amt;
}

#define RESET_TO_START(c) RESET_WORKBUF_BYTES(c, 0)

tiny_nmea_res_t tiny_nmea_work(tiny_nmea_ctx_t *ctx) {
  if (!ctx) {
    return TINY_NMEA_INVALID_ARGS;
  }

  // while the ringbuf has more bytes to process, or our current working buffer
  // has unprocessed bytes (when ctx->parse_pos < ctx->working_buf_len)
  while (!ringbuf_empty(&ctx->ringbuf) ||
       (ctx->working_buf_len > ctx->parse_pos)) {
    size_t bytes_avail = ringbuf_len(&ctx->ringbuf);

    // break immediately if waiting for data but none avail, nothing to do
    if (ctx->waiting_for_data && bytes_avail == 0) break;

    // if there is even data in the ringbuffer
    // fill the rest of the linear working buffer with whatever
    // data is available, and whatever we transfer, pop it from
    // the ringbuffer
    size_t space_in_workbuf = TINY_NMEA_MAX_SENTENCE_LEN - ctx->working_buf_len;
    size_t to_pop = min_size(space_in_workbuf, bytes_avail);
    if (to_pop > 0) {
      ctx->working_buf_len += ringbuf_pop(&ctx->ringbuf,
          ctx->working_buf + ctx->working_buf_len, to_pop);
    } else if (space_in_workbuf == 0 && ctx->waiting_for_data) {
      // buffer full but waiting for data
      // overran buffer, reset buffer
      ctx->stats.buffer_overflows++;
      RESET_TO_START(ctx);
    }

    // set waiting for data false if we enter FSM processing
    ctx->waiting_for_data = false;

    // check if anything to do this loop
    if ((ctx->working_buf_len - ctx->parse_pos) == 0) {
      if (ringbuf_empty(&ctx->ringbuf)) {
        break;  // nothing to do
      }
      // buffer empty but ringbuf has data
      // try to read into the buffer next iteration
      continue;
    }

    // proceed with FSM
    // enforce that when TINY_NMEA_PARSE_FIND_START is reached, the parse_pos
    // is always at 0 and at the start of the working buffer
    switch (ctx->parser_state) {
      case TINY_NMEA_PARSE_FIND_START: {
        // in this case we just finished parsing a sentence
        // or we just reset to the known state. this means we
        // can assume that the start of the linear working buffer
        // is the start of the unprocessed information

        // we actually have data to work with
        // now try to find the start char '$' or '!' in the
        // data we peeked using memchr
        // which is hopefully faster than parsing byte-by-byte
        ctx->has_checksum = false;
        uint8_t *start = NULL;
        if (ctx->working_buf[0] == '$' || ctx->working_buf[0] == '!') {
          // short circuit case in case of common situation
          // where we just finished parsing a sentence
          start = &ctx->working_buf[0];
        } else {
          uint8_t *start_dollar = memchr(ctx->working_buf, '$', ctx->working_buf_len);
          uint8_t *start_exclam = memchr(ctx->working_buf, '!', ctx->working_buf_len);
          if (start_dollar && start_exclam) {
            // both are found, pick the minimum
            start = min_ptr(start_dollar, start_exclam);
          } else {
            // only one is found, use whichever is not null
            start = start_dollar ? start_dollar : start_exclam;
          }
        }

        if (start) {
          // managed to find a start char
          // discard all the bytes before the start char
          size_t start_offset = start - ctx->working_buf;
          discard_bytes(ctx, start_offset);
          ctx->parse_pos = 1;// skip start char
          ctx->computed_checksum = 0; // reset running checksum

          // change FSM state to proceed with parsing
          ctx->parser_state = TINY_NMEA_PARSE_FIND_TALKER_AND_TYPE;
        } else {
          // not found, discard the contents of the working buffer
          // since there is no start. FSM state does not change
          // do not count this as an error
          RESET_TO_START(ctx);
        }
        break;
      }
      case TINY_NMEA_PARSE_FIND_TALKER_AND_TYPE: {
        // we just found the start char, now we process
        // the next 6 bytes which according to nmea 0183 should
        // contain the talker id 2 bytes and type of
        // sentence 3 bytes and end with a comma

        // check if we have enough data
        if ((ctx->working_buf_len - ctx->parse_pos) < 6) {
          // there is insufficient data for the talker id and sentence type
          // indicate we are waiting for more bytes
          ctx->waiting_for_data = true;
          // and break immediately, nothing to do
          break;
        }

        // there is sufficient data for the talker id and sentence type
        // parse talker (bytes 1-2) and sentence type (bytes 3-5)
        ctx->current_talker = parse_talker_id((const char *)&ctx->working_buf[1]);
        ctx->current_type = parse_sentence_type((const char *)&ctx->working_buf[3]);

        // check that talker id and sentence type are valid
        // and that a comma follows
        if (!tiny_nmea_talker_valid(ctx->current_talker) ||
              !tiny_nmea_sentence_valid(ctx->current_type) ||
              (ctx->working_buf[6] != ',')) {
          // invalid header
          // skip one char and restart search
          discard_bytes(ctx, 1);

          // revert back to finding a start
          ctx->stats.parse_errors++;
          RESET_WORKBUF_BYTES(ctx, ctx->working_buf_len);
        } else {
          // both are valid
          // continue parsing
          ctx->parse_pos += 6;
          ctx->parser_state = TINY_NMEA_PARSE_FIND_CHECKSUM_OR_END;
        }
        break;
      }
      case TINY_NMEA_PARSE_FIND_CHECKSUM_OR_END: {
        // we have found the talker id and sentence type.
        // now we try to either find the asterisk '*' indicating
        // presence of a checksum, or we try to find the line end
        // this serves the purpose of finding the data end
        size_t remaining = ctx->working_buf_len - ctx->parse_pos;
        uint8_t *search_start = ctx->working_buf + ctx->parse_pos;

        // scan for '*' using memchr if there is checksum
        uint8_t *asterisk = memchr(search_start, '*', remaining);

        // also scan for end tokens since checksum is optional
        // in nmea 0183 sentences
        uint8_t *cr = memchr(search_start, '\r', remaining);
        uint8_t *lf = memchr(search_start, '\n', remaining);

        // pick the earliest of all the line endings
        ctx->line_end = NULL;
        if (cr && lf) {
          ctx->line_end = min_ptr(cr, lf);
        } else {
          // only one found
          // technically malformed, but just take earliest?
          // todo: add a compile flag to strictly require the line endings or strictly require one of the endings
          ctx->line_end = cr ? cr : lf;
        }

        // determine which comes first, asterisk or line ending
        uint8_t *data_end = NULL;
        bool has_checksum = false;
        if (asterisk && ctx->line_end) {
          has_checksum = asterisk < ctx->line_end;
          data_end = has_checksum ? asterisk : ctx->line_end;
        } else if (ctx->line_end) {
          // found line end but no asterisk
          // there is definitely no checksum
          has_checksum = false;
          data_end = ctx->line_end;
        } else if (asterisk) {
          // found asterisk but no line end
          // there is definitely a checksum
          has_checksum = true;
          data_end = asterisk;
        } else {
          // may need more data, wait first
          if (ctx->parse_pos > TINY_NMEA_MAX_SENTENCE_LEN) {
            ctx->stats.parse_errors++;
            discard_bytes(ctx, ctx->parse_pos);
            RESET_WORKBUF_BYTES(ctx, ctx->working_buf_len);
          } else {
            ctx->parse_pos = ctx->working_buf_len;
            ctx->waiting_for_data = true;
          }
          break;
        }

        // else update parse position
        ctx->parse_pos = data_end - ctx->working_buf;
        ctx->has_checksum = has_checksum;
        ctx->data_end = data_end;

        if (has_checksum) {
          // if checksum exists, find the actual end
          ctx->parser_state = TINY_NMEA_PARSE_FIND_END;
        } else {
          // no checksum, and end found (did not break earlier)
          ctx->parser_state = TINY_NMEA_SENTENCE_COMPLETE;
        }
        break;
      }
      case TINY_NMEA_PARSE_FIND_END: {
        // we have found '*' which indicates there is a checksum
        // so now we try to find the actual end of the sentence (if it was
        // not already found) and we parse the checksum
        // if there is no checksum, the earlier step would skip to
        // the sentence completed state

        // check if we already found line end min and max from the earlier
        // step, so we can skip line finding to save time
        if (!ctx->line_end) {
          // find line ending which could be CR, LF, or CRLF
          // todo: should i add a define to enable stricter checking of line endings? or prefer one over the other?
          size_t remaining = ctx->working_buf_len - ctx->parse_pos;
          uint8_t *search_start = ctx->working_buf + ctx->parse_pos;

          uint8_t *cr = memchr(search_start, '\r', remaining);
          uint8_t *lf = memchr(search_start, '\n', remaining);

          if (cr && lf) {
            ctx->line_end = min_ptr(cr, lf);
          } else {
            ctx->line_end = cr ? cr : lf;
          }
        }

        // check if we actually found line end
        if (!ctx->line_end) {
          // may need more bytes until line end
          if (ctx->parse_pos > TINY_NMEA_MAX_SENTENCE_LEN) {
            ctx->stats.parse_errors++;
            discard_bytes(ctx, ctx->parse_pos);
            RESET_WORKBUF_BYTES(ctx, ctx->working_buf_len);
          } else {
            ctx->parse_pos = ctx->working_buf_len;
            ctx->waiting_for_data = true;
          }
          break;
          // FSM state does not change, wait for more bytes
        }

        // check if we actually have 2 chars between asterisk and end
        uint8_t *hex_start = ctx->data_end + 1;
        if (ctx->line_end <= hex_start || (size_t)(ctx->line_end - hex_start) != 2) {
          // wrong num of chars for checksum, reset and
          // break immediately, do not attempt parsing
          ctx->stats.parse_errors++;
          RESET_TO_START(ctx);
          break;
        }

        // check if we have a valid hex byte
        if (parse_hex_byte((const char *)hex_start, &ctx->received_checksum) != 2) {
          // not valid hex chars in checksum, reset and
          // break immediately, do not attempt parsing
          ctx->stats.parse_errors++;
          RESET_TO_START(ctx);
          break;
        }

        // compute our data checksum
        ctx->computed_checksum = nmea_checksum_helper(
          // start from talker id (sentence is always positioned at
          // the start of the workbuf) so we start 1 char after
          (const char *)ctx->working_buf + 1,
          (const char *)ctx->data_end
        );

        if (ctx->computed_checksum == ctx->received_checksum) {
          // checksum matches, sentence complete, handoff to parsers
          ctx->parser_state = TINY_NMEA_SENTENCE_COMPLETE;
        } else {
          // checksum exists but does not match, reject, reset
          // and break immediately, do not attempt parsing
          ctx->stats.checksum_errors++;
          RESET_TO_START(ctx);
        }
        break;
      }
      case TINY_NMEA_SENTENCE_COMPLETE: {
        // handle completed sentence

        // null-terminate the data field for parsing
        // the sentence is always positioned at the start of the workbuf
        // data_end points to '*' (if checksum) or line ending (if no checksum)
        const char *data_start = (const char *)ctx->working_buf;
        *ctx->data_end = '\0';

        tiny_nmea_type_t result = {0};
        // use pre-parsed type and talker to save time
        result.type = ctx->current_type;
        result.talker = ctx->current_talker;
        // hand off parsing
        tiny_nmea_res_t parse_res = tiny_nmea_parse(data_start, &result);

        if (parse_res == TINY_NMEA_OK) {
          ctx->stats.sentences_parsed++;
        } else {
          ctx->stats.parse_errors++;
        }

        // invoke callback if parsing succeeded
        if (parse_res == TINY_NMEA_OK) {
          parse_post_process(ctx, &result);
          if (ctx->parse_callback) ctx->parse_callback(&result, ctx->stats, ctx->parse_user_data);
        } else if (ctx->error_callback) {
          ctx->error_callback(&result, ctx->stats, ctx->error_user_data);
        }

        // eagerly skip any remaining line ending characters
        uint8_t *sentence_end = ctx->line_end;
        while (sentence_end < ctx->working_buf + ctx->working_buf_len &&
           (*sentence_end == '\r' || *sentence_end == '\n' || *sentence_end == '\0')) {
          sentence_end++;
        }

        size_t consumed = sentence_end - ctx->working_buf;
        discard_bytes(ctx, consumed);

        // revert back to finding a start
        RESET_WORKBUF_BYTES(ctx, ctx->working_buf_len);
        break;
      }
      default: break;
    }
  }

  return TINY_NMEA_OK;
}