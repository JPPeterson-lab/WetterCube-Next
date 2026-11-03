# Changelog

*[Deutsche Version](CHANGELOG.md)* · Module for the [WetterCube Fusion](https://github.com/JPPeterson-lab/WetterCube-Fusion)

## v0.3.0-beta5 (2026-11-07)

### Changed
- **Updates now come from wettercube.de:** The Next checks for new versions and downloads the firmware from our website (`wettercube.de/dl/next`) instead of GitHub. If the website does not answer, it falls back to GitHub. The web installer is now also on `wettercube.de/installer` (same firmware, new design). GitHub remains for the source code and the releases. The update to this version still asks GitHub; after that the device asks wettercube.de.
