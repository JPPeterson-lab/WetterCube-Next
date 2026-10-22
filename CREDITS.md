# Credits

| Bestandteil | Verwendung | Lizenz |
|---|---|---|
| [ESP-IDF](https://github.com/espressif/esp-idf) (Espressif) | Grundlage der Firmware | Apache-2.0 |
| [LVGL 8](https://lvgl.io) | Oberfläche | MIT |
| [ArduinoJson](https://arduinojson.org) | Daten der Fusion lesen | MIT |
| [esp_lcd_ili9341](https://components.espressif.com/components/espressif/esp_lcd_ili9341) (Espressif) | Display-Treiber | Apache-2.0 |
| [mDNS](https://components.espressif.com/components/espressif/mdns) (Espressif) | Fusion im Netz finden | Apache-2.0 |
| [Meteocons](https://github.com/basmilius/weather-icons) (Bas Milius) | Wettersymbole (Stil „flat“, mit Kontur) | MIT |
| [Montserrat](https://github.com/JulietaUla/Montserrat) (Julieta Ulanovsky) | Schrift | SIL OFL 1.1 |
| [PicoPixel](https://picopixel.io) | Entwurf der Oberfläche | siehe Anbieter |

Die Wetterdaten kommen vom Fusion; Einzelheiten und Lizenzen stehen im Abschnitt „Wetterdaten“ unten. Wetterdaten: „Weather data by [Open-Meteo.com](https://open-meteo.com)“ ([CC BY 4.0](https://open-meteo.com/en/licence); die kostenlose Schnittstelle ist **nur für nicht-kommerzielle Nutzung**, wer kommerziell einsetzt, braucht dort eine eigene Lizenz), im WetterCube teils umgerechnet (zum Beispiel zu Pollenstufen). Pollen und Luftqualität: „Contains modified Copernicus Atmosphere Monitoring Service information“, über Open-Meteo. Ortssuche: „Location data by [GeoNames](https://www.geonames.org) via Open-Meteo“. Warnungen, Pollenflug, Biowetter und Radar: [DWD Opendata](https://opendata.dwd.de) (CC BY 4.0, „Quelle: Deutscher Wetterdienst“).

Autor: Jan Althaus (JPPeterson-lab).

---

## Projektlizenz

Der Quellcode dieses Projekts steht unter der **Apache License 2.0** – siehe [LICENSE](LICENSE) und [NOTICE](NOTICE). Texte, Bilder und Dokumentation stehen unter **CC BY-NC 4.0** ([LICENSE-Dokumentation.txt](LICENSE-Dokumentation.txt)).

Die oben genannten Drittanbieter-Ressourcen unterliegen ihren jeweiligen Lizenzen (Meteocons: MIT, Icons8: MIT oder Good Boy License, Montserrat: SIL OFL 1.1, Bibliotheken siehe oben) und sind von der Lizenz dieses Projekts nicht erfasst.

---

## Wetterdaten

**Quelle:** [Open-Meteo](https://open-meteo.com) – Wetter, Pollen, Luftqualität, Standortsuche

„Weather data by [Open-Meteo.com](https://open-meteo.com)“. Lizenz und Quellen der Daten: [open-meteo.com/en/licence](https://open-meteo.com/en/licence). Die Werte werden im WetterCube umgerechnet und zusammengefasst (zum Beispiel Pollenstufen und Tageshöchstwerte). Orte werden über die Geocoding-Schnittstelle gesucht, deren Daten von [GeoNames](https://www.geonames.org) stammen (CC BY 4.0).

**Lizenz der Daten:** [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) (Namensnennung, Open-Meteo.com). Die kostenlose API ist nur für **nicht-kommerzielle** Nutzung vorgesehen ([Nutzungsbedingungen](https://open-meteo.com/en/terms)), ein API-Key ist nicht erforderlich. Die Vorhersagen beruhen auf Modellen nationaler Wetterdienste wie DWD, NOAA, ECMWF und Météo-France (Einzelheiten: [open-meteo.com/en/docs](https://open-meteo.com/en/docs)).

**Pollen und Luftqualität:** Daten des **Copernicus Atmosphere Monitoring Service (CAMS)** des ECMWF, bereitgestellt über Open-Meteo. „Contains modified Copernicus Atmosphere Monitoring Service information.“ Weder die Europäische Kommission noch das ECMWF haften für die Verwendung der Daten.

---

## DWD-Daten

**Quelle:** [Deutscher Wetterdienst (DWD) – Opendata](https://opendata.dwd.de)

Wetterwarnungen, Pollenflug-Vorhersagen, Biowetter und Warnkarte stammen vom DWD (frei zugängliche Geodaten). Sie dürfen nach den
[Rechtlichen Hinweisen des DWD](https://www.dwd.de/DE/service/rechtliche_hinweise/rechtliche_hinweise_node.html) unter der Lizenz
[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) mit Quellenvermerk weiterverwendet werden. Der Vermerk lautet
**„Quelle: Deutscher Wetterdienst“** und steht unmittelbar an den angezeigten DWD-Informationen; bei weitergehend bearbeiteten Daten
nennt der DWD Hinweise wie „Datenbasis: Deutscher Wetterdienst, …“ in zentralen Quellenverzeichnissen.

---
