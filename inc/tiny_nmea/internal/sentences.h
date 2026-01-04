//
// Created by dst on 12/31/25.
//

#ifndef TINY_NMEA_SENTENCES_H
#define TINY_NMEA_SENTENCES_H

#include "../tiny_nmea.h"

tiny_nmea_res_t handle_parse_rmc(const char *sentence, size_t len, tiny_nmea_rmc_t *data);
tiny_nmea_res_t handle_parse_gga(const char *sentence, size_t len, tiny_nmea_gga_t *data);
tiny_nmea_res_t handle_parse_gns(const char *sentence, size_t len, tiny_nmea_gns_t *data);
tiny_nmea_res_t handle_parse_gsa(const char *sentence, size_t len, tiny_nmea_gsa_t *data);
tiny_nmea_res_t handle_parse_gsv(const char *sentence, size_t len, tiny_nmea_gsv_t *data);
tiny_nmea_res_t handle_parse_vtg(const char *sentence, size_t len, tiny_nmea_vtg_t *data);
tiny_nmea_res_t handle_parse_gll(const char *sentence, size_t len, tiny_nmea_gll_t *data);
tiny_nmea_res_t handle_parse_zda(const char *sentence, size_t len, tiny_nmea_zda_t *data);
tiny_nmea_res_t handle_parse_gbs(const char *sentence, size_t len, tiny_nmea_gbs_t *data);
tiny_nmea_res_t handle_parse_gst(const char *sentence, size_t len, tiny_nmea_gst_t *data);
tiny_nmea_res_t handle_parse_ais(const char *sentence, size_t len, tiny_nmea_ais_t *data);

#endif //TINY_NMEA_SENTENCES_H