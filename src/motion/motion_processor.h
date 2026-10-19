/**
 * @file motion_processor.h
 * @brief Motion processing pipeline converting angular velocity into HID mouse displacement.
 */

#pragma once

#include <Arduino.h>
#include "../config/settings.h"

namespace Motion {

struct MouseDisplacement {
    int8_t dx;
    int8_t dy;
    bool hasMovement;
};

class MotionProcessor {
public:
    MotionProcessor();

    /**
     * @brief Reset filter states and fractional accumulators.
     */
    void reset();

    /**
     * @brief Process calibrated gyroscope rates into discrete mouse displacement.
     * @param gx Calibrated Gyro X in deg/s
     * @param gy Calibrated Gyro Y in deg/s
     * @param gz Calibrated Gyro Z in deg/s
     * @param dtSec Elapsed time since last process in seconds
     * @return MouseDisplacement containing clamped int8_t dx and dy
     */
    MouseDisplacement process(float gx, float gy, float gz, float dtSec);

    void setPaused(bool paused) {
        _isPaused = paused;
        if (paused) reset();
    }
    bool isPaused() const { return _isPaused; }

    void setSensitivity(float sensX, float sensY) {
        _sensX = sensX;
        _sensY = sensY;
    }

    void setDeadzone(float deadzoneDps) { _deadzone = deadzoneDps; }
    void setFilterAlpha(float alpha) { _filterAlpha = alpha; }

    void setInvertX(bool invert) { _invertX = invert; }
    void setInvertY(bool invert) { _invertY = invert; }
    bool isInvertX() const { return _invertX; }
    bool isInvertY() const { return _invertY; }
    void toggleInvertX() { _invertX = !_invertX; }
    void toggleInvertY() { _invertY = !_invertY; }

    float getFilteredRateX() const { return _filteredRateX; }
    float getFilteredRateY() const { return _filteredRateY; }

private:
    bool  _isPaused;
    bool  _invertX;
    bool  _invertY;
    float _sensX;
    float _sensY;
    float _deadzone;
    float _filterAlpha;

    // Filter memory
    float _filteredRateX;
    float _filteredRateY;

    // Sub-pixel fractional accumulators
    float _accumulatedX;
    float _accumulatedY;

    float applyDeadzone(float rate) const;
    float applyAcceleration(float rate) const;
};

} // namespace Motion
