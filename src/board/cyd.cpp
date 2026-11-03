// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jan Althaus (JPPeterson-lab)
#include "cyd.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9341.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

extern "C++" void logDiag(const char* fmt, ...);   // app_state.cpp (Eintrag im /log)
static const char* TAG = "cyd";
#define PIN_MOSI 13
#define PIN_MISO 12
#define PIN_SCLK 14
#define PIN_CS   15
#define PIN_DC    2
#define PIN_BL   21
#define PIN_T_IRQ 36
#define PIN_T_CLK 25
#define PIN_T_MOSI 32
#define PIN_T_MISO 39
#define PIN_T_CS 33

static void hintergrundInit() {
  ledc_timer_config_t t = {};
  t.speed_mode = LEDC_LOW_SPEED_MODE; t.duty_resolution = LEDC_TIMER_10_BIT; t.timer_num = LEDC_TIMER_1; t.freq_hz = 5000; t.clk_cfg = LEDC_AUTO_CLK;
  ledc_timer_config(&t);
  ledc_channel_config_t c = {};
  c.gpio_num = PIN_BL; c.speed_mode = LEDC_LOW_SPEED_MODE; c.channel = LEDC_CHANNEL_1; c.timer_sel = LEDC_TIMER_1; c.duty = 0; c.hpoint = 0;
  ledc_channel_config(&c);
}

void cydHelligkeit(int prozent) {
  if (prozent > 100) prozent = 100;
  if (prozent > 0 && prozent < 3) prozent = 3;      // 0 = Hintergrundlicht aus
  ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, (1023u * (uint32_t)prozent) / 100u);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

// Touch-Controller XPT2046 (eigener Bus, hier von Hand getaktet): Nach dem Einschalten meldet bei manchen CYD-Ausfuehrungen der Interrupt-Pin
// erst dann Beruehrungen, wenn der Controller einmal einen Befehl bekommen hat (Strom-Aus-Modus mit aktivem Pen-Interrupt). Deshalb wird
// beim Start eine Messung angestossen. Als zweite Quelle dient der Druck (Z), falls die Ruhewerte plausibel sind.
static bool s_zOk = false;
static uint16_t xptLesen(uint8_t befehl) {
  gpio_set_level((gpio_num_t)PIN_T_CS, 0); esp_rom_delay_us(2);
  for (int i = 7; i >= 0; i--) { gpio_set_level((gpio_num_t)PIN_T_MOSI, (befehl >> i) & 1); gpio_set_level((gpio_num_t)PIN_T_CLK, 1); esp_rom_delay_us(2); gpio_set_level((gpio_num_t)PIN_T_CLK, 0); esp_rom_delay_us(2); }
  uint16_t v = 0;
  for (int i = 0; i < 16; i++) { gpio_set_level((gpio_num_t)PIN_T_CLK, 1); esp_rom_delay_us(2); gpio_set_level((gpio_num_t)PIN_T_CLK, 0); esp_rom_delay_us(2); v = (v << 1) | gpio_get_level((gpio_num_t)PIN_T_MISO); }
  gpio_set_level((gpio_num_t)PIN_T_CS, 1);
  return (v >> 3) & 0x0FFF;
}
static void touchInit() {
  gpio_config_t o = {}; o.pin_bit_mask = (1ULL << PIN_T_CLK) | (1ULL << PIN_T_MOSI) | (1ULL << PIN_T_CS); o.mode = GPIO_MODE_OUTPUT; gpio_config(&o);
  gpio_config_t i = {}; i.pin_bit_mask = (1ULL << PIN_T_MISO) | (1ULL << PIN_T_IRQ); i.mode = GPIO_MODE_INPUT; gpio_config(&i);
  gpio_set_level((gpio_num_t)PIN_T_CS, 1); gpio_set_level((gpio_num_t)PIN_T_CLK, 0);
  xptLesen(0x90);                                             // Messung anstossen: danach ist der Pen-Interrupt aktiv
  const uint16_t z1 = xptLesen(0xB0), z2 = xptLesen(0xC0);
  s_zOk = z1 < 200 && z2 > 3800;                              // Ruhewerte (kein Finger): z1 nahe 0, z2 nahe 4095
  ESP_LOGI(TAG, "Touch XPT2046: Ruhewerte z1=%u z2=%u, Druckmessung %s", z1, z2, s_zOk ? "an" : "aus");
}

bool cydBerueht() {
  static bool bereit = false, war = false; static int zaehler = 0;
  if (!bereit) { touchInit(); bereit = true; }
  const int irq = gpio_get_level((gpio_num_t)PIN_T_IRQ);        // der XPT2046 zieht den Pin bei Beruehrung nach unten
  int z = 0;
  bool jetzt = irq == 0;
  if (!jetzt && s_zOk) { z = (int)xptLesen(0xB0) + 4095 - (int)xptLesen(0xC0); jetzt = z > 900; }
  if (jetzt != war && zaehler < 40) { zaehler++; ESP_LOGI(TAG, "Touch %s (Pen-Interrupt %d, Druck %d)", jetzt ? "gedrueckt" : "losgelassen", irq, z); logDiag("[Touch] %s (Pen-Interrupt %d, Druck %d)", jetzt ? "gedrueckt" : "losgelassen", irq, z); }
  war = jetzt;
  return jetzt;
}

