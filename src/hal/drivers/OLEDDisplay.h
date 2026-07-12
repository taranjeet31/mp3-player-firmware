#pragma once

#if defined(TARGET_S3MINI)

#include "hal/IDisplay.h"
#include <U8g2lib.h>

class OLEDDisplay : public IDisplay {
private:
    U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

public:
    OLEDDisplay();
    bool init() override;
    void update() override;
    
    U8G2& getU8g2();
};

#endif
