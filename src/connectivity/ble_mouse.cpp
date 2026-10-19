/**
 * @file ble_mouse.cpp
 * @brief Implementation of Windows-compliant BLE HID Mouse using NimBLE.
 */

#include "ble_mouse.h"

namespace Connectivity {

static const uint8_t hidReportMap[] = {
    // -------------------------------------------------------------------------
    // Report ID 1: Mouse (3 Buttons + Relative X/Y + Wheel)
    // -------------------------------------------------------------------------
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
    0xc0,                          // END_COLLECTION

    // -------------------------------------------------------------------------
    // Report ID 2: Keyboard (Standard 6-Key Rollover + Modifiers)
    // -------------------------------------------------------------------------
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, 0x02,                    //   REPORT_ID (2)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard/Keypad)
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard Left Control)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs) - Modifier Byte
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs) - Reserved Byte
    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard/Keypad)
    0x19, 0x00,                    //   USAGE_MINIMUM (0)
    0x29, 0x65,                    //   USAGE_MAXIMUM (101)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs) - 6 Key Array
    0xc0,                          // END_COLLECTION

    // -------------------------------------------------------------------------
    // Report ID 3: Consumer Devices (Media / Volume / Play / Pause)
    // -------------------------------------------------------------------------
    0x05, 0x0c,                    // USAGE_PAGE (Consumer Devices)
    0x09, 0x01,                    // USAGE (Consumer Control)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, 0x03,                    //   REPORT_ID (3)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x03,              //   LOGICAL_MAXIMUM (1023)
    0x19, 0x00,                    //   USAGE_MINIMUM (0)
    0x2a, 0xff, 0x03,              //   USAGE_MAXIMUM (1023)
    0x75, 0x10,                    //   REPORT_SIZE (16)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0                           // END_COLLECTION
};

BLEMouse::BLEMouse()
    : _pServer(nullptr),
      _pHid(nullptr),
      _pInputReport(nullptr),
      _pInputReportKeyboard(nullptr),
      _pInputReportConsumer(nullptr),
      _isConnected(false),
      _currentButtons(0),
      _lastReportedButtons(0),
      _connectionCallback(nullptr) {}

void BLEMouse::begin(const char* deviceName, const char* manufacturer) {
    NimBLEDevice::init(deviceName);

    // =========================================================================
    // Windows 10/11 HID Security & Bonding Configuration
    // =========================================================================
    // Windows strictly enforces encryption and LE Secure Connections (SC) for HID devices.
    // 1. Bonding = true, MITM = false (Just Works, no PIN required), SC = true (Secure Connections)
    NimBLEDevice::setSecurityAuth(true, false, true);

    // 2. IO Capability: No Input, No Output (standard for mouse/pointer devices)
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

    // 3. Key distribution: exchange encryption and identity keys
    NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
    NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);

    _pServer = NimBLEDevice::createServer();
    _pServer->setCallbacks(this);

    _pHid = new NimBLEHIDDevice(_pServer);
    _pHid->reportMap(const_cast<uint8_t*>(hidReportMap), sizeof(hidReportMap));
    _pHid->manufacturer(manufacturer);
    _pHid->pnp(0x02, 0xe502, 0xa111, 0x0210);
    _pHid->hidInfo(0x00, 0x01);
    _pHid->setBatteryLevel(Config::Settings::BLE_BATTERY_LVL);

    _pInputReport         = _pHid->inputReport(1);
    _pInputReportKeyboard = _pHid->inputReport(2);
    _pInputReportConsumer = _pHid->inputReport(3);

    _pHid->startServices();

    // =========================================================================
    // BLE Advertising Configuration for Windows Host Discovery
    // =========================================================================
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setAppearance(Config::Settings::BLE_APPEARANCE); // Generic HID
    pAdvertising->addServiceUUID(_pHid->hidService()->getUUID()); // 0x1812 (HID Service)
    pAdvertising->setScanResponse(true);
    pAdvertising->start();
}

void BLEMouse::end() {
    if (_pServer != nullptr) {
        NimBLEDevice::deinit(true);
        _pServer = nullptr;
        _pHid = nullptr;
        _pInputReport = nullptr;
        _pInputReportKeyboard = nullptr;
        _pInputReportConsumer = nullptr;
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

void BLEMouse::onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
    _isConnected = true;
    _currentButtons = 0;
    _lastReportedButtons = 0;

    // Negotiate low-latency HID connection parameters (7.5ms to 15ms interval, 0 latency, 2s timeout)
    pServer->updateConnParams(desc->conn_handle, 6, 12, 0, 200);

    if (_connectionCallback) {
        _connectionCallback(true);
    }
}

void BLEMouse::onAuthenticationComplete(ble_gap_conn_desc* desc) {
    if (!desc->sec_state.encrypted) {
        // If host refused encryption, disconnect and restart advertising
        if (_pServer) {
            _pServer->disconnect(desc->conn_handle);
        }
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

void BLEMouse::scroll(int8_t wheel) {
    sendReport(_currentButtons, 0, 0, wheel);
}

void BLEMouse::sendKey(uint8_t keycode, uint8_t modifier) {
    if (!_isConnected || _pInputReportKeyboard == nullptr) {
        return;
    }
    uint8_t keyReport[8] = {modifier, 0, keycode, 0, 0, 0, 0, 0};
    _pInputReportKeyboard->setValue(keyReport, sizeof(keyReport));
    _pInputReportKeyboard->notify();
    delay(15);
    memset(keyReport, 0, sizeof(keyReport));
    _pInputReportKeyboard->setValue(keyReport, sizeof(keyReport));
    _pInputReportKeyboard->notify();
}

void BLEMouse::sendConsumer(uint16_t usage) {
    if (!_isConnected || _pInputReportConsumer == nullptr) {
        return;
    }
    uint8_t consumerReport[2] = {
        static_cast<uint8_t>(usage & 0xFF),
        static_cast<uint8_t>((usage >> 8) & 0xFF)
    };
    _pInputReportConsumer->setValue(consumerReport, sizeof(consumerReport));
    _pInputReportConsumer->notify();
    delay(15);
    consumerReport[0] = 0;
    consumerReport[1] = 0;
    _pInputReportConsumer->setValue(consumerReport, sizeof(consumerReport));
    _pInputReportConsumer->notify();
}

void BLEMouse::releaseAll() {
    _currentButtons = 0;
    sendReport(0, 0, 0, 0);

    if (_pInputReportKeyboard != nullptr && _isConnected) {
        uint8_t emptyKeyReport[8] = {0};
        _pInputReportKeyboard->setValue(emptyKeyReport, sizeof(emptyKeyReport));
        _pInputReportKeyboard->notify();
    }
    if (_pInputReportConsumer != nullptr && _isConnected) {
        uint8_t emptyConsumerReport[2] = {0};
        _pInputReportConsumer->setValue(emptyConsumerReport, sizeof(emptyConsumerReport));
        _pInputReportConsumer->notify();
    }
}

} // namespace Connectivity
