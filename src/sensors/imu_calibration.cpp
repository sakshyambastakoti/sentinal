/**
 * @file imu_calibration.cpp
 * @brief Implementation of Gyroscope Bias Estimation with Stationary Detection.
 */

#include "imu_calibration.h"
#include <cmath>

namespace Sensors {

IMUCalibration::IMUCalibration()
    : _isCalibrated(false),
      _startTimeMs(0),
      _targetDurationMs(Config::Settings::CALIBRATION_DURATION_MS),
      _collectedSamples(0) {
    _result.offsetX = 0.0f;
    _result.offsetY = 0.0f;
    _result.offsetZ = 0.0f;
    _result.stdevX  = 0.0f;
    _result.stdevY  = 0.0f;
    _result.stdevZ  = 0.0f;
    _result.sampleCount = 0;
    _result.status = CalibrationStatus::IDLE;
    resetAccumulators();
}

void IMUCalibration::resetAccumulators() {
    _sumX = 0.0;
    _sumY = 0.0;
    _sumZ = 0.0;
    _sumSqX = 0.0;
    _sumSqY = 0.0;
    _sumSqZ = 0.0;

    _minX = 1e6f;  _maxX = -1e6f;
    _minY = 1e6f;  _maxY = -1e6f;
    _minZ = 1e6f;  _maxZ = -1e6f;

    _collectedSamples = 0;
}

void IMUCalibration::start() {
    resetAccumulators();
    _startTimeMs = millis();
    _targetDurationMs = Config::Settings::CALIBRATION_DURATION_MS;
    _result.status = CalibrationStatus::IN_PROGRESS;
}

uint8_t IMUCalibration::getProgressPercent() const {
    if (_result.status != CalibrationStatus::IN_PROGRESS) {
        return (_result.status == CalibrationStatus::SUCCESS) ? 100 : 0;
    }
    uint32_t elapsed = millis() - _startTimeMs;
    if (elapsed >= _targetDurationMs) return 100;
    return static_cast<uint8_t>((elapsed * 100) / _targetDurationMs);
}

CalibrationStatus IMUCalibration::update(const IMUData& rawData) {
    if (_result.status != CalibrationStatus::IN_PROGRESS) {
        return _result.status;
    }

    if (!rawData.valid) {
        _result.status = CalibrationStatus::FAILED_SENSOR_ERROR;
        return _result.status;
    }

    // Accumulate sums and sums of squares
    _sumX += rawData.gyroX;
    _sumY += rawData.gyroY;
    _sumZ += rawData.gyroZ;

    _sumSqX += (rawData.gyroX * rawData.gyroX);
    _sumSqY += (rawData.gyroY * rawData.gyroY);
    _sumSqZ += (rawData.gyroZ * rawData.gyroZ);

    if (rawData.gyroX < _minX) _minX = rawData.gyroX;
    if (rawData.gyroX > _maxX) _maxX = rawData.gyroX;

    if (rawData.gyroY < _minY) _minY = rawData.gyroY;
    if (rawData.gyroY > _maxY) _maxY = rawData.gyroY;

    if (rawData.gyroZ < _minZ) _minZ = rawData.gyroZ;
    if (rawData.gyroZ > _maxZ) _maxZ = rawData.gyroZ;

    _collectedSamples++;

    // Check if calibration period has elapsed
    if ((millis() - _startTimeMs) >= _targetDurationMs) {
        finalizeCalculation();
    }

    return _result.status;
}

void IMUCalibration::finalizeCalculation() {
    if (_collectedSamples < 30) {
        _result.status = CalibrationStatus::FAILED_SENSOR_ERROR;
        return;
    }

    // Motion Detection: evaluate peak-to-peak variance during calibration
    float spanX = _maxX - _minX;
    float spanY = _maxY - _minY;
    float spanZ = _maxZ - _minZ;

    if (spanX > Config::Settings::CALIBRATION_MAX_MOTION_THRESHOLD_DPS ||
        spanY > Config::Settings::CALIBRATION_MAX_MOTION_THRESHOLD_DPS ||
        spanZ > Config::Settings::CALIBRATION_MAX_MOTION_THRESHOLD_DPS) {
        // Device moved during calibration!
        _result.status = CalibrationStatus::FAILED_MOTION_DETECTED;
        return;
    }

    // Calculate mean offsets
    double count = static_cast<double>(_collectedSamples);
    _result.offsetX = static_cast<float>(_sumX / count);
    _result.offsetY = static_cast<float>(_sumY / count);
    _result.offsetZ = static_cast<float>(_sumZ / count);

    // Calculate standard deviations
    double varX = (_sumSqX / count) - (_result.offsetX * _result.offsetX);
    double varY = (_sumSqY / count) - (_result.offsetY * _result.offsetY);
    double varZ = (_sumSqZ / count) - (_result.offsetZ * _result.offsetZ);

    _result.stdevX = (varX > 0.0) ? static_cast<float>(sqrt(varX)) : 0.0f;
    _result.stdevY = (varY > 0.0) ? static_cast<float>(sqrt(varY)) : 0.0f;
    _result.stdevZ = (varZ > 0.0) ? static_cast<float>(sqrt(varZ)) : 0.0f;

    _result.sampleCount = _collectedSamples;
    _result.status = CalibrationStatus::SUCCESS;
    _isCalibrated = true;
}

bool IMUCalibration::runBlocking(IMUDriver& driver, uint32_t durationMs) {
    _targetDurationMs = durationMs;
    start();

    Serial.println("\n[CALIBRATION] Starting Gyroscope Calibration...");
    Serial.println("[CALIBRATION] >> PLEASE KEEP DEVICE PERFECTLY STILL ON A FLAT SURFACE <<");

    uint32_t lastPrint = millis();
    IMUData sample;

    while (_result.status == CalibrationStatus::IN_PROGRESS) {
        uint32_t now = millis();
        if (driver.readSensor(sample)) {
            update(sample);
        }

        if (now - lastPrint >= 500) {
            lastPrint = now;
            Serial.printf("[CALIBRATION] Progress: %u%%\n", getProgressPercent());
        }
        delay(10);
    }

    if (_result.status == CalibrationStatus::SUCCESS) {
        Serial.printf("[CALIBRATION] SUCCESS! Samples: %u\n", _result.sampleCount);
        Serial.printf("[CALIBRATION] Gyro Offsets: X=%+.2f, Y=%+.2f, Z=%+.2f deg/s\n",
                      _result.offsetX, _result.offsetY, _result.offsetZ);
        Serial.printf("[CALIBRATION] Noise (StdDev): X=%.3f, Y=%.3f, Z=%.3f deg/s\n\n",
                      _result.stdevX, _result.stdevY, _result.stdevZ);
        return true;
    } else if (_result.status == CalibrationStatus::FAILED_MOTION_DETECTED) {
        Serial.println("[CALIBRATION] FAILED: Motion detected! The device was disturbed during calibration.");
        Serial.println("[CALIBRATION] Retaining default or previous calibration offsets.\n");
        return false;
    } else {
        Serial.println("[CALIBRATION] FAILED: Sensor communication error during calibration.\n");
        return false;
    }
}

void IMUCalibration::apply(const IMUData& raw, float& outGx, float& outGy, float& outGz) const {
    if (_isCalibrated) {
        outGx = raw.gyroX - _result.offsetX;
        outGy = raw.gyroY - _result.offsetY;
        outGz = raw.gyroZ - _result.offsetZ;
    } else {
        outGx = raw.gyroX;
        outGy = raw.gyroY;
        outGz = raw.gyroZ;
    }
}

} // namespace Sensors
