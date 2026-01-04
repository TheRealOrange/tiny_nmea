//
// Created by Lin Yicheng on 2/1/26.
//

#ifndef TINY_NMEA_NMEA_0183_DEFS_H
#define TINY_NMEA_NMEA_0183_DEFS_H

// nmea 0183 constellation types
// X(NAME, char1, char2, desc)
#define TINY_NMEA_CONSTELLATION_LIST(X) \
  X(GP, 'G', 'P', "GPS")       \
  X(GL, 'G', 'L', "GLONASS")   \
  X(GA, 'G', 'A', "Galileo")   \
  X(GB, 'G', 'B', "BeiDou")    \
  X(BD, 'B', 'D', "BeiDou")    \
  X(GN, 'G', 'N', "GNSS")      \
  X(GQ, 'G', 'Q', "QZSS")      \
  X(GI, 'G', 'I', "NavIC")

// nmea 0183 talker ids (extends constellation types)
// X(NAME, char1, char2, desc)
#define TINY_NMEA_TALKER_LIST(X)   \
    TINY_NMEA_CONSTELLATION_LIST(X) \
    X(AI, 'A', 'I', "AIS")         \
    X(AB, 'A', 'B', "AIS Base")    \
    X(AD, 'A', 'D', "AIS Depend")  \
    X(AN, 'A', 'N', "AIS Aid Nav") \
    X(AR, 'A', 'R', "AIS Receive") \
    X(AS, 'A', 'S', "AIS Station") \
    X(AT, 'A', 'T', "AIS Transmit")\
    X(AX, 'A', 'X', "AIS Simplex")

// nmea 0183 sentence types
// X(NAME, char1, char2, char3)
#define TINY_NMEA_SENTENCE_LIST(X)   \
    X(RMC, 'R', 'M', 'C')            \
    X(GGA, 'G', 'G', 'A')            \
    X(GNS, 'G', 'N', 'S')            \
    X(GSA, 'G', 'S', 'A')            \
    X(GSV, 'G', 'S', 'V')            \
    X(VTG, 'V', 'T', 'G')            \
    X(GLL, 'G', 'L', 'L')            \
    X(ZDA, 'Z', 'D', 'A')            \
    X(GBS, 'G', 'B', 'S')            \
    X(GST, 'G', 'S', 'T')            \
    X(VDM, 'V', 'D', 'M')            \
    X(VDO, 'V', 'D', 'O')

// nmea 0183 fix qualities (GGA sentence)
// X(NAME, value, desc)
#define TINY_NMEA_FIX_QUALITY_LIST(X)  \
    X(INVALID,    0, "Invalid")        \
    X(GPS,        1, "GPS")            \
    X(DGPS,       2, "DGPS")           \
    X(PPS,        3, "PPS")            \
    X(RTK,        4, "RTK")            \
    X(FLOAT_RTK,  5, "RTK Float")      \
    X(ESTIMATED,  6, "Estimated")      \
    X(MANUAL,     7, "Manual")         \
    X(SIMULATION, 8, "Sim")

// nmea 0183 faa modes / gns mode indicators
// used in RMC, VTG, GLL, GNS sentences
// X(NAME, char, desc)
#define TINY_NMEA_FAA_MODE_LIST(X)     \
    X(AUTONOMOUS,   'A', "Auto")       \
    X(DIFFERENTIAL, 'D', "Diff")       \
    X(ESTIMATED,    'E', "Est")        \
    X(RTK_FLOAT,    'F', "RTK-F")      \
    X(MANUAL,       'M', "Manual")     \
    X(NOT_VALID,    'N', "Invalid")    \
    X(PRECISE,      'P', "Precise")    \
    X(RTK_INTEGER,  'R', "RTK")        \
    X(SIMULATOR,    'S', "Sim")

// nmea 0183 gsa fix type
// X(NAME, value, desc)
#define TINY_NMEA_GSA_FIX_LIST(X)  \
    X(NONE,   1, "None")           \
    X(FIX_2D, 2, "2D")             \
    X(FIX_3D, 3, "3D")

// nmea 0183 navigation status (NMEA 4.1+)
// used in RMC, GNS sentences
// X(NAME, char, desc)
#define TINY_NMEA_NAV_STATUS_LIST(X)   \
    X(SAFE,      'S', "Safe")          \
    X(CAUTION,   'C', "Caution")       \
    X(UNSAFE,    'U', "Unsafe")        \
    X(NOT_VALID, 'V', "Not Valid")

#endif //TINY_NMEA_NMEA_0183_DEFS_H