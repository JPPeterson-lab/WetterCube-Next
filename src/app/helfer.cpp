// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jan Althaus (JPPeterson-lab)
// Hilfen: Wettersymbol zum WMO-Code, Farben und Texte fuer Pollen und Biowetter
#include "app_state.h"
#include "../icons/wetter.h"

// WMO-Wettercode -> Symbol (gleiche Zuordnung wie in der Fusion), je mit Tag- und Nachtvariante; klein = 40x40 (Vorhersage)
const lv_img_dsc_t* wmoZuImage(int wmo, bool tag, bool klein) {
  const char* art = "overcast"; const char* zusatz = "";
  switch (wmo) {
    case 0:  art = "clear"; break;
    case 1:  art = "mostly_clear"; break;
    case 2:  art = "partly_cloudy"; break;
    case 3:  break;
    case 45: case 48: art = "fog"; break;
    case 51: art = "partly_cloudy"; zusatz = "drizzle"; break;
    case 53: zusatz = "drizzle"; break;
    case 55: zusatz = "rain"; break;
    case 56: case 57: case 66: case 67: zusatz = "sleet"; break;
    case 61: case 80: art = "partly_cloudy"; zusatz = "rain"; break;
    case 63: case 81: zusatz = "rain"; break;
    case 65: case 82: art = "extreme"; zusatz = "rain"; break;
    case 71: case 77: case 85: art = "partly_cloudy"; zusatz = "snow"; break;
    case 73: zusatz = "snow"; break;
    case 75: case 86: art = "extreme"; zusatz = "snow"; break;
    case 95: art = "thunderstorms"; zusatz = "rain"; break;
    case 96: case 99: art = "thunderstorms"; zusatz = "hail"; break;
    default:
      if (wmo >= 95) { art = "thunderstorms"; zusatz = "rain"; }
      else if (wmo >= 71) zusatz = "snow";
      else if (wmo >= 51) zusatz = "rain";
  }
  char name[48];
  snprintf(name, sizeof(name), "%s_%s%s%s", art, tag ? "day" : "night", zusatz[0] ? "_" : "", zusatz);
  const lv_img_dsc_t* r = wxSuche(name, klein);
  return r ? r : wxSuche(tag ? "overcast_day" : "overcast_night", klein);
}

// Pollenstufe (DWD-Skala 0-3, dazwischen Zehntel; < 0 = unbekannt)
lv_color_t pollenFarbe(float s) {
  if (s < 0)    return lv_color_hex(0x9CA3AF);
  if (s <= 0.0f) return lv_color_hex(0x9CA3AF);
  if (s <= 1.0f) return lv_color_hex(0x22C55E);
  if (s <= 2.0f) return lv_color_hex(0xEAB308);
  if (s <= 2.5f) return lv_color_hex(0xF97316);
  return lv_color_hex(0xDC2626);
}
const char* pollenText(float s) {
  if (s < 0) return "--";
  if (s <= 0.0f) return "keine";
  if (s <= 1.0f) return "gering";
  if (s <= 2.0f) return "mittel";
  if (s <= 2.5f) return "hoch";
  return "stark";
}
// Biowetter: 0 kein Einfluss, 1 positiv, 2 gering, 3 hoch
const char* bioText(int w) { return w == 0 ? "kein" : w == 1 ? "gut" : w == 2 ? "leicht" : "stark"; }
lv_color_t bioFarbe(int w) { return lv_color_hex(w == 0 ? 0x9CA3AF : w == 1 ? 0x22C55E : w == 2 ? 0xEAB308 : 0xDC2626); }
