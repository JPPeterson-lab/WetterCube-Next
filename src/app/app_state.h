// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jan Althaus (JPPeterson-lab)
#pragma once
// Zustand von WetterCube Next: Einstellungen (NVS), Daten der Fusion, Protokoll
#include "platform/Arduino.h"
#include "platform/WiFi.h"
#include "app_config.h"
#include <lvgl.h>
#include "../board/cyd.h"
#define CYD_BREITE CYD_H_RES

struct Config {
  String ssid, pass;
  String web_pw;                // Passwort der Web-Oberflaeche als "Salz:Hash" (leer = kein Passwort)
  String fusion_host;           // leer = die Fusion per mDNS (_wettercube._tcp) suchen; sonst IP-Adresse oder Name
  int    seiten = 3;            // Bit 0: Temperatur und Biowetter, Bit 1: Allergene und Vorhersage
  int    wechsel_sek = 0;       // automatisch weiterblaettern alle n Sekunden (0 = nur per Antippen)
  int    helligkeit = 80;       // Prozent
  int    panel = 1;             // 0 = ILI9341 mit der Init ILI9341_2 (so lief der CYD im DeskRadar), 1 = ST7789
  bool   drehen = false;        // Anzeige um 180 Grad drehen
  String raeume;                // Sensor-IDs der Fusion fuer die Kachel "Raeume" (Komma-Liste, bis 4); leer = die ersten vier
  int    nachtQuelle = 0;       // Nachtmodus: 0 = aus, 1 = wie die Fusion (nach deren Einstellungen), 2 = eigene Zeiten
  int    nachtVon = 22 * 60, nachtBis = 6 * 60 + 30;   // eigene Zeiten (Minuten seit Mitternacht)
  int    nachtVerhalten = 0;    // 0 = Display aus (Antippen schaltet kurz an), 1 = dunkles Design, gedimmt
  int    nachtHell = 10;        // Helligkeit in der Nacht (Prozent)
  int    nachtWach = 15;        // Sekunden, die Antippen das Display nachts anschaltet
  bool   autoUpdate = false;    // automatisch (alle 6 Stunden) nach Updates suchen; Vorgabe aus, die Suche per Knopf geht immer
  bool   invert = false;        // Farben invertieren (ST7789: aus; die ILI9341_2-Ausfuehrung braucht es)
};
extern Config cfg;
void ladeCfg();
void speichereCfg();

#define SEITE_TEMP_BIO   0
#define SEITE_POLLEN_VOR 1
#define SEITE_ANZAHL     2

// Daten aus /api/next der Fusion (nur, was die Seiten brauchen)
struct NextDaten {
  bool  wetterOk = false;
  float temp = 0, gefuehlt = 0, tmin = 0, tmax = 0, uv = 0;
  int   feuchte = 0, wmo = 0;
  bool  tag = true;
  char  text[28] = "";
  bool  vorOk = false;
  struct { char zeit[8]; float temp; int wmo; bool tag; char text[28]; } vor[4] = {};
  bool  pollenOk = false;
  struct { char name[16]; bool gewaehlt; float jetzt, heute, morgen, uebermorgen; } pollen[8] = {};
  bool  bioOk = false;
  char  bioName[7][20] = {};
  int   bio[5][7] = {};
  int   bioSel[4] = {0, 1, 2, 5};      // Kategorien der Kurzuebersicht laut Einstellung der Fusion
  bool  bioVorh[5] = {};
  struct Sensor { char id[24]; char raum[20]; char name[24]; char einheit[8]; float wert; bool ok; };
  Sensor sensoren[16] = {};
  int   nSensoren = 0;
  char  tz[48] = "";
  bool  nacht = false, nachtOk = false;   // Nachtmodus der Fusion (nachtOk = die Fusion liefert das Feld)
};
extern NextDaten daten;
extern unsigned long datenStand;       // millis() des letzten erfolgreichen Abrufs (0 = noch nie)
extern String fusionAdresse;           // IP, unter der die Fusion gerade erreicht wird
extern int    fusionFehler;            // aufeinanderfolgende Fehlversuche
extern volatile uint8_t wifiLetzterGrund;

// Protokoll (Ringpuffer, /log)
void logDiag(const char* fmt, ...);
String logAlsText();

// Abruf und Suche (daten.cpp)
void datenTick();                      // aus der Hauptschleife: sucht die Fusion, holt Daten
void datenNeuAnfordern();              // Pruefsumme verwerfen (nach geaenderten Einstellungen)
String bloeckeFuerSeiten();            // z. B. "wetter,sensoren,bio" je nach gewaehlten Seiten
// Anzeige (ui.cpp)
void uiInit();
void uiAktualisieren();                // mit den aktuellen Daten neu zeichnen (nimmt den LVGL-Lock selbst)
void uiResetCodeZeigen(const char* code, unsigned sekunden);   // Code zum Zuruecksetzen des Web-Passworts am Display
void uiResetCodeWeg();
extern char portalPasswort[12];            // Passwort des Einrichtungs-WLANs (8 Ziffern, steht am Display)
void portalPasswortNeu();
void uiHinweis(const char* titel, const char* text);   // Vollbild-Hinweis (Portal, Start)
void uiSeiteWeiter();
bool uiNachtAktiv();                   // gilt gerade der Nachtmodus?
void uiHauptseite();
void uiSeiteZeigen(int seite);          // -1 = Startbild, sonst SEITE_TEMP_BIO / SEITE_POLLEN_VOR (fuer Bildschirmfotos)                   // vom Startbild auf die gewaehlte erste Seite wechseln
void uiTick();                         // etwa einmal pro Sekunde: Uhr, Hinweisleiste, automatisches Weiterblaettern
// Update ueber GitHub (ota.cpp)
extern bool   updateVerfuegbar;
extern String updateNeueVersion;
extern String updateLetzterFehler;
extern volatile bool otaLaeuft;
bool versionNeuer(const String& a, const String& b);
void otaTick();                        // aus der Hauptschleife: erste Pruefung nach 2 Minuten, danach alle 6 Stunden
bool otaPruefenJetzt();                // sofort pruefen (true = Antwort erhalten)
bool otaInstallierenStarten();         // startet die Installation in einem eigenen Task (false = laeuft schon / keine Verbindung)
void uiFortschritt(int prozent);       // Fortschrittsbalken auf dem Startbild
// Hilfen (helfer.cpp)
const lv_img_dsc_t* wmoZuImage(int wmo, bool tag, bool klein);
lv_color_t pollenFarbe(float stufe);
const char* pollenText(float stufe);
const char* bioText(int wert);
lv_color_t bioFarbe(int wert);
// Web-Oberflaeche (webui.cpp)
void starteWebUI();
void starteWlanPortal();
extern bool portalModus;
