/**
 * @file state_manager.h
 * @brief Centralized System State Machine and Transition Controller.
 */

#pragma once

#include <Arduino.h>
#include "../config/settings.h"

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
typedef void (*ModeChangeCallback)(Config::Settings::OperatingMode oldMode, Config::Settings::OperatingMode newMode);

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

    Config::Settings::OperatingMode getOperatingMode() const { return _currentMode; }
    Config::Settings::OperatingMode cycleOperatingMode();
    bool setOperatingMode(Config::Settings::OperatingMode newMode);
    const char* getOperatingModeName(Config::Settings::OperatingMode mode) const;
    const char* getCurrentOperatingModeName() const { return getOperatingModeName(_currentMode); }

    bool canEmitMouseReports() const {
        return (_currentState == SystemState::AIR_MOUSE_ACTIVE &&
                _currentMode == Config::Settings::OperatingMode::AIR_MOUSE);
    }

    void setStateChangeCallback(StateChangeCallback cb) {
        _callback = cb;
    }

    void setModeChangeCallback(ModeChangeCallback cb) {
        _modeCallback = cb;
    }

private:
    SystemState _currentState;
    SystemState _previousState;
    Config::Settings::OperatingMode _currentMode;
    StateChangeCallback _callback;
    ModeChangeCallback _modeCallback;

    bool isValidTransition(SystemState from, SystemState to) const;
};

} // namespace System
