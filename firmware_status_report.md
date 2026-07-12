# Firmware Status & Project Scope Report

This document details the current state, capabilities, architecture, and hardware configurations of the ESP32-S3 Hi-Fi MP3 Player firmware, outlining what is currently operational and suggesting future directions for the project.

---

## 🚀 Current Capabilities & Features

The firmware is structured around a dual-board/dual-target design with a shared core logic module. Below is a detailed breakdown of what is fully functional:

### 1. Audio Playback Engine
* **Asynchronous Decoding:** Uses the robust `ESP32-audioI2S` library to stream and decode audio streams in the background.
* **Audio Format Support:** Decodes both `.mp3` and `.m4a` (AAC) formats.
* **Playback Controls:** Core support for Play, Pause, Next Track, Previous Track, Volume adjustment (0–21), and Toggle Play/Pause.
* **Dynamic Time Tracking:** Tracks and formats both current elapsed playback time and total track duration.
* **Smart Transitions:** Automatically transitions to the next track in the playlist upon EOF (End of File) detection.

### 2. Media & Playlist Management
* **SD Card Auto-Scanning:** Automatically scans the root directory of the mounted SD card.
* **Intelligent File Filtering:** Detects and skips hidden macOS metadata files (e.g., `._*` and `.DS_Store`).
* **Repeat & Shuffle Modes:** Implements logical states for shuffling track selection randomly or looping back to the beginning of the playlist once finished.
* **Interactive Playlist Selection:** Supports selecting specific tracks directly from list menus.

### 3. Dual Hardware Configurations (HAL)
The codebase employs a strict Hardware Abstraction Layer (`HAL`), making it easy to swap hardware configurations by compiling with specific flags.

#### Target 1: LilyGO T-HMI (`TARGET_THMI`)
* **Display Output:** 8-bit Parallel ST7789 TFT display controller driven via a custom low-overhead display driver.
* **User Input:** SPI-driven XPT2046 resistive touch screen controller.
* **Audio Output:** External PCM5102 I2S DAC output (wired via Grove header connections).
* **Storage:** 1-bit SD_MMC high-speed interface (supporting fast resource loading and decoding).

#### Target 2: ESP32-S3 Super Mini (`TARGET_S3MINI`)
* **Display Output:** 1.3" I2C SH1106/SSD1306 OLED monochrome display.
* **User Input:** Rotary Encoder (with click selection) + 3 dedicated tactile media buttons (Play/Pause, Next, Prev).
* **Audio Output:** External PCM5102 I2S DAC.
* **Storage:** Standard SPI SD card interface.

### 4. Background Wi-Fi Manager
* **Non-Blocking Operations:** WiFi tasks (scan, connect, disconnect) run in a separate FreeRTOS task driven by an event queue, keeping the UI and playback perfectly smooth.
* **NVS Storage:** Integrates ESP32 `Preferences` API to automatically persist and load the last-connected Wi-Fi SSID and password.
* **Fallback Credentials:** Built-in compiler macro fallback configuration to default credentials when no preferences are saved.

---

## 🏗️ Architecture & System Health

The firmware is designed with stability, responsiveness, and skip-free audio playback as top priorities:

```mermaid
graph TD
    subgraph Core 0 (Audio Core)
        A[audioTask - Pri 2] --> B[I2SAudio Driver / Decoder]
        A --> C[PlaybackManager State Engine]
        D[WifiManager Task - Pri 1] --> E[WiFi Connection Control]
    end
    subgraph Core 1 (UI Core)
        F[uiTask - Pri 2] --> G[HAL Display Update]
        F --> H[Active UI Engine - LVGL/U8g2]
    end
    C <--> |Safe Read/Write| H
    F -.-> |xGuiSemaphore Mutex| F
```

* **Dual-Core FreeRTOS Partitioning:**
  * **Core 0 (`audioTask`):** Dedicated to high-priority audio decoding and playback loops, preventing UI render latency from stuttering the I2S stream.
  * **Core 1 (`uiTask`):** Handles input polling, display driver flushing, and screen redraws.
