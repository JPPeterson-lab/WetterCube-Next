# Changelog

*[Deutsche Version](CHANGELOG.md)* · Module for the [WetterCube Fusion](https://github.com/JPPeterson-lab/WetterCube-Fusion)

## v0.2.0-alpha3 (2026-10-22)

### Fixed
- **Touch:** On some displays tapping did not respond (the touch chip needed a wake command and uses different pins than expected). Switching pages by tapping now works there too.

### Changed
- **Source credits:** The notes follow Open-Meteo's guidelines and the notes of the German Weather Service: the start screen reads "Weather data by Open-Meteo.com" and "Quelle: Deutscher Wetterdienst", the pollen page "Quelle: Deutscher Wetterdienst". The "Datenquellen" card on the settings page names Open-Meteo, Copernicus (CAMS) and the German Weather Service with the licence (CC BY 4.0) and a note that values are converted. The notes are now elements of the interface (PicoPixel) instead of being created in code.
