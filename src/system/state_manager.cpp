/**
 * @file state_manager.cpp
 * @brief Implementation of State Manager transitions and validation.
 */

#include "state_manager.h"

namespace System {

StateManager::StateManager()
    : _currentState(SystemState::INITIALIZING),
      _previousState(SystemState::INITIALIZING),
      _currentMode(Config::Settings::OperatingMode::AIR_MOUSE),
      _callback(nullptr),
      _modeCallback(nullptr) {}

void StateManager::init() {
    _currentState = SystemState::INITIALIZING;
    _previousState = SystemState::INITIALIZING;
    _currentMode = Config::Settings::OperatingMode::AIR_MOUSE;
}

const char* StateManager::getStateName(SystemState state) const {
    switch (state) {
        case SystemState::INITIALIZING:     return "INITIALIZING";
        case SystemState::CALIBRATING:      return "CALIBRATING";
        case SystemState::READY:            return "READY";
        case SystemState::AIR_MOUSE_ACTIVE: return "AIR_MOUSE_ACTIVE";
        case SystemState::AIR_MOUSE_PAUSED: return "AIR_MOUSE_PAUSED";
        case SystemState::SENSOR_ERROR:     return "SENSOR_ERROR";
        case SystemState::BLE_DISCONNECTED: return "BLE_DISCONNECTED";
        default:                            return "UNKNOWN";
    }
}

bool StateManager::isValidTransition(SystemState from, SystemState to) const {
    if (from == to) return true;

    switch (from) {
        case SystemState::INITIALIZING:
            return (to == SystemState::CALIBRATING ||
                    to == SystemState::SENSOR_ERROR ||
                    to == SystemState::BLE_DISCONNECTED);

        case SystemState::CALIBRATING:
            return (to == SystemState::READY ||
                    to == SystemState::AIR_MOUSE_ACTIVE ||
                    to == SystemState::BLE_DISCONNECTED ||
                    to == SystemState::SENSOR_ERROR);

        case SystemState::READY:
            return (to == SystemState::AIR_MOUSE_ACTIVE ||
                    to == SystemState::AIR_MOUSE_PAUSED ||
                    to == SystemState::CALIBRATING ||
                    to == SystemState::BLE_DISCONNECTED ||
                    to == SystemState::SENSOR_ERROR);

        case SystemState::AIR_MOUSE_ACTIVE:
            return (to == SystemState::AIR_MOUSE_PAUSED ||
                    to == SystemState::CALIBRATING ||
                    to == SystemState::BLE_DISCONNECTED ||
                    to == SystemState::SENSOR_ERROR ||
                    to == SystemState::READY);

        case SystemState::AIR_MOUSE_PAUSED:
            return (to == SystemState::AIR_MOUSE_ACTIVE ||
                    to == SystemState::CALIBRATING ||
                    to == SystemState::BLE_DISCONNECTED ||
                    to == SystemState::SENSOR_ERROR ||
                    to == SystemState::READY);

        case SystemState::BLE_DISCONNECTED:
            return (to == SystemState::READY ||
                    to == SystemState::AIR_MOUSE_ACTIVE ||
                    to == SystemState::AIR_MOUSE_PAUSED ||
                    to == SystemState::CALIBRATING ||
                    to == SystemState::SENSOR_ERROR);

        case SystemState::SENSOR_ERROR:
            // Allow recovery attempts to re-initialize or recalibrate
            return (to == SystemState::INITIALIZING ||
                    to == SystemState::CALIBRATING ||
                    to == SystemState::READY ||
                    to == SystemState::BLE_DISCONNECTED);

        default:
            return false;
    }
}

bool StateManager::transitionTo(SystemState newState) {
    if (_currentState == newState) {
        return true;
    }

    if (!isValidTransition(_currentState, newState)) {
        return false;
    }

    _previousState = _currentState;
    _currentState = newState;

    if (_callback) {
        _callback(_previousState, _currentState);
    }

    return true;
}

Config::Settings::OperatingMode StateManager::cycleOperatingMode() {
    uint8_t next = static_cast<uint8_t>(_currentMode) + 1;
    if (next > static_cast<uint8_t>(Config::Settings::OperatingMode::REEL_SCROLLER)) {
        next = static_cast<uint8_t>(Config::Settings::OperatingMode::AIR_MOUSE);
    }
    setOperatingMode(static_cast<Config::Settings::OperatingMode>(next));
    return _currentMode;
}

bool StateManager::setOperatingMode(Config::Settings::OperatingMode newMode) {
    if (_currentMode == newMode) {
        return true;
    }
    Config::Settings::OperatingMode oldMode = _currentMode;
    _currentMode = newMode;
    if (_modeCallback) {
        _modeCallback(oldMode, _currentMode);
    }
    return true;
}

const char* StateManager::getOperatingModeName(Config::Settings::OperatingMode mode) const {
    switch (mode) {
        case Config::Settings::OperatingMode::AIR_MOUSE:
            return "AIR_MOUSE (Mode 1: 6-Axis Desktop Air Mouse)";
        case Config::Settings::OperatingMode::PRESENTATION:
            return "PRESENTATION (Mode 2: Presentation Remote Slide Clicker)";
        case Config::Settings::OperatingMode::REEL_SCROLLER:
            return "REEL_SCROLLER (Mode 3: Social Reel / Shorts Scroller)";
        default:
            return "UNKNOWN_MODE";
    }
}

} // namespace System
