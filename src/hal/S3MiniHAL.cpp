#if defined(TARGET_S3MINI)

#include "hal/HAL.h"
#include "drivers/OLEDDisplay.h"
#include "drivers/OLEDInput.h"
#include "drivers/I2SAudio.h"

class S3MiniHAL : public HAL {
private:
    OLEDDisplay display;
    OLEDInput input;
    I2SAudio audio;

public:
    S3MiniHAL() {}

    bool init() override {
        bool ok = true;
        ok &= display.init();
        ok &= input.init();
        ok &= audio.init();
        return ok;
    }

    IDisplay* getDisplay() override {
        return &display;
    }

    IInputSource* getInput() override {
        return &input;
    }

    IAudioOutput* getAudio() override {
        return &audio;
    }
};

HAL& HAL::get() {
    static S3MiniHAL instance;
    return instance;
}

#endif
