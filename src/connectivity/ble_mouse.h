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

namespace KeyCode {
    constexpr uint8_t KEY_NONE        = 0x00;
    constexpr uint8_t KEY_A           = 0x04;
    constexpr uint8_t KEY_B           = 0x05;
    constexpr uint8_t KEY_ENTER       = 0x28;
    constexpr uint8_t KEY_ESCAPE      = 0x29;
    constexpr uint8_t KEY_SPACE       = 0x2C;
    constexpr uint8_t KEY_F5          = 0x3E;
    constexpr uint8_t KEY_PAGE_UP     = 0x4B;
    constexpr uint8_t KEY_PAGE_DOWN   = 0x4E;
    constexpr uint8_t KEY_RIGHT_ARROW = 0x4F;
    constexpr uint8_t KEY_LEFT_ARROW  = 0x50;
    constexpr uint8_t KEY_DOWN_ARROW  = 0x51;
    constexpr uint8_t KEY_UP_ARROW    = 0x52;
}

namespace ConsumerKey {
    constexpr uint16_t PLAY_PAUSE     = 0x00CD;
    constexpr uint16_t VOLUME_UP      = 0x00E9;
    constexpr uint16_t VOLUME_DOWN    = 0x00EA;
    constexpr uint16_t NEXT_TRACK     = 0x00B5;
    constexpr uint16_t PREV_TRACK     = 0x00B6;
}

typedef void (*ConnectionChangeCallback)(bool connected);

class BLEMouse : public NimBLEServerCallbacks {
public:
    BLEMouse();

    /**
     * @brief Initialize NimBLE stack, configure Windows-compliant security, create HID service, and advertise.
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
     */
    bool sendReport(uint8_t buttons, int8_t dx, int8_t dy, int8_t wheel = 0);

    /**
     * @brief Send relative cursor movement.
     */
    void move(int8_t dx, int8_t dy, int8_t wheel = 0);

    /**
     * @brief Send wheel scroll report.
     */
    void scroll(int8_t wheel);

    /**
     * @brief Press a mouse button.
     */
    void press(uint8_t button);

    /**
     * @brief Release a mouse button.
     */
    void release(uint8_t button);

    /**
     * @brief Perform an atomic click.
     */
    void click(uint8_t button);

    /**
     * @brief Send keyboard key press and immediate release.
     */
    void sendKey(uint8_t keycode, uint8_t modifier = 0);

    /**
     * @brief Send consumer control media key press and release.
     */
    void sendConsumer(uint16_t usage);

    /**
     * @brief Release all currently held buttons and keys.
     */
    void releaseAll();

    void setConnectionChangeCallback(ConnectionChangeCallback cb) {
        _connectionCallback = cb;
    }

private:
    NimBLEServer*         _pServer;
    NimBLEHIDDevice*      _pHid;
    NimBLECharacteristic* _pInputReport;
    NimBLECharacteristic* _pInputReportKeyboard;
    NimBLECharacteristic* _pInputReportConsumer;
    bool                  _isConnected;
    uint8_t               _currentButtons;
    uint8_t               _lastReportedButtons;
    ConnectionChangeCallback _connectionCallback;

    // NimBLEServerCallbacks overrides
    void onConnect(NimBLEServer* pServer) override;
    void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) override;
    void onDisconnect(NimBLEServer* pServer) override;
    void onAuthenticationComplete(ble_gap_conn_desc* desc) override;
};

} // namespace Connectivity
