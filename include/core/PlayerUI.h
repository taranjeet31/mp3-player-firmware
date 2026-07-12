#pragma once

#if defined(TARGET_THMI)

#include "core/PlaybackManager.h"
#include "lvgl.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

extern SemaphoreHandle_t xGuiSemaphore;

class PlayerUI {
private:
    PlaybackManager& manager;

    // ── Now Playing widgets (all on lv_scr_act()) ──────────────────────────
    lv_obj_t* status_label;      // "NOW PLAYING" / "PAUSED"
    lv_obj_t* vol_label;         // "VOL 12"
    lv_obj_t* monogram_label;    // Large accent-coloured first letter
    lv_obj_t* track_label;       // Track title (LV_LABEL_LONG_DOT)
    lv_obj_t* artist_label;      // "SD CARD"
    lv_obj_t* elapsed_label;     // "00:23"
    lv_obj_t* total_label;       // "03:42"
    lv_obj_t* progress_bar;      // lv_bar, display-only, range 0..1000, 3 px tall
    lv_obj_t* play_btn;          // Play/pause button
    lv_obj_t* play_btn_label;    // Symbol label inside play_btn
    lv_obj_t* position_label;    // "1 / 20"

    // ── UI state cache – only call LVGL setters when value changes ─────────
    String   last_track_name;
    bool     last_is_playing;
    uint8_t  last_volume;
    uint32_t last_elapsed;
    uint32_t last_duration;
    int      last_index;
    int      last_count;

    // No seek state, no drag state, no screen-switching state.

public:
    PlayerUI(PlaybackManager& pm);
    void init();
    void update(); // Called once per second from uiTask

    // ── Interactive button callbacks (immediate, via LVGL) ─────────────────
    static void play_pause_cb(lv_event_t* e);
    static void next_cb(lv_event_t* e);
    static void prev_cb(lv_event_t* e);
};

#endif
