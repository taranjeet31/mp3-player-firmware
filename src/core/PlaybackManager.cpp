#include "core/PlaybackManager.h"
#include "config.h"
#include "Logging.h"
#include <Arduino.h>

#if defined(TARGET_THMI)
#include "core/SpotifySource.h"
#endif

PlaybackManager::PlaybackManager(IAudioOutput* audio) 
    : activeSource(nullptr), localSource(nullptr), spotifySource(nullptr), currentSourceType(AudioSourceType::LOCAL_SD) {
    localSource = new LocalSDSource(audio);
    activeSource = localSource;
}

PlaybackManager::~PlaybackManager() {
    delete localSource;
#if defined(TARGET_THMI)
    if (spotifySource) {
        delete spotifySource;
    }
#endif
}

void PlaybackManager::init() {
    localSource->activate(); // Start with local SD active by default
    
#if defined(TARGET_THMI)
    // Spotify is lazy-initialized, so we don't allocate SpotifySource yet.
    // We will allocate it on demand in setSource(AudioSourceType::SPOTIFY).
#endif
}

void PlaybackManager::update() {
    if (activeSource) {
        activeSource->loop();
    }
}

void PlaybackManager::play() {
    if (activeSource) activeSource->play();
}

void PlaybackManager::pause() {
    if (activeSource) activeSource->pause();
}

void PlaybackManager::togglePlayPause() {
    if (activeSource) {
        if (activeSource->isPlaying()) {
            activeSource->pause();
        } else {
            activeSource->play();
        }
    }
}

void PlaybackManager::next() {
    if (activeSource) activeSource->next();
}

void PlaybackManager::prev() {
    if (activeSource) activeSource->prev();
}

void PlaybackManager::setVolume(uint8_t vol) {
    if (activeSource) activeSource->setVolume(vol);
}

uint8_t PlaybackManager::getVolume() const {
    return activeSource ? activeSource->getVolume() : 0;
}

Playlist& PlaybackManager::getPlaylist() {
    return localSource->getPlaylist();
}

bool PlaybackManager::isPaused() const {
    return activeSource ? activeSource->isPaused() : true;
}

bool PlaybackManager::isPlaying() const {
    return activeSource ? activeSource->isPlaying() : false;
}

String PlaybackManager::getCurrentTrackName() const {
    return activeSource ? activeSource->getTrackName() : "";
}

String PlaybackManager::getCurrentArtistName() const {
    return activeSource ? activeSource->getArtistName() : "";
}

uint32_t PlaybackManager::getTrackDuration() const {
    return activeSource ? activeSource->getTrackDuration() : 0;
}

uint32_t PlaybackManager::getTrackElapsed() const {
    return activeSource ? activeSource->getTrackElapsed() : 0;
}

void PlaybackManager::seekTo(uint16_t seconds) {
    if (activeSource) activeSource->seekTo(seconds);
}

void PlaybackManager::setSource(AudioSourceType type) {
    if (currentSourceType == type) return;
    
    logPrintf("[PlaybackManager] Switching source to %s...\n", 
              type == AudioSourceType::LOCAL_SD ? "Local SD" : "Spotify");
              
    if (activeSource) {
        activeSource->deactivate();
    }
    
    currentSourceType = type;
    
    if (type == AudioSourceType::LOCAL_SD) {
        activeSource = localSource;
    } else {
#if defined(TARGET_THMI)
        if (spotifySource == nullptr) {
            logPrintln("[PlaybackManager] Lazily instantiating SpotifySource...");
            Serial.printf("[Heap Log] Pre-Spotify Init - Free Internal: %u bytes, Free PSRAM: %u bytes\n",
                          heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                          heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
                          
            spotifySource = new SpotifySource();
            
            Serial.printf("[Heap Log] Post-Spotify Init - Free Internal: %u bytes, Free PSRAM: %u bytes\n",
                          heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                          heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        }
        activeSource = spotifySource;
#else
        // Spotify is not supported on other targets, fall back to Local SD
        activeSource = localSource;
        currentSourceType = AudioSourceType::LOCAL_SD;
        logPrintln("[PlaybackManager] Spotify not supported on this board target.");
#endif
    }
    
    if (activeSource) {
        activeSource->activate();
    }
}

AudioSourceType PlaybackManager::getSource() const {
    return currentSourceType;
}

IAudioSource* PlaybackManager::getActiveSource() const {
    return activeSource;
}
