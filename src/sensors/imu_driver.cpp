/**
 * @file imu_driver.cpp
 * @brief Implementation of high-reliability MPU6050 I2C driver.
 */

#include "imu_driver.h"

namespace Sensors {

IMUDriver::IMUDriver()
    : _sdaPin(Config::Pins::I2C_SDA),
      _sclPin(Config::Pins::I2C_SCL),
      _frequency(Config::Settings::I2C_CLOCK_SPEED_HZ),
      _detectedAddress(0),
      _whoAmI(0),
      _isInitialized(false),
      _errorCount(0),
      _consecutiveErrors(0) {}

bool IMUDriver::probeAddress(uint8_t address) {
    Wire.beginTransmission(address);
    return (Wire.endTransmission() == 0);
}

bool IMUDriver::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(_detectedAddress);
    Wire.write(reg);
    Wire.write(value);
    uint8_t status = Wire.endTransmission();
    if (status != 0) {
        _errorCount++;
        _consecutiveErrors++;
        return false;
    }
    _consecutiveErrors = 0;
    return true;
}

bool IMUDriver::readRegisters(uint8_t reg, uint8_t* buffer, size_t length) {
    Wire.beginTransmission(_detectedAddress);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) { // Repeated start
        _errorCount++;
        _consecutiveErrors++;
        return false;
    }

    size_t bytesRead = Wire.requestFrom(_detectedAddress, static_cast<uint8_t>(length));
    if (bytesRead != length) {
        _errorCount++;
        _consecutiveErrors++;
        return false;
    }

    for (size_t i = 0; i < length; ++i) {
        buffer[i] = Wire.read();
    }

    _consecutiveErrors = 0;
    return true;
}

bool IMUDriver::init(uint8_t sdaPin, uint8_t sclPin, uint32_t frequency) {
    _sdaPin = sdaPin;
    _sclPin = sclPin;
    _frequency = frequency;
    _isInitialized = false;
    _consecutiveErrors = 0;

    // Start I2C peripheral
    if (!Wire.begin(_sdaPin, _sclPin, _frequency)) {
        return false;
    }
    Wire.setTimeOut(Config::Settings::I2C_TIMEOUT_MS);

    // Probe primary (0x68) and secondary (0x69) addresses
    if (probeAddress(Config::Settings::MPU6050_ADDR_PRIMARY)) {
        _detectedAddress = Config::Settings::MPU6050_ADDR_PRIMARY;
    } else if (probeAddress(Config::Settings::MPU6050_ADDR_SECONDARY)) {
        _detectedAddress = Config::Settings::MPU6050_ADDR_SECONDARY;
    } else {
        // Neither address responded; attempt bus recovery and retry once
        recoverBus();
        Wire.begin(_sdaPin, _sclPin, _frequency);
        if (probeAddress(Config::Settings::MPU6050_ADDR_PRIMARY)) {
            _detectedAddress = Config::Settings::MPU6050_ADDR_PRIMARY;
        } else if (probeAddress(Config::Settings::MPU6050_ADDR_SECONDARY)) {
            _detectedAddress = Config::Settings::MPU6050_ADDR_SECONDARY;
        } else {
            return false;
        }
    }

    // Read WHO_AM_I register (0x75)
    uint8_t whoAmIVal = 0;
    if (!readRegisters(REG_WHO_AM_I, &whoAmIVal, 1)) {
        return false;
    }
    _whoAmI = whoAmIVal;

    // Reset device to default register state
    writeRegister(REG_PWR_MGMT_1, 0x80);
    delay(100);

    // Wake device from sleep and select PLL with X-axis gyroscope clock source
    // (Recommended by InvenSense for optimal clock stability over internal 8MHz RC)
    if (!writeRegister(REG_PWR_MGMT_1, 0x01)) {
        return false;
    }
    delay(15);

    // Enable all 6 axes (Accel X/Y/Z and Gyro X/Y/Z)
    if (!writeRegister(REG_PWR_MGMT_2, 0x00)) {
        return false;
    }

    // Configure Digital Low Pass Filter (DLPF)
    // 0x02 = ~98Hz bandwidth, 2.8ms delay (great trade-off between anti-aliasing and responsiveness)
    if (!writeRegister(REG_CONFIG, Config::Settings::MPU6050_DLPF_CONFIG)) {
        return false;
    }

    // Set sample rate divider (1 kHz / (1 + 0) = 1 kHz internal sampling rate)
    if (!writeRegister(REG_SMPLRT_DIV, 0x00)) {
        return false;
    }

    // Set Gyroscope Full Scale Range to +/- 250 deg/s (FS_SEL = 0)
    if (!writeRegister(REG_GYRO_CONFIG, 0x00)) {
        return false;
    }

    // Set Accelerometer Full Scale Range to +/- 2g (AFS_SEL = 0)
    if (!writeRegister(REG_ACCEL_CONFIG, 0x00)) {
        return false;
    }

    _isInitialized = true;
    return true;
}

