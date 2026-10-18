/**
 * @file ble_mouse.cpp
 * @brief Implementation of BLE HID Mouse using NimBLE.
 */

#include "ble_mouse.h"

namespace Connectivity {

static const uint8_t hidReportMap[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                    // USAGE (Mouse)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, 0x01,                    //   REPORT_ID (1)
    0x09, 0x01,                    //   USAGE (Pointer)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x75, 0x05,                    //     REPORT_SIZE (5)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs) - Padding
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x09, 0x38,                    //     USAGE (Wheel)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
    0xc0,                          //   END_COLLECTION
    0xc0                           // END_COLLECTION
};

BLEMouse::BLEMouse()
    : _pServer(nullptr),
      _pHid(nullptr),
      _pInputReport(nullptr),
      _isConnected(false),
      _currentButtons(0),
      _lastReportedButtons(0),
      _connectionCallback(nullptr) {}

void BLEMouse::begin(const char* deviceName, const char* manufacturer) {
    NimBLEDevice::init(deviceName);
    NimBLEDevice::setSecurityAuth(true, false, false); // Just-works bonding

    _pServer = NimBLEDevice::createServer();
    _pServer->setCallbacks(this);

    _pHid = new NimBLEHIDDevice(_pServer);
    _pHid->reportMap(const_cast<uint8_t*>(hidReportMap), sizeof(hidReportMap));
    _pHid->manufacturer(manufacturer);
    _pHid->pnp(0x02, 0xe502, 0xa111, 0x0210);
    _pHid->hidInfo(0x00, 0x01);
    _pHid->setBatteryLevel(Config::Settings::BLE_BATTERY_LVL);

    _pInputReport = _pHid->inputReport(1);

    _pHid->startServices();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setAppearance(Config::Settings::BLE_APPEARANCE);
    pAdvertising->addServiceUUID(_pHid->hidService()->getUUID());
    pAdvertising->start();
}

void BLEMouse::end() {
    if (_pServer != nullptr) {
        NimBLEDevice::deinit(true);
        _pServer = nullptr;
        _pHid = nullptr;
        _pInputReport = nullptr;
        _isConnected = false;
    }
}

void BLEMouse::onConnect(NimBLEServer* pServer) {
    _isConnected = true;
    _currentButtons = 0;
    _lastReportedButtons = 0;

    if (_connectionCallback) {
        _connectionCallback(true);
    }
}

void BLEMouse::onDisconnect(NimBLEServer* pServer) {
    _isConnected = false;
    _currentButtons = 0;
    _lastReportedButtons = 0;

    if (_connectionCallback) {
        _connectionCallback(false);
    }

    // Automatically resume advertising on disconnection for reliable reconnect
    NimBLEDevice::startAdvertising();
}

bool BLEMouse::sendReport(uint8_t buttons, int8_t dx, int8_t dy, int8_t wheel) {
    if (!_isConnected || _pInputReport == nullptr) {
        return false;
    }

    // Report suppression: do not send redundant report if neither buttons nor movement changed
    if (dx == 0 && dy == 0 && wheel == 0 && buttons == _lastReportedButtons) {
        return false;
    }

    uint8_t report[4] = {
        buttons,
        static_cast<uint8_t>(dx),
        static_cast<uint8_t>(dy),
        static_cast<uint8_t>(wheel)
    };

    _pInputReport->setValue(report, sizeof(report));
    _pInputReport->notify();
    _lastReportedButtons = buttons;
    return true;
}

void BLEMouse::move(int8_t dx, int8_t dy, int8_t wheel) {
    sendReport(_currentButtons, dx, dy, wheel);
}

void BLEMouse::press(uint8_t button) {
    _currentButtons |= button;
    sendReport(_currentButtons, 0, 0, 0);
}

void BLEMouse::release(uint8_t button) {
    _currentButtons &= ~button;
    sendReport(_currentButtons, 0, 0, 0);
}

void BLEMouse::click(uint8_t button) {
    press(button);
    delay(15);
    release(button);
}

void BLEMouse::releaseAll() {
    _currentButtons = 0;
    sendReport(0, 0, 0, 0);
}

} // namespace Connectivity
