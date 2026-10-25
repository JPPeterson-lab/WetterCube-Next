// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jan Althaus (JPPeterson-lab)
// Update ueber GitHub: version.json pruefen, firmware.bin ueber GitHub Pages laden und in den freien Slot schreiben (esp_ota_*)
#include "app_state.h"
#include "platform/HTTPClient.h"
#include "esp_ota_ops.h"
#include "esp_heap_caps.h"
#include <ArduinoJson.h>

bool   updateVerfuegbar = false;
String updateNeueVersion;
String updateLetzterFehler;
volatile bool otaLaeuft = false;
static unsigned long s_letztePruefung = 0, s_intervall = UPDATE_CHECK_ERST;

// true, wenn Version a neuer ist als b. Vorabversionen ("0.1.0-alpha1", "1.0.0-beta1", "1.0.0-rc1") sind aelter als das Release ohne Zusatz
// und werden untereinander nach Name (alpha < beta < rc) und dann nach Nummer verglichen
bool versionNeuer(const String& a, const String& b) {
  int pa[3] = {0, 0, 0}, pb[3] = {0, 0, 0};
  sscanf(a.c_str(), "%d.%d.%d", &pa[0], &pa[1], &pa[2]);
  sscanf(b.c_str(), "%d.%d.%d", &pb[0], &pb[1], &pb[2]);
  for (int i = 0; i < 3; i++) if (pa[i] != pb[i]) return pa[i] > pb[i];
  const char* sa = strchr(a.c_str(), '-');
  const char* sb = strchr(b.c_str(), '-');
  if (!sa || !sb) return sb && !sa;
  sa++; sb++;
  size_t za = 0, zb = 0;
  while (sa[za] && !isdigit((unsigned char)sa[za])) za++;
  while (sb[zb] && !isdigit((unsigned char)sb[zb])) zb++;
  int c = strncmp(sa, sb, za < zb ? za : zb);
  if (c == 0 && za != zb) c = za > zb ? 1 : -1;
  if (c != 0) return c > 0;
  return atoi(sa + za) > atoi(sb + zb);
}

bool otaPruefenJetzt() {
  s_letztePruefung = millis();
  if (WiFi.status() != WL_CONNECTED) return false;
  WiFiClientSecure sc;
  HTTPClient http;
  http.begin(sc, OTA_VERSION_URL);
  http.setTimeout(10000);
  int code = http.GET();
  bool ok = false;
  if (code == 200) {
    JsonDocument doc;
    if (deserializeJson(doc, http.getString()) == DeserializationError::Ok) {
      const char* v = doc["version"] | "";
      if (v[0]) {
        String latest(v);
        updateVerfuegbar = versionNeuer(latest, FIRMWARE_VERSION);
        updateNeueVersion = latest;
        logDiag("[Update] Pruefung: installiert %s, verfuegbar %s", FIRMWARE_VERSION, v);
        ok = true;
      }
    }
  } else { char e[80]; sc.lastError(e, sizeof(e)); logDiag("[Update] Pruefung: HTTP %d %s", code, e); }
  http.end();
  s_intervall = ok ? UPDATE_CHECK_INTERVALL : UPDATE_CHECK_RETRY;
  return ok;
}

void otaTick() {
  if (!cfg.autoUpdate || otaLaeuft || millis() - s_letztePruefung < s_intervall) return;
  otaPruefenJetzt();
}

static void installierenTask(void*) {
  String fehler;
  const esp_partition_t* ziel = esp_ota_get_next_update_partition(nullptr);
  esp_ota_handle_t h = 0; bool offen = false;
  uint8_t* buf = (uint8_t*)heap_caps_malloc(4096, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  uiHinweis("Update wird geladen", "Das Gerät bitte nicht vom Strom trennen");
  uiFortschritt(0);
  WiFi.setSleep(false);
  logDiag("[OTA] Lade %s", OTA_BIN_URL);
  WiFiClientSecure sc; HTTPClient http;
  int contentLen = -1; size_t geschrieben = 0;
  if (!ziel) fehler = "Kein Platz für das Update vorhanden";
  else if (!buf) fehler = "Zu wenig Speicher";
  else if (!http.begin(sc, OTA_BIN_URL)) fehler = "Adresse ungültig";
  if (fehler.isEmpty()) {
    http.setTimeout(20000);
    int code = http.GETStream();
    contentLen = http.getSize();
    logDiag("[OTA] nach dem Verbindungsaufbau: freier Speicher %u, kleinster Wert %u, groesster Block %u", (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMinFreeHeap(), (unsigned)ESP.getMaxAllocHeap());
    if (code != 200) { char e[80]; sc.lastError(e, sizeof(e)); fehler = String("Server antwortet mit ") + String(code) + " " + e; }
    else if (contentLen > 0 && (size_t)contentLen > ziel->size) fehler = "Die Datei ist zu groß für den Update-Platz";
    else if (esp_ota_begin(ziel, OTA_WITH_SEQUENTIAL_WRITES, &h) != ESP_OK) fehler = "Der Update-Platz lässt sich nicht beschreiben";
    else offen = true;
  }
  unsigned long letzteDaten = millis(), letzterBericht = 0;
  while (fehler.isEmpty()) {
    int n = http.read(buf, 4096);
    if (n > 0) {
      letzteDaten = millis();
      if (esp_ota_write(h, buf, n) != ESP_OK) { fehler = "Schreibfehler im Flash"; break; }
      geschrieben += n;
      if (contentLen > 0) uiFortschritt((int)((uint64_t)geschrieben * 100 / contentLen));
      if (millis() - letzterBericht > 5000) { letzterBericht = millis(); printf("[OTA] %u von %d Bytes\n", (unsigned)geschrieben, contentLen); }
    } else if (n == 0) break;
    else if (millis() - letzteDaten > 20000) { fehler = "Die Verbindung ist abgebrochen"; break; }
    else delay(20);
  }
  if (buf) free(buf);
  http.end();
  if (fehler.isEmpty() && contentLen > 0 && geschrieben != (size_t)contentLen) fehler = "Die Datei ist unvollständig angekommen";
  if (offen) {
    if (fehler.isEmpty()) {
      esp_err_t e = esp_ota_end(h);
      if (e != ESP_OK) fehler = e == ESP_ERR_OTA_VALIDATE_FAILED ? "Die Firmware-Datei ist beschädigt" : "Abschluss fehlgeschlagen";
      else if (esp_ota_set_boot_partition(ziel) != ESP_OK) fehler = "Startpartition lässt sich nicht setzen";
    } else esp_ota_abort(h);
  }
  if (fehler.isEmpty()) {
    logDiag("[OTA] %u Bytes geschrieben, Neustart", (unsigned)geschrieben);
    uiFortschritt(100); uiHinweis("Update installiert", "Das Gerät startet neu ...");
    delay(1500); ESP.restart();
  }
  logDiag("[OTA] Fehler: %s", fehler.c_str());
  updateLetzterFehler = fehler;
  WiFi.setSleep(false);
  uiHinweis("Update fehlgeschlagen", fehler.c_str());
  delay(6000);
  uiHauptseite();
  otaLaeuft = false;
  vTaskDelete(nullptr);
}

bool otaInstallierenStarten() {
  if (otaLaeuft || WiFi.status() != WL_CONNECTED) return false;
  otaLaeuft = true; updateLetzterFehler = "";
  xTaskCreatePinnedToCore(installierenTask, "ota", 8192, nullptr, 3, nullptr, 0);
  return true;
}
