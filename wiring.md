# Hardware Wiring Specification — ESP32-S3FH4R2 Super Mini

This document specifies the exact electrical wiring tailored to your **ESP32-S3FH4R2 Super Mini** board revision.

---

## 1. Physical Board Pinout & Wiring Diagram

The I2C pins are mapped to **GPIO 1** and **GPIO 2** (the verified I2C bus pins directly below `3V3` on your board):

```
                              ESP32-S3FH4R2 Super Mini
                               ┌─────────────────────┐
        Toy Laser (+) Wire <── │ [5V/VBUS]  [GPIO 7] │ <── Button Mode (Cycle Operating Modes)
         Common Ground GND <── │ [  GND  ]  [GPIO 6] │ <── Button B (Right Click / Recalib / Prev)
          MPU6050 VCC (3V) <── │ [  3V3  ]  [GPIO 5] │ <── Button A (Left Click / Pause / Next)
         MPU6050 SDA (I2C) <── │ [GPIO 1 ]  [GPIO 4] │ ──> Mode LED (Optional External Indicator)
         MPU6050 SCL (I2C) <── │ [GPIO 2 ]  [GPIO 3] │ ──> (Unused Expansion)
        (Unused Expansion) <── │ [GPIO 8 ]  [GPIO 10]│ ──> (Unused Expansion)
        (Unused Expansion) <── │ [GPIO 9 ]  [GPIO 11]│ ──> (Unused Expansion)
                               │         [USB-C]     │
                               └─────────────────────┘
```

---

## 2. Complete Connection Table for Your Hardware

| Board Pin | Physical Position | Target Peripheral Pin | Wire Function | Notes |
|:---|:---|:---|:---|:---|
| **5V/VBUS** | Left Header, Pin 1 (Top) | **Toy Laser (+)** | Power | Raw 5V directly from USB-C. Powers toy laser without loading 3.3V LDO. |
| **GND** | Left Header, Pin 2 | **Common Ground** | Ground Return | Tied to MPU6050 GND, Toy Laser (-), and all 3 Buttons common ground. |
| **3V3** | Left Header, Pin 3 | **MPU6050 VCC** | 3.3V Logic Power | Powers MPU6050 logic cleanly from onboard 3.3V regulator. |
| **GPIO 1** | Left Header, Pin 4 | **MPU6050 SDA** | I2C Data | Hardware I2C SDA (100 kHz Standard Mode). Directly below 3V3. |
| **GPIO 2** | Left Header, Pin 5 | **MPU6050 SCL** | I2C Clock | Hardware I2C SCL (100 kHz Standard Mode). Directly below GPIO 1. |
| **GPIO 8** | Left Header, Pin 6 | *Unused* | — | Free GPIO. |
| **GPIO 9** | Left Header, Pin 7 (Bottom)| *Unused* | — | Free GPIO. |
| **GPIO 7** | Right Header, Pin 1 (Top)| **Button Mode** Terminal 1 | Input (`PULLUP`) | **Mode Switch Button.** Terminal 2 to GND. Cycles modes on tap; resets on hold. |
| **GPIO 6** | Right Header, Pin 2 | **Button B** Terminal 1 | Input (`PULLUP`) | Right-Click / Gyro Recalibration / Prev. Terminal 2 goes to GND. |
| **GPIO 5** | Right Header, Pin 3 | **Button A** Terminal 1 | Input (`PULLUP`) | Left-Click / Motion Pause / Next. Terminal 2 goes to GND. |
| **GPIO 4** | Right Header, Pin 4 | **Mode LED Anode (+)** | Output | Optional external Mode LED (via 220Ω resistor to GND). |
| **GPIO 3** | Right Header, Pin 5 | *Unused* | — | Free GPIO. |
| **GPIO 10**| Right Header, Pin 6 | *Unused* | — | Free GPIO. |
| **GPIO 11**| Right Header, Pin 7 (Bottom)| *Unused* | — | Free GPIO. |

---

## 3. Peripheral Details

### A. MPU6050 6-Axis Motion Sensor
- **VCC** $\rightarrow$ **`3V3`** (Left Header, Pin 3)
- **GND** $\rightarrow$ **`GND`** (Left Header, Pin 2)
- **SDA** $\rightarrow$ **`GPIO 1`** (Left Header, Pin 4)
- **SCL** $\rightarrow$ **`GPIO 2`** (Left Header, Pin 5)
- **AD0** $\rightarrow$ Connect to **`GND`** (Selects default I2C address `0x68`)
- **INT** $\rightarrow$ *Leave disconnected*

---

### B. Tactile Push Buttons (Button A, Button B & Mode Button)
The firmware automatically activates the ESP32-S3 internal pull-up resistors (`INPUT_PULLUP`):
- **Button Mode (Dedicated Mode Switch Button):**
  - Pin 1 $\rightarrow$ **`GPIO 7`** (Right Header, Pin 1 — Top Right)
  - Pin 2 $\rightarrow$ **`GND`** (Left Header, Pin 2)
  - *Tap:* Cycles Mode 1 (Air Mouse) $\rightarrow$ Mode 2 (Presentation Remote) $\rightarrow$ Mode 3 (Reel Scroller)
  - *Hold ($\ge 700\text{ ms}$):* Resets directly to Mode 1 (Air Mouse)
- **Button A (Left Click / Motion Pause / Next Slide / Next Reel):**
  - Pin 1 $\rightarrow$ **`GPIO 5`** (Right Header, Pin 3)
  - Pin 2 $\rightarrow$ **`GND`** (Left Header, Pin 2)
- **Button B (Right Click / Gyro Recalibrate / Prev Slide / Prev Reel):**
  - Pin 1 $\rightarrow$ **`GPIO 6`** (Right Header, Pin 2)
  - Pin 2 $\rightarrow$ **`GND`** (Left Header, Pin 2)

*All 3 buttons connect between their respective GPIO and common GND. No external resistors or capacitors are needed.*

---

### C. Toy Laser Pointer Hookup
- **Laser Positive (+)** $\rightarrow$ **`5V/VBUS`** (Left Header, Pin 1)
- **Laser Negative (-)** $\rightarrow$ **`GND`** (Left Header, Pin 2)
- Use the physical push button built into the toy laser body to turn the laser beam on and off.

---

### D. Mode Indicator Feedback
- **Onboard LED (GPIO 48) & External Mode LED (GPIO 4):**
  - Mode 1 (`AIR_MOUSE`): **1 Blink**
  - Mode 2 (`PRESENTATION`): **2 Blinks**
  - Mode 3 (`REEL_SCROLLER`): **3 Blinks**
