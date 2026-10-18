# ESP32-S3 Air Mouse V1 — Complete System, Hardware & Wiring Manual

This all-in-one guide covers:
1. **Laser Driver Hardware**: Why an external transistor/MOSFET is mandatory, and how to build one.
2. **Circuit Diagrams & Pin Connections**: Complete pin-by-pin wiring tables and ASCII schematics.
3. **Complete Feature Guide**: Air mouse motion processing, button gestures, calibration, and safety systems.
4. **Flashing & Setup**: How to compile, flash, pair with Windows, and run tests.

---

## 1. Laser Driver: Do You Need a MOSFET or Transistor?

### **YES, you MUST use a MOSFET or an NPN Transistor.**
You must **never connect a laser diode directly to an ESP32-S3 GPIO pin**. Here is why:

1. **Current Rating Limits:**
   - The ESP32-S3 GPIO pins can safely source only **12 mA to 20 mA** maximum.
   - Even a small 5 mW 650 nm red laser diode module draws between **30 mA and 50 mA**.
   - Connecting it directly will overload the ESP32-S3 internal output drivers, causing brownouts, GPIO destruction, or MCU failure.
2. **Safe Defaults During Boot & Reset:**
   - When the ESP32-S3 boots up or is reset, all GPIOs enter a high-impedance (floating) state.
   - Without an external driver circuit featuring a **10 kΩ pull-down resistor to GND**, electrical noise or floating pins could turn the laser ON unexpectedly, creating a significant eye-safety hazard.

---

### Three Ways to Connect the Laser

You can use **any of the three options below**, depending on what parts you have in your toolbox:

#### Option A: Logic-Level N-Channel MOSFET (Best Performance)
A logic-level MOSFET has almost zero voltage drop, runs completely cool, and turns on cleanly with the ESP32-S3's 3.3 V logic.

* **Suitable MOSFETs:** 2N7000 (standard TO-92 through-hole), IRLML2502, AO3400A, or BS170.
* **Gate Resistor ($R_1$):** 220 Ω (protects GPIO7 from capacitive inrush current).
* **Pull-Down Resistor ($R_2$):** 10 kΩ (forces Gate to GND so laser stays OFF during reboot).

```
                       +5V (from USB) or +3.3V
                                 |
                                 +
                           [ Laser Module ]  (Red wire / VCC)
                           [  with built- ]
                           [ in resistor  ]  (Blue or Black wire / GND)
                                 -
                                 |
                               Drain
                             +---+
  ESP32-S3                   |   |  2N7000 / IRLML2502
  GPIO7 -----> [ 220 Ohm ] --|   |  N-Channel MOSFET
                 R1 (Gate)   |   |
                     |       +---+
                     |         Source
                [ 10k Ohm ]      |
                 R2 (Pulldown)   |
                     |           |
                    GND         GND
```

---

#### Option B: NPN Bipolar Transistor (Most Common / Starter Kit Friendly)
If you don't have a MOSFET, a standard general-purpose NPN bipolar junction transistor works just as well.

* **Suitable Transistors:** 2N2222, 2N3904, BC547, SS8050, or S8050.
* **Base Resistor ($R_1$):** 1 kΩ (limits base current to $\approx 2.6\text{ mA}$, driving the transistor into full saturation).
* **Pull-Down Resistor ($R_2$):** 10 kΩ (base to GND pull-down).

```
                       +5V (from USB) or +3.3V
                                 |
                                 +
                           [ Laser Module ]  (Red wire / VCC)
                           [  with built- ]
                           [ in resistor  ]  (Black wire / GND)
                                 -
                                 |
                             Collector
                             +---+
  ESP32-S3                   |   |  2N2222 / 2N3904 / BC547
  GPIO7 -----> [ 1.0k Ohm ] -|   |  NPN Transistor
                 R1 (Base)   |   |
                     |       +---+
                     |        Emitter
                [ 10k Ohm ]      |
                 R2 (Pulldown)   |
                     |           |
                    GND         GND
```