bool IMUDriver::readSensor(IMUData& data) {
    data.valid = false;
    data.timestampMs = millis();

    if (!_isInitialized) {
        return false;
    }

    uint8_t rawBuffer[14];
    if (!readRegisters(REG_ACCEL_XOUT_H, rawBuffer, 14)) {
        // If 3 consecutive errors occur, attempt recovery
        if (_consecutiveErrors >= 3) {
            recoverBus();
            Wire.begin(_sdaPin, _sclPin, _frequency);
        }
        return false;
    }

    // Parse Accelerometer data (big-endian 16-bit signed)
    int16_t rawAccelX = static_cast<int16_t>((rawBuffer[0] << 8) | rawBuffer[1]);
    int16_t rawAccelY = static_cast<int16_t>((rawBuffer[2] << 8) | rawBuffer[3]);
    int16_t rawAccelZ = static_cast<int16_t>((rawBuffer[4] << 8) | rawBuffer[5]);

    // Parse Temperature (big-endian 16-bit signed)
    int16_t rawTemp = static_cast<int16_t>((rawBuffer[6] << 8) | rawBuffer[7]);

    // Parse Gyroscope data (big-endian 16-bit signed)
    int16_t rawGyroX = static_cast<int16_t>((rawBuffer[8] << 8) | rawBuffer[9]);
    int16_t rawGyroY = static_cast<int16_t>((rawBuffer[10] << 8) | rawBuffer[11]);
    int16_t rawGyroZ = static_cast<int16_t>((rawBuffer[12] << 8) | rawBuffer[13]);

    // Convert to engineering units
    data.accelX = static_cast<float>(rawAccelX) / Config::Settings::ACCEL_SCALE_2G;
    data.accelY = static_cast<float>(rawAccelY) / Config::Settings::ACCEL_SCALE_2G;
    data.accelZ = static_cast<float>(rawAccelZ) / Config::Settings::ACCEL_SCALE_2G;

    // Formula from InvenSense MPU6050 datasheet: Temperature in °C = (TEMP_OUT / 340) + 36.53
    data.temperature = (static_cast<float>(rawTemp) / 340.0f) + 36.53f;

    data.gyroX = static_cast<float>(rawGyroX) / Config::Settings::GYRO_SCALE_250DPS;
    data.gyroY = static_cast<float>(rawGyroY) / Config::Settings::GYRO_SCALE_250DPS;
    data.gyroZ = static_cast<float>(rawGyroZ) / Config::Settings::GYRO_SCALE_250DPS;

    data.valid = true;
    return true;
}

bool IMUDriver::recoverBus() {
    // Release I2C hardware control
    Wire.end();

    pinMode(_sdaPin, INPUT_PULLUP);
    pinMode(_sclPin, OUTPUT_OPEN_DRAIN);

    // Toggle SCL clock line up to 16 times to flush hung slave state machines
    for (int i = 0; i < 16; ++i) {
        digitalWrite(_sclPin, LOW);
        delayMicroseconds(5);
        digitalWrite(_sclPin, HIGH);
        delayMicroseconds(5);

        // Check if slave released SDA
        if (digitalRead(_sdaPin) == HIGH) {
            break;
        }
    }

    // Generate STOP condition: SDA transitions from LOW to HIGH while SCL is HIGH
    pinMode(_sdaPin, OUTPUT_OPEN_DRAIN);
    digitalWrite(_sdaPin, LOW);
    delayMicroseconds(5);
    digitalWrite(_sclPin, HIGH);
    delayMicroseconds(5);
    digitalWrite(_sdaPin, HIGH);
    delayMicroseconds(5);

    return true;
}

} // namespace Sensors
