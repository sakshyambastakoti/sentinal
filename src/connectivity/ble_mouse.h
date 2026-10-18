/**
 * @file ble_mouse.h
 * @brief High-performance BLE HID Mouse for ESP32-S3 using NimBLE.
 */

#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <NimBLEServer.h>
#include "../config/settings.h"

namespace Connectivity {

constexpr uint8_t MOUSE_LEFT   = 0x01;
constexpr uint8_t MOUSE_RIGHT  = 0x02;
constexpr uint8_t MOUSE_MIDDLE = 0x04;

typedef void (*ConnectionChangeCallback)(bool connected);

class BLEMouse : public NimBLEServerCallbacks {
public:
    BLEMouse();

    /**
     * @brief Initialize NimBLE stack, create HID service, and start advertising.
     */
    void begin(const char* deviceName = Config::Settings::BLE_DEVICE_NAME,
               const char* manufacturer = Config::Settings::BLE_MANUFACTURER);

    /**
     * @brief Stop advertising or disconnect.
     */
    void end();

    bool isConnected() const { return _isConnected; }

    /**
     * @brief Send raw HID mouse report if state has changed or movement occurred.
     * @param buttons Bitmask of pressed buttons (MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE)
     * @param dx Relative horizontal movement (-127 to +127)
     * @param dy Relative vertical movement (-127 to +127)
     * @param wheel Relative scroll wheel movement (-127 to +127)
     * @return true if report was dispatched over BLE
     */
    bool sendReport(uint8_t buttons, int8_t dx, int8_t dy, int8_t wheel = 0);

    /**
     * @brief Send relative cursor movement.
     */
    void move(int8_t dx, int8_t dy, int8_t wheel = 0);

    /**
     * @brief Press a mouse button (maintains pressed state across reports).
     */
    void press(uint8_t button);

    /**
     * @brief Release a mouse button.
     */
    void release(uint8_t button);

    /**
     * @brief Perform an atomic click (press followed immediately by release).
     */
    void click(uint8_t button);

    /**
     * @brief Release all currently held buttons (safety reset on disconnect).
     */
    void releaseAll();

    void setConnectionChangeCallback(ConnectionChangeCallback cb) {
        _connectionCallback = cb;
    }

private:
    NimBLEServer*         _pServer;
    NimBLEHIDDevice*      _pHid;
    NimBLECharacteristic* _pInputReport;
    bool                  _isConnected;
    uint8_t               _currentButtons;
    uint8_t               _lastReportedButtons;
    ConnectionChangeCallback _connectionCallback;

    // NimBLEServerCallbacks overrides
    void onConnect(NimBLEServer* pServer) override;
    void onDisconnect(NimBLEServer* pServer) override;
};

} // namespace Connectivity
