#pragma once
#include "hal/IInputSource.h"

class I2CInput : public IInputSource {
public:
    I2CInput();
    bool init() override;
    InputEvent poll() override;
};
