# ESP32-S3 Hi-Fi MP3 Player

![C++](https://img.shields.io/badge/Language-C++-blue.svg)
![Platform](https://img.shields.io/badge/Platform-ESP32--S3-orange.svg)
![RTOS](https://img.shields.io/badge/OS-FreeRTOS-green.svg)
![UI](https://img.shields.io/badge/UI-LVGL%20%7C%20U8g2-lightgrey.svg)

A high-performance, dual-core MP3 player firmware designed for ESP32-S3 microcontrollers. This project features a robust Hardware Abstraction Layer (HAL), asynchronous I2S audio decoding, and a thread-safe FreeRTOS architecture to ensure skip-free playback and responsive user interfaces. 

It currently supports two distinct hardware configurations: a rich TFT Touch interface using LVGL (LilyGO T-HMI) and a minimal OLED interface using U8g2 (ESP32-S3 Super Mini).

## 🌟 Features

* **Dual-Core Architecture:** Audio decoding (Core 0) and UI rendering (Core 1) are strictly separated using FreeRTOS tasks to guarantee uninterrupted playback.
* **Hardware Abstraction Layer (HAL):** Clean object-oriented HAL making it incredibly easy to port to new boards, displays, or audio DACs.
* **Versatile UI Support:**
  * **T-HMI Target:** Implements an elegant, dark-themed LVGL 8.3 UI with touch controls, progress bars, and track information.
  * **S3 Mini Target:** Implements a retro-style monochrome UI via U8g2, driven by a rotary encoder and tactile buttons.
* **Advanced Audio Playback:** MP3/M4A parsing and I2S output using an external PCM5102 DAC.
* **SD Card Media Library:** Fast 1-bit SD_MMC scanning and playlist management with shuffle and repeat support.
* **Thread-Safe Logging:** Custom lock-protected logging system to prevent ISR crashes and UART race conditions.

---

## 📂 Folder Structure

```text
.
├── include/
│   ├── core/           # Core business logic (PlaybackManager, Playlist, UIs)
│   ├── hal/            # Hardware Abstraction Layer interfaces
│   ├── Logging.h       # Thread-safe logging service
│   └── config.h        # Pinouts and global diagnostic macros
└── src/
    ├── core/           # UI implementations (LVGL, U8g2) and Playback control
    ├── hal/
    │   ├── drivers/    # Concrete hardware drivers (I2SAudio, OLEDDisplay, THMIDisplay, etc.)
    │   ├── S3MiniHAL.cpp # S3 Mini Hardware config wiring
    │   └── THMIHAL.cpp   # T-HMI Hardware config wiring
    ├── Logging.cpp     # Logging implementation
    ├── lv_conf.h       # LVGL configuration file
    └── main.cpp        # Application entry point & FreeRTOS task orchestration
```

---

## 🛠️ Hardware Targets & Pinouts

The project is highly configurable. Select your target by defining either `TARGET_THMI` or `TARGET_S3MINI` in your build environment.

### Target 1: LilyGO T-HMI (`TARGET_THMI`)
A complete GUI experience using a parallel ST7789 display and XPT2046 touch controller.

* **Display:** 8-bit Parallel ST7789 TFT
* **Touch:** XPT2046 (SPI)
* **Audio DAC (PCM5102):** 
  * BCLK: `GPIO 17`
  * LRC: `GPIO 18`
  * DOUT: `GPIO 44`
* **SD Card:** SD_MMC 1-bit mode (CMD: `11`, CLK: `12`, DATA0: `13`)

### Target 2: ESP32-S3 Super Mini (`TARGET_S3MINI`)
A compact, physical-button-driven player.

* **Display:** 1.3" I2C OLED SH1106 (SDA: `8`, SCL: `9`)
* **Inputs:** 
  * Rotary Encoder: Pins `1` (A), `2` (B), `0` (Click)
  * Media Buttons: Play/Pause (`3`), Next (`7`), Prev (`14`)
* **Audio DAC (PCM5102):**
  * BCLK: `4`
  * LRC: `5`
  * DOUT: `6`

---

## 📦 Dependencies

To build this project, ensure you have the following libraries installed in your PlatformIO or Arduino IDE environment:

1. [LVGL](https://github.com/lvgl/lvgl) (v8.3.x) - Ensure `lv_conf.h` is picked up from the `src` directory.
2. [U8g2](https://github.com/olikraus/u8g2) - For the OLED display target.
3. [ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S) - For robust MP3/AAC decoding to I2S.
4. **SD_MMC** & **SPI** - Built into the ESP32 Arduino Core.

---

## 🚀 Getting Started

### PlatformIO Configuration

Add the necessary build flags to your `platformio.ini` to select the target. For example, to build for the T-HMI:

```ini
[env:lilygo-t-hmi]
platform = espressif32
board = esp32s3box
framework = arduino
build_flags = 
    -D TARGET_THMI
    -D CORE_DEBUG_LEVEL=3
    -I src/
lib_deps =
    lvgl/lvgl @ ~8.3.1
    schreibfaul1/ESP32-audioI2S
```

### Arduino IDE
1. Open the project folder.
2. Select your ESP32-S3 board variant (e.g., *ESP32S3 Dev Module*).
3. Ensure **PSRAM is enabled** (Required for LVGL DMA buffers on the T-HMI target).
4. Add `#define TARGET_THMI` or `#define TARGET_S3MINI` at the very top of `config.h` or as a compiler flag.
5. Compile and upload.

---

## 🏗️ Architecture Deep Dive

### FreeRTOS Orchestration
* **Core 0 (`audioTask`):** Runs the I2S audio loop and decoder. It is assigned priority 2. Separating this guarantees that UI redraws or touch interrupts never cause audio stutter.
* **Core 1 (`uiTask`):** Handles polling hardware inputs, updating the active UI (LVGL/U8g2), and flushing DMA buffers. It relies on a Mutex (`xGuiSemaphore`) to ensure thread-safe LVGL API calls.

### The HAL (Hardware Abstraction Layer)
The `HAL::get()` singleton vends interfaces for `IDisplay`, `IAudioOutput`, and `IInputSource`. This means `PlaybackManager` and `PlayerUI` contain **zero hardware-specific code**. Adding a new board is as simple as creating a new subclass of `HAL` and wiring up the required drivers.

---

## 📝 License

This project is open-source. Feel free to fork, modify, and build your own Boutique Hi-Fi MP3 player!
