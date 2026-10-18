# Hardware Wiring & Electrical Specification — ESP32-S3 Air Mouse V1

This document specifies the exact electrical connections, component ratings, and schematic diagrams for the **ESP32-S3 Air Mouse Prototype V1**.

---

## 1. ESP32-S3 Super Mini Pinout & Allocation Table

The ESP32-S3 Super Mini exposes 18 outer castellated/through-hole header pins (9 on each side) when oriented with the USB-C port facing upward.

| Header Side | Board Pin | Assigned Function | Target Component / Signal | Electrical Characteristics & Verification Notes |
|:------------|:----------|:------------------|:--------------------------|:------------------------------------------------|
| **Left** | **3V3** | Power Output | MPU6050 `VCC` | Regulated 3.3 V rail from onboard LDO. Maximum external draw ~300 mA. |
| **Left** | **GND** | Ground | Common Ground | Common return for IMU, Buttons, and Laser Driver. |
| **Left** | **GPIO1** | *Unused* | Available Expansion | Clean ADC1/RTC GPIO; reserved for future V2 wheel or analog joystick. |
| **Left** | **GPIO2** | *Unused* | Available Expansion | Clean ADC1/RTC GPIO; reserved for future V2 scroll button. |
| **Left** | **GPIO3** | *Avoided* | Onboard VBUS Sense | Internal resistor divider connected on many Super Mini clones. Avoided. |
| **Left** | **GPIO4** | *Unused* | Available Expansion | Clean GPIO; reserved for V2 battery ADC voltage sensing. |
| **Left** | **GPIO5** | **Button A** | Left-Click / Pause | Configured as `INPUT_PULLUP`. Switched to `GND`. No external resistor needed. |
| **Left** | **GPIO6** | **Button B** | Right-Click / Recalib | Configured as `INPUT_PULLUP`. Switched to `GND`. No external resistor needed. |
| **Left** | **GPIO7** | **Laser Gate** | MOSFET Driver Gate | Active-HIGH digital output. Connected through 220 Ω series gate resistor. |
| **Right** | **5V** | Power Input/Output| Laser Module VCC (Opt) | Raw 5V from USB-C. Use if laser module has an integrated 5V constant-current driver. |
| **Right** | **GND** | Ground | Common Ground | Second common ground pin. |
| **Right** | **GPIO8** | **I2C SDA** | MPU6050 `SDA` | Standard ESP32-S3 I2C SDA. Internal or module 4.7 kΩ pull-up to 3.3 V. |
| **Right** | **GPIO9** | **I2C SCL** | MPU6050 `SCL` | Standard ESP32-S3 I2C SCL. 400 kHz Fast-Mode clock. |
| **Right** | **GPIO10** | *Unused* | Reserved Expansion | Clean GPIO. |
| **Right** | **GPIO11** | *Unused* | Reserved Expansion | Clean GPIO. |
| **Right** | **GPIO12** | *Unused* | Reserved Expansion | Clean GPIO. |
| **Right** | **GPIO13** | *Unused* | Reserved Expansion | Clean GPIO. |
| **Right** | **GPIO43** | UART0 TX | Serial Programming | Reserved for hardware UART0; not used for peripherals. |
| **Right** | **GPIO44** | UART0 RX | Serial Programming | Reserved for hardware UART0; not used for peripherals. |

> [!IMPORTANT]
> **Reserved Pins Avoided:**
> - `GPIO0` (Strapping / Bootloader button)
> - `GPIO19` & `GPIO20` (Native USB D- and D+ CDC serial line)
> - `GPIO26` through `GPIO32` (Internal Quad/Octal SPI Flash and PSRAM bus)
> - `GPIO45` & `GPIO46` (Strapping pins for VDD_SPI voltage and ROM boot printing)

---

## 2. Complete Breadboard & Jumper Wiring

```
                  +-----------------------------------+
                  |         ESP32-S3 Super Mini       |
                  |             [ USB-C ]             |
                  |                                   |
    MPU6050 VCC <-| [3V3]                       [5V]  |-> Laser VCC (if 5V module)
     Common GND <-| [GND]                       [GND] |-> Common GND
                  | [IO1]                       [IO8] |-> MPU6050 SDA
                  | [IO2]                       [IO9] |-> MPU6050 SCL
                  | [IO3] (Avoid)              [IO10] |
                  | [IO4]                      [IO11] |
  Button A (Sw) <-| [IO5]                      [IO12] |
  Button B (Sw) <-| [IO6]                      [IO13] |
Laser Driver In <-| [IO7]                      [TX43] |
                  +-----------------------------------+
```

---

## 3. Detailed Component Interfacing

### A. MPU6050 6-Axis IMU Module

Most commercial breakout modules (e.g., GY-521) include an onboard 3.3 V low-dropout regulator and 4.7 kΩ pull-up resistors on SDA and SCL to 3.3 V.

