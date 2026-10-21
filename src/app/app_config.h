// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jan Althaus (JPPeterson-lab)
#pragma once
// Feste Einstellungen von WetterCube Next
#define FIRMWARE_VERSION "0.1.0-alpha9"

#define MDNS_NAME        "wettercube-next"      // http://wettercube-next.local (mit Anhang aus der MAC-Adresse, wenn mehrere im Netz sind)
#define PORTAL_SSID      "WetterCube-Next-Setup"
#define PREFS_NAMESPACE  "wcnext"

// Abruf der Daten bei der Fusion (Adresse /api/next, Dienst _wettercube._tcp)
#define INTERVALL_ABRUF_MS      30000UL    // alle 30 Sekunden (die Fusion antwortet bei unveraenderten Daten mit 304)
#define INTERVALL_SUCHE_MS     600000UL    // Fusion erneut suchen: alle 10 Minuten, nach Fehlern frueher
#define ANZEIGE_ALT_MS         180000UL    // aelter als 3 Minuten: Anzeige "keine aktuellen Daten"

// Update ueber GitHub (wie bei der Fusion): version.json im oeffentlichen Repo, Firmware ueber GitHub Pages
#define OTA_VERSION_URL        "https://raw.githubusercontent.com/JPPeterson-lab/WetterCube-Next/main/docs/version.json"
#define OTA_BIN_URL            "https://jppeterson-lab.github.io/WetterCube-Next/firmware/firmware.bin"
#define UPDATE_CHECK_ERST      120000UL    // 2 Min. nach dem Start
#define UPDATE_CHECK_INTERVALL 21600000UL  // danach alle 6 Stunden
#define UPDATE_CHECK_RETRY     3600000UL   // nach einem Fehler nach 1 Stunde erneut
