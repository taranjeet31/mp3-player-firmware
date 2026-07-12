#if defined(TARGET_S3MINI)

#include "OLEDUI.h"
#include "OLEDTheme.h"
#include "hal/HAL.h"
#include <Arduino.h>

OLEDUI::OLEDUI(PlaybackManager& pm) 
    : manager(pm), 
      display(nullptr), 
      active_screen(0), 
      selected_track_idx(0), 
      boot_start_time(0), 
      boot_complete(false), 
      reel_angle(0.0f) {}

void OLEDUI::init() {
    // Retrieve the display instance from HAL
    display = (OLEDDisplay*)HAL::get().getDisplay();
    boot_start_time = millis();
    active_screen = 0; // Boot screen
}

void OLEDUI::update() {
    if (!display) return;
    U8G2& u8g2 = display->getU8g2();

    // 1. Process Hardware Inputs
    InputEvent ev = HAL::get().getInput()->poll();
    if (ev != InputEvent::NONE) {
        Serial.printf("[OLEDUI] Received input event: %d\n", (int)ev);
        
        if (boot_complete) {
            switch (ev) {
                case InputEvent::ENCODER_CLICK:
                    if (active_screen == 1) { // Now Playing -> Browse
                        active_screen = 2;
                        selected_track_idx = manager.getPlaylist().getCurrentIndex();
                        if (selected_track_idx < 0) selected_track_idx = 0;
                    } else if (active_screen == 2) { // Browse -> Now Playing
                        active_screen = 1;
                    }
                    break;

                case InputEvent::VOLUME_UP:
                    if (active_screen == 1) {
                        manager.setVolume(manager.getVolume() < 21 ? manager.getVolume() + 1 : 21);
                    } else if (active_screen == 2) {
                        int count = manager.getPlaylist().getTrackCount();
                        if (count > 0) {
                            selected_track_idx--;
                            if (selected_track_idx < 0) selected_track_idx = count - 1;
                        }
                    }
                    break;

                case InputEvent::VOLUME_DOWN:
                    if (active_screen == 1) {
                        manager.setVolume(manager.getVolume() > 0 ? manager.getVolume() - 1 : 0);
                    } else if (active_screen == 2) {
                        int count = manager.getPlaylist().getTrackCount();
                        if (count > 0) {
                            selected_track_idx++;
                            if (selected_track_idx >= count) selected_track_idx = 0;
                        }
                    }
                    break;

                case InputEvent::PLAY_PAUSE:
                    if (active_screen == 1) {
                        manager.togglePlayPause();
                    } else if (active_screen == 2) {
                        // In browse mode, click encoder/play button to select track
                        int count = manager.getPlaylist().getTrackCount();
                        if (count > 0 && selected_track_idx >= 0 && selected_track_idx < count) {
                            manager.getPlaylist().selectTrack(selected_track_idx);
                            manager.play();
                            active_screen = 1; // Switch to Now Playing
                        }
                    }
                    break;

                case InputEvent::NEXT:
                    manager.next();
                    break;

                case InputEvent::PREV:
                    manager.prev();
                    break;

                default:
                    break;
            }
        }
    }

    // 2. Draw Screen Buffer
    u8g2.clearBuffer();
    
    if (active_screen == 0) {
        draw_boot_screen();
    } else if (active_screen == 1) {
        draw_now_playing_screen();
    } else if (active_screen == 2) {
        draw_browse_screen();
    }
    
    u8g2.sendBuffer();
}

void OLEDUI::draw_boot_screen() {
    U8G2& u8g2 = display->getU8g2();
    unsigned long elapsed = millis() - boot_start_time;
    
    // Boot sequence lasts 2 seconds
    if (elapsed >= 2000) {
        boot_complete = true;
        active_screen = 1; // Transition to Now Playing
        return;
    }
    
    // Title
    u8g2.setFont(FONT_OLED_TITLE);
    u8g2.drawStr(12, 24, "BOUTIQUE HI-FI");
    
    // Sub-label
    u8g2.setFont(FONT_OLED_SUBTITLE);
    u8g2.drawStr(34, 38, "Warming Up...");
    
    // Progress Bar Frame
    u8g2.drawFrame(14, 48, 100, 6);
    
    // Solid fill progress
    int fill_width = (elapsed * 96) / 2000;
    if (fill_width > 96) fill_width = 96;
    u8g2.drawBox(16, 50, fill_width, 2);
}

