/**
 * @file motion_processor.cpp
 * @brief Implementation of motion processing pipeline.
 */

#include "motion_processor.h"
#include <cmath>

namespace Motion {

MotionProcessor::MotionProcessor()
    : _isPaused(false),
      _sensX(Config::Settings::SENSITIVITY_X),
      _sensY(Config::Settings::SENSITIVITY_Y),
      _deadzone(Config::Settings::GYRO_DEADZONE_DPS),
      _filterAlpha(Config::Settings::FILTER_ALPHA),
      _filteredRateX(0.0f),
      _filteredRateY(0.0f),
      _accumulatedX(0.0f),
      _accumulatedY(0.0f) {}

void MotionProcessor::reset() {
    _filteredRateX = 0.0f;
    _filteredRateY = 0.0f;
    _accumulatedX = 0.0f;
    _accumulatedY = 0.0f;
}

float MotionProcessor::applyDeadzone(float rate) const {
    float absRate = std::fabs(rate);
    if (absRate <= _deadzone) {
        return 0.0f;
    }
    // Continuous slope out of deadzone (preserves smooth control without jump)
    float sign = (rate > 0.0f) ? 1.0f : -1.0f;
    return sign * (absRate - _deadzone);
}

float MotionProcessor::applyAcceleration(float rate) const {
    if (!Config::Settings::ACCEL_ENABLED) {
        return 1.0f;
    }

    float absRate = std::fabs(rate);
    if (absRate <= Config::Settings::ACCEL_THRESHOLD) {
        return 1.0f;
    }

    float excess = absRate - Config::Settings::ACCEL_THRESHOLD;
    float multiplier = 1.0f + (Config::Settings::ACCEL_GAIN * excess);
    if (multiplier > Config::Settings::ACCEL_MAX_SCALE) {
        multiplier = Config::Settings::ACCEL_MAX_SCALE;
    }
    return multiplier;
}

MouseDisplacement MotionProcessor::process(float gx, float gy, float gz, float dtSec) {
    MouseDisplacement output = {0, 0, false};

    if (_isPaused) {
        reset();
        return output;
    }

    // 1. Coordinate Axis Mapping
    // Map yaw/pitch axes according to configured physical orientation
    float rawRateX = 0.0f;
    switch (Config::Settings::AXIS_MAP_X) {
        case Config::Settings::AXIS_X: rawRateX = gx; break;
        case Config::Settings::AXIS_Y: rawRateX = gy; break;
        case Config::Settings::AXIS_Z: rawRateX = gz; break;
    }
    if (Config::Settings::INVERT_X) {
        rawRateX = -rawRateX;
    }

    float rawRateY = 0.0f;
    switch (Config::Settings::AXIS_MAP_Y) {
        case Config::Settings::AXIS_X: rawRateY = gx; break;
        case Config::Settings::AXIS_Y: rawRateY = gy; break;
        case Config::Settings::AXIS_Z: rawRateY = gz; break;
    }
    if (Config::Settings::INVERT_Y) {
        rawRateY = -rawRateY;
    }

    // 2. Dead Zone Suppression
    float cleanRateX = applyDeadzone(rawRateX);
    float cleanRateY = applyDeadzone(rawRateY);

    // 3. Low-Pass Smoothing Filter (Exponential Moving Average)
    _filteredRateX = (_filterAlpha * cleanRateX) + ((1.0f - _filterAlpha) * _filteredRateX);
    _filteredRateY = (_filterAlpha * cleanRateY) + ((1.0f - _filterAlpha) * _filteredRateY);

    // 4. Cursor Acceleration Curve
    float totalSpeed = std::sqrt((_filteredRateX * _filteredRateX) + (_filteredRateY * _filteredRateY));
    float accelMultiplier = applyAcceleration(totalSpeed);

    // 5. Sensitivity and Time Scaling to Pixel Displacements
    float dispX = _filteredRateX * _sensX * accelMultiplier;
    float dispY = _filteredRateY * _sensY * accelMultiplier;

    // 6. Sub-Pixel Fractional Accumulation
    _accumulatedX += dispX;
    _accumulatedY += dispY;

    int16_t stepX = static_cast<int16_t>(_accumulatedX);
    int16_t stepY = static_cast<int16_t>(_accumulatedY);

    _accumulatedX -= static_cast<float>(stepX);
    _accumulatedY -= static_cast<float>(stepY);

    // 7. Clamping to valid 8-bit signed HID bounds [-127, 127]
    if (stepX > Config::Settings::HID_REPORT_MAX) stepX = Config::Settings::HID_REPORT_MAX;
    if (stepX < Config::Settings::HID_REPORT_MIN) stepX = Config::Settings::HID_REPORT_MIN;

    if (stepY > Config::Settings::HID_REPORT_MAX) stepY = Config::Settings::HID_REPORT_MAX;
    if (stepY < Config::Settings::HID_REPORT_MIN) stepY = Config::Settings::HID_REPORT_MIN;

    output.dx = static_cast<int8_t>(stepX);
    output.dy = static_cast<int8_t>(stepY);
    output.hasMovement = (output.dx != 0 || output.dy != 0);

    return output;
}

} // namespace Motion
