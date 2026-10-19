/**
 * @file main.cpp
 * @brief ESP32-S3 Air Mouse Prototype V1 - Main Application Lifecycle.
 *
 * Architecture:
 * - Sensors: MPU6050 6-axis IMU over I2C (GPIO8 SDA, GPIO9 SCL)
 * - Inputs: 2 Momentary Push Buttons (GPIO5 Btn A, GPIO6 Btn B)
 * - Connectivity: BLE HID Mouse (NimBLE-Arduino)
 * - State Management: Deterministic FSM with safe state transitions
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
#include "system/state_manager.h"
#include "system/diagnostics.h"

// =============================================================================
// Static Component Instances
// =============================================================================
static Sensors::IMUDriver     s_imuDriver;
static Sensors::IMUCalibration s_imuCalibration;
static Input::ButtonManager   s_buttonManager;
static Motion::MotionProcessor s_motionProcessor;
static Connectivity::BLEMouse s_bleMouse;
static System::StateManager   s_stateManager;
static System::Diagnostics    s_diagnostics;

// Timing variables (safe across millis() rollover)
static uint32_t s_lastIMUSampleMs  = 0;
static uint32_t s_lastMotionCalcMs = 0;
static uint32_t s_sensorErrorCount = 0;

// =============================================================================
// Mode LED Feedback Helper (Non-blocking)
// =============================================================================
static uint8_t  s_ledBlinksRemaining = 0;
static uint32_t s_lastLedToggleMs    = 0;
static bool     s_ledState           = false;

static void triggerModeLEDs(Config::Settings::OperatingMode mode) {
    // Mode 0 (Air Mouse): 1 flash (2 toggles)
    // Mode 1 (Presentation): 2 flashes (4 toggles)
    // Mode 2 (Reel Scroller): 3 flashes (6 toggles)
    uint8_t count = static_cast<uint8_t>(mode) + 1;
    s_ledBlinksRemaining = count * 2;
    s_lastLedToggleMs = millis();
    s_ledState = true;
    digitalWrite(Config::Pins::MODE_LED, HIGH);
    digitalWrite(Config::Pins::ONBOARD_LED, LOW); // Active-LOW on ESP32-S3 Super Mini
}

static void updateLEDs(uint32_t now) {
    if (s_ledBlinksRemaining > 0) {
        if ((now - s_lastLedToggleMs) >= 120) {
            s_lastLedToggleMs = now;
            s_ledState = !s_ledState;
            digitalWrite(Config::Pins::MODE_LED, s_ledState ? HIGH : LOW);
            digitalWrite(Config::Pins::ONBOARD_LED, s_ledState ? LOW : HIGH);
            s_ledBlinksRemaining--;
            if (s_ledBlinksRemaining == 0) {
                digitalWrite(Config::Pins::MODE_LED, LOW);
                digitalWrite(Config::Pins::ONBOARD_LED, HIGH); // Off
            }
        }
    }
}

static void cycleDeviceMode() {
    Config::Settings::OperatingMode newMode = s_stateManager.cycleOperatingMode();
    Serial.printf("\n[MODE] Operating Mode switched to: %s\n", s_stateManager.getCurrentOperatingModeName());
    s_bleMouse.releaseAll();
    s_motionProcessor.reset();
    triggerModeLEDs(newMode);
}

static void resetDeviceMode() {
    s_stateManager.setOperatingMode(Config::Settings::OperatingMode::AIR_MOUSE);
    Serial.printf("\n[MODE] Operating Mode reset to default: %s\n", s_stateManager.getCurrentOperatingModeName());
    s_bleMouse.releaseAll();
    s_motionProcessor.reset();
    triggerModeLEDs(Config::Settings::OperatingMode::AIR_MOUSE);
}

// =============================================================================
// State Machine, Mode and Connection Callbacks
// =============================================================================
static void handleStateChange(System::SystemState oldState, System::SystemState newState) {
    s_diagnostics.printStateTransition(oldState, newState);

    if (newState == System::SystemState::AIR_MOUSE_PAUSED) {
        s_motionProcessor.setPaused(true);
        s_bleMouse.releaseAll();
    } else if (newState == System::SystemState::AIR_MOUSE_ACTIVE) {
        if (s_stateManager.getOperatingMode() == Config::Settings::OperatingMode::AIR_MOUSE) {
            s_motionProcessor.setPaused(false);
        }
    } else if (newState == System::SystemState::BLE_DISCONNECTED ||
               newState == System::SystemState::SENSOR_ERROR) {
        s_bleMouse.releaseAll();
        s_motionProcessor.reset();
    }
}

static void handleModeChange(Config::Settings::OperatingMode oldMode, Config::Settings::OperatingMode newMode) {
    s_diagnostics.printOperatingMode(newMode);
    s_bleMouse.releaseAll();
    s_motionProcessor.reset();
    if (newMode == Config::Settings::OperatingMode::AIR_MOUSE) {
        if (s_stateManager.getState() == System::SystemState::AIR_MOUSE_ACTIVE) {
            s_motionProcessor.setPaused(false);
        }
    } else {
        s_motionProcessor.setPaused(true);
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
        s_stateManager.transitionTo(System::SystemState::BLE_DISCONNECTED);
    }
}

// =============================================================================
// Calibration Routine Helper
// =============================================================================
static void performRecalibration() {
    s_stateManager.transitionTo(System::SystemState::CALIBRATING);

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
    // 1. Initialize indicator LEDs
    pinMode(Config::Pins::MODE_LED, OUTPUT);
    digitalWrite(Config::Pins::MODE_LED, LOW);
    pinMode(Config::Pins::ONBOARD_LED, OUTPUT);
    digitalWrite(Config::Pins::ONBOARD_LED, HIGH); // Active-LOW: set HIGH to turn OFF

    // 2. Initialize USB CDC serial diagnostics
    s_diagnostics.init(Config::Settings::SERIAL_BAUD_RATE);
    s_diagnostics.printStartupBanner();

    // 3. Initialize state manager and attach transition/mode observers
    s_stateManager.init();
    s_stateManager.setStateChangeCallback(handleStateChange);
    s_stateManager.setModeChangeCallback(handleModeChange);

    // 4. Initialize physical push buttons (Button A, Button B, Button Mode)
    s_buttonManager.init(Config::Pins::BUTTON_A, Config::Pins::BUTTON_B, Config::Pins::BUTTON_MODE);
    triggerModeLEDs(s_stateManager.getOperatingMode());

    // 5. Initialize I2C Bus and MPU6050 Motion Sensor
    Serial.println("[INIT] Initializing I2C bus and MPU6050...");
    if (!s_imuDriver.init(Config::Pins::I2C_SDA, Config::Pins::I2C_SCL, Config::Settings::I2C_CLOCK_SPEED_HZ)) {
        s_sensorErrorCount++;
        s_diagnostics.printSensorError(s_sensorErrorCount, "MPU6050 not detected at 0x68 or 0x69!");
        s_stateManager.transitionTo(System::SystemState::SENSOR_ERROR);
    } else {
        s_diagnostics.printI2CScanResult(s_imuDriver.getDetectedAddress(), s_imuDriver.getWhoAmI());

        // 5. Perform startup stationary calibration
        s_stateManager.transitionTo(System::SystemState::CALIBRATING);
        s_imuCalibration.runBlocking(s_imuDriver, Config::Settings::CALIBRATION_DURATION_MS);
        s_diagnostics.printCalibrationResult(s_imuCalibration.getResult());
        s_stateManager.transitionTo(System::SystemState::READY);
    }

    // 6. Initialize BLE HID Mouse subsystem
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
    // 0. Update Non-Blocking Mode Indicator LEDs
    // -------------------------------------------------------------------------
    updateLEDs(now);

    // -------------------------------------------------------------------------
    // 1. Process USB Serial Diagnostic Commands ('d', 'r', 'm', 'i')
    // -------------------------------------------------------------------------
    uint8_t cmd = s_diagnostics.processSerialCommands();
    if (cmd == 2) {
        performRecalibration();
    } else if (cmd == 3) {
        cycleDeviceMode();
    } else if (cmd == 4) {
        s_motionProcessor.toggleInvertX();
        s_motionProcessor.toggleInvertY();
        Serial.printf("[CMD] Invert X: %s | Invert Y: %s\n",
                      s_motionProcessor.isInvertX() ? "TRUE" : "FALSE",
                      s_motionProcessor.isInvertY() ? "TRUE" : "FALSE");
    }

    // -------------------------------------------------------------------------
    // 2. User Input & Button Event Processing
    // -------------------------------------------------------------------------
    Input::ButtonEvent event = s_buttonManager.update();
    if (event != Input::ButtonEvent::NONE) {
        s_diagnostics.printButtonEvent(event);

        switch (event) {
            case Input::ButtonEvent::BUTTON_A_SHORT_PRESS:
                if (s_stateManager.getOperatingMode() == Config::Settings::OperatingMode::AIR_MOUSE) {
                    // Air Mouse: Left Click
                    if (s_bleMouse.isConnected()) {
                        s_bleMouse.click(Connectivity::MOUSE_LEFT);
                    }
                } else if (s_stateManager.getOperatingMode() == Config::Settings::OperatingMode::PRESENTATION) {
                    // Presentation: Next Slide (PAGE_DOWN)
                    if (s_bleMouse.isConnected()) {
                        Serial.println("[ACTION] Presentation -> Next Slide (PAGE_DOWN)");
                        s_bleMouse.sendKey(Connectivity::KeyCode::KEY_PAGE_DOWN);
                    }
                } else if (s_stateManager.getOperatingMode() == Config::Settings::OperatingMode::REEL_SCROLLER) {
                    // Reel Scroller: Next Reel (Mouse Wheel Down + Down Arrow)
                    if (s_bleMouse.isConnected()) {
                        Serial.println("[ACTION] Reel Scroller -> Next Reel (Scroll Down)");
                        s_bleMouse.scroll(-4);
                        s_bleMouse.sendKey(Connectivity::KeyCode::KEY_DOWN_ARROW);
                    }
                }
                break;

            case Input::ButtonEvent::BUTTON_B_SHORT_PRESS:
                if (s_stateManager.getOperatingMode() == Config::Settings::OperatingMode::AIR_MOUSE) {
                    // Air Mouse: Right Click (instant on release)
                    if (s_bleMouse.isConnected()) {
                        s_bleMouse.click(Connectivity::MOUSE_RIGHT);
                    }
                } else if (s_stateManager.getOperatingMode() == Config::Settings::OperatingMode::PRESENTATION) {
                    // Presentation: Previous Slide (PAGE_UP)
                    if (s_bleMouse.isConnected()) {
                        Serial.println("[ACTION] Presentation -> Previous Slide (PAGE_UP)");
                        s_bleMouse.sendKey(Connectivity::KeyCode::KEY_PAGE_UP);
                    }
                } else if (s_stateManager.getOperatingMode() == Config::Settings::OperatingMode::REEL_SCROLLER) {
                    // Reel Scroller: Previous Reel (Mouse Wheel Up + Up Arrow)
                    if (s_bleMouse.isConnected()) {
                        Serial.println("[ACTION] Reel Scroller -> Previous Reel (Scroll Up)");
                        s_bleMouse.scroll(4);
                        s_bleMouse.sendKey(Connectivity::KeyCode::KEY_UP_ARROW);
                    }
                }
                break;

            case Input::ButtonEvent::BUTTON_A_LONG_PRESS:
                if (s_stateManager.getOperatingMode() == Config::Settings::OperatingMode::AIR_MOUSE) {
                    // Air Mouse: Toggle Motion Pause / Resume
                    if (s_stateManager.getState() == System::SystemState::AIR_MOUSE_ACTIVE) {
                        s_stateManager.transitionTo(System::SystemState::AIR_MOUSE_PAUSED);
                        Serial.println("[APP] Air Mouse Paused (re-center mouse without moving cursor).");
                    } else if (s_stateManager.getState() == System::SystemState::AIR_MOUSE_PAUSED) {
                        s_stateManager.transitionTo(System::SystemState::AIR_MOUSE_ACTIVE);
                        Serial.println("[APP] Air Mouse Resumed.");
                    }
                } else if (s_stateManager.getOperatingMode() == Config::Settings::OperatingMode::PRESENTATION) {
                    // Presentation: Start / Resume Fullscreen Slideshow (F5)
                    if (s_bleMouse.isConnected()) {
                        Serial.println("[ACTION] Presentation -> Start / Resume Slideshow (F5)");
                        s_bleMouse.sendKey(Connectivity::KeyCode::KEY_F5);
                    }
                } else if (s_stateManager.getOperatingMode() == Config::Settings::OperatingMode::REEL_SCROLLER) {
                    // Reel Scroller: Play / Pause Video (Spacebar)
                    if (s_bleMouse.isConnected()) {
                        Serial.println("[ACTION] Reel Scroller -> Play / Pause (Spacebar)");
                        s_bleMouse.sendKey(Connectivity::KeyCode::KEY_SPACE);
                    }
                }
                break;

            case Input::ButtonEvent::BUTTON_B_LONG_PRESS:
                // Stationary Gyro Recalibration across all modes
                performRecalibration();
                break;

            case Input::ButtonEvent::BUTTON_MODE_SHORT_PRESS:
            case Input::ButtonEvent::SIMULTANEOUS_PRESS:
                // Dedicated Mode Switch Button tap (or A+B simultaneous): Cycle Mode
                cycleDeviceMode();
                break;

            case Input::ButtonEvent::BUTTON_MODE_LONG_PRESS:
                // Dedicated Mode Switch Button hold: Reset directly to Air Mouse Mode
                resetDeviceMode();
                break;

            default:
                break;
        }
    }

    // -------------------------------------------------------------------------
    // 3. IMU Sensor Acquisition & Motion Pipeline (100 Hz Loop)
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
    // 4. Sensor Error Auto-Recovery Attempt
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
    // 5. Periodic Diagnostic Heartbeat (Every 5 seconds)
    // -------------------------------------------------------------------------
    s_diagnostics.logHeartbeat(s_stateManager.getState(), s_bleMouse.isConnected(), s_sensorErrorCount);
}
