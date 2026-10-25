// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jan Althaus (JPPeterson-lab)
// Fusion suchen (mDNS _wettercube._tcp oder feste Adresse) und Daten per HTTP holen (/api/next, mit Pruefsumme: unveraendert = 304)
#include "app_state.h"
#include "platform/HTTPClient.h"
#include "mdns.h"
#include <ArduinoJson.h>

static String s_etag;
static unsigned long s_tSuche = 0, s_tAbruf = 0;
static bool s_neu = true;
static bool s_seitenGeaendert = false;

String bloeckeFuerSeiten() {
  String s;
  if (cfg.seiten & 1) s = "wetter,sensoren,bio";
  if (cfg.seiten & 2) { if (s.length()) s += ","; s += "pollen,vorhersage"; }
  return s;
}

void datenNeuAnfordern() { s_etag = ""; s_tAbruf = 0; s_neu = true; }

// Fusion im Netz suchen: feste Adresse aus den Einstellungen, sonst mDNS
static bool suchen() {
  if (cfg.fusion_host.length()) { fusionAdresse = cfg.fusion_host; return true; }
  mdns_result_t* res = nullptr;
  esp_err_t e = mdns_query_ptr("_wettercube", "_tcp", 3000, 6, &res);
  if (e != ESP_OK || !res) { logDiag("[Suche] keine Fusion gefunden"); return false; }
  bool ok = false;
  for (mdns_result_t* r = res; r && !ok; r = r->next) {
    for (mdns_ip_addr_t* a = r->addr; a; a = a->next) {
      if (a->addr.type == ESP_IPADDR_TYPE_V4) { fusionAdresse = IPAddress(a->addr.u_addr.ip4.addr).toString(); ok = true; break; }
    }
  }
  mdns_query_results_free(res);
  if (ok) logDiag("[Suche] Fusion gefunden: %s", fusionAdresse.c_str());
  return ok;
}

static void kopieren(char* ziel, size_t n, const char* q) { strlcpy(ziel, q ? q : "", n); }

static String urlKodiert(const String& s) {
  String o; char b[4];
  for (size_t i = 0; i < s.length(); i++) {
    const char c = s[i];
    if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == ',') o += c;
    else { snprintf(b, sizeof(b), "%%%02X", (unsigned char)c); o += b; }
  }
  return o;
}

// Einstellungen vom Fusion (Reiter dieses Geraets auf wettercube-next.local): nur wenn der Fusion dieses Geraet verwaltet
static void cfgVomFusion(JsonObject c) {
  const int seiten = constrain((int)(c["seiten"] | cfg.seiten), 1, 3);
  const int wechsel = constrain((int)(c["wechsel"] | cfg.wechsel_sek), 0, 600);
  const int hell = constrain((int)(c["hell"] | cfg.helligkeit), 3, 100);
  const int nq = constrain((int)(c["nq"] | cfg.nachtQuelle), 0, 2);
  const int nvon = constrain((int)(c["nvon"] | cfg.nachtVon), 0, 1439), nbis = constrain((int)(c["nbis"] | cfg.nachtBis), 0, 1439);
  const int nverh = constrain((int)(c["nverh"] | cfg.nachtVerhalten), 0, 2);
  const int nhell = constrain((int)(c["nhell"] | cfg.nachtHell), 1, 100), nwach = constrain((int)(c["nwach"] | cfg.nachtWach), 3, 120);
  String r = c["r"] | "";
  if (r.length() > 200) r = r.substring(0, 200);
  const bool gleich = cfg.verwaltet && seiten == cfg.seiten && wechsel == cfg.wechsel_sek && hell == cfg.helligkeit && nq == cfg.nachtQuelle &&
                      nvon == cfg.nachtVon && nbis == cfg.nachtBis && nverh == cfg.nachtVerhalten && nhell == cfg.nachtHell && nwach == cfg.nachtWach && r == cfg.raeume;
  if (gleich) return;
  if (seiten != cfg.seiten) s_seitenGeaendert = true;
  cfg.verwaltet = true; cfg.seiten = seiten; cfg.wechsel_sek = wechsel; cfg.helligkeit = hell; cfg.nachtQuelle = nq; cfg.nachtVon = nvon; cfg.nachtBis = nbis;
  cfg.nachtVerhalten = nverh; cfg.nachtHell = nhell; cfg.nachtWach = nwach; cfg.raeume = r;
  speichereCfg();
  logDiag("[Fusion] Einstellungen vom Fusion uebernommen");
}

