/**
 * @file pins.h
 * @brief Centralized Hardware Pin Definitions for ESP32-S3FH4R2 Super Mini 3-in-1 Controller.
 *
 * Board: ESP32-S3FH4R2 Super Mini (4MB Flash + 2MB PSRAM)
 * Left Header:  5V/VBUS, GND, 3V3, GPIO1, GPIO2, GPIO8, GPIO9
 * Right Header: GPIO7, GPIO6, GPIO5, GPIO4, GPIO3, GPIO10, GPIO11
 */

#pragma once

#include <Arduino.h>

namespace Config {
namespace Pins {

    // =========================================================================
    // I2C Bus Pins (MPU6050 6-Axis Motion Sensor)
    // =========================================================================
    constexpr uint8_t I2C_SDA = 1; // Left Header, Pin 4 (directly below 3V3)
    constexpr uint8_t I2C_SCL = 2; // Left Header, Pin 5 (directly below GPIO 1)

    // =========================================================================
    // User Push Buttons (Momentary, Normally Open, Active-LOW)
    // =========================================================================
    // All buttons connect between the GPIO pin and GND.
    // Configured with internal pull-up (INPUT_PULLUP).
    constexpr uint8_t BUTTON_A    = 5; // Right Header, Pin 3 (Left-Click / Next Slide / Next Reel)
    constexpr uint8_t BUTTON_B    = 6; // Right Header, Pin 2 (Right-Click / Prev Slide / Prev Reel)
    constexpr uint8_t BUTTON_MODE = 7; // Right Header, Pin 1 (Dedicated Mode Switch Button)

    // =========================================================================
    // Mode Indicator LEDs
    // =========================================================================
    // External LED: Connect positive (anode) to GPIO4 via a 220-330 ohm resistor, negative (cathode) to GND.
    constexpr uint8_t MODE_LED    = 4;  // Right Header, Pin 4 (External Mode Indicator LED)
    constexpr uint8_t ONBOARD_LED = 48; // ESP32-S3 Super Mini Onboard Indicator LED

    // =========================================================================
    // Free Expansion Pins
    // =========================================================================
    constexpr uint8_t EXPANSION_GPIO8  = 8;
    constexpr uint8_t EXPANSION_GPIO9  = 9;
    constexpr uint8_t EXPANSION_GPIO3  = 3;
    constexpr uint8_t EXPANSION_GPIO10 = 10;
    constexpr uint8_t EXPANSION_GPIO11 = 11;

} // namespace Pins
} // namespace Config
