# Changelog

*[Deutsche Version](CHANGELOG.md)* · Module for the [WetterCube Fusion](https://github.com/JPPeterson-lab/WetterCube-Next)

## v0.2.0-alpha1 (2026-10-21)

### New
- **Several Next displays:** Every Next is now called `wettercube-next-<end of MAC>` on the network and reports to the Fusion with its ID, version and settings (from v0.9.9-beta1). The Fusion shows one tab per Next at `http://wettercube-next.local`. With "Vom Fusion verwalten" (managed by the Fusion) the pages, rooms, brightness and night mode set there apply; the Next fetches and stores them. The Next's own page then shows a note and only Fusion address, display, update and Wi-Fi.
