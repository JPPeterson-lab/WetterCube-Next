#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Jan Althaus (JPPeterson-lab)
"""Wandelt die Wetter-Symbole (Meteocons, MIT) in LVGL-Bilder (Palette, 8 Bit) fuer WetterCube Next um: 64x64 (Hauptsymbol) und 40x40 (Vorhersage).

Aufruf:  python3 tools/wetter_icons.py
Quellen: assets/icons/wetter/<name>.png (aus der Fusion, 112x112 mit Alpha)
Ausgabe: assets/icons/wetter/wx_<name>.inc (64) und wxk_<name>.inc (40), src/icons/wetter.h und src/icons/wetter.cpp (mit Suchfunktion wxSuche)
"""
import os, glob
from PIL import Image
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DIR = os.path.join(ROOT, "assets/icons/wetter")
GROESSEN = [("wx_", 64), ("wxk_", 40)]

def basis(name): return name.replace("-", "_")

def palette8(im):
    """256-Farben-Palette (Blau, Gruen, Rot, Alpha je Eintrag, wie lv_color32_t) gefolgt von den 8-Bit-Indizes: LV_IMG_CF_INDEXED_8BIT.
    Spart gegenueber RGB565A8 etwa 60 % Flash; die Palette ist unabhaengig von LV_COLOR_16_SWAP."""
    q = im.quantize(colors=256, method=Image.FASTOCTREE, dither=Image.NONE)
    pal = q.getpalette("RGBA")
    out = bytearray()
    for i in range(256):
        r, g, b, a = pal[i * 4:i * 4 + 4] if i * 4 + 3 < len(pal) else (0, 0, 0, 0)
        out += bytes((b, g, r, a))
    return bytes(out) + bytes(q.getdata())

def main():
    namen = sorted(os.path.basename(f)[:-4] for f in glob.glob(os.path.join(DIR, "*.png")))
    namen = [n for n in namen if not n.startswith("wx")]
    gesamt = 0
    for n in namen:
        quelle = Image.open(os.path.join(DIR, n + ".png")).convert("RGBA")
        for pre, ziel in GROESSEN:
            im = quelle.resize((ziel, ziel), Image.LANCZOS)
            d = palette8(im); gesamt += len(d); idn = pre + basis(n)
            z = [f"// Erzeugt von tools/wetter_icons.py aus assets/icons/wetter/{n}.png ({ziel}x{ziel}, Palette, 8 Bit). Nicht von Hand aendern.", "#include <lvgl.h>",
                 f"static const uint8_t {idn}_map[] = {{"]
            for i in range(0, len(d), 32): z.append("  " + ",".join(f"0x{b:02x}" for b in d[i:i + 32]) + ",")
            z += ["};", f"extern const lv_img_dsc_t {idn};", f"const lv_img_dsc_t {idn} = {{",
                  f"  .header = {{ .cf = LV_IMG_CF_INDEXED_8BIT, .always_zero = 0, .reserved = 0, .w = {ziel}, .h = {ziel} }},",
                  f"  .data_size = {len(d)},", f"  .data = {idn}_map,", "};", ""]
            open(os.path.join(DIR, idn + ".inc"), "w").write("\n".join(z))
    os.makedirs(os.path.join(ROOT, "src/icons"), exist_ok=True)
    h = ["// Erzeugt von tools/wetter_icons.py. Nicht von Hand aendern.", "#pragma once", "#include <lvgl.h>", "#ifdef __cplusplus", 'extern "C" {', "#endif"]
    for pre, _ in GROESSEN: h += [f"extern const lv_img_dsc_t {pre}{basis(n)};" for n in namen]
    h += ["#ifdef __cplusplus", "}", "#endif",
          "// Symbol zum Namen (z. B. \"partly_cloudy_day_rain\"); klein = 40x40 statt 64x64; nullptr, wenn unbekannt", "const lv_img_dsc_t* wxSuche(const char* name, bool klein);", ""]
    open(os.path.join(ROOT, "src/icons/wetter.h"), "w").write("\n".join(h))
    c = ["// Erzeugt von tools/wetter_icons.py. Nicht von Hand aendern.", '#include "wetter.h"', "#include <string.h>", ""]
    for pre, _ in GROESSEN: c += [f'#include "assets/icons/wetter/{pre}{basis(n)}.inc"' for n in namen]
    c += ["", "struct WxEintrag { const char* name; const lv_img_dsc_t* gross; const lv_img_dsc_t* klein; };", "static const WxEintrag s_wx[] = {"]
    c += [f'  {{ "{basis(n)}", &wx_{basis(n)}, &wxk_{basis(n)} }},' for n in namen]
    c += ["};", "const lv_img_dsc_t* wxSuche(const char* name, bool klein) {",
          "  for (const WxEintrag& e : s_wx) if (strcmp(e.name, name) == 0) return klein ? e.klein : e.gross;", "  return nullptr;", "}", ""]
    open(os.path.join(ROOT, "src/icons/wetter.cpp"), "w").write("\n".join(c))
    print(f"{len(namen)} Symbole, {gesamt/1024:.0f} KB Bilddaten")
main()
