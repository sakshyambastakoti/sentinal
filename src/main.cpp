/**
 * @file main.cpp
 * @brief ESP32-S3 Air Mouse Prototype V1 - Main Application Lifecycle.
 *
 * Architecture:
 * - Sensors: MPU6050 6-axis IMU over I2C (GPIO8 SDA, GPIO9 SCL)
 * - Inputs: 2 Momentary Push Buttons (GPIO5 Btn A, GPIO6 Btn B)
 * - Connectivity: BLE HID Mouse (NimBLE-Arduino)
 * - Peripheral: Low-Power Laser Module Driver (GPIO7)
 * - State Management: Deterministic 7-state FSM with safety interlocks
 * - Diagnostics: Hardware USB CDC Serial telemetry and commands
 */

#include <Arduino.h>
#include "config/pins.h"
#include "config/settings.h"
#include "sensors/imu_driver.h"
#include "sensors/imu_calibration.h"
#include "input/button_manager.h"
#include "motion/motion_processor.h"
#include "connectivity/ble_mouse.h"
#include "peripherals/laser_controller.h"
#include "system/state_manager.h"
#include "system/diagnostics.h"

// =============================================================================
// Static Component Instances
// =============================================================================
static Sensors::IMUDriver           s_imuDriver;
static Sensors::IMUCalibration       s_imuCalibration;
static Input::ButtonManager         s_buttonManager;
static Motion::MotionProcessor       s_motionProcessor;
static Connectivity::BLEMouse       s_bleMouse;
static Peripherals::LaserController s_laserController;
static System::StateManager         s_stateManager;
static System::Diagnostics          s_diagnostics;

// Timing variables (safe across millis() rollover)
static uint32_t s_lastIMUSampleMs   = 0;
static uint32_t s_lastMotionCalcMs  = 0;
static uint32_t s_sensorErrorCount  = 0;

// =============================================================================
// State Machine and Connection Callbacks
// =============================================================================
static void handleStateChange(System::SystemState oldState, System::SystemState newState) {
    s_diagnostics.printStateTransition(oldState, newState);

    // Safety interlock enforcement on state transitions
    if (!s_stateManager.isLaserPermitted()) {
        s_laserController.setInterlock(true);
    } else {
        s_laserController.setInterlock(false);
    }

    if (newState == System::SystemState::AIR_MOUSE_PAUSED) {
        s_motionProcessor.setPaused(true);
        s_bleMouse.releaseAll();
    } else if (newState == System::SystemState::AIR_MOUSE_ACTIVE) {
        s_motionProcessor.setPaused(false);
    } else if (newState == System::SystemState::BLE_DISCONNECTED ||
               newState == System::SystemState::SENSOR_ERROR) {
        s_bleMouse.releaseAll();
        s_motionProcessor.reset();
    }
}

static void handleBLEConnection(bool connected) {
    s_diagnostics.printBLEStatus(!connected, connected);

    if (connected) {
        if (s_stateManager.getState() == System::SystemState::BLE_DISCONNECTED ||
            s_stateManager.getState() == System::SystemState::READY) {
            s_stateManager.transitionTo(System::SystemState::AIR_MOUSE_ACTIVE);
        }
    } else {
        s_laserController.emergencyShutdown();
        s_stateManager.transitionTo(System::SystemState::BLE_DISCONNECTED);
    }
}

// =============================================================================
// Calibration Routine Helper
// =============================================================================
static void performRecalibration() {
    System::SystemState previousState = s_stateManager.getState();
    s_stateManager.transitionTo(System::SystemState::CALIBRATING);

    // Laser must be unconditionally OFF during calibration
    s_laserController.emergencyShutdown();
    s_motionProcessor.reset();
    s_bleMouse.releaseAll();

    bool success = s_imuCalibration.runBlocking(s_imuDriver, Config::Settings::CALIBRATION_DURATION_MS);
    s_diagnostics.printCalibrationResult(s_imuCalibration.getResult());

    if (success) {
        if (s_bleMouse.isConnected()) {
            s_stateManager.transitionTo(System::SystemState::AIR_MOUSE_ACTIVE);
        } else {
            s_stateManager.transitionTo(System::SystemState::BLE_DISCONNECTED);
        }
    } else {
        // Retain prior state or fall back to safe ready/disconnected
        if (s_bleMouse.isConnected()) {
            s_stateManager.transitionTo(System::SystemState::AIR_MOUSE_ACTIVE);
        } else {
            s_stateManager.transitionTo(System::SystemState::BLE_DISCONNECTED);
        }
    }
}

