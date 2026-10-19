/**
 * @file settings.h
 * @brief Centralized Firmware Operational Settings and Tunable Parameters.
 */

#pragma once

#include <Arduino.h>

namespace Config {
namespace Settings {

    // =========================================================================
    // System Information
    // =========================================================================
    constexpr const char* FIRMWARE_NAME    = "Sentinal";
    constexpr const char* FIRMWARE_VERSION = "1.0.0";
    constexpr uint32_t    SERIAL_BAUD_RATE = 115200;

    // =========================================================================
    // Operating Modes
    // =========================================================================
    enum class OperatingMode : uint8_t {
        AIR_MOUSE    = 0, // Mode 1: 6-Axis Air Mouse (Desktop PC / Mac)
        PRESENTATION = 1, // Mode 2: Presentation Remote (PowerPoint / Keynote / PDF)
        REEL_SCROLLER= 2  // Mode 3: Mobile Reel / Short Scroller (Instagram / TikTok / YT)
    };

    // =========================================================================
    // BLE HID Settings
    // =========================================================================
    constexpr const char* BLE_DEVICE_NAME  = "Sentinal";
    constexpr const char* BLE_MANUFACTURER = "Sentinal";
    constexpr uint16_t    BLE_APPEARANCE   = 0x03C2; // HID Mouse (0x03C2) - Required for Android & iOS pointer routing
    constexpr uint8_t     BLE_BATTERY_LVL  = 100;
    constexpr uint32_t    BLE_REPORT_INTERVAL_MS = 10;

    // =========================================================================
    // I2C Bus Settings
    // =========================================================================
    constexpr uint32_t I2C_CLOCK_SPEED_HZ = 100000; // 100 kHz Standard-Mode
    constexpr uint32_t I2C_TIMEOUT_MS     = 100;    // Hardware timeout in ms
    constexpr uint8_t  MPU6050_ADDR_PRIMARY   = 0x68; // AD0 pulled LOW
    constexpr uint8_t  MPU6050_ADDR_SECONDARY = 0x69; // AD0 pulled HIGH (3.3V)

    // =========================================================================
    // IMU Sampling and Hardware Configuration
    // =========================================================================
    constexpr uint32_t IMU_SAMPLE_INTERVAL_MS = 10;   // 100 Hz IMU acquisition loop
    constexpr float    GYRO_SCALE_250DPS      = 131.0f; // LSB / (deg/sec)
    constexpr float    ACCEL_SCALE_2G         = 16384.0f; // LSB / g
    constexpr uint8_t  MPU6050_DLPF_CONFIG    = 0x02; // ~98 Hz DLPF bandwidth

    // =========================================================================
    // IMU Stationary Calibration
    // =========================================================================
    constexpr uint32_t CALIBRATION_DURATION_MS = 2000;
    constexpr uint16_t CALIBRATION_SAMPLE_COUNT = 200;
    constexpr float    CALIBRATION_MAX_MOTION_THRESHOLD_DPS = 4.0f;

    // =========================================================================
    // Motion Processing & Cursor Tuning
    // =========================================================================
    enum GyroAxis : uint8_t {
        AXIS_X = 0,
        AXIS_Y = 1,
        AXIS_Z = 2
    };

    constexpr GyroAxis AXIS_MAP_X   = AXIS_Z;  // Yaw rate drives cursor X
    constexpr bool     INVERT_X     = true;    // Corrected: Inverted to match physical motion (left = left, right = right)
    constexpr GyroAxis AXIS_MAP_Y   = AXIS_X;  // Pitch rate drives cursor Y
    constexpr bool     INVERT_Y     = true;    // Matches physical motion (up = up, down = down)

    constexpr float GYRO_DEADZONE_DPS = 1.25f;
    constexpr float SENSITIVITY_X     = 0.15f;
    constexpr float SENSITIVITY_Y     = 0.15f;
    constexpr float FILTER_ALPHA      = 0.45f;

    constexpr bool  ACCEL_ENABLED     = true;
    constexpr float ACCEL_THRESHOLD   = 22.0f;
    constexpr float ACCEL_GAIN        = 0.018f;
    constexpr float ACCEL_MAX_SCALE   = 3.5f;

    constexpr int8_t HID_REPORT_MIN   = -127;
    constexpr int8_t HID_REPORT_MAX   = 127;

    // =========================================================================
    // Button Timing and Debounce Parameters
    // =========================================================================
    constexpr uint32_t BUTTON_DEBOUNCE_MS     = 25;  // 25 ms debounce filter
    constexpr uint32_t BUTTON_LONG_PRESS_MS   = 700; // 700 ms to trigger long press
    constexpr uint32_t BUTTON_SIMULTANEOUS_MS = 50;  // Window to detect chorded press
    constexpr uint32_t BUTTON_STUCK_WARN_MS   = 25000;

    // =========================================================================
    // Diagnostics & Watchdog
    // =========================================================================
    constexpr uint32_t HEARTBEAT_INTERVAL_MS  = 5000;
    constexpr uint32_t MAX_I2C_RECOVERY_TRIES = 5;

} // namespace Settings
} // namespace Config
