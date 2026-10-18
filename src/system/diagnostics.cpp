/**
 * @file diagnostics.cpp
 * @brief Implementation of USB Serial Diagnostics and Telemetry.
 */

#include "diagnostics.h"

namespace System {

Diagnostics::Diagnostics()
    : _debugStreaming(false),
      _lastHeartbeatMs(0) {}

void Diagnostics::init(uint32_t baudRate) {
    Serial.begin(baudRate);
    // On ESP32-S3 Native USB CDC, allow a brief window for terminal connection
    uint32_t start = millis();
    while (!Serial && (millis() - start < 1500)) {
        delay(10);
    }
}

void Diagnostics::printStartupBanner() {
    Serial.println("\n=======================================================");
    Serial.printf("   %s (v%s)\n", Config::Settings::FIRMWARE_NAME, Config::Settings::FIRMWARE_VERSION);
    Serial.println("   Target Board: ESP32-S3 Super Mini");
    Serial.println("=======================================================");
    Serial.printf("MCU:           ESP32-S3 (Rev %d) @ %u MHz\n", ESP.getChipRevision(), ESP.getCpuFreqMHz());
    Serial.printf("Flash Size:    %u KB\n", ESP.getFlashChipSize() / 1024);
    Serial.printf("Free Heap:     %u Bytes\n", ESP.getFreeHeap());
    Serial.printf("Pins:          SDA: GPIO%u | SCL: GPIO%u\n", Config::Pins::I2C_SDA, Config::Pins::I2C_SCL);
    Serial.printf("               Btn A: GPIO%u | Btn B: GPIO%u | Laser: GPIO%u\n",
                  Config::Pins::BUTTON_A, Config::Pins::BUTTON_B, Config::Pins::LASER_DRIVER);
    Serial.println("Type 'd' to toggle motion debug stream; 'r' to recalibrate.");
    Serial.println("=======================================================\n");
}

void Diagnostics::printI2CScanResult(uint8_t address, uint8_t whoAmI) {
    Serial.printf("[I2C] MPU6050 detected at address: 0x%02X\n", address);
    Serial.printf("[I2C] WHO_AM_I register returned:  0x%02X ", whoAmI);
    if (whoAmI == 0x68) {
        Serial.println("(Genuine InvenSense MPU6050)");
    } else {
        Serial.println("(Compatible MPU6050 clone / variant)");
    }
}

void Diagnostics::printCalibrationResult(const Sensors::GyroCalibrationResult& result) {
    if (result.status == Sensors::CalibrationStatus::SUCCESS) {
        Serial.println("[CALIB] Gyroscope calibration successful:");
        Serial.printf("        Offsets: X=%+.2f, Y=%+.2f, Z=%+.2f deg/s\n",
                      result.offsetX, result.offsetY, result.offsetZ);
        Serial.printf("        StdDev:  X=%.3f, Y=%.3f, Z=%.3f deg/s\n",
                      result.stdevX, result.stdevY, result.stdevZ);
    } else {
        Serial.println("[CALIB] Warning: Calibration failed. Motion detected or sensor error.");
    }
}

void Diagnostics::printBLEStatus(bool advertising, bool connected) {
    if (connected) {
        Serial.println("[BLE] Device connected to Host PC! HID reports active.");
    } else if (advertising) {
        Serial.printf("[BLE] Advertising as '%s'. Awaiting pairing/connection...\n",
                      Config::Settings::BLE_DEVICE_NAME);
    } else {
        Serial.println("[BLE] BLE HID stack offline or stopped.");
    }
}

void Diagnostics::printButtonEvent(Input::ButtonEvent event) {
    switch (event) {
        case Input::ButtonEvent::BUTTON_A_SHORT_PRESS:
            Serial.println("[INPUT] Button A Short Press: Left Click");
            break;
        case Input::ButtonEvent::BUTTON_B_SHORT_PRESS:
            Serial.println("[INPUT] Button B Short Press: Right Click");
            break;
        case Input::ButtonEvent::BUTTON_A_LONG_PRESS:
            Serial.println("[INPUT] Button A Long Press: Toggle Motion Pause/Resume");
            break;
        case Input::ButtonEvent::BUTTON_B_LONG_PRESS:
            Serial.println("[INPUT] Button B Long Press: Trigger Gyro Recalibration");
            break;
        case Input::ButtonEvent::SIMULTANEOUS_PRESS:
            Serial.println("[INPUT] Simultaneous Press (Both A & B): Reserved for future mode selection");
            break;
        case Input::ButtonEvent::LASER_HOLD_ENGAGE:
            Serial.println("[INPUT] Gesture Detected: Laser Hold Engaged");
            break;
        case Input::ButtonEvent::LASER_HOLD_RELEASE:
            Serial.println("[INPUT] Gesture Released: Laser Hold Disengaged");
            break;
        default:
            break;
    }
}

void Diagnostics::printLaserEvent(Peripherals::LaserState state, uint32_t activeDurationMs) {
    switch (state) {
        case Peripherals::LaserState::ACTIVE:
            Serial.println("[LASER] >> Laser Active (Driver Pin HIGH) <<");
            break;
        case Peripherals::LaserState::OFF:
            Serial.printf("[LASER] Laser Deactivated (Total ON Time: %u ms)\n", activeDurationMs);
            break;
        case Peripherals::LaserState::TIMED_OUT:
            Serial.println("[LASER] >> SAFETY TIMEOUT: Max continuous duration exceeded (5s limit)! Laser forced OFF <<");
            break;
        case Peripherals::LaserState::INTERLOCK_INHIBITED:
            Serial.println("[LASER] Interlock Tripped: Laser Inhibited due to state condition");
            break;
    }
}

void Diagnostics::printStateTransition(SystemState oldState, SystemState newState) {
    const char* oldName = "UNKNOWN";
    const char* newName = "UNKNOWN";

    switch (oldState) {
        case SystemState::INITIALIZING:     oldName = "INITIALIZING"; break;
        case SystemState::CALIBRATING:      oldName = "CALIBRATING"; break;
        case SystemState::READY:            oldName = "READY"; break;
        case SystemState::AIR_MOUSE_ACTIVE: oldName = "AIR_MOUSE_ACTIVE"; break;
        case SystemState::AIR_MOUSE_PAUSED: oldName = "AIR_MOUSE_PAUSED"; break;
        case SystemState::SENSOR_ERROR:     oldName = "SENSOR_ERROR"; break;
        case SystemState::BLE_DISCONNECTED: oldName = "BLE_DISCONNECTED"; break;
    }

    switch (newState) {
        case SystemState::INITIALIZING:     newName = "INITIALIZING"; break;
        case SystemState::CALIBRATING:      newName = "CALIBRATING"; break;
        case SystemState::READY:            newName = "READY"; break;
        case SystemState::AIR_MOUSE_ACTIVE: newName = "AIR_MOUSE_ACTIVE"; break;
        case SystemState::AIR_MOUSE_PAUSED: newName = "AIR_MOUSE_PAUSED"; break;
        case SystemState::SENSOR_ERROR:     newName = "SENSOR_ERROR"; break;
        case SystemState::BLE_DISCONNECTED: newName = "BLE_DISCONNECTED"; break;
    }

    Serial.printf("[STATE] Transition: %s -> %s\n", oldName, newName);
}

void Diagnostics::printSensorError(uint32_t errorCount, const char* action) {
    Serial.printf("[SENSOR_ERROR] Error count: %u. Action: %s\n", errorCount, action);
}

void Diagnostics::logHeartbeat(SystemState state, bool bleConnected, uint32_t sensorErrors) {
    uint32_t now = millis();
    if ((now - _lastHeartbeatMs) >= Config::Settings::HEARTBEAT_INTERVAL_MS) {
        _lastHeartbeatMs = now;
        uint32_t sec = now / 1000;
        uint32_t min = sec / 60;
        uint32_t hr  = min / 60;

        Serial.printf("[HEARTBEAT] Up: %02u:%02u:%02u | State: %d | BLE: %s | I2C Errs: %u | FreeHeap: %u B\n",
                      hr, min % 60, sec % 60,
                      static_cast<int>(state),
                      bleConnected ? "CONNECTED" : "ADVERTISING",
                      sensorErrors,
                      ESP.getFreeHeap());
    }
}

void Diagnostics::logMotionStream(float gx, float gy, int8_t dx, int8_t dy) {
    if (_debugStreaming) {
        Serial.printf("[MOTION] Gx:%+6.1f Gy:%+6.1f | HID dx:%+4d dy:%+4d\n",
                      gx, gy, dx, dy);
    }
}

uint8_t Diagnostics::processSerialCommands() {
    if (Serial.available()) {
        char ch = Serial.read();
        if (ch == 'd' || ch == 'D') {
            _debugStreaming = !_debugStreaming;
            Serial.printf("[CMD] Motion debug streaming: %s\n", _debugStreaming ? "ENABLED" : "DISABLED");
            return 1;
        } else if (ch == 'r' || ch == 'R') {
            Serial.println("[CMD] Manual Gyro Recalibration requested via USB serial.");
            return 2;
        } else if (ch == 'h' || ch == 'H') {
            Serial.println("[CMD] Available commands:");
            Serial.println("  'd' - Toggle motion telemetry stream");
            Serial.println("  'r' - Initiate stationary gyro recalibration");
            Serial.println("  'h' - Display this help text");
        }
    }
    return 0;
}

} // namespace System
