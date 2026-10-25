#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Jan Althaus (JPPeterson-lab)
# PicoPixel-Export fuer WetterCube Next vorbereiten (Exportziel: src/ui/, der Build uebersetzt src/ rekursiv)
SRC="$(cd "$(dirname "$0")/.." && pwd)/src/ui"
[ -d "$SRC" ] || { echo "FEHLER: src/ui/ nicht gefunden (PicoPixel-Exportpfad auf $SRC setzen)"; exit 1; }

# PicoPixel generiert "lvgl/lvgl.h" - hier heisst der Header <lvgl.h>
find "$SRC" -name "*.c" -o -name "*.h" | while read f; do
  if grep -q '"lvgl/lvgl.h"' "$f"; then sed -i '' 's|#include "lvgl/lvgl.h"|#include <lvgl.h>|g' "$f"; echo "  lvgl-Include gepatcht: $(basename "$f")"; fi
done

# images/*.c werden von images.c per #include eingebunden UND vom Build direkt kompiliert -> doppelte Definitionen: zu .inc umbenennen
if [ -d "$SRC/images" ]; then
  for f in "$SRC/images"/*.c; do
    [ -f "$f" ] || continue
    mv "$f" "${f%.c}.inc"; echo "  Image umbenannt: $(basename "$f")"
  done
  if [ -f "$SRC/images.c" ]; then
    sed -i '' 's|#include "images/\(.*\)\.c"|#include "src/ui/images/\1.inc"|g' "$SRC/images.c"
    sed -i '' 's|#include "images/\(.*\)\.inc"|#include "src/ui/images/\1.inc"|g' "$SRC/images.c"
  fi
fi

# ui.c: FADE_IN-Animation -> NONE (kein Ueberblenden, spart RAM)
[ -f "$SRC/ui.c" ] && sed -i '' 's|lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false)|lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false)|g' "$SRC/ui.c"

echo "Verifikation screens.h - erwartete Objekte:"
EXPECTED=(screenboot screentempbio screenpollenvor labelwetter labelcube labelstatus barwifi
  imagewetter labeltemp labelwettertext labeluhr labelminmax kachelsensoren kachelbio labelsenstitel labelbiotitel
  labelsens1name labelsens1wert labelsens4name labelsens4wert labelbio1name labelbio1wert labelbio4name labelbio4wert
  kachelpollen kachelvor labelpollentitel labelvortitel labelpollen1name labelpollen1wert labelpollen8name labelpollen8wert
  imagevor1 labelvor1zeit labelvor1temp imagevor4 labelvor4zeit labelvor4temp)
MISSING=0
for obj in "${EXPECTED[@]}"; do
  grep -q "\b${obj}\b" "$SRC/screens.h" || { echo "  WARNUNG: '$obj' nicht in screens.h"; MISSING=$((MISSING+1)); }
done
[ $MISSING -eq 0 ] && echo "  Alle erwarteten Objekte vorhanden." || echo "  !! $MISSING Objekte fehlen - ui.cpp pruefen !!"
echo "Fertig. src/ui/ bereit fuer den Build (bash build.sh)."
