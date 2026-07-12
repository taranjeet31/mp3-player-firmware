#pragma once
#include <Arduino.h>

class IAudioOutput {
public:
    virtual ~IAudioOutput() {}
    virtual bool init() = 0;
    virtual void playFile(const char* filepath) = 0;
    virtual void stop() = 0;
    virtual void setVolume(uint8_t vol) = 0; // 0 to 21
    virtual uint8_t getVolume() const = 0;
    virtual void loop() = 0;
    virtual bool isPlaying() const = 0;
    virtual uint32_t getAudioFileDuration() = 0; // in seconds
    virtual uint32_t getAudioCurrentTime() = 0; // in seconds
    virtual void setAudioPlayPosition(uint16_t seconds) = 0;
    virtual void activate() {}
    virtual void deactivate() {}
};
