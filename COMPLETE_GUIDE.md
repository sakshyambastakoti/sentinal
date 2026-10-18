# ESP32-S3 Air Mouse V1 — Complete System, Hardware & Wiring Manual
### Specifically Tailored for the **ESP32-S3FH4R2 Super Mini** (4MB Flash + 2MB PSRAM)

---

## 1. Physical Board Pinout & Wiring Diagram

Here is the exact pinout matching the silkscreen of your **ESP32-S3FH4R2 Super Mini**:

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

## 2. Complete Connection Table

| Board Pin | Physical Position | Target Peripheral Pin | Function | Notes |
|:---|:---|:---|:---|:---|
| **5V/VBUS** | Left Header, Pin 1 (Top) | **Toy Laser (+)** | Laser Power | Direct 5V from USB-C. Powers toy laser without loading 3.3V LDO. |
| **GND** | Left Header, Pin 2 | **Common Ground** | Ground Return | Tied to MPU6050 GND, Toy Laser (-), and Buttons common. |
| **3V3** | Left Header, Pin 3 | **MPU6050 VCC** | 3.3V Power | Powers MPU6050 logic cleanly from onboard 3.3V regulator. |
| **GPIO 8** | Left Header, Pin 6 | **MPU6050 SDA** | I2C Data | Hardware I2C SDA (400 kHz Fast-Mode). |
| **GPIO 9** | Left Header, Pin 7 (Bottom)| **MPU6050 SCL** | I2C Clock | Hardware I2C SCL (400 kHz Fast-Mode). |
| **GPIO 6** | Right Header, Pin 2 | **Button B** Pin 1 | Right-Click | Active-LOW (`INPUT_PULLUP`). Pin 2 goes to GND. Instant click response. |
| **GPIO 5** | Right Header, Pin 3 | **Button A** Pin 1 | Left-Click | Active-LOW (`INPUT_PULLUP`). Pin 2 goes to GND. Instant click response. |
| **GPIO 1, 2, 7, 4, 3, 10, 11** | Various | *Unused* | Expansion | Free GPIOs available for future features. |

---

## 3. Peripheral Details

### A. MPU6050 6-Axis Motion Sensor
- **VCC** $\rightarrow$ **`3V3`** (Left Header, Pin 3)
- **GND** $\rightarrow$ **`GND`** (Left Header, Pin 2)
- **SDA** $\rightarrow$ **`GPIO 8`** (Left Header, Pin 6)
- **SCL** $\rightarrow$ **`GPIO 9`** (Left Header, Pin 7)
- **AD0** $\rightarrow$ Connect to **`GND`** (Selects default I2C address `0x68`)
- **INT** $\rightarrow$ *Leave disconnected*

### B. Tactile Push Buttons (Button A & Button B)
The firmware automatically activates the ESP32-S3 internal pull-up resistors (`INPUT_PULLUP`):
- **Button A (Left Click / Motion Pause):**
  - Pin 1 $\rightarrow$ **`GPIO 5`** (Right Header, Pin 3)
  - Pin 2 $\rightarrow$ **`GND`** (Left Header, Pin 2)
- **Button B (Right Click / Gyro Recalibrate):**
  - Pin 1 $\rightarrow$ **`GPIO 6`** (Right Header, Pin 2)
  - Pin 2 $\rightarrow$ **`GND`** (Left Header, Pin 2)

*No external resistors or capacitors are needed.*

### C. Toy Laser Pointer Hookup
- **Laser Positive (+)** $\rightarrow$ **`5V/VBUS`** (Left Header, Pin 1)
  *(Or connect to `3V3` if your toy laser originally ran on 2 button cells / 3V).*
- **Laser Negative (-)** $\rightarrow$ **`GND`** (Left Header, Pin 2)
- Use the physical push button built into the toy laser body to turn the laser beam on and off.

---

## 4. Complete Firmware Feature Guide

