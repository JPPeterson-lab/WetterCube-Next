// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jan Althaus (JPPeterson-lab)
// Start und Hauptschleife von WetterCube Next
#include "app_state.h"
#include "../display/lvgl_port.h"
#include "../board/cyd.h"
#include "esp_ota_ops.h"

extern void zeitServerStarten();   // platform/Zeit.cpp

static const char* grundName(uint8_t g) {
  switch (g) { case 2: return "Anmeldung abgelaufen"; case 15: case 204: return "Handshake-Timeout (Passwort?)"; case 201: return "Netzwerk nicht gefunden";
               case 202: return "Authentifizierung fehlgeschlagen"; case 200: return "Signal verloren"; default: return "sonstiger Grund"; }
}

void appStart() {
  ladeCfg();
  logDiag("[Start] WetterCube Next v%s", FIRMWARE_VERSION);
  ESP_ERROR_CHECK(lvglPortInit(cfg.panel, cfg.drehen, cfg.invert));
  cydHelligkeit(cfg.helligkeit);
  uiInit();

  if (cfg.ssid.isEmpty()) {
    portalPasswortNeu();
    { char t[160]; snprintf(t, sizeof(t), "WLAN \"" PORTAL_SSID "\" wählen,\nPasswort: %s\ndann 192.168.4.1 öffnen", portalPasswort); uiHinweis("Einrichtung", t); }
    starteWlanPortal();
    for (;;) delay(1000);
  }
  uiHinweis("Verbinde mit WLAN ...", cfg.ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(cfg.ssid.c_str(), cfg.pass.c_str());
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) delay(100);
  if (WiFi.status() != WL_CONNECTED) {
    logDiag("[WLAN] keine Verbindung: %s", grundName(wifiLetzterGrund));
    portalPasswortNeu();
    { char t[160]; snprintf(t, sizeof(t), "Einrichtung: WLAN \"" PORTAL_SSID "\" wählen,\nPasswort: %s\ndann 192.168.4.1 öffnen", portalPasswort); uiHinweis("Keine WLAN-Verbindung", t); }
    starteWlanPortal();
    for (;;) delay(1000);
  }
  logDiag("[WLAN] verbunden, IP %s", WiFi.localIP().toString().c_str());
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1); tzset();
  zeitServerStarten();
  starteWebUI();
  esp_ota_mark_app_valid_cancel_rollback();      // gesunder Start: neue Firmware gilt als gueltig
  logDiag("[Start] freier Speicher %u, groesster Block %u", (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());
  uiHinweis("Suche die Fusion ...", "");
  uiHauptseite();
}

void appLoop() {
  static unsigned long tUi = 0;
  if (!otaLaeuft) { datenTick(); otaTick(); }
  if (millis() - tUi >= 1000) { tUi = millis(); uiTick(); }
  delay(50);
}
