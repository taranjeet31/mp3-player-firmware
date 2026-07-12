#if defined(TARGET_THMI)
#include "THMIDisplay.h"
#include "Logging.h"
#include "config.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "init_code.h"
#include "lvgl.h"
#include "soc/soc_memory_types.h"
#include <Arduino.h>
#include <SPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define LCD_PIXEL_CLOCK_HZ (5 * 1000 * 1000)
#define LCD_BK_LIGHT_ON_LEVEL (1)
#define LCD_BK_LIGHT_OFF_LEVEL (0)

#define LCD_H_RES (240)
#define LCD_V_RES (320)
#define LCD_CMD_BITS (8)
#define LCD_PARAM_BITS (8)

// Centralized LVGL buffer constants
static constexpr size_t LVGL_BUFFER_LINES = 32;
static constexpr size_t LVGL_BUFFER_PIXELS = LCD_H_RES * LVGL_BUFFER_LINES;
static constexpr size_t LVGL_BUFFER_BYTES =
    LVGL_BUFFER_PIXELS * sizeof(lv_color_t);

// Task handle registered by UITask for direct notifications
static TaskHandle_t lvglTaskHandle = nullptr;
static volatile bool lcdFaulted = false;

// Heartbeat Telemetry Counters
extern volatile uint32_t lvglHandlerEnterCounter;
extern volatile uint32_t lvglHandlerExitCounter;
extern volatile uint32_t lcdFlushSubmitCounter;
extern volatile uint32_t lcdFlushCompleteCounter;
extern volatile uint32_t touchReadCounter;

extern volatile uint32_t touchReadCount;
extern volatile uint32_t touchPressedCount;
extern volatile uint32_t touchRejectedPressureCount;
extern volatile uint32_t touchRejectedBoundsCount;

// Extra telemetry for validation and safety
extern volatile uint32_t flushRejectedBounds;
extern volatile uint32_t flushRejectedOversize;
extern volatile uint32_t largestFlushPixels;

// Static pointers for pointer validation in flush callback
static lv_color_t *p_buf1 = nullptr;
static lv_color_t *p_buf2 = nullptr;

// Global touch controller instance
XPT2046 touch(SPI, TOUCH_CS_PIN, TOUCH_IRQ_PIN);

THMIDisplay::THMIDisplay()
    : panel_handle(nullptr), io_handle(nullptr), i80_bus(nullptr) {}

void THMIDisplay::setLvglTaskHandle(TaskHandle_t handle) {
  lvglTaskHandle = handle;
}

// ---------------------------------------------------------------------------
// DMA completion callback — runs in ISR context.
// ---------------------------------------------------------------------------
static bool IRAM_ATTR notify_dma_done(esp_lcd_panel_io_handle_t io,
                                      esp_lcd_panel_io_event_data_t *edata,
                                      void *user_ctx) {
  BaseType_t higherPriorityTaskWoken = pdFALSE;

  if (lvglTaskHandle != nullptr) {
    vTaskNotifyGiveFromISR(lvglTaskHandle, &higherPriorityTaskWoken);
  }

  return higherPriorityTaskWoken == pdTRUE;
}

