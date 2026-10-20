/**
 * @file main.cpp
 * @brief Sentinal - 3-in-1 ESP32-S3 BLE Controller (v1.0.0).
 *
 * Architecture:
 * - Sensors: MPU6050 6-axis IMU over I2C (GPIO1 SDA, GPIO2 SCL)
 * - Inputs: 3 Momentary Push Buttons (GPIO5 Btn A, GPIO6 Btn B, GPIO7 Btn Mode)
 * - Multi-Action LEDs: GPIO4 Mode LED, GPIO3 Status LED, GPIO10 Action LED, GPIO48 Onboard LED
 * - Connectivity: BLE HID Composite Mouse & Keyboard (NimBLE-Arduino)
 * - Operating Modes: Air Mouse (Mode 1), Presentation Remote (Mode 2), Reel Scroller (Mode 3)
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
// Non-Blocking Multi-Action LED Controller
// =============================================================================
// 1. Mode LED (GPIO 4): Dedicated solely to mode switching (1x, 2x, 3x blinks)
// 2. Status LED (GPIO 3): Bluetooth status (Blinking=Disconnected/Searching, Solid=Connected)
// 3. Action LED (GPIO 10): Instant visual feedback while pressing buttons / clicks
// 4. Onboard LED (GPIO 48): Active-LOW, mirrors Status and Action
static uint8_t  s_modeBlinksRemaining = 0;
static uint32_t s_lastModeToggleMs    = 0;
static bool     s_modeLedState        = false;

static uint32_t s_lastBleBlinkMs      = 0;
static bool     s_bleBlinkState       = false;

static uint32_t s_actionLedUntilMs    = 0;
static bool     s_actionActive        = false;

static void triggerModeLEDs(Config::Settings::OperatingMode mode) {
    uint8_t count = static_cast<uint8_t>(mode) + 1;
    s_modeBlinksRemaining = count * 2;
    s_lastModeToggleMs = millis();
    s_modeLedState = true;
    digitalWrite(Config::Pins::MODE_LED, HIGH);
    digitalWrite(Config::Pins::ONBOARD_LED, LOW); // Active-LOW on Super Mini: LOW turns it ON!
}

static void triggerActionLED(uint16_t durationMs = 80) {
    s_actionLedUntilMs = millis() + durationMs;
    s_actionActive = true;
    digitalWrite(Config::Pins::ACTION_LED, HIGH);
}

static void updateLEDs(uint32_t now) {
    // -------------------------------------------------------------------------
    // 1. Mode LED (GPIO 4) & Onboard LED (GPIO 48): Burst blinks on mode changes
    // -------------------------------------------------------------------------
    if (s_modeBlinksRemaining > 0) {
        if ((now - s_lastModeToggleMs) >= 120) {
            s_lastModeToggleMs = now;
            s_modeLedState = !s_modeLedState;
            digitalWrite(Config::Pins::MODE_LED, s_modeLedState ? HIGH : LOW);
            digitalWrite(Config::Pins::ONBOARD_LED, s_modeLedState ? LOW : HIGH); // Active-LOW
            s_modeBlinksRemaining--;
            if (s_modeBlinksRemaining == 0) {
                digitalWrite(Config::Pins::MODE_LED, LOW);
                digitalWrite(Config::Pins::ONBOARD_LED, HIGH); // Off
            }
        }
        // While blinking mode bursts, do not let Status LED logic overwrite the onboard LED!
        return;
    }

    // -------------------------------------------------------------------------
    // 2. Action LED (GPIO 10): Lights while button held or for minimum duration
    // -------------------------------------------------------------------------
    bool physicalPress = s_buttonManager.isButtonAPressed() ||
                         s_buttonManager.isButtonBPressed() ||
                         s_buttonManager.isButtonModePressed();

    if (s_actionActive && (now >= s_actionLedUntilMs) && !physicalPress) {
        s_actionActive = false;
        digitalWrite(Config::Pins::ACTION_LED, LOW);
    } else if (physicalPress) {
        s_actionActive = true;
        digitalWrite(Config::Pins::ACTION_LED, HIGH);
    }

    // -------------------------------------------------------------------------
    // 3. Status LED (GPIO 3) & Onboard LED (GPIO 48): BLE Connection + Activity
    // -------------------------------------------------------------------------
    bool statusLedOn = false;

    if (s_stateManager.getState() == System::SystemState::CALIBRATING) {
        // Fast flicker during IMU calibration (80ms)
        if ((now - s_lastBleBlinkMs) >= 80) {
            s_lastBleBlinkMs = now;
            s_bleBlinkState = !s_bleBlinkState;
        }
        statusLedOn = s_bleBlinkState;
    } else if (!s_bleMouse.isConnected()) {
        // BLE Disconnected / Searching: Slow rhythmic blink (500ms ON / 500ms OFF)
        if ((now - s_lastBleBlinkMs) >= 500) {
            s_lastBleBlinkMs = now;
            s_bleBlinkState = !s_bleBlinkState;
        }
        statusLedOn = s_bleBlinkState;
        // If button is pressed while disconnected, light up for confirmation
        if (s_actionActive || physicalPress) {
            statusLedOn = true;
        }
    } else {
        // BLE Connected: Solid ON, with a crisp negative dip (brief OFF) on button clicks
        statusLedOn = (s_actionActive || physicalPress) ? false : true;
    }

    digitalWrite(Config::Pins::STATUS_LED, statusLedOn ? HIGH : LOW);
    digitalWrite(Config::Pins::ONBOARD_LED, statusLedOn ? LOW : HIGH); // Active-LOW on Super Mini
}

static void setDeviceMode(Config::Settings::OperatingMode mode) {
    s_stateManager.setOperatingMode(mode);
    Serial.printf("\n[MODE] Operating Mode set to: %s\n", s_stateManager.getCurrentOperatingModeName());
    s_bleMouse.releaseAll();
    s_motionProcessor.reset();
    triggerModeLEDs(mode);
}

static void cycleDeviceMode() {
    Config::Settings::OperatingMode newMode = s_stateManager.cycleOperatingMode();
    Serial.printf("\n[MODE] Operating Mode switched to: %s\n", s_stateManager.getCurrentOperatingModeName());
    s_bleMouse.releaseAll();
    s_motionProcessor.reset();
    triggerModeLEDs(newMode);
}

static void resetDeviceMode() {
    setDeviceMode(Config::Settings::OperatingMode::AIR_MOUSE);
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
    // 1. Initialize multi-action indicator LEDs
    pinMode(Config::Pins::MODE_LED, OUTPUT);
    digitalWrite(Config::Pins::MODE_LED, LOW);
    pinMode(Config::Pins::STATUS_LED, OUTPUT);
    digitalWrite(Config::Pins::STATUS_LED, LOW);
    pinMode(Config::Pins::ACTION_LED, OUTPUT);
    digitalWrite(Config::Pins::ACTION_LED, LOW);
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
    // 1. Process USB Serial Diagnostic Commands ('d', 'r', 'm', 'i', 'x', 'y')
    // -------------------------------------------------------------------------
    uint8_t cmd = s_diagnostics.processSerialCommands();
    if (cmd == 2) {
        performRecalibration();
    } else if (cmd == 3) {
        cycleDeviceMode();
    } else if (cmd == 4) {
        s_motionProcessor.toggleInvertX();
        s_motionProcessor.toggleInvertY();
        Serial.printf("[CMD] Invert Both -> Invert X: %s | Invert Y: %s\n",
                      s_motionProcessor.isInvertX() ? "TRUE" : "FALSE",
                      s_motionProcessor.isInvertY() ? "TRUE" : "FALSE");
    } else if (cmd == 5) {
        s_motionProcessor.toggleInvertX();
        Serial.printf("[CMD] Invert X -> Invert X: %s | Invert Y: %s\n",
                      s_motionProcessor.isInvertX() ? "TRUE" : "FALSE",
                      s_motionProcessor.isInvertY() ? "TRUE" : "FALSE");
    } else if (cmd == 6) {
        s_motionProcessor.toggleInvertY();
        Serial.printf("[CMD] Invert Y -> Invert X: %s | Invert Y: %s\n",
                      s_motionProcessor.isInvertX() ? "TRUE" : "FALSE",
                      s_motionProcessor.isInvertY() ? "TRUE" : "FALSE");
    } else if (cmd == 7) {
        setDeviceMode(Config::Settings::OperatingMode::AIR_MOUSE);
    } else if (cmd == 8) {
        setDeviceMode(Config::Settings::OperatingMode::PRESENTATION);
    } else if (cmd == 9) {
        setDeviceMode(Config::Settings::OperatingMode::REEL_SCROLLER);
    } else if (cmd == 10) {
        Serial.println("[CMD] Test Left Click");
        s_bleMouse.click(Connectivity::MOUSE_LEFT);
    } else if (cmd == 11) {
        Serial.println("[CMD] Test Right Click");
        s_bleMouse.click(Connectivity::MOUSE_RIGHT);
    } else if (cmd == 12) {
        Serial.println("[CMD] Test Active Mode Action");
        if (s_stateManager.getOperatingMode() == Config::Settings::OperatingMode::PRESENTATION) {
            s_bleMouse.presentationNext();
        } else {
            s_bleMouse.reelNext();
        }
    }

    // -------------------------------------------------------------------------
    // 2. User Input & Button Event Processing
    // -------------------------------------------------------------------------
    Input::ButtonEvent event = s_buttonManager.update();
    if (event != Input::ButtonEvent::NONE) {
        s_diagnostics.printButtonEvent(event);
        triggerActionLED(80);

        switch (event) {
            case Input::ButtonEvent::BUTTON_A_SHORT_PRESS:
                if (s_stateManager.getOperatingMode() == Config::Settings::OperatingMode::AIR_MOUSE) {
                    // Air Mouse: Left Click
                    if (s_bleMouse.isConnected()) {
                        s_bleMouse.click(Connectivity::MOUSE_LEFT);
                    }
                } else if (s_stateManager.getOperatingMode() == Config::Settings::OperatingMode::PRESENTATION) {
                    // Presentation: Next Slide (Canva, PowerPoint, Google Slides)
                    s_bleMouse.presentationNext();
                } else if (s_stateManager.getOperatingMode() == Config::Settings::OperatingMode::REEL_SCROLLER) {
                    // Reel Scroller: Next Reel (Shorts, Reels, TikTok)
                    s_bleMouse.reelNext();
                }
                break;

            case Input::ButtonEvent::BUTTON_B_SHORT_PRESS:
                if (s_stateManager.getOperatingMode() == Config::Settings::OperatingMode::AIR_MOUSE) {
                    // Air Mouse: Right Click (instant on release)
                    if (s_bleMouse.isConnected()) {
                        s_bleMouse.click(Connectivity::MOUSE_RIGHT);
                    }
                } else if (s_stateManager.getOperatingMode() == Config::Settings::OperatingMode::PRESENTATION) {
                    // Presentation: Previous Slide (Canva, PowerPoint, Google Slides)
                    s_bleMouse.presentationPrev();
                } else if (s_stateManager.getOperatingMode() == Config::Settings::OperatingMode::REEL_SCROLLER) {
                    // Reel Scroller: Previous Reel (Shorts, Reels, TikTok)
                    s_bleMouse.reelPrev();
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
                    // Reel Scroller: Play / Pause (Space)
                    if (s_bleMouse.isConnected()) {
                        Serial.println("[ACTION] Reel Scroller -> Play/Pause Toggle (Space)");
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