static bool auswerten(const char* json) {
  JsonDocument doc;
  if (deserializeJson(doc, json) != DeserializationError::Ok) return false;
  NextDaten d = daten;                                   // bisherige Daten behalten, nur gelieferte Bloecke ersetzen
  kopieren(d.tz, sizeof(d.tz), doc["tz"] | "");
  d.nachtOk = doc["nacht"].is<bool>(); d.nacht = doc["nacht"] | false;
  JsonObject cf = doc["cfg"];
  if (!cf.isNull()) cfgVomFusion(cf);
  else if (cfg.verwaltet) { cfg.verwaltet = false; speichereCfg(); logDiag("[Fusion] Der Fusion verwaltet dieses Geraet nicht mehr, die eigenen Einstellungen gelten wieder"); }
  JsonObject w = doc["wetter"];
  if (!w.isNull()) {
    d.wetterOk = true;
    d.temp = w["temp"] | 0.0f; d.gefuehlt = w["gefuehlt"] | 0.0f; d.tmin = w["tmin"] | 0.0f; d.tmax = w["tmax"] | 0.0f; d.uv = w["uv"] | 0.0f;
    d.feuchte = w["feuchte"] | 0; d.wmo = w["wmo"] | 0; d.tag = w["tag"] | true;
    kopieren(d.text, sizeof(d.text), w["text"] | "");
  }
  JsonArray v = doc["vorhersage"];
  if (!v.isNull()) {
    d.vorOk = true; int i = 0;
    for (JsonObject o : v) {
      if (i >= 4) break;
      kopieren(d.vor[i].zeit, sizeof(d.vor[i].zeit), o["zeit"] | ""); d.vor[i].temp = o["temp"] | 0.0f; d.vor[i].wmo = o["wmo"] | 0;
      d.vor[i].tag = o["tag"] | true; kopieren(d.vor[i].text, sizeof(d.vor[i].text), o["text"] | ""); i++;
    }
  }
  JsonArray p = doc["pollen"];
  if (!p.isNull()) {
    d.pollenOk = true; int i = 0;
    for (JsonObject o : p) {
      if (i >= 8) break;
      kopieren(d.pollen[i].name, sizeof(d.pollen[i].name), o["n"] | ""); d.pollen[i].gewaehlt = (o["g"] | 0) != 0;
      d.pollen[i].jetzt = o["j"] | -1.0f; d.pollen[i].heute = o["h"] | -1.0f; d.pollen[i].morgen = o["m"] | -1.0f; d.pollen[i].uebermorgen = o["u"] | -1.0f; i++;
    }
  }
  JsonObject b = doc["bio"];
  if (!b.isNull()) {
    d.bioOk = true; int c = 0;
    JsonArray ka = b["k"].as<JsonArray>();
    for (JsonVariant k : ka) {
      if (c >= 7) break;
      kopieren(d.bioName[c++], sizeof(d.bioName[0]), k | "");
    }
    int pi = 0;
    JsonArray oka = b["ok"].as<JsonArray>();
    JsonArray sela = b["sel"].as<JsonArray>();
    int si = 0;
    for (JsonVariant x : sela) { if (si >= 4) break; int c = x | 0; d.bioSel[si++] = (c >= 0 && c < 7) ? c : 0; }
    for (JsonVariant ok : oka) {
      if (pi >= 5) break;
      d.bioVorh[pi++] = (ok | 0) != 0;
    }
    pi = 0;
    JsonArray pa = b["p"].as<JsonArray>();
    for (JsonArray zeile : pa) {
      if (pi >= 5) break;
      int ci = 0;
      for (JsonVariant x : zeile) {
        if (ci >= 7) break;
        d.bio[pi][ci++] = x | 0;
      }
      pi++;
    }
  }
  JsonArray s = doc["sensoren"];
  if (!s.isNull()) {
    d.nSensoren = 0;
    for (JsonObject o : s) {
      if (d.nSensoren >= 16) break;
      NextDaten::Sensor& x = d.sensoren[d.nSensoren++];
      kopieren(x.id, sizeof(x.id), o["id"] | ""); kopieren(x.raum, sizeof(x.raum), o["raum"] | ""); kopieren(x.name, sizeof(x.name), o["name"] | ""); kopieren(x.einheit, sizeof(x.einheit), o["einheit"] | "");
      x.wert = o["wert"] | 0.0f; x.ok = o["ok"] | true;
    }
  }
  daten = d;
  s_etag = String(doc["etag"] | "");
  if (s_seitenGeaendert) { s_etag = ""; s_tAbruf = 0; }                    // andere Seiten: andere Datenbloecke holen
  if (d.tz[0] && s_neu) { setenv("TZ", d.tz, 1); tzset(); s_neu = false; }
  return true;
}