// ---------------------------------------------------------------------------
// LVGL flush callback — runs in UI-task context (Core 1), never in ISR.
// ---------------------------------------------------------------------------
void THMIDisplay::lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area,
                                lv_color_t *color_map) {
  if (drv == nullptr || area == nullptr || color_map == nullptr) {
    if (drv != nullptr) {
      lv_disp_flush_ready(drv);
    }
    return;
  }

  // If the display has faulted, abort immediately to protect the draw buffer.
  if (lcdFaulted) {
    lv_disp_flush_ready(drv);
    return;
  }

  esp_lcd_panel_handle_t panel =
      static_cast<esp_lcd_panel_handle_t>(drv->user_data);

  if (panel == nullptr) {
    lv_disp_flush_ready(drv);
    return;
  }

  // 1. Validate area coordinates and sizes
  int32_t width = area->x2 - area->x1 + 1;
  int32_t height = area->y2 - area->y1 + 1;

  if (area->x1 < 0 || area->x2 >= LCD_H_RES || area->y1 < 0 ||
      area->y2 >= LCD_V_RES || area->x1 > area->x2 || area->y1 > area->y2) {
    flushRejectedBounds++;
    lv_disp_flush_ready(drv);
    return;
  }

  if (width <= 0 || height <= 0) {
    flushRejectedBounds++;
    lv_disp_flush_ready(drv);
    return;
  }

  uint32_t submitted_pixels = (uint32_t)width * (uint32_t)height;
  if (submitted_pixels > LVGL_BUFFER_PIXELS) {
    flushRejectedOversize++;
    lv_disp_flush_ready(drv);
    return;
  }

  // Validate that color_map points to our allocated DMA buffers (full submitted region)
  bool is_valid_ptr = false;
  if (p_buf1 != nullptr && color_map >= p_buf1 &&
      (color_map + submitted_pixels) <= (p_buf1 + LVGL_BUFFER_PIXELS)) {
    is_valid_ptr = true;
  }
#if !defined(LVGL_SINGLE_BUFFER_DIAGNOSTIC) ||                                 \
    (LVGL_SINGLE_BUFFER_DIAGNOSTIC == 0)
  if (p_buf2 != nullptr && color_map >= p_buf2 &&
      (color_map + submitted_pixels) <= (p_buf2 + LVGL_BUFFER_PIXELS)) {
    is_valid_ptr = true;
  }
#endif

  if (!is_valid_ptr) {
    logPrintf("[Display] Rejecting non-buffer color_map pointer: %p (submitted_pixels=%u, buf1=%p-%p, buf2=%p-%p)\n",
              (void *)color_map, submitted_pixels, (void *)p_buf1, (void *)(p_buf1 ? p_buf1 + LVGL_BUFFER_PIXELS : nullptr),
              (void *)p_buf2, (void *)(p_buf2 ? p_buf2 + LVGL_BUFFER_PIXELS : nullptr));
    flushRejectedBounds++;
    lv_disp_flush_ready(drv);
    return;
  }

  if (submitted_pixels > largestFlushPixels) {
    largestFlushPixels = submitted_pixels;
  }

  // Rate-limited log for first 10 flushes
  static uint32_t flushLogCount = 0;
  bool should_log = false;
  if (flushLogCount < 10) {
    flushLogCount++;
    should_log = true;
    logPrintf("[Flush Log %u] area=(%d,%d) to (%d,%d) w=%d h=%d pixels=%u color_map=%p\n",
              (unsigned int)flushLogCount,
              area->x1, area->y1, area->x2, area->y2,
              width, height, submitted_pixels, (void*)color_map);
    logPrintf("[Flush Log %u] buffer start/end: buf1=%p-%p, buf2=%p-%p\n",
              (unsigned int)flushLogCount,
              (void*)p_buf1, (void*)(p_buf1 + LVGL_BUFFER_PIXELS),
              (void*)p_buf2, (void*)(p_buf2 ? p_buf2 + LVGL_BUFFER_PIXELS : nullptr));
  }

  // 2. Clear any stale task notification states.
  ulTaskNotifyTake(pdTRUE, 0);

  // 3. Submit the display frame.
  lcdFlushSubmitCounter++;
  esp_err_t err = esp_lcd_panel_draw_bitmap(
      panel, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_map);

  if (should_log) {
    logPrintf("[Flush Log %u] draw call result=%s\n", (unsigned int)flushLogCount, esp_err_to_name(err));
  }

  if (err != ESP_OK) {
    logPrintf("[Display] LCD bitmap submission failed: %s\n",
              esp_err_to_name(err));
    lv_disp_flush_ready(drv);
    return;
  }

  // 4. Wait for one DMA completion notification from the ISR.
  uint32_t notified = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));

  if (should_log) {
    logPrintf("[Flush Log %u] DMA completion result notified=%u\n", (unsigned int)flushLogCount, notified);
  }

  if (notified == 0) {
    logPrintln("[Display] LCD transfer timeout");
    lcdFaulted = true;
  } else {
    lcdFlushCompleteCounter++;
  }

  // 5. Complete display refresh.
  lv_disp_flush_ready(drv);
}

