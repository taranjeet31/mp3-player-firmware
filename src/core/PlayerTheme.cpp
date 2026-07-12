#if defined(TARGET_THMI)

#include "PlayerTheme.h"

// Define the styles
lv_style_t style_scr_bg;
lv_style_t style_panel;
lv_style_t style_btn;
lv_style_t style_btn_active;
lv_style_t style_slider_bg;
lv_style_t style_slider_indic;
lv_style_t style_slider_knob;
lv_style_t style_text_primary;
lv_style_t style_text_muted;
lv_style_t style_text_accent;

void player_theme_init() {
    // 1. Screen background style
    lv_style_init(&style_scr_bg);
    lv_style_set_bg_color(&style_scr_bg, lv_color_hex(COLOR_HEX_BG));
    lv_style_set_bg_opa(&style_scr_bg, LV_OPA_COVER);

    // 2. Panel card style (now flat black, no border, no shadow)
    lv_style_init(&style_panel);
    lv_style_set_bg_color(&style_panel, lv_color_hex(COLOR_HEX_BG));
    lv_style_set_bg_opa(&style_panel, LV_OPA_COVER);
    lv_style_set_border_width(&style_panel, 0);
    lv_style_set_radius(&style_panel, 0);
    lv_style_set_pad_all(&style_panel, 8);

    // 3. Touch button style (flat black, subtle dark border)
    lv_style_init(&style_btn);
    lv_style_set_bg_color(&style_btn, lv_color_hex(COLOR_HEX_BG));
    lv_style_set_bg_opa(&style_btn, LV_OPA_COVER);
    lv_style_set_border_color(&style_btn, lv_color_hex(COLOR_HEX_BORDER));
    lv_style_set_border_width(&style_btn, 1);
    lv_style_set_radius(&style_btn, 4);
    lv_style_set_text_color(&style_btn, lv_color_hex(COLOR_HEX_TEXT_PRIMARY));
    lv_style_set_pad_all(&style_btn, 8);

    // 4. Button active / selected state (accent border and text, black bg)
    lv_style_init(&style_btn_active);
    lv_style_set_bg_color(&style_btn_active, lv_color_hex(COLOR_HEX_BG));
    lv_style_set_border_color(&style_btn_active, lv_color_hex(COLOR_HEX_ACCENT));
    lv_style_set_text_color(&style_btn_active, lv_color_hex(COLOR_HEX_ACCENT));

    // 5. Slider/Arc background track
    lv_style_init(&style_slider_bg);
    lv_style_set_bg_color(&style_slider_bg, lv_color_hex(COLOR_HEX_BORDER));
    lv_style_set_bg_opa(&style_slider_bg, LV_OPA_COVER);
    lv_style_set_radius(&style_slider_bg, 2);

    // 6. Slider/Arc indicator (filled part uses the amber accent)
    lv_style_init(&style_slider_indic);
    lv_style_set_bg_color(&style_slider_indic, lv_color_hex(COLOR_HEX_ACCENT));
    lv_style_set_bg_opa(&style_slider_indic, LV_OPA_COVER);
    lv_style_set_radius(&style_slider_indic, 2);

    // 7. Slider knob (small flat white circle)
    lv_style_init(&style_slider_knob);
    lv_style_set_bg_color(&style_slider_knob, lv_color_hex(COLOR_HEX_TEXT_PRIMARY));
    lv_style_set_bg_opa(&style_slider_knob, LV_OPA_COVER);
    lv_style_set_radius(&style_slider_knob, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&style_slider_knob, 0);

    // 8. Typography helper styles
    lv_style_init(&style_text_primary);
    lv_style_set_text_color(&style_text_primary, lv_color_hex(COLOR_HEX_TEXT_PRIMARY));

    lv_style_init(&style_text_muted);
    lv_style_set_text_color(&style_text_muted, lv_color_hex(COLOR_HEX_TEXT_MUTED));

    lv_style_init(&style_text_accent);
    lv_style_set_text_color(&style_text_accent, lv_color_hex(COLOR_HEX_ACCENT));
}

#endif
