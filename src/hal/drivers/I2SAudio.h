#pragma once
#include "hal/IAudioOutput.h"
#include "Audio.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

enum class AudioCommandType {
    PLAY_FILE,
    STOP,
    SET_VOLUME,
    SEEK
};

struct AudioCommand {
    AudioCommandType type;
    union {
        uint8_t volume;
        uint16_t seek_seconds;
    } arg;
    char path[256];
};

class I2SAudio : public IAudioOutput {
private:
    Audio audio;
    uint8_t volume;
    bool playing;
    uint32_t cached_duration;
    uint32_t cached_elapsed;

    QueueHandle_t commandQueue;
    SemaphoreHandle_t stateMutex;

    uint16_t lastSeekSeconds;
    uint32_t lastSeekMs;

    void processCommands();
    void queueCommand(const AudioCommand& cmd);

    // Thread-safe state setters
    void setStatePlaying(bool play);
    void setStateVolume(uint8_t vol);
    void setStateCachedDuration(uint32_t dur);
    void setStateCachedElapsed(uint32_t elap);

public:
    I2SAudio();
    bool init() override;
    void playFile(const char* filepath) override;
    void stop() override;
    void setVolume(uint8_t vol) override;
    uint8_t getVolume() const override;
    void loop() override;
    bool isPlaying() const override;
    uint32_t getAudioFileDuration() override;
    uint32_t getAudioCurrentTime() override;
    void setAudioPlayPosition(uint16_t seconds) override;
    void activate() override;
    void deactivate() override;
};
