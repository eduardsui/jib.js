# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

add_definitions(-DDUK_USE_ALIGN_BY=1 -DDUK_USE_ROM_STRINGS -DDUK_USE_ROM_OBJECTS -DDUK_USE_ROM_GLOBAL_INHERIT -DDUK_USE_LIGHTFUNC_BUILTINS -DNO_UI -DNO_IO -DWITH_GPIO -DWITH_NVS -DWITH_WIFI -DESP32 -DNO_COMPRESSION -DLIGHT_BUILD -DNO_TLS -DTLS_AMALGAMATION -DLTC_NO_ASM -DTLS_USE_RANDOM_SOURCE=esp_fill_random)

project(jib)
