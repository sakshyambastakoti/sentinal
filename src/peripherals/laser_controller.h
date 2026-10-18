/**
 * @file laser_controller.h
 * @brief Safety-First Dedicated Laser Diode Driver Controller.
 */

#pragma once

#include <Arduino.h>
#include "../config/pins.h"
#include "../config/settings.h"

namespace Peripherals {

enum class LaserState {
    OFF,
    ACTIVE,
    TIMED_OUT,
    INTERLOCK_INHIBITED
};

class LaserController {
public:
    LaserController();

    /**
     * @brief Initialize hardware pin and enforce safe default OFF state.
     */
    void init(uint8_t pin = Config::Pins::LASER_DRIVER);

    /**
     * @brief Request laser state based on user gesture.
     * @param active true if activation gesture is currently held, false if released.
     */
    void requestLaser(bool active);

    /**
     * @brief Continuous non-blocking safety audit loop called on every cycle.
     * Enforces hard timeouts, interlocks, and hardware state synchronization.
     */
    void update();

    /**
     * @brief Set system interlock to disable laser unconditionally during
     * disconnection, calibration, or sensor fault states.
     */
    void setInterlock(bool inhibited);

    /**
     * @brief Emergency shutdown: immediately forces hardware pin LOW and resets state.
     */
    void emergencyShutdown();

    bool isLaserActive() const { return _state == LaserState::ACTIVE; }
    LaserState getState() const { return _state; }
    uint32_t getActiveDurationMs() const;

private:
    uint8_t    _pin;
    LaserState _state;
    bool       _gestureRequested;
    bool       _interlockInhibited;
    uint32_t   _activationStartTime;
    uint32_t   _timeoutStartTime;

    void setPhysicalOutput(bool high);
};

} // namespace Peripherals
