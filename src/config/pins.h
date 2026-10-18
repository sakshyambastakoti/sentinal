/**
 * @file pins.h
 * @brief Centralized Hardware Pin Definitions for ESP32-S3 Super Mini Air Mouse V1.
 *
 * ESP32-S3 Super Mini Board Pinout Verification Notes:
 * - Left Header (top to bottom with USB-C facing up):
 *     3V3, GND, GPIO1, GPIO2, GPIO3, GPIO4, GPIO5, GPIO6, GPIO7
 * - Right Header (top to bottom with USB-C facing up):
 *     5V, GND, GPIO8, GPIO9, GPIO10, GPIO11, GPIO12, GPIO13, GPIO43(TX), GPIO44(RX)
 *
 * Reserved / Dedicated Pins Avoided:
 * - GPIO0: Boot strapping pin (must not be held LOW during reset).
 * - GPIO3: Frequently connected to onboard USB VBUS voltage divider on Super Mini boards.
 * - GPIO19/20: Native USB D-/D+ used for Hardware CDC serial programming and debugging.
 * - GPIO26-32: Internal SPI Flash / PSRAM interconnects (fatal if toggled externally).
 * - GPIO43/44: UART0 TX / RX (reserved for external hardware serial debugging if needed).
 * - GPIO45/46: Strapping pins for VDD_SPI and boot ROM messages.
 */

#pragma once

#include <Arduino.h>

namespace Config {
namespace Pins {

    // =========================================================================
    // I2C Bus Pins (MPU6050 6-Axis Motion Sensor)
    // =========================================================================
    // Standard hardware I2C peripheral pins on the right header.
    constexpr uint8_t I2C_SDA = 8;
    constexpr uint8_t I2C_SCL = 9;

    // =========================================================================
    // User Push Buttons (Momentary, Normally Open, Active-LOW)
    // =========================================================================
    // Connected between GPIO and GND. Configured with internal pull-up (INPUT_PULLUP).
    // Button A: Short Press -> Left Click; Long Press -> Pause/Resume Motion.
    // Button B: Short Press -> Right Click; Long Press -> Gyro Recalibration.
    constexpr uint8_t BUTTON_A = 5;
    constexpr uint8_t BUTTON_B = 6;

    // =========================================================================
    // Expansion Pins
    // =========================================================================
    constexpr uint8_t EXPANSION_GPIO7 = 7; // Available general GPIO (free expansion)

    // =========================================================================
    // Onboard Status LED (Optional visual indicator)
    // =========================================================================
    constexpr uint8_t ONBOARD_LED = 48;

} // namespace Pins
} // namespace Config