// =============================================================================
// Setup
// =============================================================================
void setup() {
    // 1. Hardware-level safety default: immediately lock laser OFF before anything else
    s_laserController.init(Config::Pins::LASER_DRIVER);

    // 2. Initialize USB CDC serial diagnostics
    s_diagnostics.init(Config::Settings::SERIAL_BAUD_RATE);
    s_diagnostics.printStartupBanner();

    // 3. Initialize state manager and attach transition observer
    s_stateManager.init();
    s_stateManager.setStateChangeCallback(handleStateChange);

    // 4. Initialize physical push buttons
    s_buttonManager.init(Config::Pins::BUTTON_A, Config::Pins::BUTTON_B);

    // 5. Initialize I2C Bus and MPU6050 Motion Sensor
    Serial.println("[INIT] Initializing I2C bus and MPU6050...");
    if (!s_imuDriver.init(Config::Pins::I2C_SDA, Config::Pins::I2C_SCL, Config::Settings::I2C_CLOCK_SPEED_HZ)) {
        s_sensorErrorCount++;
        s_diagnostics.printSensorError(s_sensorErrorCount, "MPU6050 not detected at 0x68 or 0x69!");
        s_stateManager.transitionTo(System::SystemState::SENSOR_ERROR);
    } else {
        s_diagnostics.printI2CScanResult(s_imuDriver.getDetectedAddress(), s_imuDriver.getWhoAmI());

        // 6. Perform startup stationary calibration
        s_stateManager.transitionTo(System::SystemState::CALIBRATING);
        s_imuCalibration.runBlocking(s_imuDriver, Config::Settings::CALIBRATION_DURATION_MS);
        s_diagnostics.printCalibrationResult(s_imuCalibration.getResult());
        s_stateManager.transitionTo(System::SystemState::READY);
    }

    // 7. Initialize BLE HID Mouse subsystem
    Serial.println("[INIT] Initializing BLE HID Stack (NimBLE)...");
    s_bleMouse.setConnectionChangeCallback(handleBLEConnection);
    s_bleMouse.begin(Config::Settings::BLE_DEVICE_NAME, Config::Settings::BLE_MANUFACTURER);
    s_diagnostics.printBLEStatus(true, false);

    if (s_stateManager.getState() == System::SystemState::READY) {
        s_stateManager.transitionTo(System::SystemState::BLE_DISCONNECTED);
    }

    s_lastIMUSampleMs  = millis();
    s_lastMotionCalcMs = millis();
    Serial.println("[INIT] System setup complete. Entering main executive loop.\n");
}