void THMIDisplay::lvgl_touch_read(lv_indev_drv_t *indev_driver,
                                  lv_indev_data_t *data) {
  static int16_t last_valid_x = 0;
  static int16_t last_valid_y = 0;

  // 1. Set data->state = LV_INDEV_STATE_REL at the start.
  data->state = LV_INDEV_STATE_REL;
  data->point.x = last_valid_x;
  data->point.y = last_valid_y;

#if defined(DISABLE_TOUCH_DIAGNOSTIC) && DISABLE_TOUCH_DIAGNOSTIC
  return;
#endif

  touchReadCounter++;
  touchReadCount++;

  // 1.5. Diagnostic logging (runs every 500ms regardless of touch state)
  static uint32_t lastDiagMs = 0;
  uint32_t nowMs = millis();
  if (nowMs - lastDiagMs >= 500) {
    lastDiagMs = nowMs;
    Serial.printf("[Touch Diag] irq_pin_level=%d\n", digitalRead(TOUCH_IRQ_PIN));
  }

  // 2. Return immediately when no touch is detected.
  if (!touch.pressed()) {
    touchRejectedPressureCount++;
    return;
  }

  // Log raw coordinates immediately upon registered press
  uint16_t rx = touch.RawX();
  uint16_t ry = touch.RawY();
  static uint32_t lastRawLogMs = 0;
  if (nowMs - lastRawLogMs >= 200) {
    lastRawLogMs = nowMs;
    Serial.printf("[Touch Press] raw_x=%u raw_y=%u\n", rx, ry);
  }

  // 4. Reject raw coordinates outside calibration bounds.
  uint16_t min_x = 285;
  uint16_t max_x = 1788;
  uint16_t min_y = 311;
  uint16_t max_y = 1877;

  if (rx < min_x || rx > max_x || ry < min_y || ry > max_y) {
    touchRejectedBoundsCount++;
    return;
  }

  // 5. Clamp final coordinates to 0..239 and 0..319.
  // int16_t tx = touch.X();
  // int16_t ty = touch.Y();

  static constexpr int32_t TOUCH_RAW_X_MIN = 285;
  static constexpr int32_t TOUCH_RAW_X_MAX = 1788;
  static constexpr int32_t TOUCH_RAW_Y_MIN = 311;
  static constexpr int32_t TOUCH_RAW_Y_MAX = 1877;

  int32_t mappedX = map(rx, TOUCH_RAW_X_MIN, TOUCH_RAW_X_MAX, 0, LCD_H_RES - 1);

  int32_t mappedY = map(ry, TOUCH_RAW_Y_MIN, TOUCH_RAW_Y_MAX, 0, LCD_V_RES - 1);

  int16_t tx = constrain(mappedX, 0, LCD_H_RES - 1);
  int16_t ty = constrain(mappedY, 0, LCD_V_RES - 1);

  if (tx < 0)
    tx = 0;
  if (tx > 239)
    tx = 239;
  if (ty < 0)
    ty = 0;
  if (ty > 319)
    ty = 319;

  // Preserve the last valid coordinates for release reporting.
  last_valid_x = tx;
  last_valid_y = ty;

  // 6. Do not report pressed using stale coordinates.
  data->point.x = tx;
  data->point.y = ty;

  static uint32_t lastTouchLogMs = 0;
uint32_t now = millis();

if (now - lastTouchLogMs >= 200) {
    lastTouchLogMs = now;

    logPrintf(
        "[Touch] raw=%u,%u mapped=%d,%d irq=%d\n",
        static_cast<unsigned>(rx),
        static_cast<unsigned>(ry),
        static_cast<int>(tx),
        static_cast<int>(ty),
        digitalRead(TOUCH_IRQ_PIN)
    );
}





  data->state = LV_INDEV_STATE_PR;

  touchPressedCount++;
}

