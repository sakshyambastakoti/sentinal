/**
 * @file imu_driver.h
 * @brief High-reliability direct I2C driver for InvenSense MPU6050 6-axis IMU.
 */

#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "../config/pins.h"
#include "../config/settings.h"

namespace Sensors {

struct IMUData {
    float accelX;      // Acceleration in g (X axis)
    float accelY;      // Acceleration in g (Y axis)
    float accelZ;      // Acceleration in g (Z axis)
    float gyroX;       // Angular velocity in deg/s (X axis)
    float gyroY;       // Angular velocity in deg/s (Y axis)
    float gyroZ;       // Angular velocity in deg/s (Z axis)
    float temperature; // Die temperature in degrees Celsius
    uint32_t timestampMs;
    bool  valid;
};

class IMUDriver {
public:
    IMUDriver();

    /**
     * @brief Initialize I2C bus and configure MPU6050.
     * Probes both primary (0x68) and secondary (0x69) addresses.
     * @return true if sensor detected, validated, and configured successfully.
     */
    bool init(uint8_t sdaPin = Config::Pins::I2C_SDA,
              uint8_t sclPin = Config::Pins::I2C_SCL,
              uint32_t frequency = Config::Settings::I2C_CLOCK_SPEED_HZ);

    /**
     * @brief Performs a fast 14-byte burst read of accelerometer, temperature, and gyro.
     * @param data Output data structure populated with converted physical units.
     * @return true if read completed successfully without I2C error.
     */
    bool readSensor(IMUData& data);

    /**
     * @brief Attempt bus recovery if I2C bus hangs (e.g. slave holds SDA low).
     */
    bool recoverBus();

    uint8_t  getDetectedAddress() const { return _detectedAddress; }
    uint8_t  getWhoAmI() const { return _whoAmI; }
    uint32_t getErrorCount() const { return _errorCount; }
    bool     isHealthy() const { return _isInitialized && (_consecutiveErrors < 3); }

private:
    // MPU6050 Register Map
    static constexpr uint8_t REG_SMPLRT_DIV   = 0x19;
    static constexpr uint8_t REG_CONFIG       = 0x1A;
    static constexpr uint8_t REG_GYRO_CONFIG  = 0x1B;
    static constexpr uint8_t REG_ACCEL_CONFIG = 0x1C;
    static constexpr uint8_t REG_ACCEL_XOUT_H = 0x3B;
    static constexpr uint8_t REG_PWR_MGMT_1   = 0x6B;
    static constexpr uint8_t REG_PWR_MGMT_2   = 0x6C;
    static constexpr uint8_t REG_WHO_AM_I     = 0x75;

    uint8_t  _sdaPin;
    uint8_t  _sclPin;
    uint32_t _frequency;
    uint8_t  _detectedAddress;
    uint8_t  _whoAmI;
    bool     _isInitialized;
    uint32_t _errorCount;
    uint32_t _consecutiveErrors;

    bool probeAddress(uint8_t address);
    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegisters(uint8_t reg, uint8_t* buffer, size_t length);
};

} // namespace Sensors
