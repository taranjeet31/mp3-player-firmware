#if defined(TARGET_THMI)

#include "hal/HAL.h"
#include "drivers/THMIDisplay.h"
#include "drivers/I2CInput.h"
#include "drivers/I2SAudio.h"

class THMIHAL : public HAL {
private:
    THMIDisplay display;
    I2CInput input;
    I2SAudio audio;

public:
    THMIHAL() {}

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
    static THMIHAL instance;
    return instance;
}

#endif