---

#### Option C: Commercial 3-Pin Laser Breakout (e.g. KY-008 Module)
If you are using a 3-pin Arduino laser breakout board (such as the KY-008):
- Check whether your module includes a built-in transistor. Most standard KY-008 boards only contain a 650 nm diode and a current-limiting resistor without a switching transistor.
- If it does **not** have an onboard transistor, wire it using Option A or B above.
- If it is an active-high driver module (with pins labeled `S` / Signal, `+` / 5V, `-` / GND):
  - Connect `S` to **GPIO7** (add a 10 kΩ pull-down resistor between `S` and `GND`).
  - Connect `+` to **5V** (or 3.3V).
  - Connect `-` to **GND**.

---

## 2. Complete Circuit Connections & Pinout

### ESP32-S3 Super Mini Board Header Layout

When looking at the board with the **USB-C port at the top**:

```
                       +--------------------+
                       |     [ USB-C ]      |
        3.3V Power --> | [3V3]         [5V] | <-- 5V USB Power
        Common GND --> | [GND]        [GND] | <-- Common GND
                       | [IO1]        [IO8] | <-- MPU6050 SDA
                       | [IO2]        [IO9] | <-- MPU6050 SCL
                       | [IO3]*      [IO10] |
                       | [IO4]       [IO11] |
  Button A (Sw to GND) | [IO5]       [IO12] |
  Button B (Sw to GND) | [IO6]       [IO13] |
    Laser Gate/Base -> | [IO7]       [TX43] |
                       +--------------------+
                       * Avoid IO3 (internal VBUS divider on some boards)
```

---

### Master Pin Connection Table

| Peripheral | Component Pin | ESP32-S3 Super Mini Pin | Wire Color (Rec.) | Notes & Internal Configuration |
|:---|:---|:---|:---|:---|
| **MPU6050 IMU** | `VCC` | **3V3** | Red | Powered by onboard 3.3V regulator |
| | `GND` | **GND** | Black | Common circuit ground |
| | `SCL` | **GPIO9** | Yellow | Hardware I2C Clock (400 kHz Fast Mode) |
| | `SDA` | **GPIO8** | Green | Hardware I2C Data (400 kHz Fast Mode) |
| | `AD0` | **GND** | Black | Selects I2C Address `0x68` (tie to 3V3 for `0x69`) |
| | `INT` | *Leave Disconnected* | — | Not needed for 100 Hz polled burst mode |
| **Button A** | Terminal 1 | **GPIO5** | Orange | Configured as `INPUT_PULLUP` (active-LOW) |
| *(Left Click / Pause)* | Terminal 2 | **GND** | Black | Connects button directly to GND |
| **Button B** | Terminal 1 | **GPIO6** | White | Configured as `INPUT_PULLUP` (active-LOW) |
| *(Right Click / Laser)*| Terminal 2 | **GND** | Black | Connects button directly to GND |
| **Laser Driver** | Driver Input | **GPIO7** | Blue | Active-HIGH through 220 Ω (MOSFET) or 1 kΩ (NPN) |
| | Gate/Base Pulldown | **GND** | Black | **10 kΩ resistor to GND is mandatory** |
| | Laser VCC | **5V** or **3V3** | Red | Connect to 5V (USB) or 3V3 rail |
| | Transistor Emitter / Source | **GND** | Black | Tied to common Ground |

---

