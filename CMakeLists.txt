# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Jan Althaus (JPPeterson-lab)
# WetterCube Next: ESP-IDF-Projekt fuer die kleinen Zusatzdisplays (ESP32 CYD, 2,8", 320x240), die Daten der WetterCube Fusion anzeigen.
# Reines ESP-IDF, die kleine Ersatzschicht fuer die frueheren Arduino-Namen liegt in src/platform/ (gleiche wie bei der Fusion).
# Bauen:  bash build.sh   (Ziel esp32)
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include_directories("${CMAKE_CURRENT_LIST_DIR}/conf")
idf_build_set_property(COMPILE_OPTIONS "-Wno-error" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=stringop-truncation;-Wno-error=format-truncation;-Wno-error=restrict;-Wno-error=array-bounds;-Wno-error=maybe-uninitialized;-Wno-error=address-of-packed-member" APPEND)
# Die Version im Firmware-Kopf (esp_app_desc_t) kommt aus app_config.h
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/app/app_config.h")
file(READ "${CMAKE_CURRENT_LIST_DIR}/src/app/app_config.h" _next_cfg)
string(REGEX MATCH "#define FIRMWARE_VERSION \"([^\"]+)\"" _next_m "${_next_cfg}")
set(PROJECT_VER "${CMAKE_MATCH_1}")
project(wettercube_next)
