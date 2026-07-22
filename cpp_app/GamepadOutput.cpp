#include "GamepadOutput.h"
#include "Logger.h"
#include <iostream>
#include <algorithm>

#pragma comment(lib, "setupapi.lib")

GamepadOutput::GamepadOutput() : client(nullptr), pad(nullptr), current_speed(0.0), target_added(false) {
    LogMessage("GamepadOutput constructor called.");
}

GamepadOutput::~GamepadOutput() {
    LogMessage("GamepadOutput destructor called.");
    if (pad) {
        if (target_added) {
            LogMessage("Removing virtual Xbox 360 target from ViGEm client...");
            vigem_target_remove(client, pad);
        }
        LogMessage("Freeing virtual Xbox 360 target...");
        vigem_target_free(pad);
        pad = nullptr;
    }
    if (client) {
        LogMessage("Disconnecting ViGEm client...");
        vigem_disconnect(client);
        LogMessage("Freeing ViGEm client...");
        vigem_free(client);
        client = nullptr;
    }
    LogMessage("GamepadOutput successfully destroyed.");
}

bool GamepadOutput::init() {
    LogMessage("GamepadOutput::init: allocating ViGEmClient...");
    client = vigem_alloc();
    if (client == nullptr) {
        LogMessage("[Gamepad] Failed to allocate ViGEmClient.");
        return false;
    }

    LogMessage("GamepadOutput::init: connecting to ViGEmBus...");
    VIGEM_ERROR err = vigem_connect(client);
    if (!VIGEM_SUCCESS(err)) {
        char errBuf[128];
        sprintf_s(errBuf, "[Gamepad] Failed to connect to ViGEmBus. Error code: 0x%X", err);
        LogMessage(errBuf);
        return false;
    }

    LogMessage("GamepadOutput::init: allocating virtual Xbox 360 target...");
    pad = vigem_target_x360_alloc();
    if (pad == nullptr) {
        LogMessage("[Gamepad] Failed to allocate virtual Xbox 360 target.");
        return false;
    }

    LogMessage("GamepadOutput::init: adding virtual Xbox 360 target to client...");
    err = vigem_target_add(client, pad);
    if (!VIGEM_SUCCESS(err)) {
        char errBuf[128];
        sprintf_s(errBuf, "[Gamepad] Failed to add Xbox 360 target. Error code: 0x%X", err);
        LogMessage(errBuf);
        return false;
    }
    target_added = true;

    LogMessage("[Gamepad] Virtual Xbox 360 controller initialized.");

    // Send a dummy packet to wake up the pad
    LogMessage("GamepadOutput::init: sending dummy wakeup packet...");
    XUSB_REPORT report;
    XUSB_REPORT_INIT(&report);
    report.wButtons |= XUSB_GAMEPAD_A;
    vigem_target_x360_update(client, pad, report);
    Sleep(100);
    report.wButtons &= ~XUSB_GAMEPAD_A;
    vigem_target_x360_update(client, pad, report);

    LogMessage("GamepadOutput::init: successfully completed.");
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
