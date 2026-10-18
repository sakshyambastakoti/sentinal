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
    constexpr const char* FIRMWARE_NAME    = "ESP32-S3 Air Mouse V1";
    constexpr const char* FIRMWARE_VERSION = "1.0.0";
    constexpr uint32_t    SERIAL_BAUD_RATE = 115200;

    // =========================================================================
    // BLE HID Settings
    // =========================================================================
    constexpr const char* BLE_DEVICE_NAME  = "S3 Air Mouse V1";
    constexpr const char* BLE_MANUFACTURER = "Embedded-Prototype";
    constexpr uint16_t    BLE_APPEARANCE   = 0x03C2; // HID Mouse (Standard Bluetooth SIG)
    constexpr uint8_t     BLE_BATTERY_LVL  = 100;    // Default simulated battery level (%)
    constexpr uint32_t    BLE_REPORT_INTERVAL_MS = 10; // Report rate target: 100 Hz

    // =========================================================================
    // I2C Bus Settings
    // =========================================================================
    constexpr uint32_t I2C_CLOCK_SPEED_HZ = 400000; // 400 kHz Fast-Mode
    constexpr uint32_t I2C_TIMEOUT_MS     = 50;     // Hardware timeout in ms
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
    constexpr uint32_t CALIBRATION_DURATION_MS = 2000; // 2.0-second startup calibration
    constexpr uint16_t CALIBRATION_SAMPLE_COUNT = 200; // Target sample count at 100Hz
    // Maximum allowable deviation from mean (deg/s) during calibration before failing:
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
    constexpr bool     INVERT_X     = false;   // Turn right -> positive X (right)
    constexpr GyroAxis AXIS_MAP_Y   = AXIS_X;  // Pitch rate drives cursor Y
    constexpr bool     INVERT_Y     = true;    // Tilt up -> negative Y (upward cursor)

    // Noise suppression: ignore angular velocity below this threshold (deg/s)
    constexpr float GYRO_DEADZONE_DPS = 1.25f;

    // Base cursor sensitivity (multiplier from deg/s to pixels per sample frame)
    constexpr float SENSITIVITY_X     = 0.15f;
    constexpr float SENSITIVITY_Y     = 0.15f;

    // First-order Low-Pass Filter coefficient (0.0 = maximum smoothing, 1.0 = raw)
    constexpr float FILTER_ALPHA      = 0.45f;

    // Velocity-based cursor acceleration:
    constexpr bool  ACCEL_ENABLED     = true;
    constexpr float ACCEL_THRESHOLD   = 22.0f;  // deg/s threshold to initiate acceleration
    constexpr float ACCEL_GAIN        = 0.018f; // Acceleration scaling coefficient
    constexpr float ACCEL_MAX_SCALE   = 3.5f;   // Maximum acceleration multiplier

    // HID Report Clamping Limits
    constexpr int8_t HID_REPORT_MIN   = -127;
    constexpr int8_t HID_REPORT_MAX   = 127;

    // =========================================================================
    // Button Timing and Debounce Parameters
    // =========================================================================
    constexpr uint32_t BUTTON_DEBOUNCE_MS     = 25;  // 25 ms debounce filter
    constexpr uint32_t BUTTON_LONG_PRESS_MS   = 750; // 750 ms to trigger long press
    constexpr uint32_t BUTTON_SIMULTANEOUS_MS = 50;  // Window to detect chorded press
    constexpr uint32_t BUTTON_STUCK_WARN_MS   = 25000; // 25s warning for stuck button

    // =========================================================================
    // Diagnostics & Watchdog
    // =========================================================================
    constexpr uint32_t HEARTBEAT_INTERVAL_MS  = 5000; // Diagnostic status log every 5s
    constexpr uint32_t MAX_I2C_RECOVERY_TRIES = 5;    // Max sensor re-inits before error lock

} // namespace Settings
} // namespace Config
