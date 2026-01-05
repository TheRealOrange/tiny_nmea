cmake_minimum_required(VERSION 3.14)
project(tiny_nmea VERSION 0.1.0 LANGUAGES C)

# only set C standard when building standalone
if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_C_EXTENSIONS OFF)
else()
    # else request min c11 standard
    target_compile_features(tiny_nmea PUBLIC c_std_11)
endif()

option(TINY_NMEA_BUILD_TESTS "build tests" ${PROJECT_IS_TOP_LEVEL})

# library source files
add_library(tiny_nmea
        src/tiny_nmea.c
        src/ringbuf.c
        src/fixed_point.c
        src/data_formats.c
        src/sentences.c
        src/nmea_0183_parse_fields.c
        src/nmea_0183_type_names.c
        src/tiny_nmea_work.c
        src/tiny_nmea_parse.c
        src/parse_sentence_fields.c
        src/sats_tracking.c
        src/sats_tracking_handler.c
)

add_library(tiny_nmea::tiny_nmea ALIAS tiny_nmea)

target_include_directories(tiny_nmea
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
        $<INSTALL_INTERFACE:include>
)

# stricter warnings when dev
if(PROJECT_IS_TOP_LEVEL)
    target_compile_options(tiny_nmea PRIVATE
            $<$<C_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
            $<$<C_COMPILER_ID:MSVC>:/W4>
    )
endif()

# tests
if(TINY_NMEA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()