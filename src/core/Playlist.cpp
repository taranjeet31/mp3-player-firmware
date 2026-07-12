#include "core/Playlist.h"
#include "config.h"
#include <SD_MMC.h>

Playlist::Playlist() : currentIndex(-1), shuffleMode(false), repeatMode(false) {}

void Playlist::scanSD() {
    tracks.clear();
    currentIndex = -1;
    
    #if defined(AUDIO_MP3_ONLY_DIAGNOSTIC) && AUDIO_MP3_ONLY_DIAGNOSTIC
    Serial.println("[Playlist] MP3-only diagnostic mode enabled.");
    #endif

    Serial.println("[Playlist] Scanning SD card for MP3 files...");
    File root = SD_MMC.open("/");
    if (!root) {
        Serial.println("[Playlist] Failed to open root directory!");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("[Playlist] Root is not a directory!");
        return;
    }
    
    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String name = file.name();
            String baseName = name;
            int lastSlash = name.lastIndexOf('/');
            if (lastSlash >= 0) {
                baseName = name.substring(lastSlash + 1);
            }

            bool isHidden = baseName.startsWith("._") || baseName == ".DS_Store";
            if (isHidden) {
                Serial.printf("[Playlist] Skipping hidden metadata file: %s\n", name.c_str());
            } else {
                bool isMp3 = name.endsWith(".mp3") || name.endsWith(".MP3");
                #if defined(AUDIO_MP3_ONLY_DIAGNOSTIC) && AUDIO_MP3_ONLY_DIAGNOSTIC
                bool match = isMp3;
                #else
                bool isM4a = name.endsWith(".m4a") || name.endsWith(".M4A");
                bool match = isMp3 || isM4a;
                #endif

                if (match) {
                    String fullPath = name;
                    // Ensure the path starts with /
                    if (!fullPath.startsWith("/")) {
                        fullPath = "/" + fullPath;
                    }
                    tracks.push_back(fullPath);
                    Serial.printf("[Playlist] Added: %s\n", fullPath.c_str());
                }
            }
        }
        file = root.openNextFile();
    }
    
    if (!tracks.empty()) {
        currentIndex = 0;
        #if defined(AUDIO_MP3_ONLY_DIAGNOSTIC) && AUDIO_MP3_ONLY_DIAGNOSTIC
        // Prefer /test.mp3 as the first selected track when present
        for (size_t i = 0; i < tracks.size(); ++i) {
            if (tracks[i] == "/test.mp3" || tracks[i] == "test.mp3") {
                currentIndex = i;
                Serial.println("[Playlist] Found /test.mp3. Selecting it as the first track.");
                break;
            }
        }
        #endif
        Serial.printf("[Playlist] Scan complete. Found %d tracks.\n", (int)tracks.size());
    } else {
        Serial.println("[Playlist] Scan complete. No tracks found.");
    }
}

bool Playlist::isEmpty() const {
    return tracks.empty();
}

size_t Playlist::getTrackCount() const {
    return tracks.size();
}

String Playlist::getCurrentTrackPath() const {
    if (currentIndex >= 0 && currentIndex < (int)tracks.size()) {
        return tracks[currentIndex];
    }
    return "";
}

String Playlist::getCurrentTrackName() const {
    String path = getCurrentTrackPath();
    if (path.length() == 0) return "No Track";
    
    // Extract filename from full path
    int lastSlash = path.lastIndexOf('/');
    if (lastSlash >= 0) {
        String filename = path.substring(lastSlash + 1);
        // Strip extension
        int lastDot = filename.lastIndexOf('.');
        if (lastDot >= 0) {
            return filename.substring(0, lastDot);
        }
        return filename;
    }
    return path;
}

int Playlist::getCurrentIndex() const {
    return currentIndex;
}

String Playlist::getTrackName(int index) const {
    if (index >= 0 && index < (int)tracks.size()) {
        String path = tracks[index];
        int lastSlash = path.lastIndexOf('/');
        if (lastSlash >= 0) {
            String filename = path.substring(lastSlash + 1);
            int lastDot = filename.lastIndexOf('.');
            if (lastDot >= 0) {
                return filename.substring(0, lastDot);
            }
            return filename;
        }
        return path;
    }
    return "";
}

String Playlist::getTrackPath(int index) const {
    if (index >= 0 && index < (int)tracks.size()) {
        return tracks[index];
    }
    return "";
}

bool Playlist::next() {
    if (tracks.empty()) return false;
    
    if (shuffleMode) {
        currentIndex = random(0, tracks.size());
    } else {
        currentIndex++;
        if (currentIndex >= (int)tracks.size()) {
            if (repeatMode) {
                currentIndex = 0;
            } else {
                currentIndex = tracks.size() - 1;
                return false; // Stopped at the end
            }
        }
    }
    return true;
}

bool Playlist::prev() {
    if (tracks.empty()) return false;
    
    if (shuffleMode) {
        currentIndex = random(0, tracks.size());
    } else {
        currentIndex--;
        if (currentIndex < 0) {
            if (repeatMode) {
                currentIndex = tracks.size() - 1;
            } else {
                currentIndex = 0;
                return false; // Stopped at the beginning
            }
        }
    }
    return true;
}

bool Playlist::selectTrack(int index) {
    if (index >= 0 && index < (int)tracks.size()) {
        currentIndex = index;
        return true;
    }
    return false;
}

void Playlist::setShuffle(bool enable) {
    shuffleMode = enable;
}

bool Playlist::isShuffle() const {
    return shuffleMode;
}

void Playlist::setRepeat(bool enable) {
    repeatMode = enable;
}

bool Playlist::isRepeat() const {
    return repeatMode;
}
