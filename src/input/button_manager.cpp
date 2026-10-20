/**
 * @file button_manager.cpp
 * @brief Implementation of debounced button events, long press separation, and chording.
 */

#include "button_manager.h"

namespace Input {

ButtonManager::ButtonManager()
    : _simultaneousTriggered(false) {
    memset(&_btnA, 0, sizeof(_btnA));
    memset(&_btnB, 0, sizeof(_btnB));
    memset(&_btnMode, 0, sizeof(_btnMode));
}

void ButtonManager::init(uint8_t pinA, uint8_t pinB, uint8_t pinModeBtn) {
    _btnA.pin = pinA;
    _btnB.pin = pinB;
    _btnMode.pin = pinModeBtn;

    pinMode(_btnA.pin, INPUT_PULLUP);
    pinMode(_btnB.pin, INPUT_PULLUP);
    pinMode(_btnMode.pin, INPUT_PULLUP);

    _btnA.rawState = digitalRead(_btnA.pin);
    _btnA.lastRawState = _btnA.rawState;
    _btnA.isPressed = (_btnA.rawState == LOW);

    _btnB.rawState = digitalRead(_btnB.pin);
    _btnB.lastRawState = _btnB.rawState;
    _btnB.isPressed = (_btnB.rawState == LOW);

    _btnMode.rawState = digitalRead(_btnMode.pin);
    _btnMode.lastRawState = _btnMode.rawState;
    _btnMode.isPressed = (_btnMode.rawState == LOW);
}

void ButtonManager::updatePhysicalButton(ButtonState& btn, uint32_t now) {
    bool currentRaw = (digitalRead(btn.pin) == LOW); // Active-LOW

    if (currentRaw != btn.lastRawState) {
        btn.lastDebounceTime = now;
        btn.lastRawState = currentRaw;
    }

    if ((now - btn.lastDebounceTime) >= Config::Settings::BUTTON_DEBOUNCE_MS) {
        if (currentRaw != btn.isPressed) {
            btn.isPressed = currentRaw;
            if (btn.isPressed) {
                btn.pressStartTime = now;
                btn.longPressTriggered = false;
            } else {
                btn.lastReleaseTime = now;
            }
        }
    }
}

ButtonEvent ButtonManager::update() {
    uint32_t now = millis();

    updatePhysicalButton(_btnA, now);
    updatePhysicalButton(_btnB, now);
    updatePhysicalButton(_btnMode, now);

    // -------------------------------------------------------------------------
    // 1. Simultaneous / Chorded Press Detection
    // -------------------------------------------------------------------------
    if (_btnA.isPressed && _btnB.isPressed) {
        _btnA.chorded = true;
        _btnB.chorded = true;

        if (!_simultaneousTriggered) {
            _simultaneousTriggered = true;
            return ButtonEvent::SIMULTANEOUS_PRESS;
        }
        return ButtonEvent::NONE;
    } else if (!_btnA.isPressed && !_btnB.isPressed) {
        _simultaneousTriggered = false;
    }

    // -------------------------------------------------------------------------
    // 2. Button A Processing (Left Click / Long Press Pause)
    // -------------------------------------------------------------------------
    if (_btnA.isPressed) {
        if (!_btnA.chorded && !_btnA.longPressTriggered) {
            if ((now - _btnA.pressStartTime) >= Config::Settings::BUTTON_LONG_PRESS_MS) {
                _btnA.longPressTriggered = true;
                return ButtonEvent::BUTTON_A_LONG_PRESS;
            }
        }
    } else {
        // Just released
        if (_btnA.pressStartTime > 0) {
            uint32_t holdDuration = _btnA.lastReleaseTime - _btnA.pressStartTime;
            bool wasChorded = _btnA.chorded;
            bool wasLong = _btnA.longPressTriggered;

            _btnA.pressStartTime = 0;
            _btnA.chorded = false;
            _btnA.longPressTriggered = false;

            if (!wasChorded && !wasLong && holdDuration < Config::Settings::BUTTON_LONG_PRESS_MS) {
                return ButtonEvent::BUTTON_A_SHORT_PRESS;
            }
        }
    }

    // -------------------------------------------------------------------------
    // 3. Button B Processing (Right Click / Long Press Recalibrate)
    // -------------------------------------------------------------------------
    if (_btnB.isPressed) {
        if (!_btnB.chorded && !_btnB.longPressTriggered) {
            if ((now - _btnB.pressStartTime) >= Config::Settings::BUTTON_LONG_PRESS_MS) {
                _btnB.longPressTriggered = true;
                return ButtonEvent::BUTTON_B_LONG_PRESS;
            }
        }
    } else {
        // Just released
        if (_btnB.pressStartTime > 0) {
            uint32_t holdDuration = _btnB.lastReleaseTime - _btnB.pressStartTime;
            bool wasChorded = _btnB.chorded;
            bool wasLong = _btnB.longPressTriggered;

            _btnB.pressStartTime = 0;
            _btnB.chorded = false;
            _btnB.longPressTriggered = false;

            if (!wasChorded && !wasLong && holdDuration < Config::Settings::BUTTON_LONG_PRESS_MS) {
                return ButtonEvent::BUTTON_B_SHORT_PRESS;
            }
        }
    }

    // -------------------------------------------------------------------------
    // 4. Button Mode Processing (Mode Switch / 3-Second Hold for OTA Web Portal)
    // -------------------------------------------------------------------------
    if (_btnMode.isPressed) {
        if (!_btnMode.chorded && !_btnMode.longPressTriggered) {
            if ((now - _btnMode.pressStartTime) >= Config::Settings::BUTTON_MODE_HOLD_MS) {
                _btnMode.longPressTriggered = true;
                return ButtonEvent::BUTTON_MODE_LONG_PRESS;
            }
        }
    } else {
        // Just released
        if (_btnMode.pressStartTime > 0) {
            uint32_t holdDuration = _btnMode.lastReleaseTime - _btnMode.pressStartTime;
            bool wasChorded = _btnMode.chorded;
            bool wasLong = _btnMode.longPressTriggered;

            _btnMode.pressStartTime = 0;
            _btnMode.chorded = false;
            _btnMode.longPressTriggered = false;

            if (!wasChorded && !wasLong && holdDuration < Config::Settings::BUTTON_MODE_HOLD_MS) {
                return ButtonEvent::BUTTON_MODE_SHORT_PRESS;
            }
        }
    }

    return ButtonEvent::NONE;
}

} // namespace Input
