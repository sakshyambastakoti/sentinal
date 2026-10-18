/**
 * @file laser_controller.cpp
 * @brief Implementation of safety-first laser driver controller.
 */

#include "laser_controller.h"

namespace Peripherals {

LaserController::LaserController()
    : _pin(Config::Pins::LASER_DRIVER),
      _state(LaserState::OFF),
      _gestureRequested(false),
      _interlockInhibited(false),
      _activationStartTime(0),
      _timeoutStartTime(0) {}

void LaserController::setPhysicalOutput(bool high) {
    digitalWrite(_pin, high ? HIGH : LOW);
}

void LaserController::init(uint8_t pin) {
    _pin = pin;
    _state = LaserState::OFF;
    _gestureRequested = false;
    _interlockInhibited = false;
    _activationStartTime = 0;
    _timeoutStartTime = 0;

    // Enforce immediate safe low state
    pinMode(_pin, OUTPUT);
    setPhysicalOutput(false);
}

void LaserController::setInterlock(bool inhibited) {
    _interlockInhibited = inhibited;
    if (_interlockInhibited && _state == LaserState::ACTIVE) {
        _state = LaserState::INTERLOCK_INHIBITED;
        setPhysicalOutput(false);
    }
}

void LaserController::emergencyShutdown() {
    setPhysicalOutput(false);
    _state = LaserState::OFF;
    _gestureRequested = false;
    _activationStartTime = 0;
}

uint32_t LaserController::getActiveDurationMs() const {
    if (_state != LaserState::ACTIVE) {
        return 0;
    }
    return millis() - _activationStartTime;
}

void LaserController::requestLaser(bool active) {
    _gestureRequested = active;
    uint32_t now = millis();

    if (!active) {
        // Gesture released
        if (_state == LaserState::ACTIVE) {
            _state = LaserState::OFF;
            setPhysicalOutput(false);
        } else if (_state == LaserState::INTERLOCK_INHIBITED) {
            _state = LaserState::OFF;
        }
        return;
    }

    // Gesture is requested (active == true)
    if (_interlockInhibited) {
        _state = LaserState::INTERLOCK_INHIBITED;
        setPhysicalOutput(false);
        return;
    }

    if (_state == LaserState::TIMED_OUT) {
        // Must release gesture and wait for cooldown before re-activation
        return;
    }

    if (_state == LaserState::OFF) {
        // Check cooldown from previous timeout
        if (_timeoutStartTime > 0 && (now - _timeoutStartTime) < Config::Settings::LASER_COOLDOWN_MS) {
            return; // In cooldown period
        }

        // Safe to engage laser
        _state = LaserState::ACTIVE;
        _activationStartTime = now;
        setPhysicalOutput(true);
    }
}

void LaserController::update() {
    uint32_t now = millis();

    // 1. Hard Timeout Enforcement
    if (_state == LaserState::ACTIVE) {
        if ((now - _activationStartTime) >= Config::Settings::LASER_MAX_ON_TIME_MS) {
            _state = LaserState::TIMED_OUT;
            _timeoutStartTime = now;
            setPhysicalOutput(false);
        }
    }

    // 2. Interlock Enforcement
    if (_interlockInhibited && _state == LaserState::ACTIVE) {
        _state = LaserState::INTERLOCK_INHIBITED;
        setPhysicalOutput(false);
    }

    // 3. Timeout Cooldown Recovery
    if (_state == LaserState::TIMED_OUT && !_gestureRequested) {
        if ((now - _timeoutStartTime) >= Config::Settings::LASER_COOLDOWN_MS) {
            _state = LaserState::OFF;
            _timeoutStartTime = 0;
        }
    }

    // 4. Hardware Safety Audit
    // Confirm physical pin matches permitted logic state on every cycle
    bool shouldBeHigh = (_state == LaserState::ACTIVE && !_interlockInhibited);
    if (digitalRead(_pin) != (shouldBeHigh ? HIGH : LOW)) {
        setPhysicalOutput(shouldBeHigh);
    }
}

} // namespace Peripherals
