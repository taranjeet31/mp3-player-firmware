#pragma once
#include "IAudioSource.h"
#include "LocalSDSource.h"

enum class AudioSourceType {
    LOCAL_SD,
    SPOTIFY
};

class PlaybackManager {
private:
    IAudioSource* activeSource;
    LocalSDSource* localSource;
    IAudioSource* spotifySource;
    AudioSourceType currentSourceType;

public:
    PlaybackManager(IAudioOutput* audio);
    ~PlaybackManager();
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
    String getCurrentArtistName() const;
    uint32_t getTrackDuration() const;
    uint32_t getTrackElapsed() const;
    void seekTo(uint16_t seconds);

    void setSource(AudioSourceType type);
    AudioSourceType getSource() const;
    IAudioSource* getActiveSource() const;
};
