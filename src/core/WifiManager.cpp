#if defined(TARGET_THMI)

#include "core/WifiManager.h"
#include "config.h"
#include "Logging.h"

WifiManager::WifiManager()
    : state(WifiState::WIFI_DISABLED), commandQueue(nullptr),
      stateMutex(nullptr), taskHandle(nullptr) {}

bool WifiManager::init() {
  stateMutex = xSemaphoreCreateMutex();
  commandQueue = xQueueCreate(5, sizeof(WifiCommand));

  if (!stateMutex || !commandQueue) {
    logPrintln("[WiFi] Failed to create synchronization objects.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);

  preferences.begin("wifi", false);

  BaseType_t result = xTaskCreatePinnedToCore(taskEntry, "WifiTask", 8192, this,
                                              1, &taskHandle, 0);

  if (result != pdPASS) {
    logPrintln("[WiFi] Failed to create WiFi task.");
    return false;
  }

  setState(WifiState::WIFI_IDLE);
  requestConnectSaved();
  return true;
}

void WifiManager::taskEntry(void *parameter) {
  static_cast<WifiManager *>(parameter)->taskLoop();
}

void WifiManager::taskLoop() {
  WifiCommand command{};

  while (true) {
    if (xQueueReceive(commandQueue, &command, pdMS_TO_TICKS(250)) == pdTRUE) {
      processCommand(command);
    }

    if (WiFi.status() == WL_CONNECTED) {
      setState(WifiState::WIFI_CONNECTED);
    }

    vTaskDelay(pdMS_TO_TICKS(25));
  }
}

void WifiManager::processCommand(const WifiCommand &command) {
  switch (command.type) {
  case WifiCommandType::SCAN:
    performScan();
    break;

  case WifiCommandType::CONNECT:
    performConnect(command.ssid, command.password);
    break;

  case WifiCommandType::DISCONNECT:
    WiFi.disconnect();
    connectedSsid = "";
    setState(WifiState::WIFI_IDLE);
    break;

  case WifiCommandType::CONNECT_SAVED: {
    String ssid = "";
    String password = "";

    if (preferences.isKey("ssid")) {
      ssid = preferences.getString("ssid", "");
      if (preferences.isKey("password")) {
        password = preferences.getString("password", "");
      }
    } else {
      logPrintln("[WiFi] No saved Wi-Fi credentials found in Preferences.");
#if defined(WIFI_SSID) && defined(WIFI_PASSWORD)
      if (strlen(WIFI_SSID) > 0 && strcmp(WIFI_SSID, "Your_SSID") != 0) {
        logPrintln("[WiFi] Attempting connection to compile-time fallback SSID.");
        ssid = WIFI_SSID;
        password = WIFI_PASSWORD;
      }
#endif
    }

    if (!ssid.isEmpty()) {
      performConnect(ssid.c_str(), password.c_str());
    } else {
      logPrintln("[WiFi] No SSID specified. Cannot connect.");
    }
    break;
  }
  }
}

void WifiManager::performScan() {
  setState(WifiState::WIFI_SCANNING);
  logPrintln("[WiFi] Starting scan.");

  int count = WiFi.scanNetworks(false, true);

  std::vector<WifiNetwork> result;

  if (count > 0) {
    result.reserve(count);

    for (int i = 0; i < count; ++i) {
      WifiNetwork network;
      network.ssid = WiFi.SSID(i);
      network.rssi = WiFi.RSSI(i);
      network.secured = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;

      bool duplicate = false;

      for (const auto &existing : result) {
        if (existing.ssid == network.ssid) {
          duplicate = true;
          break;
        }
      }

      if (!duplicate && !network.ssid.isEmpty()) {
        result.push_back(network);
      }
    }
  }

  WiFi.scanDelete();

  if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    networks = std::move(result);
    state = WifiState::WIFI_IDLE;
    xSemaphoreGive(stateMutex);
  }

  logPrintf("[WiFi] Scan completed. Networks=%u\n",
            static_cast<unsigned>(networks.size()));
}

void WifiManager::performConnect(const char *ssid, const char *password) {
  if (!ssid || strlen(ssid) == 0) {
    return;
  }

  setState(WifiState::WIFI_CONNECTING);
  lastError = "";

  WiFi.disconnect();
  vTaskDelay(pdMS_TO_TICKS(100));

  WiFi.begin(ssid, password);

  uint32_t start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    vTaskDelay(pdMS_TO_TICKS(250));
  }

  if (WiFi.status() == WL_CONNECTED) {
    connectedSsid = ssid;

    preferences.putString("ssid", ssid);
    preferences.putString("password", password ? password : "");

    setState(WifiState::WIFI_CONNECTED);

    logPrintf("[WiFi] Connected to %s, IP=%s\n", ssid,
              WiFi.localIP().toString().c_str());
  } else {
    lastError = "Connection failed";
    setState(WifiState::WIFI_FAILED);
    logPrintf("[WiFi] Failed to connect to %s\n", ssid);
  }
}

void WifiManager::requestScan() {
  WifiCommand command{};
  command.type = WifiCommandType::SCAN;
  xQueueSend(commandQueue, &command, 0);
}

void WifiManager::requestConnect(const String &ssid, const String &password) {
  WifiCommand command{};
  command.type = WifiCommandType::CONNECT;

  strlcpy(command.ssid, ssid.c_str(), sizeof(command.ssid));
  strlcpy(command.password, password.c_str(), sizeof(command.password));

  xQueueSend(commandQueue, &command, 0);
}

void WifiManager::requestDisconnect() {
  WifiCommand command{};
  command.type = WifiCommandType::DISCONNECT;
  xQueueSend(commandQueue, &command, 0);
}

void WifiManager::requestConnectSaved() {
  WifiCommand command{};
  command.type = WifiCommandType::CONNECT_SAVED;
  xQueueSend(commandQueue, &command, 0);
}

void WifiManager::setState(WifiState newState) {
  if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    state = newState;
    xSemaphoreGive(stateMutex);
  }
}

WifiState WifiManager::getState() {
  WifiState result = WifiState::WIFI_DISABLED;

  if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    result = state;
    xSemaphoreGive(stateMutex);
  }

  return result;
}

std::vector<WifiNetwork> WifiManager::getNetworks() {
  std::vector<WifiNetwork> copy;

  if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    copy = networks;
    xSemaphoreGive(stateMutex);
  }

  return copy;
}

String WifiManager::getConnectedSsid() {
  String copy;

  if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    copy = connectedSsid;
    xSemaphoreGive(stateMutex);
  }

  return copy;
}

String WifiManager::getLastError() {
  String copy;

  if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    copy = lastError;
    xSemaphoreGive(stateMutex);
  }

  return copy;
}

#endif