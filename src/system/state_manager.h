/**
 * @file state_manager.h
 * @brief Centralized System State Machine and Transition Controller.
 */

#pragma once

#include <Arduino.h>

namespace System {

enum class SystemState : uint8_t {
    INITIALIZING,
    CALIBRATING,
    READY,
    AIR_MOUSE_ACTIVE,
    AIR_MOUSE_PAUSED,
    SENSOR_ERROR,
    BLE_DISCONNECTED
};

typedef void (*StateChangeCallback)(SystemState oldState, SystemState newState);

class StateManager {
public:
    StateManager();

    void init();

    /**
     * @brief Transition to a new system state if valid.
     * @param newState Target state
     * @return true if transition was permitted and executed
     */
    bool transitionTo(SystemState newState);

    SystemState getState() const { return _currentState; }
    SystemState getPreviousState() const { return _previousState; }

    const char* getStateName(SystemState state) const;
    const char* getCurrentStateName() const { return getStateName(_currentState); }

    bool canEmitMouseReports() const {
        return (_currentState == SystemState::AIR_MOUSE_ACTIVE);
    }

    bool isLaserPermitted() const {
        return (_currentState == SystemState::READY ||
                _currentState == SystemState::AIR_MOUSE_ACTIVE ||
                _currentState == SystemState::AIR_MOUSE_PAUSED);
    }

    void setStateChangeCallback(StateChangeCallback cb) {
        _callback = cb;
    }

private:
    SystemState _currentState;
    SystemState _previousState;
    StateChangeCallback _callback;

    bool isValidTransition(SystemState from, SystemState to) const;
};

} // namespace System
