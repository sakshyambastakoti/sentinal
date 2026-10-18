#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <NimBLEServer.h>

static const uint8_t hidReportMap[] = {
  0x05, 0x01,       // Usage Page (Generic Desktop)
  0x09, 0x02,       // Usage (Mouse)
  0xA1, 0x01,       // Collection (Application)
  0x85, 0x01,       //   Report ID (1)
  0x09, 0x01,       //   Usage (Pointer)
  0xA1, 0x00,       //   Collection (Physical)
  0x05, 0x09,       //     Usage Page (Buttons)
  0x19, 0x01,       //     Usage Minimum (1)
  0x29, 0x03,       //     Usage Maximum (3)
  0x15, 0x00,       //     Logical Minimum (0)
  0x25, 0x01,       //     Logical Maximum (1)
  0x75, 0x01,       //     Report Size (1)
  0x95, 0x03,       //     Report Count (3)
  0x81, 0x02,       //     Input (Data, Variable, Absolute)
  0x75, 0x05,       //     Report Size (5)
  0x95, 0x01,       //     Report Count (1)
  0x81, 0x01,       //     Input (Constant) - padding
  0x05, 0x01,       //     Usage Page (Generic Desktop)
  0x09, 0x30,       //     Usage (X)
  0x09, 0x31,       //     Usage (Y)
  0x09, 0x38,       //     Usage (Wheel)
  0x15, 0x81,       //     Logical Minimum (-127)
  0x25, 0x7F,       //     Logical Maximum (127)
  0x75, 0x08,       //     Report Size (8)
  0x95, 0x03,       //     Report Count (3)
  0x81, 0x06,       //     Input (Data, Variable, Relative)
  0xC0,             //   End Collection
  0xC0              // End Collection
};

NimBLEServer* pServer = nullptr;
NimBLEHIDDevice* pHid = nullptr;
NimBLECharacteristic* pInputReport = nullptr;
bool isConnected = false;

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) override {
        isConnected = true;
    }
    void onDisconnect(NimBLEServer* pServer) override {
        isConnected = false;
    }
};

void setup() {
    Serial.begin(115200);
    NimBLEDevice::init("S3 Air Mouse V1");
    NimBLEDevice::setSecurityAuth(true, false, false); // Just Works bonding

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    pHid = new NimBLEHIDDevice(pServer);
    pHid->reportMap((uint8_t*)hidReportMap, sizeof(hidReportMap));
    pHid->manufacturer("Espressif");
    pHid->pnp(0x02, 0xe502, 0xa111, 0x0210);
    pHid->hidInfo(0x00, 0x01);

    pInputReport = pHid->inputReport(1);

    pHid->startServices();

    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
    pAdv->setAppearance(0x03C2); // HID Mouse
    pAdv->addServiceUUID(pHid->hidService()->getUUID());
    pAdv->start();
}

void loop() {
    if (isConnected && pInputReport != nullptr) {
        uint8_t report[4] = {0, 0, 0, 0};
        pInputReport->setValue(report, sizeof(report));
        pInputReport->notify();
    }
    delay(100);
}
