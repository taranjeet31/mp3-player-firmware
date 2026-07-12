#if defined(TARGET_THMI)
#include "core/PlayerUI.h"
#include "Logging.h"
#include "PlayerTheme.h"
#include <Arduino.h>

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────



  // PlayerUI::PlayerUI(PlaybackManager &pm)
  // PlayerUI::PlayerUI(PlaybackManager &pm, WifiManager &wifi)
  //     : manager(pm), wifiManager(wifi), status_label(nullptr),
  //     vol_label(nullptr),
  //       monogram_label(nullptr), track_label(nullptr), artist_label(nullptr),
  //       elapsed_label(nullptr), total_label(nullptr), progress_bar(nullptr),
  //       play_btn(nullptr), play_btn_label(nullptr), position_label(nullptr),
  //       last_track_name(""), last_is_playing(false),
  //       last_volume(255), // sentinel – force first update
  //       last_elapsed(0xFFFFFFFF), last_duration(0xFFFFFFFF), last_index(-1),
  //       last_count(-1) {}
  PlayerUI::PlayerUI(PlaybackManager& pm, WifiManager& wifi)
      : manager(pm), wifiManager(wifi),

        status_label(nullptr), vol_label(nullptr), monogram_label(nullptr),
        track_label(nullptr), artist_label(nullptr), elapsed_label(nullptr),
        total_label(nullptr), progress_bar(nullptr), play_btn(nullptr),
        play_btn_label(nullptr), position_label(nullptr),

        last_track_name(""), last_is_playing(false), last_volume(255),
        last_elapsed(0xFFFFFFFF), last_duration(0xFFFFFFFF), last_index(-1),
        last_count(-1),

        active_screen(PlayerScreen::NOW_PLAYING),

        now_playing_screen(nullptr), playlist_screen(nullptr),
        wifi_screen(nullptr),

        playlist_list(nullptr), wifi_status_label(nullptr),
        wifi_network_list(nullptr), wifi_password_area(nullptr),
        wifi_keyboard(nullptr) 
        {}

  // ─────────────────────────────────────────────────────────────────────────────
  //  Button event callbacks  (immediate, wired via LVGL, no seek)
  // ─────────────────────────────────────────────────────────────────────────────
  void PlayerUI::play_pause_cb(lv_event_t * e) {
    PlayerUI *ui = (PlayerUI *)lv_event_get_user_data(e);
    ui->manager.togglePlayPause();
  }

  void PlayerUI::next_cb(lv_event_t * e) {
    PlayerUI *ui = (PlayerUI *)lv_event_get_user_data(e);
    ui->manager.next();
  }

  void PlayerUI::prev_cb(lv_event_t * e) {
    PlayerUI *ui = (PlayerUI *)lv_event_get_user_data(e);
    ui->manager.prev();
  }

  // ─────────────────────────────────────────────────────────────────────────────
  //  init()  –  build the single Now Playing screen on lv_scr_act()
  // ─────────────────────────────────────────────────────────────────────────────
  // void PlayerUI::init() {
  //   // 1. Theme styles
  //   player_theme_init();

  //   // 2. Style the active screen directly – no extra container objects
  //   lv_obj_t *scr = lv_scr_act();
  //   lv_obj_add_style(scr, &style_scr_bg, 0);
  //   lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  //   lv_obj_t *libraryBtn = lv_btn_create(now_playing_screen);
  //   lv_obj_set_size(libraryBtn, 72, 30);
  //   lv_obj_align(libraryBtn, LV_ALIGN_TOP_LEFT, 8, 24);
  //   lv_obj_add_style(libraryBtn, &style_btn, 0);
  //   lv_obj_add_event_cb(libraryBtn, open_playlist_cb, LV_EVENT_CLICKED,
  //   this);

  //   lv_obj_t *libraryLabel = lv_label_create(libraryBtn);
  //   lv_label_set_text(libraryLabel, "LIBRARY");
  //   lv_obj_set_style_text_font(libraryLabel, &lv_font_montserrat_10, 0);
  //   lv_obj_center(libraryLabel);

  //   createPlaylistScreen();

  //   active_screen = PlayerScreen::NOW_PLAYING;

  //   now_playing_screen = lv_obj_create(now_playing_screen);
  //   lv_obj_set_size(now_playing_screen, 240, 320);
  //   lv_obj_set_pos(now_playing_screen, 0, 0);
  //   lv_obj_set_style_bg_opa(now_playing_screen, LV_OPA_TRANSP, 0);
  //   lv_obj_set_style_border_width(now_playing_screen, 0, 0);
  //   lv_obj_set_style_pad_all(now_playing_screen, 0, 0);
  //   lv_obj_clear_flag(now_playing_screen, LV_OBJ_FLAG_SCROLLABLE);

  //   playlist_screen = lv_obj_create(now_playing_screen);
  //   lv_obj_set_size(playlist_screen, 240, 320);
  //   lv_obj_set_pos(playlist_screen, 0, 0);
  //   lv_obj_set_style_bg_opa(playlist_screen, LV_OPA_TRANSP, 0);
  //   lv_obj_set_style_border_width(playlist_screen, 0, 0);
  //   lv_obj_set_style_pad_all(playlist_screen, 0, 0);
  //   lv_obj_add_flag(playlist_screen, LV_OBJ_FLAG_HIDDEN);

  //   wifi_screen = lv_obj_create(now_playing_screen);
  //   lv_obj_set_size(wifi_screen, 240, 320);
  //   lv_obj_set_pos(wifi_screen, 0, 0);
  //   lv_obj_set_style_bg_opa(wifi_screen, LV_OPA_TRANSP, 0);
  //   lv_obj_set_style_border_width(wifi_screen, 0, 0);
  //   lv_obj_set_style_pad_all(wifi_screen, 0, 0);
  //   lv_obj_add_flag(wifi_screen, LV_OBJ_FLAG_HIDDEN);

  void PlayerUI::init() {
    player_theme_init();

    lv_obj_t *scr = lv_scr_act();

    if (scr == nullptr) {
      logPrintln("[PlayerUI] ERROR: lv_scr_act() returned nullptr.");
      return;
    }
    Serial.printf("[PlayerUI] Active LVGL screen pointer=%p\n", (void*)scr);

#if defined(MINIMAL_UI_DIAGNOSTIC) && MINIMAL_UI_DIAGNOSTIC
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t* label = lv_label_create(scr);
    lv_label_set_text(label, "MP3 PLAYER");
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(label);

    Serial.println("[PlayerUI] Minimal widgets created");
    return;
#endif

    lv_obj_add_style(scr, &style_scr_bg, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    active_screen = PlayerScreen::NOW_PLAYING;

    // ---------------------------------------------------------------------
    // Create the three root containers first.
    // Every later widget must use one of these valid parents.
    // ---------------------------------------------------------------------

    now_playing_screen = lv_obj_create(scr);

    if (now_playing_screen == nullptr) {
      logPrintln("[PlayerUI] ERROR: Failed to create Now Playing container.");
      return;
    }

    lv_obj_set_size(now_playing_screen, 240, 320);
    lv_obj_set_pos(now_playing_screen, 0, 0);
    lv_obj_set_style_bg_color(now_playing_screen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(now_playing_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(now_playing_screen, 0, 0);
    lv_obj_set_style_pad_all(now_playing_screen, 0, 0);
    lv_obj_clear_flag(now_playing_screen, LV_OBJ_FLAG_SCROLLABLE);

    playlist_screen = lv_obj_create(scr);

    if (playlist_screen == nullptr) {
      logPrintln("[PlayerUI] ERROR: Failed to create Playlist container.");
      return;
    }

    lv_obj_set_size(playlist_screen, 240, 320);
    lv_obj_set_pos(playlist_screen, 0, 0);
    lv_obj_set_style_bg_color(playlist_screen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(playlist_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(playlist_screen, 0, 0);
    lv_obj_set_style_pad_all(playlist_screen, 0, 0);
    lv_obj_clear_flag(playlist_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(playlist_screen, LV_OBJ_FLAG_HIDDEN);

    wifi_screen = lv_obj_create(scr);

    if (wifi_screen == nullptr) {
      logPrintln("[PlayerUI] ERROR: Failed to create Wi-Fi container.");
      return;
    }

    lv_obj_set_size(wifi_screen, 240, 320);
    lv_obj_set_pos(wifi_screen, 0, 0);
    lv_obj_set_style_bg_color(wifi_screen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(wifi_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(wifi_screen, 0, 0);
    lv_obj_set_style_pad_all(wifi_screen, 0, 0);
    lv_obj_clear_flag(wifi_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(wifi_screen, LV_OBJ_FLAG_HIDDEN);

    Serial.printf("[PlayerUI] Root containers created: now=%p playlist=%p wifi=%p\n",
                  (void*)now_playing_screen, (void*)playlist_screen, (void*)wifi_screen);
    logPrintf("[PlayerUI] scr=%p now=%p playlist=%p wifi=%p\n",
              scr, now_playing_screen, playlist_screen, wifi_screen);

    // ---------------------------------------------------------------------
    // Now it is safe to create children.
    // ---------------------------------------------------------------------

    lv_obj_t *libraryBtn = lv_btn_create(now_playing_screen);

    lv_obj_set_size(libraryBtn, 72, 30);
    lv_obj_align(libraryBtn, LV_ALIGN_TOP_LEFT, 8, 24);
    lv_obj_add_style(libraryBtn, &style_btn, 0);
    lv_obj_add_event_cb(libraryBtn, open_playlist_cb, LV_EVENT_CLICKED, this);

    lv_obj_t *libraryLabel = lv_label_create(libraryBtn);

    lv_label_set_text(libraryLabel, "LIBRARY");
    lv_obj_set_style_text_font(libraryLabel, &lv_font_montserrat_10, 0);
    lv_obj_center(libraryLabel);

    createPlaylistScreen();

    // Do not call createWifiScreen() until it is implemented.

    // ── Row 1: Status label (left) + Volume label (right) ─────────────────
    // y = 8 px from top

    status_label = lv_label_create(now_playing_screen);
    lv_label_set_text(status_label, "NOW PLAYING");
    lv_obj_set_pos(status_label, 8, 8);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_10, 0);
    lv_obj_add_style(status_label, &style_text_muted, 0);

    vol_label = lv_label_create(now_playing_screen);
    lv_label_set_text(vol_label, "VOL --");
    lv_obj_set_style_text_font(vol_label, &lv_font_montserrat_10, 0);
    lv_obj_add_style(vol_label, &style_text_muted, 0);
    // Right-align: place after knowing it renders – we use LV_ALIGN
    lv_obj_align(vol_label, LV_ALIGN_TOP_RIGHT, -8, 8);

    // ── Row 2: Large monogram letter (accent, centre) ──────────────────────
    // y ≈ 35 px

    monogram_label = lv_label_create(now_playing_screen);
    lv_label_set_text(monogram_label, "?");
    lv_obj_set_style_text_font(monogram_label, &lv_font_montserrat_48, 0);
    lv_obj_add_style(monogram_label, &style_text_accent, 0);
    lv_obj_align(monogram_label, LV_ALIGN_TOP_MID, 0, 30);

    // ── Row 3: Track title ─────────────────────────────────────────────────
    // y ≈ 130 px

    track_label = lv_label_create(now_playing_screen);
    lv_label_set_text(track_label, "No Track Loaded");
    lv_obj_set_width(track_label, 224);
    lv_label_set_long_mode(track_label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(track_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(track_label, &lv_font_montserrat_14, 0);
    lv_obj_add_style(track_label, &style_text_primary, 0);
    lv_obj_align(track_label, LV_ALIGN_TOP_MID, 0, 130);

    // ── Row 4: Artist / source subtitle ───────────────────────────────────
    // y ≈ 152 px

    artist_label = lv_label_create(now_playing_screen);
    lv_label_set_text(artist_label, "SD CARD");
    lv_obj_set_style_text_font(artist_label, &lv_font_montserrat_10, 0);
    lv_obj_add_style(artist_label, &style_text_muted, 0);
    lv_obj_align(artist_label, LV_ALIGN_TOP_MID, 0, 152);

    // ── Row 5: Elapsed | thin progress bar | Total ─────────────────────────
    // y ≈ 178 px

    elapsed_label = lv_label_create(now_playing_screen);
    lv_label_set_text(elapsed_label, "00:00");
    lv_obj_set_pos(elapsed_label, 8, 178);
    lv_obj_set_style_text_font(elapsed_label, &lv_font_montserrat_10, 0);
    lv_obj_add_style(elapsed_label, &style_text_muted, 0);

    total_label = lv_label_create(now_playing_screen);
    lv_label_set_text(total_label, "00:00");
    lv_obj_set_style_text_font(total_label, &lv_font_montserrat_10, 0);
    lv_obj_add_style(total_label, &style_text_muted, 0);
    lv_obj_align(total_label, LV_ALIGN_TOP_RIGHT, -8, 178);

    // Progress bar – display only, no event callback, no knob
    progress_bar = lv_bar_create(now_playing_screen);
    lv_obj_set_size(progress_bar, 160, 3);
    lv_obj_align(progress_bar, LV_ALIGN_TOP_MID, 0, 182);
    lv_bar_set_range(progress_bar, 0, 1000);
    lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
    lv_obj_clear_flag(progress_bar, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(progress_bar, LV_OBJ_FLAG_SCROLLABLE);
    // Main track (background)
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(COLOR_HEX_BORDER),
                              LV_PART_MAIN);
    lv_obj_set_style_bg_opa(progress_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(progress_bar, 2, LV_PART_MAIN);
    lv_obj_set_style_border_width(progress_bar, 0, LV_PART_MAIN);
    // Indicator (filled portion – amber accent)
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(COLOR_HEX_ACCENT),
                              LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(progress_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(progress_bar, 2, LV_PART_INDICATOR);
    // No event callback is attached to progress_bar – intentionally.

    // ── Row 6: Playback controls ───────────────────────────────────────────
    // y ≈ 205 px

    lv_obj_t *ctrl = lv_obj_create(now_playing_screen);
    lv_obj_set_size(ctrl, 224, 52);
    lv_obj_align(ctrl, LV_ALIGN_TOP_MID, 0, 205);
    lv_obj_set_style_bg_opa(ctrl, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ctrl, 0, 0);
    lv_obj_set_style_pad_all(ctrl, 0, 0);
    lv_obj_set_flex_flow(ctrl, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ctrl, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(ctrl, LV_OBJ_FLAG_SCROLLABLE);

    // Prev
    lv_obj_t *prev_btn = lv_btn_create(ctrl);
    lv_obj_set_size(prev_btn, 44, 44);
    lv_obj_add_style(prev_btn, &style_btn, 0);
    lv_obj_add_event_cb(prev_btn, prev_cb, LV_EVENT_CLICKED, this);
    lv_obj_t *prev_lbl = lv_label_create(prev_btn);
    lv_label_set_text(prev_lbl, LV_SYMBOL_PREV);
    lv_obj_set_style_text_font(prev_lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(prev_lbl, LV_ALIGN_CENTER, 0, 0);

    // Play/Pause
    play_btn = lv_btn_create(ctrl);
    lv_obj_set_size(play_btn, 52, 52);
    lv_obj_add_style(play_btn, &style_btn, 0);
    lv_obj_add_event_cb(play_btn, play_pause_cb, LV_EVENT_CLICKED, this);
    play_btn_label = lv_label_create(play_btn);
    lv_label_set_text(play_btn_label, LV_SYMBOL_PLAY);
    lv_obj_set_style_text_font(play_btn_label, &lv_font_montserrat_18, 0);
    lv_obj_align(play_btn_label, LV_ALIGN_CENTER, 0, 0);

    // Next
    lv_obj_t *next_btn = lv_btn_create(ctrl);
    lv_obj_set_size(next_btn, 44, 44);
    lv_obj_add_style(next_btn, &style_btn, 0);
    lv_obj_add_event_cb(next_btn, next_cb, LV_EVENT_CLICKED, this);
    lv_obj_t *next_lbl = lv_label_create(next_btn);
    lv_label_set_text(next_lbl, LV_SYMBOL_NEXT);
    lv_obj_set_style_text_font(next_lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(next_lbl, LV_ALIGN_CENTER, 0, 0);

    // ── Row 7: Track position indicator ───────────────────────────────────
    // y ≈ 295 px

    position_label = lv_label_create(now_playing_screen);
    lv_label_set_text(position_label, "-- / --");
    lv_obj_set_style_text_font(position_label, &lv_font_montserrat_10, 0);
    lv_obj_add_style(position_label, &style_text_muted, 0);
    lv_obj_align(position_label, LV_ALIGN_BOTTOM_MID, 0, -8);

    // lv_obj_t *testBtn = lv_btn_create(now_playing_screen);
    // lv_obj_set_size(testBtn, 120, 60);
    // lv_obj_align(testBtn, LV_ALIGN_CENTER, 0, 0);

    // lv_obj_t *testLabel = lv_label_create(testBtn);
    // lv_label_set_text(testLabel, "TOUCH TEST");
    // lv_obj_center(testLabel);

    // lv_obj_add_event_cb(
    //     testBtn, [](lv_event_t *e) { logPrintln("[TouchTest] Button
    //     clicked"); }, LV_EVENT_CLICKED, nullptr);

    logPrintln(
        "[PlayerUI] Now Playing screen initialised (single-screen, no seek).");
  }

  // ─────────────────────────────────────────────────────────────────────────────
  //  showScreen()
  // ─────────────────────────────────────────────────────────────────────────────

  void PlayerUI::showScreen(PlayerScreen screen) {
    active_screen = screen;

    lv_obj_add_flag(now_playing_screen, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(playlist_screen, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(wifi_screen, LV_OBJ_FLAG_HIDDEN);

    switch (screen) {
    case PlayerScreen::NOW_PLAYING:
      lv_obj_clear_flag(now_playing_screen, LV_OBJ_FLAG_HIDDEN);
      break;

    case PlayerScreen::PLAYLIST:
      rebuildPlaylistRows();
      lv_obj_clear_flag(playlist_screen, LV_OBJ_FLAG_HIDDEN);
      break;

    case PlayerScreen::WIFI:
      lv_obj_clear_flag(wifi_screen, LV_OBJ_FLAG_HIDDEN);
      break;
    }
  }

  // ─────────────────────────────────────────────────────────────────────────────
  //  createPlaylistScreen()
  // ─────────────────────────────────────────────────────────────────────────────

  void PlayerUI::createPlaylistScreen() {

    if (playlist_screen == nullptr) {
      logPrintln("[PlayerUI] ERROR: Playlist parent is null.");
      return;
    }

    lv_obj_t *title = lv_label_create(playlist_screen);
    lv_label_set_text(title, "MUSIC LIBRARY");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_add_style(title, &style_text_primary, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    lv_obj_t *backBtn = lv_btn_create(playlist_screen);
    lv_obj_set_size(backBtn, 50, 32);
    lv_obj_align(backBtn, LV_ALIGN_TOP_LEFT, 8, 8);
    lv_obj_add_style(backBtn, &style_btn, 0);
    lv_obj_add_event_cb(backBtn, back_to_player_cb, LV_EVENT_CLICKED, this);

    lv_obj_t *backLabel = lv_label_create(backBtn);
    lv_label_set_text(backLabel, LV_SYMBOL_LEFT);
    lv_obj_center(backLabel);

    playlist_list = lv_list_create(playlist_screen);
    lv_obj_set_size(playlist_list, 224, 250);
    lv_obj_align(playlist_list, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(playlist_list, lv_color_hex(COLOR_HEX_BG), 0);
    lv_obj_set_style_border_width(playlist_list, 0, 0);
    lv_obj_set_style_pad_row(playlist_list, 4, 0);
  }

  // ─────────────────────────────────────────────────────────────────────────────
  //  rebuildPlaylistRows()
  // ─────────────────────────────────────────────────────────────────────────────

  void PlayerUI::rebuildPlaylistRows() {
    lv_obj_clean(playlist_list);

    Playlist &playlist = manager.getPlaylist();
    int count = static_cast<int>(playlist.getTrackCount());
    int current = playlist.getCurrentIndex();

    if (count == 0) {
      lv_obj_t *emptyLabel = lv_label_create(playlist_list);
      lv_label_set_text(emptyLabel, "No music found");
      lv_obj_add_style(emptyLabel, &style_text_muted, 0);
      lv_obj_center(emptyLabel);
      return;
    }

    // Begin with a safety limit.
    constexpr int MAX_VISIBLE_TRACKS = 100;
    int rows = count < MAX_VISIBLE_TRACKS ? count : MAX_VISIBLE_TRACKS;

    for (int i = 0; i < rows; ++i) {
      String title = playlist.getTrackName(i);

      lv_obj_t *row = lv_list_add_btn(playlist_list,
                                      i == current ? LV_SYMBOL_AUDIO : nullptr,
                                      title.c_str());

      lv_obj_set_height(row, 42);
      lv_obj_set_user_data(row,
                           reinterpret_cast<void *>(static_cast<intptr_t>(i)));

      lv_obj_add_event_cb(row, playlist_track_cb, LV_EVENT_CLICKED, this);

      if (i == current) {
        lv_obj_add_style(row, &style_btn_active, 0);
      }
    }
  }

  void PlayerUI::open_playlist_cb(lv_event_t * e) {
    auto *ui = static_cast<PlayerUI *>(lv_event_get_user_data(e));
    if (ui) {
      ui->showScreen(PlayerScreen::PLAYLIST);
    }
  }

  void PlayerUI::back_to_player_cb(lv_event_t * e) {
    auto *ui = static_cast<PlayerUI *>(lv_event_get_user_data(e));
    if (ui) {
      ui->showScreen(PlayerScreen::NOW_PLAYING);
    }
  }

  void PlayerUI::playlist_track_cb(lv_event_t * e) {
    auto *ui = static_cast<PlayerUI *>(lv_event_get_user_data(e));
    if (!ui) {
      return;
    }

    lv_obj_t *row = lv_event_get_target(e);
    int index =
        static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(row)));

    Playlist &playlist = ui->manager.getPlaylist();

    if (playlist.selectTrack(index)) {
      ui->manager.play();
      ui->showScreen(PlayerScreen::NOW_PLAYING);
    }
  }

  // ─────────────────────────────────────────────────────────────────────────────
  //  update()  –  called once per second from uiTask (rate-limited in main.cpp)
  //  Only touches LVGL when the displayed value has actually changed.
  // ─────────────────────────────────────────────────────────────────────────────
  void PlayerUI::update() {
#if defined(MINIMAL_UI_DIAGNOSTIC) && MINIMAL_UI_DIAGNOSTIC
    // In minimal diagnostic mode, there are no widgets to update.
    return;
#endif

    // ── 1. Track name + monogram ───────────────────────────────────────────
    String trackName = manager.getCurrentTrackName();
    if (trackName != last_track_name) {
      last_track_name = trackName;

      if (track_label != nullptr) {
        if (trackName.length() == 0) {
          lv_label_set_text(track_label, "No Track");
        } else {
          lv_label_set_text(track_label, trackName.c_str());
        }
      }

      // Monogram: first uppercase letter of track name
      if (monogram_label != nullptr) {
        char mono[2] = {'?', '\0'};
        if (trackName.length() > 0) {
          mono[0] = (char)toupper((unsigned char)trackName[0]);
        }
        lv_label_set_text(monogram_label, mono);
      }
    }

    // ── 2. Playing state → status label + play/pause button symbol ────────
    bool playing = manager.isPlaying();
    if (playing != last_is_playing) {
      last_is_playing = playing;

      if (status_label != nullptr) {
        if (playing) {
          lv_label_set_text(status_label, "NOW PLAYING");
        } else {
          lv_label_set_text(status_label, "PAUSED");
        }
      }
      if (play_btn != nullptr && play_btn_label != nullptr) {
        if (playing) {
          lv_label_set_text(play_btn_label, LV_SYMBOL_PAUSE);
          lv_obj_add_style(play_btn, &style_btn_active, 0);
        } else {
          lv_label_set_text(play_btn_label, LV_SYMBOL_PLAY);
          lv_obj_remove_style(play_btn, &style_btn_active, 0);
        }
        lv_obj_invalidate(play_btn);
      }
    }

    // ── 3. Volume (text only, no slider) ──────────────────────────────────
    uint8_t vol = manager.getVolume();
    if (vol != last_volume) {
      last_volume = vol;
      if (vol_label != nullptr) {
        char vbuf[12];
        snprintf(vbuf, sizeof(vbuf), "VOL %u", (unsigned)vol);
        lv_label_set_text(vol_label, vbuf);
      }
    }

    // ── 4. Elapsed + duration timestamps ──────────────────────────────────
    uint32_t elapsed = manager.getTrackElapsed();
    uint32_t duration = manager.getTrackDuration();

    if (elapsed != last_elapsed) {
      last_elapsed = elapsed;
      if (elapsed_label != nullptr) {
        char ebuf[8];
        snprintf(ebuf, sizeof(ebuf), "%02u:%02u", (unsigned)(elapsed / 60),
                 (unsigned)(elapsed % 60));
        lv_label_set_text(elapsed_label, ebuf);
      }
    }

    if (duration != last_duration) {
      last_duration = duration;
      if (total_label != nullptr) {
        char dbuf[8];
        snprintf(dbuf, sizeof(dbuf), "%02u:%02u", (unsigned)(duration / 60),
                 (unsigned)(duration % 60));
        lv_label_set_text(total_label, dbuf);
      }
    }

    // ── 5. Progress bar (display-only, no seek, LV_ANIM_OFF) ──────────────
    if (duration > 0 && progress_bar != nullptr) {
      int32_t bar_val = (int32_t)((elapsed * 1000UL) / duration);
      if (bar_val < 0)
        bar_val = 0;
      if (bar_val > 1000)
        bar_val = 1000;
      // Only update LVGL bar if value changed (avoids redundant invalidation)
      if ((uint32_t)bar_val !=
          (uint32_t)((last_elapsed * 1000UL) /
                     (last_duration > 0 ? last_duration : 1))) {
        lv_bar_set_value(progress_bar, bar_val, LV_ANIM_OFF);
      }
    }

    // ── 6. Playlist position label ─────────────────────────────────────────
    int idx = manager.getPlaylist().getCurrentIndex();
    int count = (int)manager.getPlaylist().getTrackCount();

    if (idx != last_index || count != last_count) {
      last_index = idx;
      last_count = count;
      if (position_label != nullptr) {
        char pbuf[16];
        if (count > 0) {
          snprintf(pbuf, sizeof(pbuf), "%d / %d", idx + 1, count);
        } else {
          snprintf(pbuf, sizeof(pbuf), "-- / --");
        }
        lv_label_set_text(position_label, pbuf);
      }
    }

    // ── 7. Wi-Fi status label ──────────────────────────────────────────────
    if (active_screen == PlayerScreen::WIFI && wifi_status_label != nullptr) {
      WifiState wifiState = wifiManager.getState();

      switch (wifiState) {
      case WifiState::WIFI_CONNECTED: {
        String text = "Connected: " + wifiManager.getConnectedSsid();
        lv_label_set_text(wifi_status_label, text.c_str());
        break;
      }

      case WifiState::WIFI_SCANNING:
        lv_label_set_text(wifi_status_label, "Scanning...");
        break;

      case WifiState::WIFI_CONNECTING:
        lv_label_set_text(wifi_status_label, "Connecting...");
        break;

      case WifiState::WIFI_FAILED:
        lv_label_set_text(wifi_status_label, "Connection failed");
        break;

      case WifiState::WIFI_IDLE:
        lv_label_set_text(wifi_status_label, "Disconnected");
        break;

      case WifiState::WIFI_DISABLED:
      default:
        lv_label_set_text(wifi_status_label, "Wi-Fi unavailable");
        break;
      }
    }
  }

#endif
