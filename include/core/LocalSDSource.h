#pragma once
#include "IAudioSource.h"
#include "Playlist.h"
#include "hal/IAudioOutput.h"

class LocalSDSource : public IAudioSource {
private:
    Playlist playlist;
    IAudioOutput* audioOut;
    bool paused;

public:
    LocalSDSource(IAudioOutput* audio);
    
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

    Playlist& getPlaylist();
};