// =============================================================================
// Main Executive Loop
// =============================================================================
void loop() {
    uint32_t now = millis();

    // -------------------------------------------------------------------------
    // 1. Process USB Serial Diagnostic Commands ('d' = toggle debug, 'r' = calib)
    // -------------------------------------------------------------------------
    uint8_t cmd = s_diagnostics.processSerialCommands();
    if (cmd == 2) {
        performRecalibration();
    }

    // -------------------------------------------------------------------------
    // 2. Safety Audit: Laser Controller Update (Enforces 5s hard timeout)
    // -------------------------------------------------------------------------
    // Runs on EVERY cycle regardless of sensor or BLE connection status.
    s_laserController.update();

    // -------------------------------------------------------------------------
    // 3. User Input & Button Event Processing
    // -------------------------------------------------------------------------
    Input::ButtonEvent event = s_buttonManager.update();
    if (event != Input::ButtonEvent::NONE) {
        s_diagnostics.printButtonEvent(event);

        switch (event) {
            case Input::ButtonEvent::BUTTON_A_SHORT_PRESS:
                // Left Click
                if (s_bleMouse.isConnected()) {
                    s_bleMouse.click(Connectivity::MOUSE_LEFT);
                }
                break;

            case Input::ButtonEvent::BUTTON_B_SHORT_PRESS:
                // Right Click
                if (s_bleMouse.isConnected()) {
                    s_bleMouse.click(Connectivity::MOUSE_RIGHT);
                }
                break;

            case Input::ButtonEvent::BUTTON_A_LONG_PRESS:
                // Toggle Motion Pause / Resume
                if (s_stateManager.getState() == System::SystemState::AIR_MOUSE_ACTIVE) {
                    s_stateManager.transitionTo(System::SystemState::AIR_MOUSE_PAUSED);
                    Serial.println("[APP] Air Mouse Paused (re-center mouse without moving cursor).");
                } else if (s_stateManager.getState() == System::SystemState::AIR_MOUSE_PAUSED) {
                    s_stateManager.transitionTo(System::SystemState::AIR_MOUSE_ACTIVE);
                    Serial.println("[APP] Air Mouse Resumed.");
                }
                break;

            case Input::ButtonEvent::BUTTON_B_LONG_PRESS:
                // Trigger Stationary Recalibration
                performRecalibration();
                break;

            case Input::ButtonEvent::LASER_HOLD_ENGAGE:
                // Deliberate hold gesture: engage laser
                s_laserController.requestLaser(true);
                s_diagnostics.printLaserEvent(s_laserController.getState(), s_laserController.getActiveDurationMs());
                break;

            case Input::ButtonEvent::LASER_HOLD_RELEASE:
                // Gesture released: immediately disengage laser
                s_laserController.requestLaser(false);
                s_diagnostics.printLaserEvent(s_laserController.getState(), s_laserController.getActiveDurationMs());
                break;

            case Input::ButtonEvent::SIMULTANEOUS_PRESS:
                // Chorded press: reserved for V2 Mode Switching
                // Suppressed from causing laser activation or clicks in V1
                break;

            default:
                break;
        }
    }

    // -------------------------------------------------------------------------
    // 4. IMU Sensor Acquisition & Motion Pipeline (100 Hz Loop)
    // -------------------------------------------------------------------------
    if ((now - s_lastIMUSampleMs) >= Config::Settings::IMU_SAMPLE_INTERVAL_MS) {
        s_lastIMUSampleMs = now;

        if (s_stateManager.getState() != System::SystemState::SENSOR_ERROR &&
            s_stateManager.getState() != System::SystemState::CALIBRATING) {

            Sensors::IMUData sample;
            if (s_imuDriver.readSensor(sample)) {
                // Apply calibrated bias offsets
                float gx = 0.0f, gy = 0.0f, gz = 0.0f;
                s_imuCalibration.apply(sample, gx, gy, gz);

                // If air mouse active and BLE connected, process motion
                if (s_stateManager.canEmitMouseReports() && s_bleMouse.isConnected()) {
                    float dtSec = (now - s_lastMotionCalcMs) * 0.001f;
                    if (dtSec <= 0.0f || dtSec > 0.1f) dtSec = 0.01f; // Sanity clamp
                    s_lastMotionCalcMs = now;

                    Motion::MouseDisplacement disp = s_motionProcessor.process(gx, gy, gz, dtSec);

                    if (disp.hasMovement) {
                        s_bleMouse.move(disp.dx, disp.dy, 0);
                    }

                    // Optional streaming debug telemetry
                    s_diagnostics.logMotionStream(gx, gy, disp.dx, disp.dy);
                } else {
                    s_lastMotionCalcMs = now;
                }
            } else {
                // Read failed
                s_sensorErrorCount++;
                if (!s_imuDriver.isHealthy()) {
                    s_diagnostics.printSensorError(s_sensorErrorCount, "Repeated I2C read failures. Entering SENSOR_ERROR.");
                    s_stateManager.transitionTo(System::SystemState::SENSOR_ERROR);
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // 5. Sensor Error Auto-Recovery Attempt
    // -------------------------------------------------------------------------
    if (s_stateManager.getState() == System::SystemState::SENSOR_ERROR) {
        static uint32_t lastRecoveryAttemptMs = 0;
        if ((now - lastRecoveryAttemptMs) >= 2000) {
            lastRecoveryAttemptMs = now;
            Serial.println("[RECOVERY] Attempting I2C bus recovery and sensor re-init...");
            if (s_imuDriver.init(Config::Pins::I2C_SDA, Config::Pins::I2C_SCL, Config::Settings::I2C_CLOCK_SPEED_HZ)) {
                Serial.println("[RECOVERY] Sensor recovered! Performing recalibration...");
                performRecalibration();
            }
        }
    }

    // -------------------------------------------------------------------------
    // 6. Periodic Diagnostic Heartbeat (Every 5 seconds)
    // -------------------------------------------------------------------------
    s_diagnostics.logHeartbeat(s_stateManager.getState(), s_bleMouse.isConnected(), s_sensorErrorCount);
}