| MPU6050 Pin | ESP32-S3 Connection | Notes |
|:------------|:---------------------|:------|
| `VCC` | `3V3` (or `5V` if GY-521 has 662K 3.3V LDO) | Powering from 3V3 directly prevents level-shift mismatch on I2C. |
| `GND` | `GND` | Common reference ground. |
| `SCL` | `GPIO9` | I2C Clock line. |
| `SDA` | `GPIO8` | I2C Data line. |
| `XDA` | *Leave Unconnected* | Auxiliary I2C master (not used). |
| `XCL` | *Leave Unconnected* | Auxiliary I2C clock (not used). |
| `AD0` | Connect to `GND` | Sets I2C Address to `0x68`. (If tied to `3V3`, address becomes `0x69`). |
| `INT` | *Leave Unconnected* | Not required for 100 Hz polled burst-read mode. |

> [!TIP]
> Place a 100 nF ceramic decoupling capacitor directly across the MPU6050 `VCC` and `GND` pins to minimize power supply noise during RF transmission bursts from the ESP32-S3 BLE radio.

---

### B. Tactile Push Buttons (Button A and Button B)

The firmware configures internal pull-up resistors (`pinMode(pin, INPUT_PULLUP)`):

- **Button A (Left Click / Motion Pause):**
  - Terminal 1 -> `GPIO5`
  - Terminal 2 -> `GND`
- **Button B (Right Click / Gyro Recalibration / Laser Activation Gesture):**
  - Terminal 1 -> `GPIO6`
  - Terminal 2 -> `GND`

*No external resistors or capacitors are needed.* Debouncing (25 ms) and edge-detection are handled entirely by the deterministic state machine in [button_manager.cpp](file:///e:/Downloads/sentinal/src/input/button_manager.cpp).

---

### C. Low-Power Laser Module Driver Circuit

> [!CAUTION]
> **NEVER drive a laser diode directly from an ESP32-S3 GPIO pin.**
> 1. An ESP32-S3 GPIO pin is rated for a maximum source/sink current of **20 mA** (recommended ≤ 12 mA). Typical 650 nm 5 mW laser modules draw **25 mA to 45 mA**, which will overload and permanently damage the MCU output driver.
> 2. Bare laser diodes require constant-current regulation. Use a self-contained laser diode module with internal current-limiting resistor or driver.
> 3. Software alone cannot guarantee laser safety. A hardware pull-down resistor on the driver gate ensures that floating GPIO states during MCU reset, brownout, or firmware flashing hold the laser securely OFF.

#### Option 1: Logic-Level N-Channel MOSFET Driver (Recommended)

```
                              +3.3V (or +5V from USB)
                                     |
                                     +
                                 [ + Laser - ]  <-- Laser Module (with internal current limiting)
                                     -
                                     |
                                   Drain
                                 +---+
      ESP32-S3                   |   |
      GPIO7 -----> [ 220 Ohm ] --|   |  2N7000 / IRLML2502 / AO3400A (N-MOSFET)
                    R_gate       |   |
                         |       +---+
                         |         Source
                     [ 10k Ohm ]     |
                      R_pulldown     |
                         |           |
                        GND         GND
```

**BOM for Option 1:**
- **MOSFET:** 2N7000 (Through-hole TO-92) or IRLML2502 / AO3400A (SMD SOT-23). Must have logic-level gate threshold ($V_{GS(th)} \le 1.8\text{ V}$).
- **$R_{gate}$:** 220 Ω (limits gate capacitance inrush current from GPIO7).
- **$R_{pulldown}$:** 10 kΩ (guarantees gate voltage stays at 0 V during reset, boot, and high-impedance tri-state periods).
- **Laser Module:** 5 mW 650 nm red laser diode module with internal current limiter.

#### Option 2: NPN Bipolar Transistor Driver (Alternative)

```
                              +3.3V (or +5V from USB)
                                     |
                                     +
                                 [ + Laser - ]  <-- Laser Module
                                     -
                                     |
                                 Collector
                                 +---+
      ESP32-S3                   |   |
      GPIO7 -----> [ 1.0k Ohm ] -|   |  2N2222 / 2N3904 / SS8050 (NPN)
                     R_base      |   |
                         |       +---+
                         |        Emitter
                     [ 10k Ohm ]     |
                      R_pulldown     |
                         |           |
                        GND         GND
```

**BOM for Option 2:**
- **Transistor:** 2N2222, 2N3904, or SS8050.
- **$R_{base}$:** 1.0 kΩ ($I_B \approx (3.3\text{ V} - 0.7\text{ V}) / 1\text{ k}\Omega \approx 2.6\text{ mA}$, driving transistor into full saturation for up to 50 mA laser current).
- **$R_{pulldown}$:** 10 kΩ base-to-emitter pull-down.

---

## 4. Hardware Safety Precautions

1. **Laser Eye Safety (Class 2 / 3R):**
   - Use only low-power laser modules ($< 5\text{ mW}$). Never stare directly into the beam or point at reflective surfaces, eyes, or aircraft.
2. **Fail-Safe Electrical Design:**
   - If the ESP32-S3 is held in reset (boot button held, debugger attached, or firmware flashing), GPIO7 is configured as high-impedance (`HIGH-Z`). The 10 kΩ pull-down resistor discharges the gate/base to GND, ensuring the laser is completely OFF.
3. **Power Decoupling:**
   - When the laser driver switches ON, a brief current surge occurs. If the 3.3 V rail drops below 2.8 V, the ESP32-S3 brown-out detector (`BOD`) will trigger a system reset. Powering the laser from the 5V USB rail (isolated by the transistor) prevents dips on the sensitive 3.3V rail.
