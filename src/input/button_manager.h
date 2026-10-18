/**
 * @file button_manager.h
 * @brief Deterministic Debounced Button Manager with Event Detection.
 */

#pragma once

#include <Arduino.h>
#include "../config/pins.h"
#include "../config/settings.h"

namespace Input {

enum class ButtonId : uint8_t {
    BUTTON_A,
    BUTTON_B
};

enum class ButtonEvent : uint8_t {
    NONE,
    BUTTON_A_SHORT_PRESS,
    BUTTON_B_SHORT_PRESS,
    BUTTON_A_LONG_PRESS,
    BUTTON_B_LONG_PRESS,
    SIMULTANEOUS_PRESS,
    LASER_HOLD_ENGAGE,
    LASER_HOLD_RELEASE
};

class ButtonManager {
public:
    ButtonManager();

    void init(uint8_t pinA = Config::Pins::BUTTON_A,
              uint8_t pinB = Config::Pins::BUTTON_B);

    /**
     * @brief Polling update called on every iteration of the main loop.
     * @return ButtonEvent detected during this update cycle, or ButtonEvent::NONE.
     */
    ButtonEvent update();

    bool isButtonAPressed() const { return _btnA.isPressed; }
    bool isButtonBPressed() const { return _btnB.isPressed; }
    bool isLaserRequested() const { return _laserRequested; }

private:
    struct ButtonState {
        uint8_t  pin;
        bool     isPressed;
        bool     rawState;
        bool     lastRawState;
        uint32_t lastDebounceTime;
        uint32_t pressStartTime;
        bool     longPressTriggered;
        bool     chorded;
        uint32_t lastReleaseTime;
        bool     waitingForSecondTap;
        bool     doubleTapHoldEngaged;
    };

    ButtonState _btnA;
    ButtonState _btnB;
    bool _laserRequested;
    uint32_t _simultaneousPressStart;
    bool _simultaneousTriggered;

    void updatePhysicalButton(ButtonState& btn, uint32_t now);
};

} // namespace Input
