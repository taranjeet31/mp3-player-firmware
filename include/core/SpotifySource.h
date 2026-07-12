#pragma once
#if defined(TARGET_THMI)
#include "core/IAudioSource.h"
#include "SpircHandler.h"
#include <memory>
#include <string>
#include <atomic>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Forward declarations of cspot/bell classes
namespace cspot {
    class Context;
}
namespace bell {
    class BellHTTPServer;
    class CircularBuffer;
}

class SpotifySource : public IAudioSource {
private:
    std::shared_ptr<cspot::Context> ctx;
    std::shared_ptr<cspot::SpircHandler> handler;
    std::unique_ptr<bell::BellHTTPServer> server;
    std::unique_ptr<bell::CircularBuffer> circularBuffer;
    
    // Task Handles
    TaskHandle_t cspotTaskHandle;
    TaskHandle_t audioTaskHandle;
    
    // State
    std::atomic<bool> active;
    std::atomic<bool> isPlayingState;
    std::atomic<bool> isPausedState;
    std::atomic<bool> isCspotInitialized;
    
    // Metadata
    std::string currentTrackName;
    std::string currentArtistName;
    uint32_t trackDurationSec;
    uint32_t lastPositionUpdateMs;
    uint32_t lastPositionTimeMs;
    uint8_t volume; // 0 to 21

    // Private helpers
    void initI2S();
    void deinitI2S();
    static void cspotThread(void* param);
    static void audioThread(void* param);
    void handleEvent(std::unique_ptr<cspot::SpircHandler::Event> event);
    void feedData(uint8_t* data, size_t bytes);

public:
    SpotifySource();
    ~SpotifySource() override;

    void play() override;
    void pause() override;
    void stop() override;
    void next() override;
    void prev() override;
    void setVolume(uint8_t vol) override;
    uint8_t getVolume() const override;
    bool isPlaying() const override;
    bool isPaused() const override;
    
    String getTrackName() const override;
    String getArtistName() const override;
    uint32_t getTrackDuration() const override;
    uint32_t getTrackElapsed() const override;
    void seekTo(uint16_t seconds) override;

    void activate() override;
    void deactivate() override;
    void loop() override;
};
#endif
