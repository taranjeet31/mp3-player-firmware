#pragma once
#include <Arduino.h>

class IAudioSource {
public:
    virtual ~IAudioSource() {}
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void next() = 0;
    virtual void prev() = 0;
    virtual void setVolume(uint8_t vol) = 0; // 0 to 21
    virtual uint8_t getVolume() const = 0;
    virtual bool isPlaying() const = 0;
    virtual bool isPaused() const = 0;
    
    virtual String getTrackName() const = 0;
    virtual String getArtistName() const = 0;
    virtual uint32_t getTrackDuration() const = 0; // in seconds
    virtual uint32_t getTrackElapsed() const = 0; // in seconds
    virtual void seekTo(uint16_t seconds) = 0;

    virtual void activate() = 0;   // Cleanly set up resource / reclaim I2S
    virtual void deactivate() = 0; // Stop decoding / release I2S
    virtual void loop() = 0;       // Periodic tick / servicing loop
};
