#pragma once
#include <Arduino.h>
#include <vector>

class Playlist {
private:
    std::vector<String> tracks;
    int currentIndex;
    bool shuffleMode;
    bool repeatMode;

public:
    Playlist();
    void scanSD();
    
    bool isEmpty() const;
    size_t getTrackCount() const;
    
    String getCurrentTrackPath() const;
    String getCurrentTrackName() const;
    int getCurrentIndex() const;
    
    String getTrackName(int index) const;
    String getTrackPath(int index) const;
    
    bool next();
    bool prev();
    bool selectTrack(int index);
    
    void setShuffle(bool enable);
    bool isShuffle() const;
    
    void setRepeat(bool enable);
    bool isRepeat() const;
};
