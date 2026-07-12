#pragma once

enum class InputEvent {
    NONE,
    PLAY_PAUSE,
    NEXT,
    PREV,
    VOLUME_UP,
    VOLUME_DOWN,
    ENCODER_CLICK
};

class IInputSource {
public:
    virtual ~IInputSource() {}
    virtual bool init() = 0;
    virtual InputEvent poll() = 0;
};
