// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jan Althaus (JPPeterson-lab)
#include "lvgl_port.h"
#include "../board/cyd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char* TAG = "lv_port";
static SemaphoreHandle_t s_mux = nullptr;
static esp_lcd_panel_handle_t s_panel = nullptr;
static lv_disp_draw_buf_t s_buf;
static lv_disp_drv_t s_drv;
#define PUFFER_ZEILEN 24

static bool farbeFertig(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t*, void* ctx) {
  lv_disp_flush_ready((lv_disp_drv_t*)ctx);
  return false;
}

// Bildschirmfoto: Streifen werden beim Zeichnen in einen RGB888-Puffer gewandelt und an den Abholer uebergeben
static volatile bool s_capAktiv = false;
static SemaphoreHandle_t s_capDa = nullptr, s_capFrei = nullptr;
static uint8_t* s_capPuffer = nullptr;
static volatile int s_capZeilen = 0, s_capSumme = 0, s_capNaechsteY = 0;
#define CAP_MAX_ZEILEN PUFFER_ZEILEN

static void capStreifen(const lv_area_t* a, const lv_color_t* farben) {
  const int w = a->x2 - a->x1 + 1, z = a->y2 - a->y1 + 1;
  if (w != CYD_H_RES || z > CAP_MAX_ZEILEN || !s_capPuffer || a->y1 != s_capNaechsteY) return;   // nur ganze Zeilen, lueckenlos von oben nach unten
  s_capNaechsteY += z;
  const uint8_t* q = (const uint8_t*)farben;
  uint8_t* o = s_capPuffer;
  for (int i = 0; i < w * z; i++) {                                         // LV_COLOR_16_SWAP: hoeheres Byte zuerst
    const uint8_t hi = q[2 * i], lo = q[2 * i + 1];
    const uint8_t r5 = hi >> 3, g6 = ((hi & 7) << 3) | (lo >> 5), b5 = lo & 0x1F;
    *o++ = (b5 << 3) | (b5 >> 2); *o++ = (g6 << 2) | (g6 >> 4); *o++ = (r5 << 3) | (r5 >> 2);   // BMP: Blau, Gruen, Rot
  }
  s_capZeilen = z;
  xSemaphoreGive(s_capDa);
  xSemaphoreTake(s_capFrei, pdMS_TO_TICKS(3000));                           // warten, bis der Streifen abgeholt ist
}

bool lvglPortCaptureStart() {
  if (s_capAktiv) return false;
  if (!s_capDa) { s_capDa = xSemaphoreCreateBinary(); s_capFrei = xSemaphoreCreateBinary(); }
  while (xSemaphoreTake(s_capDa, 0) == pdTRUE) {}
  while (xSemaphoreTake(s_capFrei, 0) == pdTRUE) {}
  s_capPuffer = (uint8_t*)heap_caps_malloc((size_t)CYD_H_RES * CAP_MAX_ZEILEN * 3, MALLOC_CAP_8BIT);
  if (!s_capPuffer) return false;
  s_capSumme = 0; s_capZeilen = 0; s_capNaechsteY = 0; s_capAktiv = true;
  return true;
}

bool lvglPortCaptureHole(const uint8_t** daten, int* zeilen, int timeout_ms) {
  if (s_capSumme > 0 && s_capSumme <= CYD_V_RES) xSemaphoreGive(s_capFrei);   // den vorigen Streifen freigeben
  if (s_capSumme >= CYD_V_RES) return false;
  if (xSemaphoreTake(s_capDa, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) return false;
  *daten = s_capPuffer; *zeilen = s_capZeilen; s_capSumme += s_capZeilen;
  return true;
}

void lvglPortCaptureEnde() {
  s_capAktiv = false;
  if (s_capFrei) xSemaphoreGive(s_capFrei);
  vTaskDelay(pdMS_TO_TICKS(50));
  uint8_t* p = s_capPuffer; s_capPuffer = nullptr;
  if (p) heap_caps_free(p);
}

static void flush_cb(lv_disp_drv_t* drv, const lv_area_t* a, lv_color_t* farben) {
  if (s_capAktiv) capStreifen(a, farben);
  esp_lcd_panel_draw_bitmap(s_panel, a->x1, a->y1, a->x2 + 1, a->y2 + 1, farben);
}

static void touch_cb(lv_indev_drv_t*, lv_indev_data_t* d) {
  d->point.x = CYD_H_RES / 2; d->point.y = CYD_V_RES / 2;       // nur Tippen ist wichtig: die ganze Anzeige ist eine Flaeche
  d->state = cydBerueht() ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void lvTask(void*) {
  for (;;) {
    uint32_t ms = 10;
    if (lvglPortLock(-1)) { ms = lv_timer_handler(); lvglPortUnlock(); }
    if (ms > 100) ms = 100;
    if (ms < 5) ms = 5;
    vTaskDelay(pdMS_TO_TICKS(ms));
  }
}

esp_err_t lvglPortInit(int panel, bool drehen, bool invert) {
  lv_init();
  s_mux = xSemaphoreCreateRecursiveMutex();
  lv_disp_drv_init(&s_drv);
  s_panel = cydPanelInit(panel, drehen, invert, farbeFertig, &s_drv);
  const size_t px = (size_t)CYD_H_RES * PUFFER_ZEILEN;
  void* buf = heap_caps_malloc(px * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  if (!buf) { ESP_LOGE(TAG, "kein Speicher fuer den Zeichenpuffer"); return ESP_ERR_NO_MEM; }
  lv_disp_draw_buf_init(&s_buf, buf, nullptr, px);
  s_drv.hor_res = CYD_H_RES; s_drv.ver_res = CYD_V_RES; s_drv.flush_cb = flush_cb; s_drv.draw_buf = &s_buf;
  lv_disp_drv_register(&s_drv);
  static lv_indev_drv_t in;
  lv_indev_drv_init(&in); in.type = LV_INDEV_TYPE_POINTER; in.read_cb = touch_cb; lv_indev_drv_register(&in);
  xTaskCreatePinnedToCore(lvTask, "lvgl", 6144, nullptr, 2, nullptr, 1);
  return ESP_OK;
}

bool lvglPortLock(int timeout_ms) { return xSemaphoreTakeRecursive(s_mux, timeout_ms < 0 ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms)) == pdTRUE; }
void lvglPortUnlock(void) { xSemaphoreGiveRecursive(s_mux); }
