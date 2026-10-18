/**
 * @file diagnostics.h
 * @brief USB Serial Diagnostics, System Telemetry, and Banner Reporting.
 */

#pragma once

#include <Arduino.h>
#include "../config/pins.h"
#include "../config/settings.h"
#include "../sensors/imu_driver.h"
#include "../sensors/imu_calibration.h"
#include "../system/state_manager.h"
#include "../input/button_manager.h"
#include "../peripherals/laser_controller.h"

namespace System {

class Diagnostics {
public:
    Diagnostics();

    void init(uint32_t baudRate = Config::Settings::SERIAL_BAUD_RATE);

    void printStartupBanner();
    void printI2CScanResult(uint8_t address, uint8_t whoAmI);
    void printCalibrationResult(const Sensors::GyroCalibrationResult& result);
    void printBLEStatus(bool advertising, bool connected);
    void printButtonEvent(Input::ButtonEvent event);
    void printLaserEvent(Peripherals::LaserState state, uint32_t activeDurationMs);
    void printStateTransition(SystemState oldState, SystemState newState);
    void printSensorError(uint32_t errorCount, const char* action);

    /**
     * @brief Periodic heartbeat status log (e.g. every 5s).
     */
    void logHeartbeat(SystemState state, bool bleConnected, uint32_t sensorErrors);

    /**
     * @brief High-rate streaming debug telemetry (only active when debug mode is enabled).
     */
    void logMotionStream(float gx, float gy, int8_t dx, int8_t dy);

    /**
     * @brief Check for incoming serial commands ('d' to toggle debug stream, 'r' to recalibrate).
     * @return 1 for toggle debug, 2 for recalibrate, 0 for none.
     */
    uint8_t processSerialCommands();

    void setDebugStreaming(bool enabled) { _debugStreaming = enabled; }
    bool isDebugStreaming() const { return _debugStreaming; }

private:
    bool     _debugStreaming;
    uint32_t _lastHeartbeatMs;
};

} // namespace System