bool THMIDisplay::init() {
  Serial.println("[Display] Initializing Power and Backlight...");

  // Enable peripheral/board power (crucial for battery and general boot)
  pinMode(PWR_EN_PIN, OUTPUT);
  digitalWrite(PWR_EN_PIN, HIGH);

  pinMode(PWR_ON_PIN, OUTPUT);
  digitalWrite(PWR_ON_PIN, HIGH);

  pinMode(BK_LIGHT_PIN, OUTPUT);
  digitalWrite(BK_LIGHT_PIN, LCD_BK_LIGHT_ON_LEVEL);

  delay(200);

  Serial.printf("[Display] LVGL buffer: %u pixels, %u bytes\n",
                (unsigned int)LVGL_BUFFER_PIXELS,
                (unsigned int)LVGL_BUFFER_BYTES);
  Serial.printf("[Display] I80 max transfer: %u bytes\n",
                (unsigned int)(LVGL_BUFFER_BYTES + 64));

  Serial.println("[Display] Initializing Intel 8080 LCD Bus...");
  esp_lcd_i80_bus_config_t bus_config = {.dc_gpio_num = LCD_DC_PIN,
                                         .wr_gpio_num = LCD_PCLK_PIN,
                                         .data_gpio_nums =
                                             {
                                                 LCD_D0_PIN,
                                                 LCD_D1_PIN,
                                                 LCD_D2_PIN,
                                                 LCD_D3_PIN,
                                                 LCD_D4_PIN,
                                                 LCD_D5_PIN,
                                                 LCD_D6_PIN,
                                                 LCD_D7_PIN,
                                             },
                                         .bus_width = 8,
                                         .max_transfer_bytes =
                                             LVGL_BUFFER_BYTES + 64};
  esp_err_t err = esp_lcd_new_i80_bus(&bus_config, &i80_bus);
  if (err != ESP_OK) {
    Serial.printf("[Display] Failed to create I80 bus: %s\n",
                  esp_err_to_name(err));
    return false;
  }
  Serial.println("[Display] I80 bus created");

  esp_lcd_panel_io_i80_config_t io_config = {
      .cs_gpio_num = LCD_CS_PIN,
      .pclk_hz = LCD_PIXEL_CLOCK_HZ,
      .trans_queue_depth = 1,
      .user_ctx = nullptr,
      .lcd_cmd_bits = LCD_CMD_BITS,
      .lcd_param_bits = LCD_PARAM_BITS,
      .dc_levels =
          {
              .dc_idle_level = 0,
              .dc_cmd_level = 0,
              .dc_dummy_level = 0,
              .dc_data_level = 1,
          },
      .flags =
          {
              .swap_color_bytes = 1,
          },
  };
  err = esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle);
  if (err != ESP_OK) {
    Serial.printf("[Display] Failed to create IO handle: %s\n",
                  esp_err_to_name(err));
    return false;
  }
  Serial.println("[Display] Panel IO created");

  Serial.println("[Display] Installing ST7789 display driver...");
  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = LCD_RST_PIN,
      .color_space = ESP_LCD_COLOR_SPACE_RGB,
      .bits_per_pixel = 16,
  };
  err = esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle);
  if (err != ESP_OK) {
    Serial.printf("[Display] Failed to create panel handle: %s\n",
                  esp_err_to_name(err));
    return false;
  }
  Serial.println("[Display] Panel created");

  // Send LilyGO custom ST7789 register initialization sequence (Approach B).
  Serial.println("[Display] Running LilyGO ST7789 custom initialization...");
  for (size_t i = 0; i < sizeof(st_init_cmds) / sizeof(lcd_init_cmd_t); i++) {
    err = esp_lcd_panel_io_tx_param(io_handle, st_init_cmds[i].cmd,
                                    st_init_cmds[i].data, st_init_cmds[i].len);
    if (err != ESP_OK) {
      Serial.printf("[Display] Init command 0x%02X failed: %s\n",
                    st_init_cmds[i].cmd, esp_err_to_name(err));
      return false;
    }
    if (st_init_cmds[i].delay > 0) {
      delay(st_init_cmds[i].delay);
    }
  }
  Serial.println("[Display] Panel initialized");

  Serial.println("[Display] Initializing LVGL Library...");
  lv_init();
  Serial.println("[Display] LVGL initialized");

  // 1. Allocate partial draw buffers explicitly from internal SRAM with DMA
  // capability
  lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(
      LVGL_BUFFER_BYTES, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  if (!buf1 || !esp_ptr_dma_capable(buf1)) {
    Serial.println(
        "[Display] Error: buf1 allocation failed or not DMA capable!");
    return false;
  }
  p_buf1 = buf1;

#if defined(LVGL_SINGLE_BUFFER_DIAGNOSTIC) && LVGL_SINGLE_BUFFER_DIAGNOSTIC
  lv_color_t *buf2 = nullptr;
  p_buf2 = nullptr;
#else
  lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(
      LVGL_BUFFER_BYTES, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  if (!buf2 || !esp_ptr_dma_capable(buf2)) {
    Serial.println(
        "[Display] Error: buf2 allocation failed or not DMA capable!");
    return false;
  }
  p_buf2 = buf2;
#endif

  Serial.println("[Display] Buffer allocated");

  static lv_disp_draw_buf_t disp_buf;
  lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LVGL_BUFFER_PIXELS);

  // 2. Setup static variables/faulted states.
  lvglTaskHandle = nullptr;
  lcdFaulted = false;

  // 3. Register the LCD DMA completion callback before registering display
  // driver.
  esp_lcd_panel_io_callbacks_t ioCallbacks = {.on_color_trans_done =
                                                  notify_dma_done};
  err = esp_lcd_panel_io_register_event_callbacks(io_handle, &ioCallbacks,
                                                  nullptr);
  if (err != ESP_OK) {
    Serial.printf("[Display] Failed to register LCD DMA callback: %s\n",
                  esp_err_to_name(err));
    return false;
  }
  Serial.println("[Display] LCD DMA completion callback registered.");

  // 4. Initialize and register the LVGL display driver.
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = LCD_H_RES;
  disp_drv.ver_res = LCD_V_RES;
  disp_drv.flush_cb = lvgl_flush_cb;
  disp_drv.draw_buf = &disp_buf;
  disp_drv.user_data = panel_handle;
  lv_disp_drv_register(&disp_drv);
  Serial.println("[Display] LVGL display registered");

  // 5. Initialize Touchscreen or keep it disabled based on config.
#if defined(ENABLE_TOUCH_INPUT) && ENABLE_TOUCH_INPUT
  Serial.println("[Display] Touch init start");

  // Ensure all SPI devices are deselected before starting the bus.
  pinMode(TOUCH_CS_PIN, OUTPUT);
  digitalWrite(TOUCH_CS_PIN, HIGH);

  pinMode(TOUCH_IRQ_PIN, INPUT_PULLUP);

  // Pass -1 to SPI.begin to keep CS pin under standard GPIO control.
  SPI.begin(TOUCH_SCLK_PIN, TOUCH_MISO_PIN, TOUCH_MOSI_PIN, -1);

  touch.begin(LCD_H_RES, LCD_V_RES);
  touch.setCal(1788, 285, 1877, 311, LCD_H_RES, LCD_V_RES);
  touch.setRotation(0);

  Serial.println("[Display] Touch init complete");

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = lvgl_touch_read;
  lv_indev_drv_register(&indev_drv);
  Serial.println("[Display] LVGL input registered");
#else
  Serial.println("[Display] Touch init disabled");
  Serial.println("[Display] LVGL input disabled");
#endif

  Serial.println("[Display] LVGL Initialization complete.");
  return true;
}

void THMIDisplay::update() {
  lvglHandlerEnterCounter++;
  lv_timer_handler();
  lvglHandlerExitCounter++;
}

#endif
