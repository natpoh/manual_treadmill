#pragma once
#include <windows.h>
#include <ViGEm/Client.h>

class GamepadOutput {
public:
    GamepadOutput();
    ~GamepadOutput();

    bool init();
    void setSpeed(double speed_normalized, unsigned short buttons_mask = 0);
    void stopMoving();

private:
    PVIGEM_CLIENT client;
    PVIGEM_TARGET pad;
    double current_speed;
};
