// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jan Althaus (JPPeterson-lab)
#include "app_state.h"
#include "platform/Preferences.h"
#include <stdarg.h>

Config cfg;
NextDaten daten;
unsigned long datenStand = 0;
String fusionAdresse;
int fusionFehler = 0;
volatile uint8_t wifiLetzterGrund = 0;
bool portalModus = false;

void ladeCfg() {
  Preferences p;
  if (!p.begin(PREFS_NAMESPACE, true)) return;
  cfg.ssid = p.getString("ssid", ""); cfg.pass = p.getString("pass", "");
  cfg.fusion_host = p.getString("fusion", "");
  cfg.seiten = p.getInt("seiten", 3); if (cfg.seiten < 1 || cfg.seiten > 3) cfg.seiten = 3;
  cfg.wechsel_sek = p.getInt("wechsel", 0);
  cfg.helligkeit = p.getInt("hell", 80);
  cfg.panel = p.getInt("panel", 1);
  cfg.drehen = p.getBool("drehen", false);
  cfg.invert = p.getBool("invert", false);
  cfg.raeume = p.getString("raeume", "");
  cfg.autoUpdate = p.getBool("autoupd", false);
  cfg.web_pw = p.getString("webpw", "");
  cfg.nachtQuelle = p.getInt("nq", 0); if (cfg.nachtQuelle < 0 || cfg.nachtQuelle > 2) cfg.nachtQuelle = 0;
  cfg.nachtVon = p.getInt("nvon", 22 * 60); cfg.nachtBis = p.getInt("nbis", 6 * 60 + 30);
  cfg.nachtVerhalten = p.getInt("nverh", 0) == 1 ? 1 : 0;
  cfg.nachtHell = p.getInt("nhell", 10); if (cfg.nachtHell < 1 || cfg.nachtHell > 100) cfg.nachtHell = 10;
  cfg.nachtWach = p.getInt("nwach", 15); if (cfg.nachtWach < 3 || cfg.nachtWach > 120) cfg.nachtWach = 15;
  p.end();
}

void speichereCfg() {
  Preferences p;
  if (!p.begin(PREFS_NAMESPACE, false)) return;
  p.putString("ssid", cfg.ssid); p.putString("pass", cfg.pass);
  p.putString("fusion", cfg.fusion_host);
  p.putInt("seiten", cfg.seiten); p.putInt("wechsel", cfg.wechsel_sek);
  p.putInt("hell", cfg.helligkeit); p.putInt("panel", cfg.panel);
  p.putBool("drehen", cfg.drehen); p.putBool("invert", cfg.invert); p.putString("raeume", cfg.raeume); p.putBool("autoupd", cfg.autoUpdate); p.putString("webpw", cfg.web_pw);
  p.putInt("nq", cfg.nachtQuelle); p.putInt("nvon", cfg.nachtVon); p.putInt("nbis", cfg.nachtBis);
  p.putInt("nverh", cfg.nachtVerhalten); p.putInt("nhell", cfg.nachtHell); p.putInt("nwach", cfg.nachtWach);
  p.end();
}

// ── Protokoll ────────────────────────────────────────────────────────────────
#define LOG_RING_SIZE 40
#define LOG_LINE_LEN  120
static char logRing[LOG_RING_SIZE][LOG_LINE_LEN];
static int logRingPos = 0, logRingCount = 0;
static portMUX_TYPE logMux = portMUX_INITIALIZER_UNLOCKED;

void logDiag(const char* fmt, ...) {
  char msg[LOG_LINE_LEN - 16];
  va_list a; va_start(a, fmt); vsnprintf(msg, sizeof(msg), fmt, a); va_end(a);
  char line[LOG_LINE_LEN];
  struct tm ti;
  if (getLocalTime(&ti, 0)) snprintf(line, sizeof(line), "%02d:%02d:%02d %s", ti.tm_hour, ti.tm_min, ti.tm_sec, msg);
  else                      snprintf(line, sizeof(line), "[%lus] %s", millis() / 1000, msg);
  portENTER_CRITICAL(&logMux);
  memcpy(logRing[logRingPos], line, LOG_LINE_LEN);
  logRingPos = (logRingPos + 1) % LOG_RING_SIZE;
  if (logRingCount < LOG_RING_SIZE) logRingCount++;
  portEXIT_CRITICAL(&logMux);
  puts(line);
}

String logAlsText() {
  String out;
  for (int i = 0; i < logRingCount; i++) {          // neueste zuerst
    int idx = (logRingPos - 1 - i + LOG_RING_SIZE * 2) % LOG_RING_SIZE;
    char z[LOG_LINE_LEN]; portENTER_CRITICAL(&logMux); memcpy(z, logRing[idx], LOG_LINE_LEN); portEXIT_CRITICAL(&logMux);
    out += z; out += "\n";
  }
  return out;
}
