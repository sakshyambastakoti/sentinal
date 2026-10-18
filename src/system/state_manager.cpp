/**
 * @file state_manager.cpp
 * @brief Implementation of State Manager transitions and validation.
 */

#include "state_manager.h"

namespace System {

StateManager::StateManager()
    : _currentState(SystemState::INITIALIZING),
      _previousState(SystemState::INITIALIZING),
      _callback(nullptr) {}

void StateManager::init() {
    _currentState = SystemState::INITIALIZING;
    _previousState = SystemState::INITIALIZING;
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

} // namespace System