* **Thread-Safe Utilities:**
  * **GUI Semaphore:** A mutual exclusion semaphore (`xGuiSemaphore`) wraps all LVGL calls to ensure rendering safety.
  * **Safe Logging System:** Prevents simultaneous UART access from multiple tasks and ISRs, reducing runtime crashes.
* **Heap and Memory Status:**
  * Displays are running stably with very low fragmentation (approx. 1% memory fragmentation reported by LVGL).
  * Leverages ESP32-S3 PSRAM for display buffer management on the parallel T-HMI target.

---

## 🎨 User Interface (UI) Implementations

### LilyGO T-HMI (LVGL v8.3)
An elegant, dark-themed interface containing multiple views:
1. **Now Playing Screen:**
   * Dynamic Status text (`NOW_PLAYING` or `PAUSED`).
   * Large single-letter track monogram (derived from the track name).
   * Volume indicator (e.g. `VOL 12`).
   * Ellipsis-scrolling long track name.
   * Progress bar (display-only) + numeric elapsed and duration labels.
   * Big player controls (Prev, Play/Pause, Next touch buttons) change state and color when active.
2. **Music Library Screen:**
   * Accessible by clicking the "Library" button.
   * Displays a scrollable, list-based browse view of all tracks on the SD Card.
   * Highlights the currently playing track with a custom audio symbol and colored highlight.
3. **Wi-Fi Setup Screen:**
   * A container is declared and toggled in the screen manager, but the visual elements (e.g. keyboard, scanning list) are not fully populated.

### ESP32-S3 Super Mini (U8g2 Monochrome)
A retro, space-saving design featuring:
1. **Boot Screen:** Shows a 2-second splash screen with a "Warming Up..." loading bar.
2. **Now Playing Screen:**
   * Current status, volume level, and SD card indicator.
   * Micro-cassette reels animation that rotates in real-time when a track is playing.
   * Horizontal striped progress bar with elapsed/total durations.
   * Scrolling text ticker for long song titles.
3. **Music Library (Browse View):**
   * Displays 4 tracks at a time.
   * Scrollbar indicator showing list relative position.
   * Rotary encoder scroll highlighting with click-to-play selection.

---

## 🚧 Unfinished & Work-in-Progress Areas

> [!WARNING]
> While the core playback is solid, a few features are either unimplemented or require optimization to move out of the developmental stage.

* **Wi-Fi Setup Screen UI:** The Wi-Fi manager works in the background, but the T-HMI graphical setup screens (keyboard input, scanned list rendering, connect button bindings) are not completed in `PlayerUI.cpp`.
* **Track Seeking:** Playback seek (`seekTo()`) is currently ignored on both platforms due to `DISABLE_SEEK_DIAGNOSTIC` being set to `1` in `config.h`. 
* **Audio Format Diversity:** Although `ESP32-audioI2S` supports many formats, the local `Playlist` library is strictly restricted to scanning for `.mp3` and `.m4a` files.
* **Volume Slider / Control UI:** Volume is displayed as text, but no interactive sliders or rotary encoder controls are available in the T-HMI touch interface to set the level.

---

## 🎯 Proposed Next Steps

Here are the logical directions to proceed with, depending on what your primary focus is:

1. **Implement T-HMI Wi-Fi Setup UI:** Code the scanned network selection menu and configure an LVGL virtual keyboard panel in `PlayerUI::createWifiScreen()` so users can connect to local networks dynamically.
2. **Enable Playback Seeking:** Remove `DISABLE_SEEK_DIAGNOSTIC` and link progress bar touch/drag interactions on T-HMI or Rotary Encoder double-press/scroll behavior on S3 Mini to support seeking within tracks.
3. **Add Interactive Volume UI:** Implement a volume slide-out panel or a persistent screen header slider on T-HMI, allowing direct volume adjustments via touch.
4. **Internet Radio Streaming:** Since Wi-Fi and the audio decoder are already integrated, you can implement online radio streams (`.m3u` or shoutcast URLs) alongside SD Card playback.
