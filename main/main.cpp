// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jan Althaus (JPPeterson-lab)
// WetterCube Next - Einstieg (reines ESP-IDF): Einstellungsspeicher bereitmachen, dann App starten
#include "src/app/app_state.h"
#include "nvs_flash.h"

extern void appStart();
extern void appLoop();

extern "C" void app_main(void) {
  esp_err_t e = nvs_flash_init();
  if (e == ESP_ERR_NVS_NO_FREE_PAGES || e == ESP_ERR_NVS_NEW_VERSION_FOUND) { nvs_flash_erase(); nvs_flash_init(); }
  appStart();
  for (;;) appLoop();
}
