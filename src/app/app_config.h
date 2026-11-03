// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jan Althaus (JPPeterson-lab)
#pragma once
// Feste Einstellungen von WetterCube Next
#define FIRMWARE_VERSION "0.3.0-beta5"

#define MDNS_NAME        "wettercube-next"      // http://wettercube-next.local (mit Anhang aus der MAC-Adresse, wenn mehrere im Netz sind)
#define PORTAL_SSID      "WetterCube-Next-Setup"
#define PREFS_NAMESPACE  "wcnext"

// Abruf der Daten bei der Fusion (Adresse /api/next, Dienst _wettercube._tcp)
#define INTERVALL_ABRUF_MS      30000UL    // alle 30 Sekunden (die Fusion antwortet bei unveraenderten Daten mit 304)
#define INTERVALL_SUCHE_MS     600000UL    // Fusion erneut suchen: alle 10 Minuten, nach Fehlern frueher
#define ANZEIGE_ALT_MS         180000UL    // aelter als 3 Minuten: Anzeige "keine aktuellen Daten"

// Updates kommen von unserem Webspace (wettercube.de/dl/next, siehe tools/firmware-hochladen.sh im Website-Repo); GitHub dient als Rueckfall
// (und fuer Geraete mit aelterer Firmware, die noch dort fragen).
#define OTA_VERSION_URL        "https://wettercube.de/dl/next/version.json"
#define OTA_VERSION_URL2       "https://raw.githubusercontent.com/JPPeterson-lab/WetterCube-Next/main/docs/version.json"
#define OTA_BIN_URL            "https://wettercube.de/dl/next/firmware.bin"
#define OTA_BIN_URL2           "https://jppeterson-lab.github.io/WetterCube-Next/firmware/firmware.bin"
#define UPDATE_CHECK_ERST      120000UL    // 2 Min. nach dem Start
// danach alle 1, 6 oder 24 Stunden (cfg.updateStd), nur wenn die automatische Suche eingeschaltet ist
#define UPDATE_CHECK_RETRY     3600000UL   // nach einem Fehler nach 1 Stunde erneut
