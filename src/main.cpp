#include "Logging.h"
#include "config.h"
#include "core/PlaybackManager.h"
#include "hal/HAL.h"
#include <Arduino.h>
#include <SD_MMC.h>
#if defined(TARGET_THMI)
#include "core/PlayerUI.h"
#include "hal/drivers/THMIDisplay.h"
// PlayerUI *playerUI = nullptr;
#elif defined(TARGET_S3MINI)
#include "core/OLEDUI.h"
OLEDUI *oledUI = nullptr;
#endif

// #if defined(TARGET_THMI)
// #include "core/WifiManager.h"
// WifiManager* wifiManager = nullptr;
#if defined(TARGET_THMI)
#include "core/PlayerUI.h"
#include "core/WifiManager.h"
#include "hal/drivers/THMIDisplay.h"

PlayerUI *playerUI = nullptr;
WifiManager *wifiManager = nullptr;
#endif
// #endif

PlaybackManager *playbackManager = nullptr;
SemaphoreHandle_t xGuiSemaphore = nullptr;

// #if defined(TARGET_THMI)
// wifiManager = new WifiManager();

// if (!wifiManager->init()) {
//     logPrintln("[Main] WiFi manager initialization failed.");
// }
// #endif

// FreeRTOS Task handles
TaskHandle_t audioTaskHandle = nullptr;
TaskHandle_t uiTaskHandle = nullptr;

// Global tracking variables for diagnostics
volatile uint32_t audioLoopIterations = 0;
volatile uint32_t lastAudioLoopReturnMs = 0;
volatile uint32_t lastUITickMs = 0;

volatile uint32_t uiLoopCounter = 0;
volatile uint32_t lvglHandlerEnterCounter = 0;
volatile uint32_t lvglHandlerExitCounter = 0;
volatile uint32_t playerUiEnterCounter = 0;
volatile uint32_t playerUiExitCounter = 0;
volatile uint32_t lcdFlushSubmitCounter = 0;
volatile uint32_t lcdFlushCompleteCounter = 0;
volatile uint32_t touchReadCounter = 0;

volatile uint32_t touchReadCount = 0;
volatile uint32_t touchPressedCount = 0;
volatile uint32_t touchRejectedPressureCount = 0;
volatile uint32_t touchRejectedBoundsCount = 0;

volatile uint32_t flushRejectedBounds = 0;
volatile uint32_t flushRejectedOversize = 0;
volatile uint32_t largestFlushPixels = 0;

