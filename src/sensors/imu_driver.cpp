/**
 * @file imu_driver.cpp
 * @brief Implementation of high-reliability MPU6050 I2C driver with bus scanner.
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

void IMUDriver::scanBus() {
    Serial.println("[I2C_SCAN] Scanning I2C bus for responding devices...");
    uint8_t count = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("[I2C_SCAN] Device found at address 0x%02X\n", addr);
            count++;
        }
    }
    if (count == 0) {
        Serial.println("[I2C_SCAN] No I2C devices found! Check wiring, 3.3V power, and GND.");
    } else {
        Serial.printf("[I2C_SCAN] Total devices found: %u\n", count);
    }
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
    if (Wire.endTransmission(false) != 0) {
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
        // Neither address responded; attempt bus recovery and scan
        recoverBus();
        Wire.begin(_sdaPin, _sclPin, _frequency);
        if (probeAddress(Config::Settings::MPU6050_ADDR_PRIMARY)) {
            _detectedAddress = Config::Settings::MPU6050_ADDR_PRIMARY;
        } else if (probeAddress(Config::Settings::MPU6050_ADDR_SECONDARY)) {
            _detectedAddress = Config::Settings::MPU6050_ADDR_SECONDARY;
        } else {
            scanBus();
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
    if (!writeRegister(REG_PWR_MGMT_1, 0x01)) {
        return false;
    }
    delay(15);

    // Enable all 6 axes
    if (!writeRegister(REG_PWR_MGMT_2, 0x00)) {
        return false;
    }

    // Configure Digital Low Pass Filter (DLPF)
    if (!writeRegister(REG_CONFIG, Config::Settings::MPU6050_DLPF_CONFIG)) {
        return false;
    }

    // Set sample rate divider (1 kHz)
    if (!writeRegister(REG_SMPLRT_DIV, 0x00)) {
        return false;
    }

    // Set Gyroscope Full Scale Range to +/- 250 deg/s
    if (!writeRegister(REG_GYRO_CONFIG, 0x00)) {
        return false;
    }

    // Set Accelerometer Full Scale Range to +/- 2g
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
        if (_consecutiveErrors >= 3) {
            recoverBus();
            Wire.begin(_sdaPin, _sclPin, _frequency);
        }
        return false;
    }

    int16_t rawAccelX = static_cast<int16_t>((rawBuffer[0] << 8) | rawBuffer[1]);
    int16_t rawAccelY = static_cast<int16_t>((rawBuffer[2] << 8) | rawBuffer[3]);
    int16_t rawAccelZ = static_cast<int16_t>((rawBuffer[4] << 8) | rawBuffer[5]);

    int16_t rawTemp = static_cast<int16_t>((rawBuffer[6] << 8) | rawBuffer[7]);

    int16_t rawGyroX = static_cast<int16_t>((rawBuffer[8] << 8) | rawBuffer[9]);
    int16_t rawGyroY = static_cast<int16_t>((rawBuffer[10] << 8) | rawBuffer[11]);
    int16_t rawGyroZ = static_cast<int16_t>((rawBuffer[12] << 8) | rawBuffer[13]);

    data.accelX = static_cast<float>(rawAccelX) / Config::Settings::ACCEL_SCALE_2G;
    data.accelY = static_cast<float>(rawAccelY) / Config::Settings::ACCEL_SCALE_2G;
    data.accelZ = static_cast<float>(rawAccelZ) / Config::Settings::ACCEL_SCALE_2G;

    data.temperature = (static_cast<float>(rawTemp) / 340.0f) + 36.53f;

    data.gyroX = static_cast<float>(rawGyroX) / Config::Settings::GYRO_SCALE_250DPS;
    data.gyroY = static_cast<float>(rawGyroY) / Config::Settings::GYRO_SCALE_250DPS;
    data.gyroZ = static_cast<float>(rawGyroZ) / Config::Settings::GYRO_SCALE_250DPS;

    data.valid = true;
    return true;
}

bool IMUDriver::recoverBus() {
    Wire.end();

    pinMode(_sdaPin, INPUT_PULLUP);
    pinMode(_sclPin, OUTPUT_OPEN_DRAIN);

    for (int i = 0; i < 16; ++i) {
        digitalWrite(_sclPin, LOW);
        delayMicroseconds(5);
        digitalWrite(_sclPin, HIGH);
        delayMicroseconds(5);

        if (digitalRead(_sdaPin) == HIGH) {
            break;
        }
    }

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
