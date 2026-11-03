#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Jan Althaus (JPPeterson-lab)
"""Macht aus gerasterten Meteocons (Stil "flat", 112x112 mit Alpha) Symbole mit dunkler Kontur (besser sichtbar auf hellen Kacheln).

Aufruf:  python3 tools/icon_kontur.py <Ordner mit <name>.png> <Zielordner>
Danach:  python3 tools/wetter_icons.py   (Quellen in assets/icons/wetter/)
Die SVGs stammen aus dem npm-Paket @meteocons/svg (Ordner flat/), gerastert mit dem Browser (Canvas, 112x112).
"""
import os, sys
from PIL import Image, ImageFilter

def kontur(im, r=3, col=(40, 56, 84)):
    d = im.split()[3].filter(ImageFilter.MaxFilter(2 * r + 1)).filter(ImageFilter.GaussianBlur(0.8)).point(lambda v: min(255, int(v * 1.6)))
    unten = Image.new("RGBA", im.size, col + (0,)); unten.putalpha(d); unten.alpha_composite(im); return unten

if len(sys.argv) != 3: sys.exit(__doc__)
os.makedirs(sys.argv[2], exist_ok=True)
for f in sorted(os.listdir(sys.argv[1])):
    if not f.endswith(".png"): continue
    quelle = Image.open(os.path.join(sys.argv[1], f)).convert("RGBA")
    blatt = Image.new("RGBA", (112, 112), (0, 0, 0, 0)); blatt.paste(quelle.resize((100, 100), Image.LANCZOS), (6, 6))   # Rand fuer die Kontur
    kontur(blatt).save(os.path.join(sys.argv[2], f))
