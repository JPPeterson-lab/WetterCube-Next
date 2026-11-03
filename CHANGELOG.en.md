# Changelog

*[Deutsche Version](CHANGELOG.md)* · Module for the [WetterCube Fusion](https://github.com/JPPeterson-lab/WetterCube-Fusion)

## v0.3.0-beta4 (2026-11-05)

### New
- **Connection to WetterCube Control:** The Next now answers `/api/status` with the values it shows (temperature, feels-like, low/high, humidity, UV, weather) and the state of its connection to the Fusion as JSON. The interface needs no password and cannot change anything, like on the other devices; it is only reachable in your home network via the device's own address. `/ota_check` also answers the question about an update as JSON (only when called explicitly, the device still never asks GitHub on its own). An update that cannot be started now reports error 409 instead of "OK".
