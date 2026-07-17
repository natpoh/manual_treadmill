#include "GamepadOutput.h"
#include <iostream>
#include <algorithm>

#pragma comment(lib, "setupapi.lib")

GamepadOutput::GamepadOutput() : client(nullptr), pad(nullptr), current_speed(0.0) {
}

GamepadOutput::~GamepadOutput() {
    if (pad) {
        vigem_target_remove(client, pad);
        vigem_target_free(pad);
    }
    if (client) {
        vigem_disconnect(client);
        vigem_free(client);
    }
}

bool GamepadOutput::init() {
    client = vigem_alloc();
    if (client == nullptr) {
        std::cerr << "[Gamepad] Failed to allocate ViGEmClient." << std::endl;
        return false;
    }

    VIGEM_ERROR err = vigem_connect(client);
    if (!VIGEM_SUCCESS(err)) {
        std::cerr << "[Gamepad] Failed to connect to ViGEmBus. Error: " << err << std::endl;
        return false;
    }

    pad = vigem_target_x360_alloc();
    err = vigem_target_add(client, pad);
    if (!VIGEM_SUCCESS(err)) {
        std::cerr << "[Gamepad] Failed to add Xbox 360 target. Error: " << err << std::endl;
        return false;
    }

    std::cout << "[Gamepad] Virtual Xbox 360 controller initialized." << std::endl;

    // Send a dummy packet to wake up the pad
    XUSB_REPORT report;
    XUSB_REPORT_INIT(&report);
    report.wButtons |= XUSB_GAMEPAD_A;
    vigem_target_x360_update(client, pad, report);
    Sleep(100);
    report.wButtons &= ~XUSB_GAMEPAD_A;
    vigem_target_x360_update(client, pad, report);

    return true;
}

void GamepadOutput::setSpeed(double speed_normalized, unsigned short buttons_mask) {
    speed_normalized = (std::max)(0.0, (std::min)(1.0, speed_normalized));
    current_speed = speed_normalized;

    XUSB_REPORT report;
    XUSB_REPORT_INIT(&report);

    SHORT y_value = static_cast<SHORT>(speed_normalized * 32767.0);
    report.sThumbLY = y_value;

    report.wButtons |= buttons_mask;

    if (pad && client) {
        vigem_target_x360_update(client, pad, report);
    }
}

void GamepadOutput::stopMoving() {
    setSpeed(0.0);
}