### A. Gyroscope-Rate Air Mouse Engine
Most novice air mouse sketches map accelerometer tilt angles directly to mouse position, which leads to drifting and tilt lock. This firmware implements an **angular velocity pipeline**:
- **Physical Rate Tracking:** Only actual rotational wrist movement translates to cursor displacement. When your hand stops, the cursor stops dead on screen.
- **2-Second Startup Bias Calibration:** On boot, the firmware samples the gyroscope at 100 Hz to measure sensor bias.
- **Motion-Disturbed Rejection:** If the device is bumped or moved during calibration, the calibration is rejected with a serial warning, retaining safe default offsets.
- **Continuous Deadzone Ramp:** Suppresses natural micro-tremors below $1.25^\circ/\text{s}$ without creating an abrupt jump when exiting the deadzone.
- **First-Order IIR Smoothing:** Attenuates sensor noise ($\alpha = 0.45$).
- **Sub-Pixel Fractional Accumulation:** Fractional pixel movements are retained in memory across frames. Micro-aiming movements are never lost to integer truncation.
- **Dynamic Velocity Acceleration:** Smooth and precise at low speeds; accelerates across large monitors when moved quickly.

---

### B. Button Mapping & Gestures

| Physical Action | Resulting Event | Function |
|:---|:---|:---|
| **Button A Short Press** ($< 750\text{ ms}$) | `BUTTON_A_SHORT_PRESS` | **Left Click** on Host PC (instant on release) |
| **Button B Short Press** ($< 750\text{ ms}$) | `BUTTON_B_SHORT_PRESS` | **Right Click** on Host PC (instant on release) |
| **Button A Long Press** ($\ge 750\text{ ms}$) | `BUTTON_A_LONG_PRESS` | **Toggle Motion Pause / Resume** (Allows lifting/repositioning your hand without moving cursor) |
| **Button B Long Press** ($\ge 750\text{ ms}$) | `BUTTON_B_LONG_PRESS` | **Trigger Gyro Recalibration** (Keep device still to re-zero bias) |
| **Both Buttons Held Together** | `SIMULTANEOUS_PRESS` | Reserved for future V2 modes. |

---

### C. Bluetooth Low Energy (BLE) HID Stack
- Implemented with **NimBLE-Arduino** configured specifically for the ESP32-S3.
- Advertises as **`S3 Air Mouse V1`**.
- Pairs seamlessly with Windows 10, Windows 11, macOS, Linux, Android, and iOS.
- **Auto-Reconnection:** Automatically resumes advertising when disconnected and cleanly clears button states so keys never get stuck on your PC.

---

### D. USB CDC Serial Diagnostics & Terminal Commands
Connect via USB serial monitor at **115200 baud** for live telemetry:
- **`'d'` or `'D'`**: Toggle real-time motion streaming (`[MOTION] Gx Gy | HID dx dy`).
- **`'r'` or `'R'`**: Initiate stationary gyroscope recalibration over USB.
- **`'h'` or `'H'`**: Print interactive command help and system status.
- **Heartbeat:** Status log printed every 5 seconds (Uptime, State, BLE status, Free Heap).

---

## 5. How to Flash and Test

### Step 1: Open in VS Code
Open the project directory:
```
e:\Downloads\sentinal
```
Ensure the **PlatformIO IDE** extension is installed.

### Step 2: Build & Flash
Connect your ESP32-S3 Super Mini via USB-C and run:
```bash
# Compile and build the binary
pio run

# Flash to the board
pio run --target upload

# Open the serial monitor
pio device monitor
```

*(Note: If the board doesn't enter bootloader mode automatically, press and hold **BOOT**, press and release **RESET**, then release **BOOT**).*

### Step 3: Windows Pairing
1. Open **Windows Settings** $\rightarrow$ **Bluetooth & devices** $\rightarrow$ **Add device**.
2. Select **Bluetooth**.
3. Click on **`S3 Air Mouse V1`**.
4. Windows will pair and configure the device as a standard BLE HID mouse.

---

## 6. Quick Functional Acceptance Checklist

1. **Boot Test:** Open the serial monitor at 115200 baud. You should see the startup banner, MPU6050 address `0x68`, and calibration completion.
2. **Cursor Motion:** Rotate the board left/right (yaw) and up/down (pitch). The PC mouse cursor should move smoothly.
3. **Stationary Test:** Rest the board on a flat table. The cursor should stay completely stationary with zero drift or jitter.
4. **Left / Right Click:** Tap Button A for left click; tap Button B for right click (both respond instantly on release).
5. **Motion Pause:** Press and hold Button A for $\ge 750\text{ ms}$. Notice the log prints `Air Mouse Paused`. Move the board—the cursor will not move. Long press Button A again to resume.
6. **Laser Activation:** Press the button on your toy laser pointer body with your finger—it lights up directly from the 5V power rail.
