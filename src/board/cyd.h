// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jan Althaus (JPPeterson-lab)
#pragma once
// Board-Schicht fuer den ESP32 CYD "Cheap Yellow Display" (ESP32-2432S028, 2,8", 320x240, resistiver Touch XPT2046).
// Anzeige an SPI2 (MOSI 13, MISO 12, SCLK 14, CS 15, DC 2), Hintergrundlicht Pin 21, Touch-Interrupt Pin 36 (Fingertipp erkennen).
#include <stdint.h>
#include <stdbool.h>
#include "esp_lcd_panel_io.h"
#include "esp_lcd_types.h"

#define CYD_H_RES 320
#define CYD_V_RES 240

// panel: 0 = ILI9341, 1 = ST7789. drehen: 180 Grad. invert: Farben invertieren (je nach CYD-Ausfuehrung noetig).
esp_lcd_panel_handle_t cydPanelInit(int panel, bool drehen, bool invert, esp_lcd_panel_io_color_trans_done_cb_t farbeFertig, void* benutzer);
void cydHelligkeit(int prozent);        // 3 bis 100
bool cydBerueht();                      // Finger auf dem Display (Interrupt-Pin des Touch-Controllers)