// Initialisierung "ILI9341_2" aus TFT_eSPI: so laeuft der CYD (ESP32-2432S028R) im Projekt DeskRadar Pro (BGR, Inversion an, Rotation 1).
// MADCTL und Pixelformat setzt der Treiber selbst (aus rgb_ele_order und bits_per_pixel), Sleep-Out und Display-An ebenso.
static const ili9341_lcd_init_cmd_t INIT_ILI9341_2[] = {
  {0xCF, (uint8_t[]){0x00, 0xC1, 0x30}, 3, 0},
  {0xED, (uint8_t[]){0x64, 0x03, 0x12, 0x81}, 4, 0},
  {0xE8, (uint8_t[]){0x85, 0x00, 0x78}, 3, 0},
  {0xCB, (uint8_t[]){0x39, 0x2C, 0x00, 0x34, 0x02}, 5, 0},
  {0xF7, (uint8_t[]){0x20}, 1, 0},
  {0xEA, (uint8_t[]){0x00, 0x00}, 2, 0},
  {0xC0, (uint8_t[]){0x10}, 1, 0},
  {0xC1, (uint8_t[]){0x00}, 1, 0},
  {0xC5, (uint8_t[]){0x30, 0x30}, 2, 0},
  {0xC7, (uint8_t[]){0xB7}, 1, 0},
  {0xB1, (uint8_t[]){0x00, 0x1A}, 2, 0},
  {0xB6, (uint8_t[]){0x08, 0x82, 0x27}, 3, 0},
  {0xF2, (uint8_t[]){0x00}, 1, 0},
  {0x26, (uint8_t[]){0x01}, 1, 0},
  {0xE0, (uint8_t[]){0x0F, 0x2A, 0x28, 0x08, 0x0E, 0x08, 0x54, 0xA9, 0x43, 0x0A, 0x0F, 0x00, 0x00, 0x00, 0x00}, 15, 0},
  {0xE1, (uint8_t[]){0x00, 0x15, 0x17, 0x07, 0x11, 0x06, 0x2B, 0x56, 0x3C, 0x05, 0x10, 0x0F, 0x3F, 0x3F, 0x0F}, 15, 0},
  {0x2B, (uint8_t[]){0x00, 0x00, 0x01, 0x3F}, 4, 0},
  {0x2A, (uint8_t[]){0x00, 0x00, 0x00, 0xEF}, 4, 0},
};

esp_lcd_panel_handle_t cydPanelInit(int panel, bool drehen, bool invert, esp_lcd_panel_io_color_trans_done_cb_t farbeFertig, void* benutzer) {
  hintergrundInit();
  spi_bus_config_t b = {};
  b.mosi_io_num = PIN_MOSI; b.miso_io_num = PIN_MISO; b.sclk_io_num = PIN_SCLK; b.quadwp_io_num = -1; b.quadhd_io_num = -1;
  b.max_transfer_sz = CYD_H_RES * 40 * 2 + 16;
  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &b, SPI_DMA_CH_AUTO));
  esp_lcd_panel_io_handle_t io = nullptr;
  esp_lcd_panel_io_spi_config_t ic = {};
  ic.dc_gpio_num = PIN_DC; ic.cs_gpio_num = PIN_CS; ic.pclk_hz = 27 * 1000 * 1000; ic.lcd_cmd_bits = 8; ic.lcd_param_bits = 8;
  ic.spi_mode = 0; ic.trans_queue_depth = 10; ic.on_color_trans_done = farbeFertig; ic.user_ctx = benutzer;
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &ic, &io));
  esp_lcd_panel_dev_config_t pc = {};
  pc.reset_gpio_num = -1; pc.bits_per_pixel = 16;
  pc.rgb_ele_order = panel == 1 ? LCD_RGB_ELEMENT_ORDER_RGB : LCD_RGB_ELEMENT_ORDER_BGR;   // ST7789 des CYD: RGB (wie in den frueheren LovyanGFX-Projekten), ILI9341: BGR
  ili9341_vendor_config_t vc = {};
  vc.init_cmds = INIT_ILI9341_2; vc.init_cmds_size = sizeof(INIT_ILI9341_2) / sizeof(INIT_ILI9341_2[0]);
  if (panel != 1) pc.vendor_config = &vc;
  esp_lcd_panel_handle_t p = nullptr;
  if (panel == 1) ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io, &pc, &p));
  else            ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io, &pc, &p));
  esp_lcd_panel_reset(p);
  esp_lcd_panel_init(p);
  esp_lcd_panel_invert_color(p, invert);
  esp_lcd_panel_swap_xy(p, true);                         // Querformat 320 x 240
  if (panel == 1) esp_lcd_panel_mirror(p, !drehen, drehen);   // ST7789
  else            esp_lcd_panel_mirror(p, drehen, drehen);    // ILI9341: wie TFT_eSPI Rotation 1 (nur MV); drehen = Rotation 3 (180 Grad)
  esp_lcd_panel_disp_on_off(p, true);
  ESP_LOGI(TAG, "Anzeige %s bereit", panel == 1 ? "ST7789" : "ILI9341");
  return p;
}
