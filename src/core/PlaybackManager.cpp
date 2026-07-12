#include "core/PlaybackManager.h"
#include "config.h"
#include "Logging.h"
#include <Arduino.h>

PlaybackManager::PlaybackManager(IAudioOutput* audio) : audioOut(audio), paused(true) {}

void PlaybackManager::init() {
    playlist.scanSD();
    if (!playlist.isEmpty()) {
        Serial.printf("[Playback] Found %d tracks. Ready to play.\n", playlist.getTrackCount());
    } else {
        Serial.println("[Playback] No tracks available on SD card.");
    }
}

void PlaybackManager::update() {
    // Check if the current track finished playing automatically
    // If we are not paused, have a playlist, and the audio driver is not running,
    // it indicates EOF has been reached.
    if (!paused && !playlist.isEmpty() && !audioOut->isPlaying()) {
        Serial.println("[Playback] Current song finished. Auto-playing next track.");
        next();
    }
}

void PlaybackManager::play() {
    if (playlist.isEmpty()) return;
    
    String trackPath = playlist.getCurrentTrackPath();
    if (trackPath.length() > 0) {
        Serial.printf("[Playback] Playing track: %s\n", trackPath.c_str());
        audioOut->playFile(trackPath.c_str());
        paused = false;
    }
}

void PlaybackManager::pause() {
    audioOut->stop();
    paused = true;
    Serial.println("[Playback] Paused.");
}

void PlaybackManager::togglePlayPause() {
    if (paused) {
        play();
    } else {
        pause();
    }
}

void PlaybackManager::next() {
    if (playlist.isEmpty()) return;
    
    bool hasNext = playlist.next();
    if (hasNext || playlist.isRepeat()) {
        play();
    } else {
        // Stopped at the end of the playlist
        pause();
    }
}

void PlaybackManager::prev() {
    if (playlist.isEmpty()) return;
    
    playlist.prev();
    play();
}

void PlaybackManager::setVolume(uint8_t vol) {
    audioOut->setVolume(vol);
}

uint8_t PlaybackManager::getVolume() const {
    return audioOut->getVolume();
}

Playlist& PlaybackManager::getPlaylist() {
    return playlist;
}

bool PlaybackManager::isPaused() const {
    return paused;
}

bool PlaybackManager::isPlaying() const {
    return !paused && audioOut->isPlaying();
}

String PlaybackManager::getCurrentTrackName() const {
    return playlist.getCurrentTrackName();
}

uint32_t PlaybackManager::getTrackDuration() const {
    return audioOut->getAudioFileDuration();
}

uint32_t PlaybackManager::getTrackElapsed() const {
    return audioOut->getAudioCurrentTime();
}

void PlaybackManager::seekTo(uint16_t seconds) {
#if defined(DISABLE_SEEK_DIAGNOSTIC) && DISABLE_SEEK_DIAGNOSTIC
    logPrintf(
        "[Playback] Seek ignored in diagnostic mode: %u seconds\n",
        static_cast<unsigned>(seconds)
    );
    return;
#else
    audioOut->setAudioPlayPosition(seconds);
#endif
}
