#if defined(TARGET_S3MINI)

#include "OLEDInput.h"
#include "config.h"
#include <Arduino.h>

#ifndef ENCODER_BTN_PIN
#define ENCODER_BTN_PIN (0) // Fallback to onboard Boot button (GPIO 0)
#endif

#define DEBOUNCE_DELAY_MS 30

OLEDInput::OLEDInput() 
    : last_encoder_val(HIGH),
      play_last_debounce(0),
      next_last_debounce(0),
      prev_last_debounce(0),
      click_last_debounce(0),
      last_play_state(HIGH),
      last_next_state(HIGH),
      last_prev_state(HIGH),
      last_click_state(HIGH) {}

bool OLEDInput::init() {
    Serial.println("[Input] Initializing OLED Inputs...");
    
    // Configure buttons with input pullups
    pinMode(BTN_PLAY_PIN, INPUT_PULLUP);
    pinMode(BTN_NEXT_PIN, INPUT_PULLUP);
    pinMode(BTN_PREV_PIN, INPUT_PULLUP);
    pinMode(ENCODER_BTN_PIN, INPUT_PULLUP);
    
    // Configure rotary encoder pins
    pinMode(ENCODER_A_PIN, INPUT_PULLUP);
    pinMode(ENCODER_B_PIN, INPUT_PULLUP);
    
    last_encoder_val = digitalRead(ENCODER_A_PIN);
    
    Serial.println("[Input] OLED Inputs configured.");
    return true;
}

InputEvent OLEDInput::poll() {
    unsigned long now = millis();
    
    // 1. Decode Rotary Encoder (Simple state change detection)
    int current_encoder_val = digitalRead(ENCODER_A_PIN);
    if (current_encoder_val != last_encoder_val) {
        last_encoder_val = current_encoder_val;
        // Only trigger on one edge to avoid double steps
        if (current_encoder_val == LOW) {
            if (digitalRead(ENCODER_B_PIN) == HIGH) {
                return InputEvent::VOLUME_UP;
            } else {
                return InputEvent::VOLUME_DOWN;
            }
        }
    }

    // 2. Debounce and read Play Button
    int play_reading = digitalRead(BTN_PLAY_PIN);
    static int play_stable_state = HIGH;
    static int play_last_reading = HIGH;
    if (play_reading != play_last_reading) {
        play_last_debounce = now;
        play_last_reading = play_reading;
    }
    if ((now - play_last_debounce) > DEBOUNCE_DELAY_MS) {
        if (play_reading != play_stable_state) {
            play_stable_state = play_reading;
            if (play_stable_state == LOW) {
                return InputEvent::PLAY_PAUSE;
            }
        }
    }

    // 3. Debounce and read Next Button
    int next_reading = digitalRead(BTN_NEXT_PIN);
    static int next_stable_state = HIGH;
    static int next_last_reading = HIGH;
    if (next_reading != next_last_reading) {
        next_last_debounce = now;
        next_last_reading = next_reading;
    }
    if ((now - next_last_debounce) > DEBOUNCE_DELAY_MS) {
        if (next_reading != next_stable_state) {
            next_stable_state = next_reading;
            if (next_stable_state == LOW) {
                return InputEvent::NEXT;
            }
        }
    }

    // 4. Debounce and read Prev Button
    int prev_reading = digitalRead(BTN_PREV_PIN);
    static int prev_stable_state = HIGH;
    static int prev_last_reading = HIGH;
    if (prev_reading != prev_last_reading) {
        prev_last_debounce = now;
        prev_last_reading = prev_reading;
    }
    if ((now - prev_last_debounce) > DEBOUNCE_DELAY_MS) {
        if (prev_reading != prev_stable_state) {
            prev_stable_state = prev_reading;
            if (prev_stable_state == LOW) {
                return InputEvent::PREV;
            }
        }
    }

    // 5. Debounce and read Encoder Click Button
    int click_reading = digitalRead(ENCODER_BTN_PIN);
    static int click_stable_state = HIGH;
    static int click_last_reading = HIGH;
    if (click_reading != click_last_reading) {
        click_last_debounce = now;
        click_last_reading = click_reading;
    }
    if ((now - click_last_debounce) > DEBOUNCE_DELAY_MS) {
        if (click_reading != click_stable_state) {
            click_stable_state = click_reading;
            if (click_stable_state == LOW) {
                return InputEvent::ENCODER_CLICK;
            }
        }
    }

    return InputEvent::NONE;
}

#endif