## 3. Complete Firmware Feature Guide

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
| **Button A Short Press** ($< 750\text{ ms}$) | `BUTTON_A_SHORT_PRESS` | **Left Click** on Host PC |
| **Button B Short Press** ($< 750\text{ ms}$) | `BUTTON_B_SHORT_PRESS` | **Right Click** on Host PC (emitted after 300 ms double-click window) |
| **Button A Long Press** ($\ge 750\text{ ms}$) | `BUTTON_A_LONG_PRESS` | **Toggle Motion Pause / Resume** (Allows lifting/repositioning your hand without moving cursor) |
| **Button B Long Press** ($\ge 750\text{ ms}$) | `BUTTON_B_LONG_PRESS` | **Trigger Gyro Recalibration** (Keep device still to re-zero bias) |
| **Button B Double-Tap & Hold** | `LASER_HOLD_ENGAGE` | **Laser ON while held**; switches OFF immediately upon release |
| **Both Buttons Held Together** | `SIMULTANEOUS_PRESS` | Reserved for future V2 modes. **Clicks & Laser are suppressed**. |

---

### C. Dedicated Laser Safety Architecture
1. **Default OFF:** GPIO7 is initialized `LOW` as the very first line of code executed in `setup()`.
2. **5.0-Second Hard Timeout:** If Button B is held down continuously (or stuck), the firmware automatically shuts OFF the laser at 5000 ms.
3. **1.0-Second Cooldown:** Following a timeout, the user must release the button and wait 1 second before the laser can be re-engaged.
4. **State Machine Interlocks:** The laser is unconditionally disabled during BLE disconnection, sensor errors, or gyro calibration.
5. **Periodic Hardware Audit:** Every loop iteration reads the physical state of GPIO7 and forces it `LOW` if safety criteria are not satisfied.

---

### D. Bluetooth Low Energy (BLE) HID Stack
- Implemented with **NimBLE-Arduino** for maximum reliability on the ESP32-S3.
- Advertises as **`S3 Air Mouse V1`**.
- Pairs seamlessly with Windows 10, Windows 11, macOS, Linux, Android, and iOS.
- **Auto-Reconnection:** Automatically resumes advertising when disconnected and cleanly clears button states so keys never get stuck on your PC.

---

### E. USB CDC Serial Diagnostics & Terminal Commands
Connect via USB serial monitor at **115200 baud** for live telemetry:
- **`'d'` or `'D'`**: Toggle real-time motion streaming (`[MOTION] Gx Gy | HID dx dy`).
- **`'r'` or `'R'`**: Initiate stationary gyroscope recalibration over USB.
- **`'h'` or `'H'`**: Print interactive command help and system status.
- **Heartbeat:** Status log printed every 5 seconds (Uptime, State, BLE status, Free Heap).

---

## 4. How to Flash and Test

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

*(Note: On ESP32-S3 boards, hold the **BOOT** button, press **RESET**, then release **BOOT** if the board doesn't automatically enter download mode).*

### Step 3: Windows Pairing
1. Open **Windows Settings** $\rightarrow$ **Bluetooth & devices** $\rightarrow$ **Add device**.
2. Select **Bluetooth**.
3. Click on **`S3 Air Mouse V1`**.
4. Windows will pair and configure the device as a standard BLE HID mouse.

---

## 5. Quick Functional Acceptance Checklist

1. **Boot Test:** Open the serial monitor at 115200 baud. You should see the startup banner, MPU6050 address `0x68`, and calibration completion.
2. **Cursor Motion:** Rotate the board left/right (yaw) and up/down (pitch). The PC mouse cursor should move smoothly.
3. **Stationary Test:** Rest the board on a flat table. The cursor should stay completely stationary with zero drift or jitter.
4. **Left / Right Click:** Tap Button A for left click; tap Button B for right click.
5. **Motion Pause:** Press and hold Button A for $\ge 750\text{ ms}$. Notice the log prints `Air Mouse Paused`. Move the board—the cursor will not move. Long press Button A again to resume.
6. **Laser Activation:** Quickly tap Button B and then immediately press and hold it. The laser turns ON and stays ON while held. Release to turn it OFF.
7. **Laser Timeout Test:** Double-tap and hold Button B for $> 5$ seconds. The laser will shut OFF automatically at 5 seconds.
