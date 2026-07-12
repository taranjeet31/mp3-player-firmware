#pragma once

#if defined(TARGET_THMI)

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <vector>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

enum class WifiState {
    WIFI_DISABLED,
    WIFI_IDLE,
    WIFI_SCANNING,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_FAILED
};

struct WifiNetwork {
    String ssid;
    int32_t rssi;
    bool secured;
};

enum class WifiCommandType {
    SCAN,
    CONNECT,
    DISCONNECT,
    CONNECT_SAVED
};

struct WifiCommand {
    WifiCommandType type;
    char ssid[33];
    char password[65];
};

class WifiManager {
private:
    WifiState state;
    std::vector<WifiNetwork> networks;

    QueueHandle_t commandQueue;
    SemaphoreHandle_t stateMutex;
    TaskHandle_t taskHandle;

    Preferences preferences;

    String connectedSsid;
    String lastError;

    static void taskEntry(void* parameter);
    void taskLoop();
    void processCommand(const WifiCommand& command);

    void performScan();
    void performConnect(const char* ssid, const char* password);

    void setState(WifiState newState);

public:
    WifiManager();

    bool init();

    void requestScan();
    void requestConnect(const String& ssid, const String& password);
    void requestDisconnect();
    void requestConnectSaved();

    WifiState getState();
    std::vector<WifiNetwork> getNetworks();
    String getConnectedSsid();
    String getLastError();
};

#endif