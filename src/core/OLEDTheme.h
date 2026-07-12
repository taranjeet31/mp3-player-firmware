#pragma once

#if defined(TARGET_S3MINI)

// Monochrome U8g2 Font mappings
#define FONT_OLED_TITLE     u8g2_font_helvB10_tr  // Bold track title
#define FONT_OLED_SUBTITLE  u8g2_font_helvR08_tr  // Regular artist/album
#define FONT_OLED_TIME      u8g2_font_5x7_tr      // Tiny numbers for progress time
#define FONT_OLED_STATUS    u8g2_font_6x12_tr     // General list items

// Screen Spacing Constants
#define OLED_WIDTH          128
#define OLED_HEIGHT         64

#define OLED_PADDING        4
#define OLED_LINE_HEIGHT_1  14
#define OLED_LINE_HEIGHT_2  10

#endif
