#include "core/LocalSDSource.h"
#include "config.h"
#include "Logging.h"
#include <Arduino.h>

LocalSDSource::LocalSDSource(IAudioOutput* audio) : audioOut(audio), paused(true) {}

void LocalSDSource::activate() {
    logPrintln("[LocalSDSource] Activating Local SD source...");
    if (audioOut) {
        audioOut->activate();
    }
    playlist.scanSD();
    paused = true;
}

void LocalSDSource::deactivate() {
    logPrintln("[LocalSDSource] Deactivating Local SD source...");
    stop();
    if (audioOut) {
        audioOut->deactivate();
    }
}

void LocalSDSource::loop() {
    if (audioOut) {
        audioOut->loop();
    }
    
    // Auto-advance playlist on EOF
    if (!paused && !playlist.isEmpty() && audioOut && !audioOut->isPlaying()) {
        logPrintln("[LocalSDSource] Song finished. Advancing playlist.");
        next();
    }
}

void LocalSDSource::play() {
    if (playlist.isEmpty()) return;
    
    String trackPath = playlist.getCurrentTrackPath();
    if (trackPath.length() > 0) {
        logPrintf("[LocalSDSource] Playing track: %s\n", trackPath.c_str());
        if (audioOut) {
            audioOut->playFile(trackPath.c_str());
        }
        paused = false;
    }
}

void LocalSDSource::pause() {
    if (audioOut) {
        audioOut->stop();
    }
    paused = true;
    logPrintln("[LocalSDSource] Paused.");
}

void LocalSDSource::stop() {
    if (audioOut) {
        audioOut->stop();
    }
    paused = true;
}

void LocalSDSource::next() {
    if (playlist.isEmpty()) return;
    
    bool hasNext = playlist.next();
    if (hasNext || playlist.isRepeat()) {
        play();
    } else {
        pause();
    }
}

void LocalSDSource::prev() {
    if (playlist.isEmpty()) return;
    
    playlist.prev();
    play();
}

void LocalSDSource::setVolume(uint8_t vol) {
    if (audioOut) {
        audioOut->setVolume(vol);
    }
}

uint8_t LocalSDSource::getVolume() const {
    return audioOut ? audioOut->getVolume() : 0;
}

bool LocalSDSource::isPlaying() const {
    return !paused && audioOut && audioOut->isPlaying();
}

bool LocalSDSource::isPaused() const {
    return paused;
}

String LocalSDSource::getTrackName() const {
    return playlist.getCurrentTrackName();
}

String LocalSDSource::getArtistName() const {
    return "SD CARD";
}

uint32_t LocalSDSource::getTrackDuration() const {
    return audioOut ? audioOut->getAudioFileDuration() : 0;
}

uint32_t LocalSDSource::getTrackElapsed() const {
    return audioOut ? audioOut->getAudioCurrentTime() : 0;
}

void LocalSDSource::seekTo(uint16_t seconds) {
    if (!audioOut) return;
#if defined(DISABLE_SEEK_DIAGNOSTIC) && DISABLE_SEEK_DIAGNOSTIC
    logPrintf(
        "[LocalSDSource] Seek ignored in diagnostic mode: %u seconds\n",
        static_cast<unsigned>(seconds)
    );
#else
    audioOut->setAudioPlayPosition(seconds);
#endif
}

Playlist& LocalSDSource::getPlaylist() {
    return playlist;
}
