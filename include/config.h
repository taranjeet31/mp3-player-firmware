#pragma once

#ifndef AUDIO_MP3_ONLY_DIAGNOSTIC
#define AUDIO_MP3_ONLY_DIAGNOSTIC 0
#endif

#ifndef DISABLE_UI_TASK_DIAGNOSTIC
#define DISABLE_UI_TASK_DIAGNOSTIC 0
#endif

#ifndef DISABLE_TOUCH_DIAGNOSTIC
#define DISABLE_TOUCH_DIAGNOSTIC 0
#endif

#ifndef ENABLE_TOUCH_INPUT
#define ENABLE_TOUCH_INPUT 1
#endif

#ifndef MINIMAL_UI_DIAGNOSTIC
#define MINIMAL_UI_DIAGNOSTIC 0
#endif

#ifndef DISABLE_PLAYER_UI_UPDATE_DIAGNOSTIC
#define DISABLE_PLAYER_UI_UPDATE_DIAGNOSTIC 0
#endif

#ifndef DISABLE_SEEK_DIAGNOSTIC
#define DISABLE_SEEK_DIAGNOSTIC 1
#endif

#ifndef LVGL_SINGLE_BUFFER_DIAGNOSTIC
#define LVGL_SINGLE_BUFFER_DIAGNOSTIC 1
#endif

#if defined(TARGET_THMI)

// Power & Backlight Control
#define PWR_EN_PIN      (10) // High to enable general peripherals / battery power
#define PWR_ON_PIN      (14) // High to latch power-on when running off DC jack
#define BK_LIGHT_PIN    (38) // High to turn LCD backlight on

// Display Pins (ST7789 Parallel 8-bit)
#define LCD_D0_PIN      (48)
#define LCD_D1_PIN      (47)
#define LCD_D2_PIN      (39)
#define LCD_D3_PIN      (40)
#define LCD_D4_PIN      (41)
#define LCD_D5_PIN      (42)
#define LCD_D6_PIN      (45)
#define LCD_D7_PIN      (46)
#define LCD_PCLK_PIN    (8)  // WR
#define LCD_CS_PIN      (6)
#define LCD_DC_PIN      (7)  // RS/DC
#define LCD_RST_PIN     (-1)

// Touch Screen SPI (XPT2046)
#define TOUCH_SCLK_PIN  (1)
#define TOUCH_MISO_PIN  (4)
#define TOUCH_MOSI_PIN  (3)
#define TOUCH_CS_PIN    (2)
#define TOUCH_IRQ_PIN   (9)

// SD Card (SD_MMC 1-bit Mode)
#define SD_SCLK_PIN     (12)
#define SD_MOSI_PIN     (11) // CMD
#define SD_MISO_PIN     (13) // DATA0

// External I2S DAC (PCM5102) Pins
// Connected via external Grove headers:
#define I2S_BCLK_PIN    (17) // Grove Port 2 pin 1
#define I2S_LRC_PIN     (18) // Grove Port 2 pin 2
#define I2S_DOUT_PIN    (44) // Grove Port 1 pin 1 (RX pin)

// Companion I2C connection (Super Mini)
#define COMPANION_I2C_SDA (15) // Grove Port 3 pin 1 (I2C SDA)
#define COMPANION_I2C_SCL (16) // Grove Port 3 pin 2 (I2C SCL)
#define COMPANION_I2C_ADDR (0x42)

#elif defined(TARGET_S3MINI)

// Standard ESP32-S3 Super Mini Configuration
// External I2S DAC (PCM5102) Pins
#define I2S_BCLK_PIN    (4)
#define I2S_LRC_PIN     (5)
#define I2S_DOUT_PIN    (6)

// Onboard / Standalone SD Card (SPI Mode)
#define SD_CS_PIN       (10)
#define SD_MOSI_PIN     (11)
#define SD_MISO_PIN     (13)
#define SD_SCLK_PIN     (12)

// 1.3" I2C OLED display (SSD1306 / SH1106)
#define OLED_I2C_SDA    (8)
#define OLED_I2C_SCL    (9)
#define OLED_I2C_ADDR   (0x3C)

// Rotary Encoder & Tactile buttons
#define ENCODER_A_PIN   (1)
#define ENCODER_B_PIN   (2)
#define BTN_PLAY_PIN    (3)
#define BTN_NEXT_PIN    (7)
#define BTN_PREV_PIN    (14)
#define WS2812_LED_PIN  (21) // Onboard/external WS2812 status LED

#endif
