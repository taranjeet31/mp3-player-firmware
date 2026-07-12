#pragma once

#if defined(TARGET_THMI)

#include "lvgl.h"

// Color definitions (Minimal Pitch-Black Palette)
#define COLOR_HEX_BG           0x000000  // True Black
#define COLOR_HEX_SURFACE      0x000000  // Flat Black
#define COLOR_HEX_TEXT_PRIMARY 0xFFFFFF  // Premium White
#define COLOR_HEX_TEXT_MUTED   0x777777  // Slate Muted Gray
#define COLOR_HEX_ACCENT       0xFF6C00  // Signature Amber Glow
#define COLOR_HEX_BORDER       0x222222  // Subtle Dark Border

// Styles
extern lv_style_t style_scr_bg;
extern lv_style_t style_panel;
extern lv_style_t style_btn;
extern lv_style_t style_btn_active;
extern lv_style_t style_slider_bg;
extern lv_style_t style_slider_indic;
extern lv_style_t style_slider_knob;
extern lv_style_t style_text_primary;
extern lv_style_t style_text_muted;
extern lv_style_t style_text_accent;

// Initialize the theme styles
void player_theme_init();

#endif
