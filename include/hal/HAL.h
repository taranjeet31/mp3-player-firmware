#pragma once
#include "IDisplay.h"
#include "IInputSource.h"
#include "IAudioOutput.h"

class HAL {
public:
    virtual ~HAL() {}
    virtual bool init() = 0;
    virtual IDisplay* getDisplay() = 0;
    virtual IInputSource* getInput() = 0;
    virtual IAudioOutput* getAudio() = 0;

    // Factory method to obtain HAL instance
    static HAL& get();
};
