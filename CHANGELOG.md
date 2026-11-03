# Changelog

*[English version](CHANGELOG.en.md)* · Modul zur [WetterCube Fusion](https://github.com/JPPeterson-lab/WetterCube-Fusion)

## v0.3.0-beta4 (2026-11-05)

### Neu
- **Anbindung an WetterCube Control:** Das Next liefert unter `/api/status` die angezeigten Werte (Temperatur, gefühlt, Tiefst-/Höchstwert, Feuchte, UV, Wetterlage) und den Zustand der Verbindung zur Fusion als JSON. Die Schnittstelle braucht kein Passwort und kann nichts verändern, wie bei den anderen Geräten; sie ist nur im Heimnetz über die eigene Adresse des Geräts erreichbar. Außerdem beantwortet `/ota_check` die Frage nach einem Update als JSON (nur auf ausdrücklichen Aufruf, das Gerät fragt weiter nie von selbst bei GitHub nach). Ein nicht startbares Update meldet jetzt den Fehler 409 statt „OK“.