static bool abrufen() {
  String url = String("http://") + fusionAdresse + "/api/next?b=" + bloeckeFuerSeiten();
  // Das Geraet meldet sich beim Fusion mit Kennung, Version und aktuellen Einstellungen (so legt der Fusion den Reiter an)
  url += String("&id=") + geraetId + "&ip=" + WiFi.localIP().toString() + "&nv=" + FIRMWARE_VERSION + "&c=" + cfg.seiten + "," + cfg.wechsel_sek + "," + cfg.helligkeit + "," + cfg.nachtQuelle + "," +
         cfg.nachtVon + "," + cfg.nachtBis + "," + cfg.nachtVerhalten + "," + cfg.nachtHell + "," + cfg.nachtWach + "&r=" + urlKodiert(cfg.raeume);
  if (s_etag.length()) url += String("&e=") + s_etag;
  HTTPClient http;
  if (!http.begin(url)) return false;
  http.setTimeout(8000);
  int code = http.GET();
  if (code == 304) { datenStand = millis(); fusionFehler = 0; return true; }       // unveraendert
  if (code != 200) { logDiag("[Abruf] HTTP %d", code); return false; }
  String body = http.getString();
  if (!auswerten(body.c_str())) { logDiag("[Abruf] Antwort nicht lesbar"); return false; }
  datenStand = millis(); fusionFehler = 0;
  if (s_seitenGeaendert) { s_seitenGeaendert = false; uiHauptseite(); }       // die Seitenauswahl hat sich geaendert: neu auf die erste Seite
  return true;
}

void datenTick() {
  if (WiFi.status() != WL_CONNECTED) return;
  const unsigned long jetzt = millis();
  if (fusionAdresse.length() == 0 || fusionFehler >= 3) {
    if (s_tSuche == 0 || jetzt - s_tSuche >= (fusionAdresse.length() ? 60000UL : 15000UL)) {
      s_tSuche = jetzt ? jetzt : 1;
      if (suchen()) fusionFehler = 0;
    }
    if (fusionAdresse.length() == 0) return;
  }
  if (s_tAbruf == 0 || jetzt - s_tAbruf >= INTERVALL_ABRUF_MS) {
    s_tAbruf = jetzt ? jetzt : 1;
    if (abrufen()) uiAktualisieren();
    else { fusionFehler++; if (fusionFehler == 3) logDiag("[Abruf] Fusion %s antwortet nicht, suche neu", fusionAdresse.c_str()); }
  }
}