void OLEDUI::draw_now_playing_screen() {
    U8G2& u8g2 = display->getU8g2();

    // A. Header Row (PLAYING status on left, volume on right)
    u8g2.setFont(FONT_OLED_SUBTITLE);
    if (manager.isPlaying()) {
        u8g2.drawStr(0, 10, "PLAYING");
    } else {
        u8g2.drawStr(0, 10, "PAUSED");
    }
    
    // Draw small SD Card icon next to PLAYING status
    u8g2.drawFrame(56, 2, 14, 9);
    u8g2.drawBox(57, 3, 10, 7);
    u8g2.setFont(FONT_OLED_TIME);
    u8g2.drawStr(74, 9, "SD");
    
    // Volume label on right
    char vol_buf[10];
    sprintf(vol_buf, "VOL %02d", manager.getVolume());
    u8g2.setFont(FONT_OLED_SUBTITLE);
    u8g2.drawStr(90, 10, vol_buf);

    // Separator line
    u8g2.drawHLine(0, 13, OLED_WIDTH);

    // B. Scrolling Track Name (Clean, robust ticker text logic)
    String trackName = manager.getCurrentTrackName();
    if (manager.getPlaylist().isEmpty()) {
        trackName = "No Media Found";
    }
    
    int str_width = u8g2.getUTF8Width(trackName.c_str());
    int x_pos = 0;
    
    if (str_width > OLED_WIDTH) {
        int max_scroll = str_width - OLED_WIDTH + 8;
        // Scroll speed: ~50ms per step. Pause for 2 seconds at start and end.
        int total_cycle_ticks = max_scroll + 60; // 30 ticks pause at start and end
        int tick = (millis() / 40) % total_cycle_ticks;
        
        if (tick < 20) {
            x_pos = 0;
        } else if (tick > max_scroll + 20) {
            x_pos = -max_scroll;
        } else {
            x_pos = -(tick - 20);
        }
    }
    u8g2.setFont(FONT_OLED_TITLE);
    u8g2.drawStr(x_pos, 25, trackName.c_str());

    // C. Artist Name
    u8g2.setFont(FONT_OLED_SUBTITLE);
    u8g2.drawStr(0, 36, "SD Library");

    // D. Micro Tape Reels Spinning animation
    u8g2.drawFrame(36, 39, 56, 12); // outer tape casing
    u8g2.drawCircle(50, 45, 4);      // left spool
    u8g2.drawCircle(78, 45, 4);      // right spool
    
    if (manager.isPlaying()) {
        reel_angle += 0.25f;
        if (reel_angle > 2 * PI) reel_angle -= 2 * PI;
    }
    
    // Draw spokes inside left spool
    int sx_l = cos(reel_angle) * 3;
    int sy_l = sin(reel_angle) * 3;
    u8g2.drawLine(50 - sx_l, 45 - sy_l, 50 + sx_l, 45 + sy_l);
    u8g2.drawLine(50 - sy_l, 45 + sx_l, 50 + sy_l, 45 - sx_l);

    // Draw spokes inside right spool
    int sx_r = cos(reel_angle) * 3;
    int sy_r = sin(reel_angle) * 3;
    u8g2.drawLine(78 - sx_r, 45 - sy_r, 78 + sx_r, 45 + sy_r);
    u8g2.drawLine(78 - sy_r, 45 + sx_r, 78 + sy_r, 45 - sx_r);

    // E. Striped progress bar
    u8g2.drawFrame(0, 52, OLED_WIDTH, 4);
    uint32_t duration = manager.getTrackDuration();
    uint32_t elapsed = manager.getTrackElapsed();
    
    int fill_width = 0;
    if (duration > 0) {
        fill_width = (elapsed * (OLED_WIDTH - 2)) / duration;
    }
    
    // Striped dither/dot-matrix pattern fill (drawn every 2 pixels)
    for (int px = 0; px < fill_width; px += 2) {
        u8g2.drawVLine(1 + px, 53, 2);
    }

    // F. Progress Time display
    char time_buf[32];
    sprintf(time_buf, "%02d:%02d", (int)(elapsed / 60), (int)(elapsed % 60));
    u8g2.setFont(FONT_OLED_TIME);
    u8g2.drawStr(0, 64, time_buf);

    sprintf(time_buf, "%02d:%02d", (int)(duration / 60), (int)(duration % 60));
    u8g2.drawStr(100, 64, time_buf);
}

void OLEDUI::draw_browse_screen() {
    U8G2& u8g2 = display->getU8g2();
    Playlist& pl = manager.getPlaylist();
    int total_tracks = pl.getTrackCount();

    // 1. Header (Reverse video banner)
    u8g2.drawBox(0, 0, OLED_WIDTH, 12);
    u8g2.setFont(FONT_OLED_SUBTITLE);
    u8g2.setDrawColor(0); // Draw in black on white banner
    u8g2.drawStr(4, 10, "SD MUSIC BROWSER");
    u8g2.setDrawColor(1); // Restore white on black

    if (total_tracks == 0) {
        u8g2.setFont(FONT_OLED_STATUS);
        u8g2.drawStr(10, 36, "No Tracks Available");
        return;
    }

    // 2. Calculate row page viewport (displays 4 tracks)
    int start_idx = selected_track_idx - 1;
    if (start_idx < 0) start_idx = 0;
    if (start_idx + 3 >= total_tracks) start_idx = total_tracks - 4;
    if (start_idx < 0) start_idx = 0;

    // 3. Draw rows
    u8g2.setFont(FONT_OLED_SUBTITLE);
    for (int i = 0; i < 4 && (start_idx + i) < total_tracks; ++i) {
        int track_idx = start_idx + i;
        String name = pl.getTrackName(track_idx);
        int y_pos = 14 + i * 12;

        if (track_idx == selected_track_idx) {
            // Draw highlight box behind selection
            u8g2.drawBox(0, y_pos, 120, 11);
            u8g2.setDrawColor(0); // Black text for selected
            u8g2.drawStr(2, y_pos + 9, name.c_str());
            u8g2.setDrawColor(1); // Restore white
        } else {
            u8g2.drawStr(2, y_pos + 9, name.c_str());
        }
    }

    // 4. Draw Scrollbar on right edge
    u8g2.drawFrame(124, 14, 4, 48);
    int sb_knob_h = 48 / total_tracks;
    if (sb_knob_h < 4) sb_knob_h = 4;
    if (sb_knob_h > 48) sb_knob_h = 48;
    
    int sb_knob_y = 14 + (selected_track_idx * (48 - sb_knob_h)) / total_tracks;
    u8g2.drawBox(124, sb_knob_y, 4, sb_knob_h);
}

#endif
