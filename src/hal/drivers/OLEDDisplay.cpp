#if defined(TARGET_S3MINI)

#include "OLEDDisplay.h"
#include "config.h"
#include <Arduino.h>

OLEDDisplay::OLEDDisplay() 
    : u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ OLED_I2C_SCL, /* data=*/ OLED_I2C_SDA) {}

bool OLEDDisplay::init() {
    Serial.println("[Display] Initializing U8g2 SH1106 OLED...");
    
    // Wire.begin is done inside u8g2.begin() but we can also ensure pin definitions are correct
    if (!u8g2.begin()) {
        Serial.println("[Display] Error: OLED begin failed!");
        return false;
    }
    
    u8g2.clearBuffer();
    u8g2.sendBuffer();
    Serial.println("[Display] OLED initialized.");
    return true;
}

void OLEDDisplay::update() {
    // The screen rendering is driven by OLEDUI which draws to the buffer
    // and calls sendBuffer() when ready.
}

U8G2& OLEDDisplay::getU8g2() {
    return u8g2;
}

#endif
