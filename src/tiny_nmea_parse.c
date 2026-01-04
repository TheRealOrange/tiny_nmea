//
// Created by linyi on 1/3/2026.
//

#include "tiny_nmea/tiny_nmea.h"
#include "tiny_nmea/internal/sentences.h"

#include <string.h>

tiny_nmea_res_t tiny_nmea_parse(const char *sentence, tiny_nmea_type_t *result) {
  tiny_nmea_res_t parse_res = TINY_NMEA_OK;
  // if result does not have type, try to parse from the sentence
  if (!tiny_nmea_talker_valid(result->talker) || !tiny_nmea_sentence_valid(result->type)) {
    result->talker = parse_talker_id(sentence + 1); // offset 1 byte for start char
    result->type = parse_sentence_type(sentence + 3); // offset 1 byte for start, 2 for talker

    if (!tiny_nmea_talker_valid(result->talker) || !tiny_nmea_sentence_valid(result->type)) {
      // still invalid
      return TINY_NMEA_MALFORMED_SENTENCE;
    }
  }

  // 1 byte start, 2 bytes talker id, 3 bytes sentence type, 1 byte comma
  const char *field_data = sentence + 7;
  size_t field_len = strlen(sentence) - 7;

  // parsers get the sentence data from the first field, stripped comma
  switch (result->type) {
    case TINY_NMEA_SENTENCE_RMC:
      parse_res = handle_parse_rmc(field_data, field_len, &result->data.rmc);
      break;
    case TINY_NMEA_SENTENCE_GGA:
      parse_res = handle_parse_gga(field_data, field_len, &result->data.gga);
      break;
    case TINY_NMEA_SENTENCE_GNS:
      parse_res = handle_parse_gns(field_data, field_len, &result->data.gns);
      break;
    case TINY_NMEA_SENTENCE_GSA:
      parse_res = handle_parse_gsa(field_data, field_len, &result->data.gsa);
      break;
    case TINY_NMEA_SENTENCE_GSV:
      parse_res = handle_parse_gsv(field_data, field_len, &result->data.gsv);
      break;
    case TINY_NMEA_SENTENCE_VTG:
      parse_res = handle_parse_vtg(field_data, field_len, &result->data.vtg);
      break;
    case TINY_NMEA_SENTENCE_GLL:
      parse_res = handle_parse_gll(field_data, field_len, &result->data.gll);
      break;
    case TINY_NMEA_SENTENCE_ZDA:
      parse_res = handle_parse_zda(field_data, field_len, &result->data.zda);
      break;
    case TINY_NMEA_SENTENCE_GBS:
      parse_res = handle_parse_gbs(field_data, field_len, &result->data.gbs);
      break;
    case TINY_NMEA_SENTENCE_GST:
      parse_res = handle_parse_gst(field_data, field_len, &result->data.gst);
      break;
    case TINY_NMEA_SENTENCE_VDM:
    case TINY_NMEA_SENTENCE_VDO:
      parse_res = handle_parse_ais(field_data, field_len, &result->data.ais);
      break;
    default:
      parse_res = TINY_NMEA_ERR_UNSUPPORTED;
      break;
  }

  return parse_res;
}
