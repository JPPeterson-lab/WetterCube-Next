# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Jan Althaus (JPPeterson-lab)
# Alle Quellen liegen unter src/ (nicht in main/): so bleibt main/ nur der Einstieg.
get_filename_component(NEXT_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
file(GLOB_RECURSE NEXT_SRCS "${NEXT_ROOT}/src/*.c" "${NEXT_ROOT}/src/*.cpp")
# Von macOS/iCloud angelegte Duplikate ("datei 2.cpp") nie mitbauen
list(FILTER NEXT_SRCS EXCLUDE REGEX " [0-9]+\\.(c|cpp)$")
idf_component_register(
    SRCS "main.cpp" ${NEXT_SRCS}
    INCLUDE_DIRS "." "${NEXT_ROOT}" "${NEXT_ROOT}/src" "${NEXT_ROOT}/src/platform"
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error)
# ArduinoJson soll unsere String-Klasse (src/platform/Arduino.h) als "Arduino-String" erkennen
target_compile_definitions(${COMPONENT_LIB} PRIVATE ARDUINOJSON_ENABLE_ARDUINO_STRING=1)
