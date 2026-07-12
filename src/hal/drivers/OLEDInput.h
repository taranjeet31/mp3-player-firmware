#pragma once

#if defined(TARGET_S3MINI)

#include "hal/IInputSource.h"

class OLEDInput : public IInputSource {
private:
    int last_encoder_val;
    
    // Debounce states for buttons
    unsigned long play_last_debounce;
    unsigned long next_last_debounce;
    unsigned long prev_last_debounce;
    unsigned long click_last_debounce;
    
    int last_play_state;
    int last_next_state;
    int last_prev_state;
    int last_click_state;

public:
    OLEDInput();
    bool init() override;
    InputEvent poll() override;
};

#endif
