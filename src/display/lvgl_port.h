// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jan Althaus (JPPeterson-lab)
#pragma once
// LVGL-Anbindung fuer den CYD: Teilrendering in einen kleinen Puffer im internen RAM, Fingertipp als "Zeiger" (nur Tippen, keine Koordinaten)
#include <stdint.h>
#include "esp_err.h"
#include "esp_lcd_types.h"
#include "lvgl.h"

esp_err_t lvglPortInit(int panel, bool drehen, bool invert);
bool lvglPortLock(int timeout_ms);     // -1 = unbegrenzt warten
void lvglPortUnlock(void);
struct LvLock { LvLock() { lvglPortLock(-1); } ~LvLock() { lvglPortUnlock(); } };

// Bildschirmfoto: nach lvglPortCaptureStart() liefert jeder Zeichenstreifen (Teilrendering) 24-Bit-RGB-Zeilen; der Aufrufer holt sie mit
// lvglPortCaptureHole() ab (blockiert, bis ein Streifen da ist), sobald das ganze Bild (CYD_V_RES Zeilen) durch ist, endet die Aufnahme.
// Die Streifen kommen von oben nach unten. Der Aufrufer darf nicht den LVGL-Lock halten, und muss die Streifen zuegig abholen.
bool lvglPortCaptureStart();
// Liefert die naechsten Zeilen (Zeiger gueltig bis zum naechsten Aufruf); false = Zeitueberschreitung oder Ende
bool lvglPortCaptureHole(const uint8_t** daten, int* zeilen, int timeout_ms);
void lvglPortCaptureEnde();
