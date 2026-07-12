#pragma once
#include "Playlist.h"
#include "hal/IAudioOutput.h"

class PlaybackManager {
private:
    Playlist playlist;
    IAudioOutput* audioOut;
    bool paused;

public:
    PlaybackManager(IAudioOutput* audio);
    void init();
    void update(); // Periodic tick handler

    void play();
    void pause();
    void togglePlayPause();
    void next();
    void prev();
    
    void setVolume(uint8_t vol);
    uint8_t getVolume() const;

    Playlist& getPlaylist();
    bool isPaused() const;
    bool isPlaying() const;
    
    String getCurrentTrackName() const;
    uint32_t getTrackDuration() const;
    uint32_t getTrackElapsed() const;
    void seekTo(uint16_t seconds);
};
