/**
 * @file imu_calibration.h
 * @brief Stationary Gyroscope Bias Estimation and Motion Validation.
 */

#pragma once

#include <Arduino.h>
#include "imu_driver.h"
#include "../config/settings.h"

namespace Sensors {

enum class CalibrationStatus {
    IDLE,
    IN_PROGRESS,
    SUCCESS,
    FAILED_MOTION_DETECTED,
    FAILED_SENSOR_ERROR
};

struct GyroCalibrationResult {
    float offsetX;
    float offsetY;
    float offsetZ;
    float stdevX;
    float stdevY;
    float stdevZ;
    uint16_t sampleCount;
    CalibrationStatus status;
};

class IMUCalibration {
public:
    IMUCalibration();

    /**
     * @brief Start or restart calibration sequence.
     */
    void start();

    /**
     * @brief Non-blocking calibration step called on each new IMU reading.
     * @param rawData New reading from IMUDriver.
     * @return Current CalibrationStatus.
     */
    CalibrationStatus update(const IMUData& rawData);

    /**
     * @brief Synchronous helper for startup sequence with serial progress prints.
     * @param driver Initialized IMUDriver instance.
     * @param durationMs Desired calibration duration in milliseconds.
     * @return true if calibration succeeded with device held stationary.
     */
    bool runBlocking(IMUDriver& driver, uint32_t durationMs = Config::Settings::CALIBRATION_DURATION_MS);

    /**
     * @brief Apply calibrated zero-rate bias offsets to a raw gyro reading.
     */
    void apply(const IMUData& raw, float& outGx, float& outGy, float& outGz) const;

    const GyroCalibrationResult& getResult() const { return _result; }
    bool isCalibrated() const { return _isCalibrated; }
    uint8_t getProgressPercent() const;

private:
    GyroCalibrationResult _result;
    bool _isCalibrated;
    uint32_t _startTimeMs;
    uint32_t _targetDurationMs;

    // Running accumulators
    double _sumX;
    double _sumY;
    double _sumZ;
    double _sumSqX;
    double _sumSqY;
    double _sumSqZ;

    float _minX, _maxX;
    float _minY, _maxY;
    float _minZ, _maxZ;

    uint16_t _collectedSamples;

    void resetAccumulators();
    void finalizeCalculation();
};

} // namespace Sensors
