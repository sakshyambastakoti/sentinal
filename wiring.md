# Hardware Wiring Specification — ESP32-S3FH4R2 Super Mini

This document specifies the exact electrical wiring tailored to your specific **ESP32-S3FH4R2 Super Mini** board revision (featuring embedded 4MB Quad-SPI Flash and 2MB PSRAM).

---

## 1. Physical Board Pinout Diagram

This matches the physical silkscreen layout of your **ESP32-S3FH4R2 Super Mini**:

```
                              ESP32-S3FH4R2 Super Mini
                               ┌─────────────────────┐
        Toy Laser (+) Wire <── │ [5V/VBUS]  [GPIO 7] │ ──> (Unused Expansion)
         Common Ground GND <── │ [  GND  ]  [GPIO 6] │ <── Button B (Right Click / Recalib)
          MPU6050 VCC (3V) <── │ [  3V3  ]  [GPIO 5] │ <── Button A (Left Click / Pause)
        (Unused Expansion) <── │ [GPIO 1 ]  [GPIO 4] │ ──> (Unused Expansion)
        (Unused Expansion) <── │ [GPIO 2 ]  [GPIO 3] │ ──> (Unused Expansion)
         MPU6050 SDA (I2C) <── │ [GPIO 8 ]  [GPIO 10]│ ──> (Unused Expansion)
         MPU6050 SCL (I2C) <── │ [GPIO 9 ]  [GPIO 11]│ ──> (Unused Expansion)
                               │         [USB-C]     │
                               └─────────────────────┘
```

---

## 2. Complete Connection Table for Your Hardware

| Board Pin | Physical Position | Target Peripheral Pin | Wire Function | Notes |
|:---|:---|:---|:---|:---|
| **5V/VBUS** | Left Header, Pin 1 (Top) | **Toy Laser (+)** | Power | Raw 5V directly from USB-C. Powers toy laser without loading 3.3V LDO. |
| **GND** | Left Header, Pin 2 | **Common Ground** | Ground Return | Tied to MPU6050 GND, Toy Laser (-), and Buttons common. |
| **3V3** | Left Header, Pin 3 | **MPU6050 VCC** | 3.3V Logic Power | Powers MPU6050 logic cleanly from onboard 3.3V regulator. |
| **GPIO 1** | Left Header, Pin 4 | *Unused* | — | Free GPIO. |
| **GPIO 2** | Left Header, Pin 5 | *Unused* | — | Free GPIO. |
| **GPIO 8** | Left Header, Pin 6 | **MPU6050 SDA** | I2C Data | Hardware I2C SDA (400 kHz Fast-Mode). |
| **GPIO 9** | Left Header, Pin 7 (Bottom)| **MPU6050 SCL** | I2C Clock | Hardware I2C SCL (400 kHz Fast-Mode). |
| **GPIO 7** | Right Header, Pin 1 (Top)| *Unused* | — | Free GPIO. |
| **GPIO 6** | Right Header, Pin 2 | **Button B** Terminal 1 | Input (`PULLUP`) | Right-Click / Gyro Recalibration. Terminal 2 goes to GND. |
| **GPIO 5** | Right Header, Pin 3 | **Button A** Terminal 1 | Input (`PULLUP`) | Left-Click / Motion Pause-Resume. Terminal 2 goes to GND. |
| **GPIO 4** | Right Header, Pin 4 | *Unused* | — | Free GPIO. |
| **GPIO 3** | Right Header, Pin 5 | *Unused* | — | Free GPIO. |
| **GPIO 10**| Right Header, Pin 6 | *Unused* | — | Free GPIO. |
| **GPIO 11**| Right Header, Pin 7 (Bottom)| *Unused* | — | Free GPIO. |

---

## 3. Peripheral Details

### A. MPU6050 6-Axis Motion Sensor
- **VCC** $\rightarrow$ **`3V3`** (Left Header, Pin 3)
- **GND** $\rightarrow$ **`GND`** (Left Header, Pin 2)
- **SDA** $\rightarrow$ **`GPIO 8`** (Left Header, Pin 6)
- **SCL** $\rightarrow$ **`GPIO 9`** (Left Header, Pin 7)
- **AD0** $\rightarrow$ Connect to **`GND`** (Selects default I2C address `0x68`)
- **INT** $\rightarrow$ *Leave disconnected*

---

### B. Tactile Push Buttons (Button A & Button B)
The firmware automatically activates the ESP32-S3 internal pull-up resistors (`INPUT_PULLUP`):
- **Button A (Left Click / Motion Pause):**
  - Pin 1 $\rightarrow$ **`GPIO 5`** (Right Header, Pin 3)
  - Pin 2 $\rightarrow$ **`GND`** (Left Header, Pin 2)
- **Button B (Right Click / Gyro Recalibrate):**
  - Pin 1 $\rightarrow$ **`GPIO 6`** (Right Header, Pin 2)
  - Pin 2 $\rightarrow$ **`GND`** (Left Header, Pin 2)

*No external resistors or capacitors are needed.*

---

### C. Toy Laser Pointer Hookup
- **Laser Positive (+)** $\rightarrow$ **`5V/VBUS`** (Left Header, Pin 1)
  *(Or connect to `3V3` if your toy laser originally ran on 2 button cells / 3V).*
- **Laser Negative (-)** $\rightarrow$ **`GND`** (Left Header, Pin 2)
- Use the physical push button built into the toy laser body to turn the laser beam on and off.
