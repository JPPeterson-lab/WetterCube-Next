// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jan Althaus (JPPeterson-lab)
// Anzeige von WetterCube Next: Seiten aus dem PicoPixel-Export (src/ui/), Schriften und Ausrichtung hier im Code, Daten aus daten/NextDaten
#include "app_state.h"
#include "../display/lvgl_port.h"
#include "../ui/ui.h"
#include "../ui/screens.h"
#include "../ui/fonts/fonts.h"
extern "C" const lv_font_t font_montserrat_18;
extern "C" const lv_font_t font_montserrat_32;
extern "C" const lv_font_t font_montserrat_22;     // aus src/fonts/ (PicoPixel exportiert nur die im Entwurf benutzten Groessen)

#define F16 (&font_montserrat_16)
#define F18 (&font_montserrat_18)
#define F22 (&font_montserrat_22)
#define F32 (&font_montserrat_32)
#define F48 (&font_montserrat_48)
// Farben wie das helle WetterCube-Thema (Fusion): weisser Grund, schwarze Schrift, orange Ueberschriften, gruene Werte
#define ORANGE   lv_color_hex(0xCE4B11)
#define GRUEN    lv_color_hex(0x0FA64A)
#define GRAU     lv_color_hex(0x9CA3AF)
#define SCHWARZ  lv_color_hex(0x000000)

static int s_seite = -1;                 // SEITE_TEMP_BIO / SEITE_POLLEN_VOR, -1 = Startbild
static lv_obj_t* s_hinweis = nullptr;    // Hinweisleiste unten (layer_top)
static lv_obj_t* s_hinweisText = nullptr;
static unsigned long s_tSeite = 0;