void audioTask(void *pvParameters) {
  logPrintf("[Task] Audio Task started on Core %d\n", xPortGetCoreID());

  while (true) {
    // Service the decoder first.
    HAL::get().getAudio()->loop();

    if (playbackManager != nullptr) {
      playbackManager->update();
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

// UI rendering task (Core 1)
void uiTask(void *pvParameters) {
  logPrintln("[Task] UI Task started on Core 1");

#if defined(TARGET_THMI)
  auto *display = static_cast<THMIDisplay *>(HAL::get().getDisplay());
  if (display) {
    display->setLvglTaskHandle(xTaskGetCurrentTaskHandle());
  }
#endif

  while (true) {
    uiLoopCounter++;
    lastUITickMs = millis();
    if (xGuiSemaphore &&
        xSemaphoreTake(xGuiSemaphore, portMAX_DELAY) == pdTRUE) {

      HAL::get().getDisplay()->update();

#if !defined(DISABLE_PLAYER_UI_UPDATE_DIAGNOSTIC) ||                           \
    (DISABLE_PLAYER_UI_UPDATE_DIAGNOSTIC == 0)
      // PlayerUI::update() is rate-limited to once per 1000 ms.
      // The LVGL handler above (touch / render) still runs every ~30 ms.
      static uint32_t lastPlayerUiUpdateMs = 0;
      {
        uint32_t nowMs = millis();
        if ((nowMs - lastPlayerUiUpdateMs) >= 1000U) {
          lastPlayerUiUpdateMs = nowMs;
#if defined(TARGET_THMI)
          if (playerUI) {
            playerUiEnterCounter++;
            playerUI->update();
            playerUiExitCounter++;
          }
#elif defined(TARGET_S3MINI)
          if (oledUI) {
            playerUiEnterCounter++;
            oledUI->update();
            playerUiExitCounter++;
          }
#endif
        }
      }
#endif

      xSemaphoreGive(xGuiSemaphore);
    }

    vTaskDelay(pdMS_TO_TICKS(30));
  }
}

void logHeap(const char *stage) {
  Serial.printf(
      "[Heap Log] %s - Free Internal: %u bytes, Free PSRAM: %u bytes\n", stage,
      heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
      heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  initLogging(); // Must come before any concurrent task is created.
  Serial.println("\n==================================");
  Serial.println("  ESP32-S3 MP3 Player Starting...");
  Serial.println("==================================");
  logHeap("Boot Start");

  // Initialize GUI Mutex early before any tasks or LVGL setup
  xGuiSemaphore = xSemaphoreCreateMutex();
  if (xGuiSemaphore == nullptr) {
    Serial.println("[Main] Error: Failed to create GUI Mutex!");
  }

#if defined(TARGET_THMI)
  // Perform early power-on setup for LilyGO T-HMI board
  pinMode(PWR_ON_PIN, OUTPUT);
  digitalWrite(PWR_ON_PIN, HIGH);
  pinMode(PWR_EN_PIN, OUTPUT);
  digitalWrite(PWR_EN_PIN, HIGH);
  pinMode(BK_LIGHT_PIN, OUTPUT);
  digitalWrite(BK_LIGHT_PIN, HIGH);
  delay(100);
#endif

  // Mount SD card in 1-bit MMC mode (high-speed interface for audio/display
  // resources)
  Serial.println("[Main] Initializing SD Card...");
  SD_MMC.setPins(SD_SCLK_PIN, SD_MOSI_PIN, SD_MISO_PIN);
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("[Main] Error: SD Card mount failed!");
  } else {
    Serial.println("[Main] SD Card mounted successfully.");
  }
  logHeap("Post SD Card");

  // Initialize Hardware Abstraction Layer
  Serial.println("[Main] Initializing HAL...");
  if (!HAL::get().init()) {
    Serial.println("[Main] Error: HAL Initialization Failed!");
  }
  logHeap("Post HAL Init");

  // Create PlaybackManager & UI
  // Serial.println("[Main] Initializing Core Managers...");
  // playbackManager = new PlaybackManager(HAL::get().getAudio());
  // playbackManager->init();

  Serial.println("[Main] Initializing Core Managers...");

  playbackManager = new PlaybackManager(HAL::get().getAudio());

  if (playbackManager == nullptr) {
    logPrintln("[Main] FATAL: PlaybackManager allocation failed.");
    return;
  }

  playbackManager->init();

#if defined(TARGET_THMI)

  wifiManager = new WifiManager();

  if (wifiManager == nullptr) {
    logPrintln("[Main] FATAL: WifiManager allocation failed.");
    return;
  }

  if (!wifiManager->init()) {
    logPrintln("[Main] WiFi manager initialization failed.");
  }

  playerUI = new PlayerUI(*playbackManager, *wifiManager);

  if (playerUI == nullptr) {
    logPrintln("[Main] FATAL: PlayerUI allocation failed.");
    return;
  }

  playerUI->init();

#elif defined(TARGET_S3MINI)

  oledUI = new OLEDUI(*playbackManager);

  if (oledUI != nullptr) {
    oledUI->init();
  }

#endif

  logHeap("Post UI Init");

  // Start audio service task.
  BaseType_t audioTaskResult = xTaskCreatePinnedToCore(
      audioTask, "AudioTask", 49152, nullptr, 2, &audioTaskHandle, 0);

  if (audioTaskResult != pdPASS) {
    logPrintln("[Main] ERROR: Failed to create AudioTask.");
  } else {
    logPrintln("[Main] Audio task created");
  }

#if defined(DISABLE_PLAYER_UI_UPDATE_DIAGNOSTIC) &&                            \
    DISABLE_PLAYER_UI_UPDATE_DIAGNOSTIC
  logPrintln("[Diagnostic] PlayerUI updates disabled.");
#endif

#if !defined(DISABLE_UI_TASK_DIAGNOSTIC) || (DISABLE_UI_TASK_DIAGNOSTIC == 0)
  // UI task gets lower priority (2) on Core 1
  BaseType_t uiTaskResult = xTaskCreatePinnedToCore(
      uiTask, "UITask",
      32768, // Support complex UI render paths safely
      NULL, 2, &uiTaskHandle,
      1 // Core 1
  );
  if (uiTaskResult != pdPASS) {
    logPrintln("[Main] ERROR: Failed to create UITask.");
  } else {
    logPrintln("[Main] UI task created");
  }
#else
  logPrintln("[Diagnostic] UI task disabled.");
#endif
  logHeap("Post Task Creation");

  // Auto-start playback of the first track if files are available
  if (playbackManager && !playbackManager->getPlaylist().isEmpty()) {
    playbackManager->play();
  }
  logHeap("Setup Complete");
}

void loop() {
  static uint32_t lastHealthLog = 0;
  const uint32_t now = millis();

  if (now - lastHealthLog >= 1000) {
    lastHealthLog = now;
    uint32_t returnAgo = now - lastAudioLoopReturnMs;
    bool playing = false;
    if (playbackManager) {
      playing = playbackManager->isPlaying();
    }
    bool uiAlive = false;
#if !defined(DISABLE_UI_TASK_DIAGNOSTIC) || (DISABLE_UI_TASK_DIAGNOSTIC == 0)
    uiAlive = (now - lastUITickMs < 2000);
#endif
    logPrintf(
        "[Health] audioLoops=%u lastReturnAgo=%ums playing=%d uiAlive=%d\n",
        (unsigned int)audioLoopIterations, (unsigned int)returnAgo,
        playing ? 1 : 0, uiAlive ? 1 : 0);

    logPrintf(
        "[FlushHealth] rejectedBounds=%u rejectedOversize=%u largest=%u\n",
        (unsigned int)flushRejectedBounds, (unsigned int)flushRejectedOversize,
        (unsigned int)largestFlushPixels);

    logPrintf(
        "[UIHealth] loops=%u lvgl=%u/%u player=%u/%u flush=%u/%u touch=%u\n",
        (unsigned int)uiLoopCounter, (unsigned int)lvglHandlerEnterCounter,
        (unsigned int)lvglHandlerExitCounter,
        (unsigned int)playerUiEnterCounter, (unsigned int)playerUiExitCounter,
        (unsigned int)lcdFlushSubmitCounter,
        (unsigned int)lcdFlushCompleteCounter, (unsigned int)touchReadCounter);

    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    logPrintf("[LVGL Mem] free=%u used=%u frag=%u%%\n",
              (unsigned int)mon.free_size,
              (unsigned int)(128U * 1024U - mon.free_size),
              (unsigned int)mon.frag_pct);

    logPrintf("[TouchHealth] read=%u pressed=%u rejectedPressure=%u "
              "rejectedBounds=%u\n",
              (unsigned int)touchReadCount, (unsigned int)touchPressedCount,
              (unsigned int)touchRejectedPressureCount,
              (unsigned int)touchRejectedBoundsCount);
  }

  delay(100);
}
