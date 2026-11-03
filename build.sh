#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Jan Althaus (JPPeterson-lab)
# Baut WetterCube Next (ESP-IDF 5.5.5 muss unter ~/esp/esp-idf-v5.5.5 installiert sein).
#   bash build.sh                bauen (Ziel esp32, ESP32 CYD)
#   bash build.sh flash PORT     bauen und flashen (z. B. /dev/cu.usbserial-110)
#   bash build.sh erstflash PORT erst den ganzen Flash loeschen (erase-flash), dann bauen und flashen (beim ersten Mal auf einem CYD mit fremder Software)
#   bash build.sh clean          build/ und sdkconfig loeschen (nach Aenderungen an sdkconfig.defaults)
source ~/esp/esp-idf-v5.5.5/export.sh > /dev/null 2>&1 || { echo "ESP-IDF 5.5.5 nicht gefunden"; exit 1; }
cd "$(dirname "$0")" || exit 1
case "$1" in
  clean) rm -rf build sdkconfig sdkconfig.old; echo "aufgeraeumt"; exit 0 ;;
esac
[ -f sdkconfig ] || idf.py set-target esp32
if [ "$1" = "erstflash" ]; then
  [ -n "$2" ] || { echo "Verwendung: bash build.sh erstflash <PORT>"; exit 1; }
  idf.py -p "$2" erase-flash && idf.py -p "$2" flash
elif [ "$1" = "flash" ]; then
  [ -n "$2" ] || { echo "Verwendung: bash build.sh flash <PORT>"; exit 1; }
  idf.py -p "$2" flash
else
  idf.py build
fi
