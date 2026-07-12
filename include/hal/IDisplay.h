#pragma once

class IDisplay {
public:
    virtual ~IDisplay() {}
    virtual bool init() = 0;
    virtual void update() = 0;
};
