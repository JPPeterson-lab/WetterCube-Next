// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jan Althaus (JPPeterson-lab)
// Web-Oberflaeche von WetterCube Next: Einrichtungsportal (WLAN), Einstellungen, Firmware-Datei, Protokoll
#include "app_state.h"
#include "esp_random.h"
#include "../platform/WebServer.h"
#include "../platform/DnsPortal.h"
#include "../board/cyd.h"
#include "../display/lvgl_port.h"
#include "esp_ota_ops.h"
#include "mdns.h"
#include "mbedtls/sha256.h"
#include "mbedtls/base64.h"

static WebServer server(80);

static String htmlEsc(const String& t) {
  String o;
  for (size_t i = 0; i < t.length(); i++) { char c = t[i]; if (c == '&') o += "&amp;"; else if (c == '<') o += "&lt;"; else if (c == '>') o += "&gt;"; else if (c == '"') o += "&quot;"; else o += c; }
  return o;
}

static const char KOPF[] = R"(<!DOCTYPE html><html lang="de"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>WetterCube Next</title><style>
body{background:#0b2545;color:#e6edf3;font-family:Arial,sans-serif;margin:0;padding:16px;max-width:520px;margin:auto}
h1{font-size:22px}h1 span{color:#87ceeb}h2{font-size:16px;color:#87ceeb;margin:22px 0 6px}
.k{background:#13315c;border-radius:10px;padding:12px 14px;margin-bottom:12px}
label{display:block;margin:10px 0 3px;font-size:14px}input[type=text],input[type=password],input[type=number],select{width:100%;box-sizing:border-box;padding:9px;border-radius:6px;border:1px solid #3b5b8c;background:#0b2545;color:#fff;font-size:15px}
.z{display:flex;align-items:center;gap:8px;margin:8px 0}.z input{width:auto}
button{background:#2ea043;color:#fff;border:0;border-radius:8px;padding:11px 18px;font-size:15px;margin-top:14px;width:100%}
small,.s{color:#9ca3af;font-size:12px}a{color:#87ceeb}
</style></head><body>)";

static String seite(const String& inhalt) { return String(KOPF) + inhalt + "</body></html>"; }
static String passwortKarte();   // unten definiert

// ── Einrichtungsportal ───────────────────────────────────────────────────────
static String netzwerkOptionen() {
  int n = WiFi.scanNetworks();
  String o;
  for (int i = 0; i < n; i++) o += "<option value=\"" + htmlEsc(WiFi.SSID(i)) + "\">" + htmlEsc(WiFi.SSID(i)) + " (" + String(WiFi.RSSI(i)) + " dBm)</option>";
  return o;
}

static void portalStart() {
  logDiag("[Portal] GET /");
  String h = "<h1>WetterCube <span>Next</span></h1><div class=\"k\"><b>WLAN einrichten</b><form method=\"POST\" action=\"/speichern\">"
             "<label>Netzwerk</label><select name=\"ssid\">" + netzwerkOptionen() + "</select>"
             "<label>Passwort</label><input type=\"password\" name=\"pass\">"
             "<label>Fusion (leer = automatisch suchen)</label><input type=\"text\" name=\"fusion\" placeholder=\"z. B. 192.168.178.101\">"
             "<button type=\"submit\">Speichern und neu starten</button></form></div>";
  server.send(200, "text/html", seite(h));
}

static void neustart(const char* text) {
  server.send(200, "text/html", seite(String("<div class=\"k\"><b>") + text + "</b><p>Das Gerät startet neu …</p></div>"));
  delay(1500);
  ESP.restart();
}

static void portalSpeichern() {
  if (server.hasArg("ssid")) cfg.ssid = server.arg("ssid");
  if (server.hasArg("pass")) cfg.pass = server.arg("pass");
  if (server.hasArg("fusion")) cfg.fusion_host = server.arg("fusion");
  speichereCfg();
  neustart("Gespeichert");
}

// Passwort des Einrichtungs-WLANs: 8 Ziffern (WPA2 verlangt mindestens 8 Zeichen), bei jedem Einrichtungsmodus neu, steht am Display
char portalPasswort[12] = "";
void portalPasswortNeu() { snprintf(portalPasswort, sizeof(portalPasswort), "%08u", (unsigned)(esp_random() % 100000000UL)); }

void starteWlanPortal() {
  portalModus = true;
  if (!portalPasswort[0]) portalPasswortNeu();
  WiFi.disconnect(true); delay(100);
  WiFi.setAutoReconnect(false);
  if (!WiFi.mode(WIFI_AP) || !WiFi.softAP(PORTAL_SSID, portalPasswort)) {
    logDiag("[Portal] WLAN-Treiber startet nicht");
    uiHinweis("WLAN nicht verfügbar", "Bitte das Gerät neu starten.");
    return;
  }
  dnsPortalStart(WiFi.softAPIP().raw());
  server.on("/", HTTP_GET, portalStart);
  server.on("/speichern", HTTP_POST, portalSpeichern);
  server.on("/log", HTTP_GET, []() { server.send(200, "text/plain; charset=utf-8", logAlsText()); });
  server.onNotFound([]() {            // Handy-Systeme pruefen per HTTP auf "Internet": Weiterleitung oeffnet das Portal-Fenster
    server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/");
    server.send(302, "text/plain", "");
  });
  server.begin();
  logDiag("[Portal] AP gestartet: %s", PORTAL_SSID);
}

// ── Einstellungen im Normalbetrieb ───────────────────────────────────────────
static String kreuz(const char* name, bool an, const char* text) {
  return String("<div class=\"z\"><input type=\"checkbox\" name=\"") + name + "\" value=\"1\"" + (an ? " checked" : "") + "><span>" + text + "</span></div>";
}

// Untere Haelfte der ersten Seite: Raeume und Biowetter nebeneinander oder nur eines davon ueber die ganze Breite (gilt auch, wenn der Fusion das Geraet verwaltet)
static String untenKarte() {
  auto opt = [](int v, const char* t) { return String("<option value=\"") + v + "\"" + (cfg.unten == v ? " selected" : "") + ">" + t + "</option>"; };
  return String("<div class=\"k\"><b>Erste Seite, unten</b><div class=\"s\" style=\"margin-top:8px\">Was zeigt die untere Hälfte? Allein belegt eine Kachel die ganze Breite, die Namen werden nicht mehr abgeschnitten.</div>"
                "<input type=\"hidden\" name=\"unten_form\" value=\"1\"><select name=\"unten\">")
         + opt(0, "Räume und Biowetter nebeneinander") + opt(1, "Nur Räume") + opt(2, "Nur Biowetter") + "</select></div>";
}

// Nachtmodus: wann ist Nacht (Fusion oder eigene Zeiten) und was passiert
static String zeitFeld(const char* name, int min) {
  char b[8]; snprintf(b, sizeof(b), "%02d:%02d", (min / 60) % 24, min % 60);
  return String("<input type=\"time\" name=\"") + name + "\" value=\"" + b + "\">";
}
static String nachtKarte() {
  auto opt = [](int wert, int aktuell, const char* text) { return String("<option value=\"") + String(wert) + "\"" + (wert == aktuell ? " selected" : "") + ">" + text + "</option>"; };
  String h = "<div class=\"k\"><b>Nachtmodus</b><div class=\"s\" style=\"margin-top:6px\">Gerade: " + String(uiNachtAktiv() ? "Nacht" : "Tag") + "</div>"
             "<label>Wann ist Nacht?</label><select name=\"nq\">" + opt(0, cfg.nachtQuelle, "Nie (kein Nachtmodus)") + opt(1, cfg.nachtQuelle, "Wie die Fusion (nach deren Einstellungen)") + opt(2, cfg.nachtQuelle, "Eigene Zeiten") + "</select>"
             "<label>Eigene Zeiten: von / bis</label><div style=\"display:flex;gap:8px\">" + zeitFeld("nvon", cfg.nachtVon) + zeitFeld("nbis", cfg.nachtBis) + "</div>"
             "<label>Was passiert nachts?</label><select name=\"nverh\">" + opt(0, cfg.nachtVerhalten, "Display aus (Antippen schaltet kurz an)") + opt(1, cfg.nachtVerhalten, "Dunkles Design, gedimmt") + opt(2, cfg.nachtVerhalten, "Nacht-Uhr (Uhrzeit, Datum, Wetter, gedimmt)") + "</select>"
             "<label>Helligkeit nachts (%)</label><input type=\"number\" name=\"nhell\" min=\"1\" max=\"100\" value=\"" + String(cfg.nachtHell) + "\">"
             "<label>Antippen schaltet nachts an für (Sekunden)</label><input type=\"number\" name=\"nwach\" min=\"3\" max=\"120\" value=\"" + String(cfg.nachtWach) + "\">";
  if (cfg.nachtQuelle == 1 && !daten.nachtOk) h += "<p class=\"s\">Die Fusion liefert den Nachtmodus erst ab v0.9.9-alpha7.</p>";
  return h + "</div>";
}

// Auswahl der Werte fuer die Kachel "Raeume": alle Sensoren, die die Fusion gerade liefert (bis vier, in der Reihenfolge der Auswahl der Fusion)
static String raeumeAuswahl() {
  String h = "<div class=\"k\"><b>Räume</b><div class=\"s\" style=\"margin-top:8px\">Welche Werte zeigt die Kachel „Räume“? Bis zu vier. Ohne Auswahl erscheinen die ersten vier. Das Biowetter folgt den Einstellungen der Fusion.</div>"
             "<input type=\"hidden\" name=\"raeume_form\" value=\"1\">";
  if (daten.nSensoren == 0) return h + "<p class=\"s\">Noch keine Sensorwerte von der Fusion erhalten.</p></div>";
  for (int i = 0; i < daten.nSensoren; i++) {
    const NextDaten::Sensor& s = daten.sensoren[i];
    const bool an = ("," + cfg.raeume + ",").indexOf(String(",") + s.id + ",") >= 0;
    String bez = String(s.raum[0] ? s.raum : "") + (s.raum[0] && s.name[0] ? " – " : "") + s.name;
    h += String("<div class=\"z\"><input type=\"checkbox\" name=\"r") + String(i) + "\" value=\"" + htmlEsc(s.id) + "\"" + (an ? " checked" : "") + "><span>" + htmlEsc(bez) + " <small>(" + String(s.wert, 1) + " " + htmlEsc(s.einheit) + ")</small></span></div>";
  }
  return h + "</div>";
}

static String updateKarte() {
  String h = "<h2>Update</h2><div class=\"k\"><div>Installiert: <b>v" FIRMWARE_VERSION "</b></div>";
  if (otaLaeuft) h += "<p class=\"s\">Ein Update wird gerade installiert ...</p>";
  else if (updateNeueVersion.length()) {
    if (updateVerfuegbar) h += "<p>Neue Version verfügbar: <b>v" + htmlEsc(updateNeueVersion) + "</b></p><form method=\"POST\" action=\"/ota_update\"><button type=\"submit\">Jetzt installieren</button></form>";
    else h += "<p class=\"s\">Das Gerät ist auf dem neuesten Stand.</p>";
  } else h += "<p class=\"s\">Noch nicht geprüft.</p>";
  if (updateLetzterFehler.length()) h += "<p class=\"s\">Letztes Update fehlgeschlagen: " + htmlEsc(updateLetzterFehler) + "</p>";
  h += "<form method=\"POST\" action=\"/ota_pruefen\"><button type=\"submit\" style=\"background:#3b5b8c\">Nach Updates suchen</button></form>"
       "<form method=\"POST\" action=\"/ota_auto\"><div class=\"z\"><input type=\"checkbox\" name=\"auto\" value=\"1\"" + String(cfg.autoUpdate ? " checked" : "") + "><span>Automatisch alle 6 Stunden nach Updates suchen</span></div>"
       "<div class=\"s\">Aus: Das Gerät fragt nie von selbst. Installiert wird immer nur auf Knopfdruck.</div><button type=\"submit\" style=\"background:#3b5b8c\">Übernehmen</button></form></div>";
  return h;
}

// Bildschirmfoto: /screenshot?seite=0 (Temperatur und Biowetter), 1 (Allergene und Vorhersage), boot (Startbild); ohne Angabe die aktuelle Seite.
// Das Bild kommt als BMP (320x240, 24 Bit) streifenweise direkt aus dem Zeichenspeicher.
static void screenshot() {
  const String a = server.arg("seite");
  const int seite = (a == "boot") ? -1 : (a == "1" ? SEITE_POLLEN_VOR : SEITE_TEMP_BIO);
  const bool umschalten = server.hasArg("seite");
  if (umschalten) { uiSeiteZeigen(seite); delay(700); }       // erst fertig umschalten und zeichnen lassen, dann aufnehmen
  if (!lvglPortCaptureStart()) { server.send(503, "text/plain", "Bildschirmfoto gerade nicht moeglich"); return; }
  const uint32_t gesamt = 54 + (uint32_t)CYD_H_RES * CYD_V_RES * 3;
  uint8_t k[54] = {'B', 'M'};
  auto le32 = [&](int o, uint32_t v) { k[o] = v; k[o + 1] = v >> 8; k[o + 2] = v >> 16; k[o + 3] = v >> 24; };
  le32(2, gesamt); le32(10, 54); le32(14, 40); le32(18, CYD_H_RES); le32(22, (uint32_t)(-CYD_V_RES));
  k[26] = 1; k[28] = 24; le32(34, (uint32_t)CYD_H_RES * CYD_V_RES * 3); le32(38, 2835); le32(42, 2835);
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "image/bmp", "");
  server.sendContent((const char*)k, sizeof(k));
  { LvLock lock; lv_obj_invalidate(lv_scr_act()); }          // ein ganzes Neuzeichnen: die Streifen kommen lueckenlos von oben
  const uint8_t* daten; int zeilen;
  while (lvglPortCaptureHole(&daten, &zeilen, 3000)) server.sendContent((const char*)daten, (size_t)zeilen * CYD_H_RES * 3);
  server.sendContent("", 0);
  lvglPortCaptureEnde();
}

static void einstellungen() {
  String h = "<h1>WetterCube <span>Next</span></h1><p class=\"s\">Version " FIRMWARE_VERSION " · " + WiFi.localIP().toString() +
             " · Fusion: " + (fusionAdresse.length() ? fusionAdresse : String("wird gesucht")) + "</p>"
             "<form method=\"POST\" action=\"/speichern\">";
  if (!(cfg.verwaltet && daten.untenVomFusion)) h += untenKarte();
  if (cfg.verwaltet) {
    h += "<div class=\"k\"><b>Vom Fusion verwaltet</b><div class=\"s\" style=\"margin-top:8px\">Seiten, Räume, Helligkeit, Weiterblättern, Nachtmodus und die Auswahl unten auf der ersten Seite dieses Displays stellst du in der Oberfläche des Fusion ein: "
         "<a href=\"http://" + (fusionAdresse.length() ? fusionAdresse : String("wettercube-next.local")) + "/next\">wettercube-next.local</a>, Reiter dieses Geräts. Dort lässt sich „Vom Fusion verwalten“ auch wieder ausschalten.</div></div>";
  } else {
    h += "<div class=\"k\"><b>Anzeige</b>"
         "<div class=\"s\" style=\"margin-top:8px\">Seiten (Antippen des Displays blättert weiter)</div>";
    h += kreuz("s0", cfg.seiten & 1, "Temperatur und Biowetter");
    h += kreuz("s1", cfg.seiten & 2, "Allergene und Vorhersage");
    h += "<label>Automatisch weiterblättern (Sekunden, 0 = nur per Antippen)</label><input type=\"number\" name=\"wechsel\" min=\"0\" max=\"600\" value=\"" + String(cfg.wechsel_sek) + "\">"
         "<label>Helligkeit am Tag (%)</label><input type=\"number\" name=\"hell\" min=\"3\" max=\"100\" value=\"" + String(cfg.helligkeit) + "\"></div>"
         + nachtKarte() + raeumeAuswahl();
  }
  h +=
       "<div class=\"k\"><b>Fusion</b><label>Adresse (leer = automatisch suchen)</label><input type=\"text\" name=\"fusion\" value=\"" + htmlEsc(cfg.fusion_host) + "\" placeholder=\"z. B. 192.168.178.101\"></div>"
       "<div class=\"k\"><b>Display (erst nach Neustart)</b><label>Controller</label><select name=\"panel\"><option value=\"0\"" + (cfg.panel == 0 ? " selected" : "") + ">ILI9341 (Ausführung mit ILI9341_2-Init, braucht Invertieren)</option>"
       "<option value=\"1\"" + (cfg.panel == 1 ? " selected" : "") + ">ST7789 (Vorgabe)</option></select>";
  h += kreuz("drehen", cfg.drehen, "Anzeige um 180° drehen");
  h += kreuz("invert", cfg.invert, "Farben invertieren (Vorgabe aus; einschalten, falls die Farben falsch herum aussehen)");
  h += "</div><button type=\"submit\">Speichern</button></form>"
       + updateKarte() + passwortKarte() +
       "<h2>Firmware-Datei</h2><div class=\"k\"><input type=\"file\" id=\"f\" accept=\".bin\"><button type=\"button\" onclick=\"up()\">Datei einspielen</button><p class=\"s\" id=\"m\"></p></div>"
       "<h2>Weiteres</h2><div class=\"k\"><a href=\"/log\">Protokoll</a> · <a href=\"/wlan\">WLAN ändern</a></div>"
       "<h2>Datenquellen</h2><div class=\"k\"><p class=\"s\">Wetterdaten: <a href=\"https://open-meteo.com\" target=\"_blank\" rel=\"noopener\">Weather data by Open-Meteo.com</a> (<a href=\"https://open-meteo.com/en/licence\" target=\"_blank\" rel=\"noopener\">CC BY 4.0</a>; die Werte werden teils umgerechnet, zum Beispiel zu Pollenstufen). Pollen und Luftqualität: Contains modified Copernicus Atmosphere Monitoring Service information, über Open-Meteo. Warnungen, Pollenflug und Biowetter: Quelle: <a href=\"https://opendata.dwd.de\" target=\"_blank\" rel=\"noopener\">Deutscher Wetterdienst</a> (<a href=\"https://www.dwd.de/DE/service/rechtliche_hinweise/rechtliche_hinweise_node.html\" target=\"_blank\" rel=\"noopener\">CC BY 4.0</a>). Die Daten kommen über den Fusion.</p></div>"
       "<script>function up(){var f=document.getElementById('f').files[0],m=document.getElementById('m');if(!f){m.textContent='Bitte eine Datei wählen';return}"
       "m.textContent='Wird übertragen …';fetch('/update_datei',{method:'POST',body:f}).then(function(r){return r.text()}).then(function(t){m.textContent=t=='ok'?'Fertig, das Gerät startet neu.':t}).catch(function(){m.textContent='Fehler'})}</script>";
  server.send(200, "text/html", seite(h));
}

static void speichern() {
  const int altPanel = cfg.panel; const bool altDrehen = cfg.drehen, altInvert = cfg.invert;
  const String altFusion = cfg.fusion_host;
  if (!cfg.verwaltet) {                                      // Seiten, Raeume, Helligkeit und Nachtmodus: nur hier, solange sie nicht vom Fusion kommen
  cfg.seiten = (server.hasArg("s0") ? 1 : 0) | (server.hasArg("s1") ? 2 : 0);
  if (cfg.seiten == 0) cfg.seiten = 1;
  cfg.wechsel_sek = constrain(server.arg("wechsel").toInt(), 0, 600);
  cfg.helligkeit = constrain(server.arg("hell").toInt(), 3, 100);
  if (server.hasArg("nq")) {
    auto minuten = [](const String& s, int vorgabe) { int h, m; return sscanf(s.c_str(), "%d:%d", &h, &m) == 2 && h >= 0 && h < 24 && m >= 0 && m < 60 ? h * 60 + m : vorgabe; };
    cfg.nachtQuelle = constrain(server.arg("nq").toInt(), 0, 2);
    cfg.nachtVon = minuten(server.arg("nvon"), cfg.nachtVon); cfg.nachtBis = minuten(server.arg("nbis"), cfg.nachtBis);
    cfg.nachtVerhalten = constrain(server.arg("nverh").toInt(), 0, 2);
    cfg.nachtHell = constrain(server.arg("nhell").toInt(), 1, 100);
    cfg.nachtWach = constrain(server.arg("nwach").toInt(), 3, 120);
  }

  if (server.hasArg("raeume_form")) {
    String liste; int n = 0;
    for (int i = 0; i < 16 && n < 4; i++) {
      String id = server.arg(String("r") + String(i)); id.trim();
      if (id.length()) { liste += (n ? "," : "") + id; n++; }
    }
    cfg.raeume = liste;
  }
  }
  if (server.hasArg("unten_form")) cfg.unten = constrain(server.arg("unten").toInt(), 0, 2);
  cfg.fusion_host = server.arg("fusion"); cfg.fusion_host.trim();
  cfg.panel = server.arg("panel").toInt() == 1 ? 1 : 0;
  cfg.drehen = server.hasArg("drehen"); cfg.invert = server.hasArg("invert");
  speichereCfg();                                            // die Helligkeit uebernimmt der Takt der Anzeige (beachtet auch die Nacht)
  if (cfg.fusion_host != altFusion) { fusionAdresse = ""; fusionFehler = 0; }
  datenNeuAnfordern();
  if (cfg.panel != altPanel || cfg.drehen != altDrehen || cfg.invert != altInvert) { neustart("Gespeichert"); return; }
  server.sendHeader("Location", "/");
  server.send(303, "text/plain", "");
}

static void wlanSeite() {
  String h = "<h1>WLAN <span>ändern</span></h1><div class=\"k\"><form method=\"POST\" action=\"/wlan_speichern\"><label>Netzwerk</label><select name=\"ssid\">" + netzwerkOptionen() +
             "</select><label>Passwort</label><input type=\"password\" name=\"pass\"><button type=\"submit\">Speichern und neu starten</button></form></div>";
  server.send(200, "text/html", seite(h));
}

static void wlanSpeichern() {
  if (server.hasArg("ssid")) cfg.ssid = server.arg("ssid");
  if (server.hasArg("pass")) cfg.pass = server.arg("pass");
  speichereCfg();
  neustart("WLAN gespeichert");
}

// Firmware aus einer Datei: der Rumpf der Anfrage ist die Datei selbst, Block fuer Block in den freien Slot schreiben, am Ende neu starten
static void updateDatei() {
  size_t gesamt = server.clientContentLength();
  logDiag("[OTA] Datei-Upload: %u Bytes", (unsigned)gesamt);
  const esp_partition_t* ziel = esp_ota_get_next_update_partition(nullptr);
  esp_ota_handle_t h = 0; String fehler;
  if (!ziel) fehler = "Kein Platz für das Update vorhanden";
  else if (gesamt == 0 || gesamt > ziel->size) fehler = "Datei leer oder zu groß";
  else if (esp_ota_begin(ziel, OTA_WITH_SEQUENTIAL_WRITES, &h) != ESP_OK) fehler = "Der Update-Platz lässt sich nicht beschreiben";
  uint8_t* buf = (uint8_t*)malloc(4096);
  if (!buf && fehler.isEmpty()) { fehler = "Zu wenig Speicher"; esp_ota_abort(h); }
  size_t rest = gesamt;
  while (fehler.isEmpty() && rest > 0) {
    size_t fuell = 0;
    while (fuell < 4096 && fuell < rest) {
      int n = server.bodyLesen(buf + fuell, (rest < 4096 ? rest : 4096) - fuell);
      if (n <= 0) break;
      fuell += (size_t)n;
    }
    if (fuell == 0) { fehler = "Der Upload ist abgebrochen"; esp_ota_abort(h); break; }
    if (esp_ota_write(h, buf, fuell) != ESP_OK) { fehler = "Schreibfehler im Flash"; esp_ota_abort(h); break; }
    rest -= fuell;
  }
  if (buf) free(buf);
  if (fehler.isEmpty()) {
    esp_err_t e = esp_ota_end(h);
    if (e != ESP_OK) fehler = e == ESP_ERR_OTA_VALIDATE_FAILED ? "Die Firmware-Datei ist beschädigt" : "Abschluss fehlgeschlagen";
    else if (esp_ota_set_boot_partition(ziel) != ESP_OK) fehler = "Startpartition lässt sich nicht setzen";
  }
  if (fehler.isEmpty()) { server.send(200, "text/plain", "ok"); delay(1500); ESP.restart(); }
  else { logDiag("[OTA] fehlgeschlagen: %s", fehler.c_str()); server.send(500, "text/plain", fehler); }
}

// ── Zugriffsschutz (Normalbetrieb) ───────────────────────────────────────────
// 1) Host-Pruefung (gegen DNS-Rebinding): Anfragen muessen an die IP-Adresse oder den Namen des Geraets gehen
// 2) Herkunftspruefung (gegen fremde Webseiten): schreibende Anfragen aus dem Browser nur von dieser Seite selbst
static bool hostErlaubt(String h) {
  h.toLowerCase();
  if (h.isEmpty()) return true;
  if (h[0] == '[') return true;                               // IPv6-Adresse
  int dp = h.indexOf(':'); if (dp >= 0) h = h.substring(0, dp);
  bool nurZiffern = true;
  for (size_t i = 0; i < h.length(); i++) if (!isdigit((unsigned char)h[i]) && h[i] != '.') { nurZiffern = false; break; }
  if (nurZiffern) return true;                                // IP-Adresse
  return h.startsWith(MDNS_NAME) || h.endsWith(".local") || h == "localhost";
}

// ── Passwort fuer die Web-Oberflaeche und "Passwort vergessen" ───────────────
static String sha256Hex(const String& s) {
  unsigned char d[32];
  mbedtls_sha256_context c;
  mbedtls_sha256_init(&c); mbedtls_sha256_starts(&c, 0);
  mbedtls_sha256_update(&c, (const unsigned char*)s.c_str(), s.length());
  mbedtls_sha256_finish(&c, d); mbedtls_sha256_free(&c);
  char hex[65];
  for (int i = 0; i < 32; i++) snprintf(hex + 2 * i, 3, "%02x", d[i]);
  return String(hex);
}

static void webPwSetzen(const String& pw) {
  char salz[17]; snprintf(salz, sizeof(salz), "%08x%08x", (unsigned)esp_random(), (unsigned)esp_random());
  cfg.web_pw = String(salz) + ":" + sha256Hex(String(salz) + pw);
}

static bool gleichLang(const String& a, const String& b) {
  unsigned char diff = (unsigned char)(a.length() ^ b.length());
  for (size_t i = 0; i < a.length() && i < b.length(); i++) diff |= (unsigned char)(a[i] ^ b[i]);
  return diff == 0;
}

static bool webPwPruefen(const String& pw) {
  int d = cfg.web_pw.indexOf(':');
  if (d != 16) return false;
  return gleichLang(cfg.web_pw.substring(d + 1), sha256Hex(cfg.web_pw.substring(0, d) + pw));
}

static bool anmeldungOk() {
  String a = server.header("Authorization");
  if (!a.startsWith("Basic ")) return false;
  String b64 = a.substring(6); b64.trim();
  unsigned char out[160]; size_t n = 0;
  if (mbedtls_base64_decode(out, sizeof(out) - 1, &n, (const unsigned char*)b64.c_str(), b64.length()) != 0) return false;
  out[n] = 0;
  String up = String((const char*)out);
  int c = up.indexOf(':');
  if (c < 0 || up.substring(0, c) != "admin") return false;
  return webPwPruefen(up.substring(c + 1));
}

static void antwort401() {
  server.sendHeader("WWW-Authenticate", "Basic realm=\"WetterCube Next\", charset=\"UTF-8\"");
  server.send(401, "text/html; charset=utf-8", seite("<h1>Anmeldung <span>erforderlich</span></h1><div class=\"k\">Benutzername: <b>admin</b><br><a href=\"/passwort_vergessen\">Passwort vergessen?</a></div>"));
}

// Karte "Passwort" auf der Einstellungsseite
static String passwortKarte() {
  String s = String("<h2>Passwort für die Web-Oberfläche</h2><div class=\"k\"><p class=\"s\">") +
             (cfg.web_pw.isEmpty() ? "Es ist kein Passwort gesetzt: Jeder im Heimnetz kann die Einstellungen ändern." : "Ein Passwort ist gesetzt (Benutzername: admin).") + "</p>"
             "<p class=\"s\">Das Passwort wird im Heimnetz unverschlüsselt (HTTP) übertragen: Es schützt vor Zugriffen anderer Geräte und fremder Webseiten, nicht vor jemandem, der den Netzverkehr mitliest. "
             "<b>Vergessen?</b> Auf der Anmeldeseite führt „Passwort vergessen?“ weiter: Am Display erscheint ein Code, mit dem du das Passwort löschst.</p>"
             "<form method=\"POST\" action=\"/passwort_speichern\" onsubmit=\"var w=this.pw_weg;return !(w&&w.checked)||confirm('Passwort entfernen? Dann kann jeder im Heimnetz die Einstellungen ändern.')\">"
             "<label>Neues Passwort (mindestens 6 Zeichen)</label><input type=\"password\" name=\"pw1\" autocomplete=\"new-password\">"
             "<label>Wiederholen</label><input type=\"password\" name=\"pw2\" autocomplete=\"new-password\">"
             "<div class=\"z\"><input type=\"checkbox\" name=\"pw_weg\" value=\"1\"><span>Passwort entfernen</span></div>"
             "<button type=\"submit\">Passwort speichern</button></form></div>";
  return s;
}

static void handlePwSpeichern() {
  if (server.hasArg("pw_weg")) {
    cfg.web_pw = ""; speichereCfg();
    logDiag("[Sicherheit] Web-Passwort entfernt");
  } else {
    String p1 = server.arg("pw1"), p2 = server.arg("pw2");
    if (p1.length() < 6 || p1.length() > 64) { server.send(400, "text/plain; charset=utf-8", "Das Passwort muss 6 bis 64 Zeichen lang sein."); return; }
    if (p1 != p2) { server.send(400, "text/plain; charset=utf-8", "Die beiden Passwoerter sind nicht gleich."); return; }
    webPwSetzen(p1); speichereCfg();
    logDiag("[Sicherheit] Web-Passwort gesetzt");
  }
  server.sendHeader("Location", "/"); server.send(303, "text/plain", "");
}

// "Passwort vergessen": Ein 6-stelliger Code erscheint am Display. Wer ihn im Browser eingibt, loescht das Passwort.
// Gueltig 2 Minuten, nach 5 falschen Eingaben 10 Minuten gesperrt, der Code laesst sich hoechstens alle 20 Sekunden neu anfordern.
static String s_resetCode;
static unsigned long s_resetBis = 0, s_resetSperreBis = 0, s_resetStart = 0;
static uint8_t s_resetFehl = 0;
static const unsigned long RESET_GUELTIG_MS = 120000UL, RESET_SPERRE_MS = 600000UL;

static String vergessenSeite(const String& meldung) {
  String h = "<h1>Passwort <span>vergessen</span></h1>";
  if (!meldung.isEmpty()) h += "<div class=\"k\"><b>" + meldung + "</b></div>";
  if (cfg.web_pw.isEmpty()) return seite(h + "<div class=\"k\">Es ist kein Passwort gesetzt. <a href=\"/\">Zu den Einstellungen</a></div>");
  h += "<div class=\"k\"><p class=\"s\">1. Tippe auf den Knopf: Am Gerät erscheint ein Code (zwei Minuten gültig).</p>"
       "<form method=\"POST\" action=\"/passwort_vergessen/start\"><button type=\"submit\">Code am Gerät anzeigen</button></form></div>"
       "<div class=\"k\"><p class=\"s\">2. Gib den Code hier ein. Das Passwort wird dann gelöscht.</p>"
       "<form method=\"POST\" action=\"/passwort_vergessen/loeschen\"><input type=\"text\" name=\"code\" inputmode=\"numeric\" maxlength=\"6\" autocomplete=\"off\" placeholder=\"6-stelliger Code\">"
       "<button type=\"submit\">Passwort löschen</button></form></div>";
  return seite(h);
}

static bool resetGesperrt() { return s_resetSperreBis && (long)(millis() - s_resetSperreBis) < 0; }

static void handlePwVergessen() { server.send(200, "text/html; charset=utf-8", vergessenSeite("")); }

static void handlePwVergessenStart() {
  unsigned long jetzt = millis();
  if (cfg.web_pw.isEmpty()) { server.send(200, "text/html; charset=utf-8", vergessenSeite("")); return; }
  if (resetGesperrt()) { server.send(200, "text/html; charset=utf-8", vergessenSeite("Zu viele falsche Eingaben. Bitte zehn Minuten warten.")); return; }
  if (s_resetStart && jetzt - s_resetStart < 20000UL) { server.send(200, "text/html; charset=utf-8", vergessenSeite("Bitte kurz warten und dann noch einmal tippen.")); return; }
  char c[8]; snprintf(c, sizeof(c), "%06u", (unsigned)(esp_random() % 1000000UL));
  s_resetCode = c; s_resetBis = jetzt + RESET_GUELTIG_MS; s_resetStart = jetzt; s_resetFehl = 0;
  uiResetCodeZeigen(c, RESET_GUELTIG_MS / 1000);
  logDiag("[Sicherheit] Code zum Zuruecksetzen des Web-Passworts am Display gezeigt");
  server.send(200, "text/html; charset=utf-8", vergessenSeite("Der Code steht jetzt am Display des Geräts (zwei Minuten gültig)."));
}

static void handlePwVergessenLoeschen() {
  unsigned long jetzt = millis();
  if (cfg.web_pw.isEmpty()) { server.send(200, "text/html; charset=utf-8", vergessenSeite("")); return; }
  if (resetGesperrt()) { server.send(200, "text/html; charset=utf-8", vergessenSeite("Zu viele falsche Eingaben. Bitte zehn Minuten warten.")); return; }
  String c = server.arg("code"); c.trim();
  const bool gueltig = s_resetCode.length() == 6 && (long)(s_resetBis - jetzt) > 0;
  if (gueltig && gleichLang(c, s_resetCode)) {
    cfg.web_pw = ""; speichereCfg();
    s_resetCode = ""; s_resetBis = 0; s_resetFehl = 0;
    uiResetCodeWeg();
    logDiag("[Sicherheit] Web-Passwort mit dem Code vom Display geloescht");
    server.sendHeader("Location", "/"); server.send(303, "text/plain", "");
    return;
  }
  if (++s_resetFehl >= 5) {
    s_resetSperreBis = jetzt + RESET_SPERRE_MS; s_resetFehl = 0; s_resetCode = ""; s_resetBis = 0;
    uiResetCodeWeg();
    logDiag("[Sicherheit] Zu viele falsche Codes, Zuruecksetzen 10 Minuten gesperrt");
    server.send(200, "text/html; charset=utf-8", vergessenSeite("Zu viele falsche Eingaben. Bitte zehn Minuten warten."));
    return;
  }
  server.send(200, "text/html; charset=utf-8", vergessenSeite(gueltig ? "Der Code stimmt nicht." : "Es gibt keinen gültigen Code. Tippe erst auf „Code am Gerät anzeigen“."));
}

// Wird vor jedem Handler aufgerufen; false = die Anfrage wurde schon abgelehnt
static bool zugriffErlaubt() {
  if (portalModus) return true;                               // Einrichtungs-WLAN: eigenes Passwort, und das Portal braucht beliebige Namen
  String host = server.hostHeader();
  if (!hostErlaubt(host)) { server.send(403, "text/plain", "Zugriff abgelehnt: Das ist nicht die Adresse des Geraets."); return false; }
  if (!server.istGet()) {                                     // schreibende Anfragen: Browser melden ihre Herkunft
    String o = server.header("Origin");
    if (o.length() && !o.equalsIgnoreCase("http://" + host)) {
      logDiag("[Sicherheit] Anfrage von fremder Herkunft abgelehnt: %s", o.c_str());
      server.send(403, "text/plain", "Zugriff abgelehnt: Anfrage von einer fremden Seite.");
      return false;
    }
  }
  if (cfg.web_pw.isEmpty()) return true;
  if (server.uri().startsWith("/passwort_vergessen")) return true;   // der Weg zum Zuruecksetzen braucht kein Passwort
  static unsigned long sperreBis = 0; static uint8_t fehl = 0;
  if (sperreBis && (long)(millis() - sperreBis) < 0) { server.send(429, "text/plain", "Zu viele falsche Anmeldungen. Bitte eine Minute warten."); return false; }
  if (anmeldungOk()) { fehl = 0; return true; }
  if (server.header("Authorization").length()) {              // Anmeldedaten waren falsch (nicht nur die erste Anfrage ohne Daten)
    vTaskDelay(pdMS_TO_TICKS(500));
    if (++fehl >= 8) { fehl = 0; sperreBis = millis() + 60000UL; logDiag("[Sicherheit] Zu viele falsche Anmeldungen, 60 s gesperrt"); }
  }
  antwort401();
  return false;
}

void starteWebUI() {
  if (mdns_init() == ESP_OK && mdns_hostname_set(geraetName) == ESP_OK) {
    mdns_instance_name_set("WetterCube Next");
    mdns_service_add(nullptr, "_http", "_tcp", 80, nullptr, 0);
    logDiag("[Start] mDNS: %s.local", geraetName);
  }
  server.on("/", HTTP_GET, einstellungen);
  server.on("/speichern", HTTP_POST, speichern);
  server.on("/wlan", HTTP_GET, wlanSeite);
  server.on("/wlan_speichern", HTTP_POST, wlanSpeichern);
  server.on("/update_datei", HTTP_POST, updateDatei);
  server.on("/ota_pruefen", HTTP_POST, []() { otaPruefenJetzt(); server.sendHeader("Location", "/"); server.send(303, "text/plain", ""); });
  server.on("/ota_auto", HTTP_POST, []() { cfg.autoUpdate = server.hasArg("auto"); speichereCfg(); server.sendHeader("Location", "/"); server.send(303, "text/plain", ""); });
  server.on("/ota_update", HTTP_POST, []() {
    const bool ok = otaInstallierenStarten();
    server.send(200, "text/html", seite(String("<div class=\"k\"><b>") + (ok ? "Das Update wird geladen" : "Das Update lässt sich gerade nicht starten") + "</b><p>" + (ok ? "Das Gerät startet danach selbst neu (etwa eine Minute). Bitte nicht vom Strom trennen." : "Kein WLAN oder ein Update läuft schon.") + "</p><p><a href=\"/\">Zurück</a></p></div>"));
  });
  server.on("/log", HTTP_GET, []() { server.send(200, "text/plain; charset=utf-8", logAlsText()); });
  server.on("/screenshot", HTTP_GET, screenshot);
  server.on("/passwort_speichern", HTTP_POST, handlePwSpeichern);
  server.on("/passwort_vergessen", HTTP_GET, handlePwVergessen);
  server.on("/passwort_vergessen/start", HTTP_POST, handlePwVergessenStart);
  server.on("/passwort_vergessen/loeschen", HTTP_POST, handlePwVergessenLoeschen);
  server.setZugriffsPruefer(zugriffErlaubt);
  server.begin();
}
