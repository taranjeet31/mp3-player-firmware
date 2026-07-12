#pragma once

#if defined(TARGET_S3MINI)

#include "core/PlaybackManager.h"
#include "hal/drivers/OLEDDisplay.h"

class OLEDUI {
private:
    PlaybackManager& manager;
    OLEDDisplay* display;
    
    int active_screen; // 0 = Boot/Splash, 1 = Now Playing, 2 = Browse
    
    // Browse list state
    int selected_track_idx;
    
    // Boot/Splash state
    unsigned long boot_start_time;
    bool boot_complete;
    
    // Rotating cassette reel animation angle
    float reel_angle;

    // Screen drawers
    void draw_boot_screen();
    void draw_now_playing_screen();
    void draw_browse_screen();

public:
    OLEDUI(PlaybackManager& pm);
    void init();
    void update(); // Poll hardware inputs and redraw display buffer
};

#endif
