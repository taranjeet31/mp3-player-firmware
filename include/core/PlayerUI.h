// #pragma once

// #if defined(TARGET_THMI)

// #include "core/PlaybackManager.h"
// #include "lvgl.h"
// #include <freertos/FreeRTOS.h>
// #include <freertos/semphr.h>

#pragma once

#if defined(TARGET_THMI)

#include "core/PlaybackManager.h"
#include "core/WifiManager.h"
#include "lvgl.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

extern SemaphoreHandle_t xGuiSemaphore;

enum class PlayerScreen { NOW_PLAYING, PLAYLIST, WIFI };

class PlayerUI {
private:
  PlaybackManager& manager;
  WifiManager& wifiManager;

  // ── Now Playing widgets (all on lv_scr_act()) ──────────────────────────
  lv_obj_t *status_label;   // "NOW PLAYING" / "PAUSED"
  lv_obj_t *vol_label;      // "VOL 12"
  lv_obj_t *monogram_label; // Large accent-coloured first letter
  lv_obj_t *track_label;    // Track title (LV_LABEL_LONG_DOT)
  lv_obj_t *artist_label;   // "SD CARD"
  lv_obj_t *elapsed_label;  // "00:23"
  lv_obj_t *total_label;    // "03:42"
  lv_obj_t *progress_bar;   // lv_bar, display-only, range 0..1000, 3 px tall
  lv_obj_t *play_btn;       // Play/pause button
  lv_obj_t *play_btn_label; // Symbol label inside play_btn
  lv_obj_t *position_label; // "1 / 20"

  // ── UI state cache – only call LVGL setters when value changes ─────────
  String last_track_name;
  bool last_is_playing;
  uint8_t last_volume;
  uint32_t last_elapsed;
  uint32_t last_duration;
  int last_index;
  int last_count;

  PlayerScreen active_screen;

  lv_obj_t *now_playing_screen;
  lv_obj_t *playlist_screen;
  lv_obj_t *wifi_screen;

  lv_obj_t *playlist_list;
  lv_obj_t *wifi_status_label;
  lv_obj_t *wifi_network_list;
  lv_obj_t *wifi_password_area;
  lv_obj_t *wifi_keyboard;

  void createNowPlayingScreen();
  void createPlaylistScreen();
  void createWifiScreen();

  void showScreen(PlayerScreen screen);
  void rebuildPlaylistRows();

  // No seek state, no drag state, no screen-switching state.

public:
  // PlayerUI(PlaybackManager &pm);
  PlayerUI(PlaybackManager& pm, WifiManager& wifi);
  void init();
  void update(); // Called once per second from uiTask

  // ── Interactive button callbacks (immediate, via LVGL) ─────────────────
  static void play_pause_cb(lv_event_t *e);
  static void next_cb(lv_event_t *e);
  static void prev_cb(lv_event_t *e);

  static void open_playlist_cb(lv_event_t *e);
  static void open_wifi_cb(lv_event_t *e);
  static void back_to_player_cb(lv_event_t *e);
  static void playlist_track_cb(lv_event_t *e);
};

#endif