// ── Hilfen ───────────────────────────────────────────────────────────────────
static void setText(lv_obj_t* o, const char* t) { if (o && strcmp(lv_label_get_text(o), t) != 0) lv_label_set_text(o, t); }
static void zahl(char* buf, size_t n, float v, int dez, const char* einheit = "") {
  snprintf(buf, n, dez ? "%.1f%s" : "%.0f%s", v, einheit);
  for (char* p = buf; *p; p++) if (*p == '.') *p = ',';
}
static void stil(lv_obj_t* o, const lv_font_t* f, lv_color_t c) {
  lv_obj_set_style_text_font(o, f, 0);
  lv_obj_set_style_text_color(o, c, 0);
}
// Feste Breite, Textende mit "..." (Namen) bzw. rechtsbuendig (Werte)
static void feld(lv_obj_t* o, int x, int w, int h, bool rechts) {
  lv_obj_set_x(o, x);
  lv_obj_set_size(o, w, h);
  lv_label_set_long_mode(o, LV_LABEL_LONG_DOT);
  if (rechts) lv_obj_set_style_text_align(o, LV_TEXT_ALIGN_RIGHT, 0);
}
static void nachtKlick();
static void klickbar(lv_obj_t* screen, lv_obj_t* kachelA, lv_obj_t* kachelB) {
  if (kachelA) lv_obj_clear_flag(kachelA, LV_OBJ_FLAG_CLICKABLE);
  if (kachelB) lv_obj_clear_flag(kachelB, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(screen, [](lv_event_t*) { nachtKlick(); }, LV_EVENT_CLICKED, nullptr);
}

struct Zeile { lv_obj_t** name; lv_obj_t** wert; };

// Wertespalte rechtsbuendig so breit wie der Text, der Name bekommt den Rest der Kachel (sonst wird z. B. "24,8 °C" abgeschnitten)
// (gemessen wird der echte Text: ein gekuerztes Label haelt sonst schon den Text mit "...")
static void zeileAnpassen(lv_obj_t* name, lv_obj_t* wert, const char* text, int kachelBreite) {
  lv_point_t sz;
  lv_txt_get_size(&sz, text, F16, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
  const int w = sz.x + 4, rechts = kachelBreite - 6;
  lv_obj_set_width(wert, w);
  lv_obj_set_x(wert, rechts - w);
  lv_obj_set_width(name, rechts - w - 8 - 4);
}

static Zeile zSens[4], zBio[4], zPollen[8];
static lv_obj_t** vorZeit[4];
static lv_obj_t** vorTemp[4];
static lv_obj_t** vorBild[4];

// ── Aufbau ───────────────────────────────────────────────────────────────────
static void startbildAusrichten() {
  // Nur die Ausrichtung angepasst: "WetterCube" + "Next" als Paar mittig (Schrift 32, damit es in 320 px passt)
  lv_point_t a, b;
  lv_txt_get_size(&a, "WetterCube", F32, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
  lv_txt_get_size(&b, "Next", F32, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
  const int luecke = 12, x0 = (CYD_BREITE - (a.x + luecke + b.x)) / 2;
  lv_obj_set_style_text_font(objects.labelwetter, F32, 0);
  lv_obj_set_style_text_font(objects.labelcube, F32, 0);
  lv_obj_set_pos(objects.labelwetter, x0, 60);
  lv_obj_set_pos(objects.labelcube, x0 + a.x + luecke, 60);
  lv_obj_set_style_text_align(objects.labelstatus, LV_TEXT_ALIGN_CENTER, 0);
  // Datenquellen (Namensnennung) unten im Startbild
  lv_obj_t* q = lv_label_create(objects.screenboot);
  lv_obj_set_width(q, CYD_BREITE - 20);
  lv_obj_set_pos(q, 10, 196);
  lv_label_set_long_mode(q, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(q, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(q, F16, 0);
  lv_obj_set_style_text_color(q, GRAU, 0);
  lv_label_set_text(q, "Wetterdaten: Open-Meteo.com, DWD");
}

static void seitenAufbauen() {
  // Seite 1: Temperatur und Biowetter
  stil(objects.labeltemp, F48, GRUEN);
  stil(objects.labelwettertext, F16, SCHWARZ);
  lv_obj_set_size(objects.labelwettertext, 226, 22); lv_label_set_long_mode(objects.labelwettertext, LV_LABEL_LONG_DOT);
  stil(objects.labeluhr, F22, SCHWARZ);
  lv_obj_set_size(objects.labeluhr, 76, 28); lv_obj_set_style_text_align(objects.labeluhr, LV_TEXT_ALIGN_RIGHT, 0);
  stil(objects.labelminmax, F16, SCHWARZ);
  lv_obj_set_size(objects.labelminmax, 100, 22); lv_obj_set_style_text_align(objects.labelminmax, LV_TEXT_ALIGN_RIGHT, 0);
  stil(objects.labelsenstitel, F18, ORANGE);
  stil(objects.labelbiotitel, F18, ORANGE);
  Zeile s[4] = {{&objects.labelsens1name, &objects.labelsens1wert}, {&objects.labelsens2name, &objects.labelsens2wert},
                {&objects.labelsens3name, &objects.labelsens3wert}, {&objects.labelsens4name, &objects.labelsens4wert}};
  Zeile b[4] = {{&objects.labelbio1name, &objects.labelbio1wert}, {&objects.labelbio2name, &objects.labelbio2wert},
                {&objects.labelbio3name, &objects.labelbio3wert}, {&objects.labelbio4name, &objects.labelbio4wert}};
  for (int i = 0; i < 4; i++) {
    zSens[i] = s[i]; zBio[i] = b[i];
    stil(*s[i].name, F16, SCHWARZ); stil(*s[i].wert, F16, SCHWARZ);
    feld(*s[i].name, 8, 88, 22, false); feld(*s[i].wert, 92, 54, 22, true);
    stil(*b[i].name, F16, SCHWARZ); stil(*b[i].wert, F16, SCHWARZ);
    feld(*b[i].name, 8, 94, 22, false); feld(*b[i].wert, 100, 46, 22, true);
  }
  // Seite 2: Allergene und Vorhersage
  stil(objects.labelpollentitel, F18, ORANGE);
  stil(objects.labelvortitel, F18, ORANGE);
  lv_obj_t** pn[8] = {&objects.labelpollen1name, &objects.labelpollen2name, &objects.labelpollen3name, &objects.labelpollen4name,
                      &objects.labelpollen5name, &objects.labelpollen6name, &objects.labelpollen7name, &objects.labelpollen8name};
  lv_obj_t** pw[8] = {&objects.labelpollen1wert, &objects.labelpollen2wert, &objects.labelpollen3wert, &objects.labelpollen4wert,
                      &objects.labelpollen5wert, &objects.labelpollen6wert, &objects.labelpollen7wert, &objects.labelpollen8wert};
  for (int i = 0; i < 8; i++) {
    zPollen[i] = {pn[i], pw[i]};
    stil(*pn[i], F16, SCHWARZ); stil(*pw[i], F16, SCHWARZ);
    feld(*pn[i], 10, 84, 22, false); feld(*pw[i], 94, 58, 22, true);
  }
  vorZeit[0] = &objects.labelvor1zeit; vorZeit[1] = &objects.labelvor2zeit; vorZeit[2] = &objects.labelvor3zeit; vorZeit[3] = &objects.labelvor4zeit;
  vorTemp[0] = &objects.labelvor1temp; vorTemp[1] = &objects.labelvor2temp; vorTemp[2] = &objects.labelvor3temp; vorTemp[3] = &objects.labelvor4temp;
  vorBild[0] = &objects.imagevor1; vorBild[1] = &objects.imagevor2; vorBild[2] = &objects.imagevor3; vorBild[3] = &objects.imagevor4;
  for (int i = 0; i < 4; i++) {
    stil(*vorZeit[i], F16, ORANGE); stil(*vorTemp[i], F18, GRUEN);
    feld(*vorZeit[i], 52, 76, 20, false); feld(*vorTemp[i], 52, 76, 22, false);
    lv_img_set_size_mode(*vorBild[i], LV_IMG_SIZE_MODE_VIRTUAL);
    lv_obj_set_size(*vorBild[i], 40, 40);
  }
  lv_img_set_size_mode(objects.imagewetter, LV_IMG_SIZE_MODE_VIRTUAL);
  lv_obj_set_size(objects.imagewetter, 64, 64);
  // Der PicoPixel-Export versetzt diese beiden Kacheln gegenueber dem Entwurf: Position hier fest vorgeben
  lv_obj_set_pos(objects.kachelsensoren, 8, 92);
  lv_obj_set_pos(objects.kachelbio, 162, 92);
  klickbar(objects.screentempbio, objects.kachelsensoren, objects.kachelbio);
  klickbar(objects.screenpollenvor, objects.kachelpollen, objects.kachelvor);
}

static void hinweisAufbauen() {
  s_hinweis = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(s_hinweis);
  lv_obj_set_size(s_hinweis, 230, 22);
  lv_obj_align(s_hinweis, LV_ALIGN_BOTTOM_MID, 0, -4);
  lv_obj_set_style_bg_opa(s_hinweis, 235, 0);
  lv_obj_set_style_bg_color(s_hinweis, lv_color_hex(0xB45309), 0);
  lv_obj_set_style_radius(s_hinweis, 11, 0);
  lv_obj_clear_flag(s_hinweis, LV_OBJ_FLAG_CLICKABLE);
  s_hinweisText = lv_label_create(s_hinweis);
  lv_obj_set_style_text_font(s_hinweisText, F16, 0);
  lv_obj_set_style_text_color(s_hinweisText, lv_color_hex(0xFFFFFF), 0);
  lv_obj_center(s_hinweisText);
  lv_obj_add_flag(s_hinweis, LV_OBJ_FLAG_HIDDEN);
}

void uiInit() {
  LvLock lock;
  create_screens();
  seitenAufbauen();
  startbildAusrichten();
  hinweisAufbauen();
  s_seite = -1;
  loadScreen(SCREEN_ID_SCREENBOOT);
}

// ── Inhalt ───────────────────────────────────────────────────────────────────
static void seiteTempBio() {
  char b[48];
  const NextDaten& d = daten;
  if (d.wetterOk) {
    zahl(b, sizeof(b), d.temp, 0, " °C"); setText(objects.labeltemp, b);
    char g[16]; zahl(g, sizeof(g), d.gefuehlt, 0, " °C");
    snprintf(b, sizeof(b), "%s, gefühlt %s", d.text, g); setText(objects.labelwettertext, b);
    char lo[12], hi[12]; zahl(lo, sizeof(lo), d.tmin, 0, "°"); zahl(hi, sizeof(hi), d.tmax, 0, "°");
    snprintf(b, sizeof(b), "%s / %s", lo, hi); setText(objects.labelminmax, b);
    const lv_img_dsc_t* ic = wmoZuImage(d.wmo, d.tag, false);
    if (ic && lv_img_get_src(objects.imagewetter) != ic) lv_img_set_src(objects.imagewetter, ic);
  } else {
    setText(objects.labeltemp, "-- °C"); setText(objects.labelwettertext, ""); setText(objects.labelminmax, "");
  }
  // Raeume: die in der Weboberflaeche gewaehlten Sensoren (Reihenfolge der Auswahl), sonst die ersten vier
  int wahl[4], nw = 0;
  {
    String liste = cfg.raeume;
    int von = 0;
    while (nw < 4 && von <= (int)liste.length()) {
      int bis = liste.indexOf(',', von); if (bis < 0) bis = liste.length();
      String id = liste.substring(von, bis); id.trim();
      for (int k = 0; id.length() && k < d.nSensoren; k++) if (id == d.sensoren[k].id) { wahl[nw++] = k; break; }
      von = bis + 1;
    }
    if (nw == 0) for (int k = 0; k < d.nSensoren && nw < 4; k++) wahl[nw++] = k;
  }
  setText(objects.labelsenstitel, "Räume");
  for (int i = 0; i < 4; i++) {
    lv_obj_t* n = *zSens[i].name; lv_obj_t* w = *zSens[i].wert;
    if (i < nw) {
      const NextDaten::Sensor& s = d.sensoren[wahl[i]];
      const bool gleicherRaum = i > 0 && s.raum[0] && strcmp(d.sensoren[wahl[i - 1]].raum, s.raum) == 0;
      const char* bez = s.raum[0] ? s.raum : s.name;
      if (gleicherRaum) bez = strcmp(s.einheit, "%") == 0 ? "Feuchte" : strcmp(s.einheit, "hPa") == 0 ? "Druck" : strcmp(s.einheit, "°C") == 0 ? "Temperatur" : s.name;
      setText(n, bez);
      if (s.ok) { char e[16]; snprintf(e, sizeof(e), s.einheit[0] ? " %s" : "", s.einheit); zahl(b, sizeof(b), s.wert, 1, e); setText(w, b); }
      else setText(w, "--");
      zeileAnpassen(n, w, s.ok ? b : "--", 150);
      lv_obj_clear_flag(n, LV_OBJ_FLAG_HIDDEN); lv_obj_clear_flag(w, LV_OBJ_FLAG_HIDDEN);
    } else if (i == 0) {
      setText(n, "keine Sensoren"); setText(w, ""); lv_obj_clear_flag(n, LV_OBJ_FLAG_HIDDEN);
    } else { lv_obj_add_flag(n, LV_OBJ_FLAG_HIDDEN); lv_obj_add_flag(w, LV_OBJ_FLAG_HIDDEN); }
  }
  // Biowetter: die Kategorien der Fusion (Kurzuebersicht) fuer den aktuellen Zeitraum (vormittags/nachmittags)
  setText(objects.labelbiotitel, "Biowetter");
  static const char* const KURZ[7] = {"Befinden", "Hypotonie", "Hochdruck", "Rheuma e.", "Rheuma d.", "Asthma", "Wärme"};
  int per = -1;
  if (d.bioOk) {
    struct tm ti; int vorm = getLocalTime(&ti, 0) && ti.tm_hour >= 14 ? 1 : 0;
    if (d.bioVorh[vorm]) per = vorm; else for (int p = 0; p < 5; p++) if (d.bioVorh[p]) { per = p; break; }
  }
  if (per < 0) {
    for (int i = 0; i < 4; i++) {
      lv_obj_t* n = *zBio[i].name; lv_obj_t* w = *zBio[i].wert;
      if (i == 0) { setText(n, "keine Daten"); setText(w, ""); lv_obj_clear_flag(n, LV_OBJ_FLAG_HIDDEN); }
      else { lv_obj_add_flag(n, LV_OBJ_FLAG_HIDDEN); lv_obj_add_flag(w, LV_OBJ_FLAG_HIDDEN); }
    }
  } else {
    for (int i = 0; i < 4; i++) {                         // die Kategorien der Kurzuebersicht der Fusion, in deren Reihenfolge
      lv_obj_t* n = *zBio[i].name; lv_obj_t* w = *zBio[i].wert;
      const int c = d.bioSel[i], v = d.bio[per][c];
      setText(n, KURZ[c]); setText(w, bioText(v));
      lv_obj_set_style_text_color(w, bioFarbe(v), 0);
      lv_obj_clear_flag(n, LV_OBJ_FLAG_HIDDEN); lv_obj_clear_flag(w, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

static void seitePollenVor() {
  char b[24];
  const NextDaten& d = daten;
  setText(objects.labelpollentitel, "Pollen heute");
  // "Meine Allergene" der Fusion zuerst; ist keines gewaehlt, alle acht
  int idx[8], n = 0;
  for (int i = 0; i < 8; i++) if (d.pollen[i].gewaehlt) idx[n++] = i;
  if (n == 0) for (int i = 0; i < 8; i++) idx[n++] = i;
  for (int r = 0; r < 8; r++) {
    lv_obj_t* nm = *zPollen[r].name; lv_obj_t* w = *zPollen[r].wert;
    if (d.pollenOk && r < n) {
      const auto& p = d.pollen[idx[r]];
      const float st = p.heute >= 0 ? p.heute : p.jetzt;
      setText(nm, p.name); setText(w, pollenText(st));
      lv_obj_set_style_text_color(w, pollenFarbe(st), 0);
      lv_obj_clear_flag(nm, LV_OBJ_FLAG_HIDDEN); lv_obj_clear_flag(w, LV_OBJ_FLAG_HIDDEN);
    } else if (r == 0) {
      setText(nm, "keine Daten"); setText(w, ""); lv_obj_clear_flag(nm, LV_OBJ_FLAG_HIDDEN);
    } else { lv_obj_add_flag(nm, LV_OBJ_FLAG_HIDDEN); lv_obj_add_flag(w, LV_OBJ_FLAG_HIDDEN); }
  }
  // Namensnennung der Datenquellen: nur wenn in der Pollenkarte Platz ist (hoechstens 5 Allergene gewaehlt)
  static lv_obj_t* quelle = nullptr;
  if (!quelle) {
    quelle = lv_label_create(objects.screenpollenvor);
    lv_obj_set_pos(quelle, 14, 186);
    lv_obj_set_width(quelle, 146);
    lv_label_set_long_mode(quelle, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(quelle, F16, 0);
    lv_obj_set_style_text_color(quelle, GRAU, 0);
    lv_label_set_text(quelle, "Open-Meteo.com, DWD");
  }
  if (n > 5) lv_obj_add_flag(quelle, LV_OBJ_FLAG_HIDDEN); else lv_obj_clear_flag(quelle, LV_OBJ_FLAG_HIDDEN);
  setText(objects.labelvortitel, "Vorhersage");
  for (int i = 0; i < 4; i++) {
    const bool da = d.vorOk && d.vor[i].zeit[0];
    if (da) {
      setText(*vorZeit[i], d.vor[i].zeit); zahl(b, sizeof(b), d.vor[i].temp, 0, " °C"); setText(*vorTemp[i], b);
      const lv_img_dsc_t* ic = wmoZuImage(d.vor[i].wmo, d.vor[i].tag, true);
      if (ic && lv_img_get_src(*vorBild[i]) != ic) lv_img_set_src(*vorBild[i], ic);
      lv_obj_clear_flag(*vorBild[i], LV_OBJ_FLAG_HIDDEN);
    } else {
      setText(*vorZeit[i], i == 0 ? "keine Daten" : ""); setText(*vorTemp[i], "");
      lv_obj_add_flag(*vorBild[i], LV_OBJ_FLAG_HIDDEN);
    }
  }
}

void uiAktualisieren() {
  if (s_seite < 0) return;
  LvLock lock;
  seiteTempBio();
  seitePollenVor();
}

// ── Seitenwechsel, Hinweise ──────────────────────────────────────────────────
static int naechsteSeite(int ab) {
  for (int i = 1; i <= SEITE_ANZAHL; i++) {
    int s = (ab + i) % SEITE_ANZAHL;
    if (cfg.seiten & (1 << s)) return s;
  }
  return ab < 0 ? SEITE_TEMP_BIO : ab;
}
static void seiteZeigen(int s) {
  s_seite = s; s_tSeite = millis();
  loadScreen(s == SEITE_POLLEN_VOR ? SCREEN_ID_SCREENPOLLENVOR : SCREEN_ID_SCREENTEMPBIO);
}

void uiHauptseite() {
  LvLock lock;
  int s = naechsteSeite(-1);
  if (!(cfg.seiten & (1 << s))) s = SEITE_TEMP_BIO;
  seiteZeigen(s);
  uiAktualisieren();
}

void uiSeiteZeigen(int seite) {
  LvLock lock;
  if (seite < 0) { uiHinweis("Suche die Fusion ...", ""); lv_obj_invalidate(lv_scr_act()); return; }
  seiteZeigen(seite == SEITE_POLLEN_VOR ? SEITE_POLLEN_VOR : SEITE_TEMP_BIO);
  uiAktualisieren();
  lv_obj_invalidate(lv_scr_act());
}

void uiSeiteWeiter() {
  if (s_seite < 0) return;
  LvLock lock;
  seiteZeigen(naechsteSeite(s_seite));
  uiAktualisieren();
}

void uiHinweis(const char* titel, const char* text) {
  LvLock lock;
  char b[160];
  if (text && text[0]) snprintf(b, sizeof(b), "%s\n%s", titel, text); else snprintf(b, sizeof(b), "%s", titel);
  setText(objects.labelstatus, b);
  lv_obj_set_style_text_align(objects.labelstatus, LV_TEXT_ALIGN_CENTER, 0);
  if (s_seite >= 0) { s_seite = -1; loadScreen(SCREEN_ID_SCREENBOOT); }
}

// Code zum Zuruecksetzen des Web-Passworts: Fenster ueber allem, verschwindet nach der Gueltigkeit von selbst
static lv_obj_t* s_codeFenster = nullptr;
static lv_timer_t* s_codeTimer = nullptr;
static uint32_t s_codeBis = 0;

static void codeAufraeumen() {
  if (s_codeTimer) { lv_timer_del(s_codeTimer); s_codeTimer = nullptr; }
  if (s_codeFenster) { lv_obj_del(s_codeFenster); s_codeFenster = nullptr; }
}
void uiResetCodeWeg() { LvLock lock; codeAufraeumen(); }

void uiResetCodeZeigen(const char* code, unsigned sekunden) {
  LvLock lock;
  codeAufraeumen();
  s_codeFenster = lv_obj_create(lv_layer_top());
  lv_obj_set_size(s_codeFenster, LV_HOR_RES, LV_VER_RES);
  lv_obj_set_pos(s_codeFenster, 0, 0);
  lv_obj_set_style_bg_color(s_codeFenster, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(s_codeFenster, LV_OPA_90, 0);
  lv_obj_set_style_border_width(s_codeFenster, 0, 0);
  lv_obj_set_style_radius(s_codeFenster, 0, 0);
  lv_obj_clear_flag(s_codeFenster, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t* t = lv_label_create(s_codeFenster);
  lv_label_set_text(t, "Passwort vergessen");
  lv_obj_set_style_text_font(t, &font_montserrat_16, 0);
  lv_obj_set_style_text_color(t, lv_color_hex(0xf5a623), 0);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 26);
  lv_obj_t* c = lv_label_create(s_codeFenster);
  lv_label_set_text(c, code);
  lv_obj_set_style_text_font(c, &font_montserrat_48, 0);
  lv_obj_set_style_text_color(c, lv_color_hex(0xffffff), 0);
  lv_obj_align(c, LV_ALIGN_CENTER, 0, -4);
  lv_obj_t* h = lv_label_create(s_codeFenster);
  lv_label_set_text(h, "Gib diesen Code im Browser ein.\nEr gilt zwei Minuten.");
  lv_obj_set_style_text_font(h, &font_montserrat_16, 0);
  lv_obj_set_style_text_color(h, lv_color_hex(0xc8cee8), 0);
  lv_obj_set_style_text_align(h, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(h, LV_ALIGN_BOTTOM_MID, 0, -26);
  s_codeBis = lv_tick_get() + sekunden * 1000UL;
  s_codeTimer = lv_timer_create([](lv_timer_t*) { if ((int32_t)(lv_tick_get() - s_codeBis) >= 0) codeAufraeumen(); }, 1000, nullptr);
}

void uiFortschritt(int prozent) {
  LvLock lock;
  if (objects.barwifi) lv_bar_set_value(objects.barwifi, prozent < 0 ? 0 : prozent > 100 ? 100 : prozent, LV_ANIM_OFF);
}

// ── Nachtmodus ───────────────────────────────────────────────────────────────
// Tag: helles Design. Nacht: "Display aus" (Antippen schaltet kurz an) oder "dunkel und gedimmt". Die Quelle der Nacht ist die Fusion
// (nach deren Einstellungen) oder eigene Zeiten.
static bool s_nacht = false, s_dunkel = false;
static int  s_licht = -1;                // zuletzt gesetzte Helligkeit (-1 = noch keine)
static unsigned long s_wachBis = 0;      // bis dahin zeigt ein Antippen in der Nacht das Display

bool uiNachtAktiv() { return s_nacht; }

static bool nachtGilt() {
  if (cfg.nachtQuelle == 1) return daten.nachtOk && daten.nacht;
  if (cfg.nachtQuelle == 2) {
    struct tm ti;
    if (!getLocalTime(&ti, 0) || cfg.nachtVon == cfg.nachtBis) return false;
    const int jetzt = ti.tm_hour * 60 + ti.tm_min;
    return (cfg.nachtVon < cfg.nachtBis) ? (jetzt >= cfg.nachtVon && jetzt < cfg.nachtBis) : (jetzt >= cfg.nachtVon || jetzt < cfg.nachtBis);
  }
  return false;
}

// Farben des hellen (Tag) bzw. dunklen (Nacht) Designs; die Werte mit eigener Farbe (Pollen, Biowetter) setzt der Inhalt
static void designSetzen(bool dunkel) {
  const lv_color_t grund = dunkel ? lv_color_hex(0x000000) : lv_color_hex(0xFFFFFF);
  const lv_color_t kachel = dunkel ? lv_color_hex(0x0B1118) : lv_color_hex(0xE0F7FF);
  const lv_color_t rand = dunkel ? lv_color_hex(0x2A3746) : lv_color_hex(0x7C93A6);
  const lv_color_t text = dunkel ? lv_color_hex(0xA8B0BC) : SCHWARZ;
  const lv_color_t titel = dunkel ? lv_color_hex(0xB5561B) : ORANGE;
  const lv_color_t gruen = dunkel ? lv_color_hex(0x2FBF71) : GRUEN;
  lv_obj_t* screens[2] = {objects.screentempbio, objects.screenpollenvor};
  for (lv_obj_t* s : screens) lv_obj_set_style_bg_color(s, grund, 0);
  lv_obj_t* kacheln[4] = {objects.kachelsensoren, objects.kachelbio, objects.kachelpollen, objects.kachelvor};
  for (lv_obj_t* k : kacheln) { lv_obj_set_style_bg_color(k, kachel, 0); lv_obj_set_style_border_color(k, rand, 0); }
  lv_obj_t* texte[] = {objects.labelwettertext, objects.labeluhr, objects.labelminmax};
  for (lv_obj_t* l : texte) lv_obj_set_style_text_color(l, text, 0);
  for (int i = 0; i < 4; i++) { lv_obj_set_style_text_color(*zSens[i].name, text, 0); lv_obj_set_style_text_color(*zSens[i].wert, text, 0); lv_obj_set_style_text_color(*zBio[i].name, text, 0); }
  for (int i = 0; i < 8; i++) lv_obj_set_style_text_color(*zPollen[i].name, text, 0);
  lv_obj_t* titeln[] = {objects.labelsenstitel, objects.labelbiotitel, objects.labelpollentitel, objects.labelvortitel};
  for (lv_obj_t* l : titeln) lv_obj_set_style_text_color(l, titel, 0);
  lv_obj_set_style_text_color(objects.labeltemp, gruen, 0);
  for (int i = 0; i < 4; i++) { lv_obj_set_style_text_color(*vorZeit[i], titel, 0); lv_obj_set_style_text_color(*vorTemp[i], gruen, 0); }
  s_dunkel = dunkel;
}

static void licht(int prozent) { if (prozent != s_licht) { s_licht = prozent; cydHelligkeit(prozent); } }

// Zustand herstellen (bei jedem Tick und nach einem Antippen)
static void nachtAnwenden() {
  const bool wach = millis() < s_wachBis;
  if (!s_nacht) { if (s_dunkel) { designSetzen(false); uiAktualisieren(); } licht(cfg.helligkeit); return; }
  if (cfg.nachtVerhalten == 1 || wach) {            // dunkel und gedimmt (oder kurz wach)
    if (!s_dunkel) { designSetzen(true); uiAktualisieren(); }
    licht(cfg.nachtHell);
  } else {                                          // Display aus
    licht(0);
  }
}

static void nachtKlick() {
  if (s_nacht && cfg.nachtVerhalten == 0 && millis() >= s_wachBis) {   // aus: das Antippen weckt nur, es blaettert nicht
    s_wachBis = millis() + (unsigned long)cfg.nachtWach * 1000UL;
    nachtAnwenden();
    return;
  }
  uiSeiteWeiter();
}

void uiTick() {
  if (s_seite < 0) return;
  LvLock lock;
  // Uhr
  struct tm ti; char b[16];
  if (getLocalTime(&ti, 0)) snprintf(b, sizeof(b), "%02d:%02d", ti.tm_hour, ti.tm_min); else snprintf(b, sizeof(b), "--:--");
  setText(objects.labeluhr, b);
  // Hinweisleiste
  const char* h = nullptr;
  if (WiFi.status() != WL_CONNECTED) h = "Kein WLAN";
  else if (fusionAdresse.length() == 0 || fusionFehler >= 3) h = "Suche die Fusion...";
  else if (datenStand == 0 || millis() - datenStand > ANZEIGE_ALT_MS) h = "keine aktuellen Daten";
  if (s_nacht) h = nullptr;                          // nachts keine Hinweisleiste (zu hell)
  if (h) { setText(s_hinweisText, h); lv_obj_center(s_hinweisText); lv_obj_clear_flag(s_hinweis, LV_OBJ_FLAG_HIDDEN); }
  else lv_obj_add_flag(s_hinweis, LV_OBJ_FLAG_HIDDEN);
  // Nachtmodus
  const bool nacht = nachtGilt();
  if (nacht != s_nacht) { s_nacht = nacht; s_wachBis = 0; }
  nachtAnwenden();
  const bool aus = s_nacht && s_licht == 0;
  // automatisch weiterblaettern
  if (aus) return;
  if (cfg.wechsel_sek > 0 && millis() - s_tSeite >= (unsigned long)cfg.wechsel_sek * 1000UL) { int n = naechsteSeite(s_seite); if (n != s_seite) { seiteZeigen(n); uiAktualisieren(); } else s_tSeite = millis(); }
}
