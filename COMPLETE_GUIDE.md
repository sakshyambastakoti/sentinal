# Sentinal — Complete System, Hardware & Wiring Manual (v1.0.0)
### Specifically Tailored for the **ESP32-S3FH4R2 Super Mini** (4MB Flash + 2MB PSRAM)

---

## 1. Physical Board Pinout & Wiring Diagram

The I2C pins are mapped to **GPIO 1** and **GPIO 2** (directly below `3V3` on the left header):

```
                              ESP32-S3FH4R2 Super Mini
                               ┌─────────────────────┐
        Toy Laser (+) Wire <── │ [5V/VBUS]  [GPIO 7] │ <── Button Mode (Cycle Operating Modes)
         Common Ground GND <── │ [  GND  ]  [GPIO 6] │ <── Button B (Right Click / Recalib / Prev)
          MPU6050 VCC (3V) <── │ [  3V3  ]  [GPIO 5] │ <── Button A (Left Click / Pause / Next)
         MPU6050 SDA (I2C) <── │ [GPIO 1 ]  [GPIO 4] │ ──> Mode LED (1x, 2x, 3x Mode Blinks)
         MPU6050 SCL (I2C) <── │ [GPIO 2 ]  [GPIO 3] │ ──> BLE Status LED (Blink=Searching, Solid=Connected)
        (Unused Expansion) <── │ [GPIO 8 ]  [GPIO 10]│ ──> Action LED (Lights while pressing button)
        (Unused Expansion) <── │ [GPIO 9 ]  [GPIO 11]│ ──> (Unused Expansion)
                               │         [USB-C]     │
                               └─────────────────────┘
```

---

## 2. Complete Connection Table

| Board Pin | Physical Position | Target Peripheral Pin | Function | Notes |
|:---|:---|:---|:---|:---|
| **5V/VBUS** | Left Header, Pin 1 (Top) | **Toy Laser (+)** | Laser Power | Direct 5V from USB-C. Powers toy laser without loading 3.3V LDO. |
| **GND** | Left Header, Pin 2 | **Common Ground** | Ground Return | Tied to MPU6050 GND, Toy Laser (-), 3 Buttons, and LED Cathodes. |
| **3V3** | Left Header, Pin 3 | **MPU6050 VCC** | 3.3V Power | Powers MPU6050 logic cleanly from onboard 3.3V regulator. |
| **GPIO 1** | Left Header, Pin 4 | **MPU6050 SDA** | I2C Data | Hardware I2C SDA (100 kHz Standard Mode). Directly below 3V3. |
| **GPIO 2** | Left Header, Pin 5 | **MPU6050 SCL** | I2C Clock | Hardware I2C SCL (100 kHz Standard Mode). Directly below GPIO 1. |
| **GPIO 7** | Right Header, Pin 1 (Top)| **Button Mode** Pin 1 | Mode Switch | Active-LOW (`INPUT_PULLUP`). Pin 2 to GND. Tap to cycle mode; hold to reset. |
| **GPIO 6** | Right Header, Pin 2 | **Button B** Pin 1 | Action B | Active-LOW (`INPUT_PULLUP`). Pin 2 to GND. Right-Click / Recalib / Prev. |
| **GPIO 5** | Right Header, Pin 3 | **Button A** Pin 1 | Action A | Active-LOW (`INPUT_PULLUP`). Pin 2 to GND. Left-Click / Pause / Next. |
| **GPIO 4** | Right Header, Pin 4 | **Mode LED Anode (+)**| Mode Indicator | **Dedicated Mode LED:** Blinks 1x (Air Mouse), 2x (Presentation), 3x (Reels). |
| **GPIO 3** | Right Header, Pin 5 | **BLE Status LED Anode (+)**| BLE Status | **Bluetooth Link:** Blinks slowly when searching; Solid ON when connected. |
| **GPIO 10**| Right Header, Pin 6 | **Action LED Anode (+)**| Action Feedback| **Button Action:** Lights up while pressing button; flashes on click. |
| **GPIO 8, 9, 11** | Various | *Unused* | Expansion | Free GPIOs available for future features. |
| *(Onboard)*| Built-in LED on PCB | **GPIO 48 (Onboard LED)** | Internal Output | Active-LOW: Automatically mirrors BLE Status and Button Action. |

---

## 3. Peripheral Details

### A. MPU6050 6-Axis Motion Sensor
- **VCC** $\rightarrow$ **`3V3`** (Left Header, Pin 3)
- **GND** $\rightarrow$ **`GND`** (Left Header, Pin 2)
- **SDA** $\rightarrow$ **`GPIO 1`** (Left Header, Pin 4)
- **SCL** $\rightarrow$ **`GPIO 2`** (Left Header, Pin 5)
- **AD0** $\rightarrow$ Connect to **`GND`** (Selects default I2C address `0x68`)
- **INT** $\rightarrow$ *Leave disconnected*

### B. Tactile Push Buttons (Button A, Button B & Mode Button)
The firmware automatically activates the ESP32-S3 internal pull-up resistors (`INPUT_PULLUP`):
- **Button Mode (Dedicated Mode Switch Button):**
  - Pin 1 $\rightarrow$ **`GPIO 7`** (Right Header, Pin 1 — Top Right opposite 5V/VBUS)
  - Pin 2 $\rightarrow$ **`GND`** (Left Header, Pin 2)
  - *Short Tap:* Cycles Operating Modes:
    1. **Mode 1: AIR_MOUSE** (LED 1 blink) — 6-Axis Desktop Air Mouse
    2. **Mode 2: PRESENTATION** (LED 2 blinks) — Slide Clicker Remote
    3. **Mode 3: REEL_SCROLLER** (LED 3 blinks) — TikTok / Reels / Shorts Scroller
  - *Long Hold ($\ge 700\text{ ms}$):* Resets directly back to Mode 1 (Air Mouse)
- **Button A (Action A):**
  - Pin 1 $\rightarrow$ **`GPIO 5`** (Right Header, Pin 3)
  - Pin 2 $\rightarrow$ **`GND`** (Left Header, Pin 2)
  - *Air Mouse Mode:* Left Click (tap) / Pause-Resume motion (hold)
  - *Presentation Mode:* Next Slide (**`PAGE_DOWN`** / Left Click Fallback for Canva & PowerPoint) / Fullscreen slideshow (`F5` hold)
  - *Reel Scroller Mode:* Next Reel (**`DOWN_ARROW`** + Wheel Down) / Play-Pause (`SPACE` hold)
- **Button B (Action B):**
  - Pin 1 $\rightarrow$ **`GPIO 6`** (Right Header, Pin 2)
  - Pin 2 $\rightarrow$ **`GND`** (Left Header, Pin 2)
  - *Air Mouse Mode:* Right Click (tap) / Stationary Recalibration (hold)
  - *Presentation Mode:* Previous Slide (**`PAGE_UP`** / Wheel Up Fallback for Canva & PowerPoint) / Stationary Recalibration (hold)
  - *Reel Scroller Mode:* Previous Reel (**`UP_ARROW`** shortcut, zero cursor movement) / Stationary Recalibration (hold)

*All 3 buttons connect between their respective GPIO and common GND. No external resistors or capacitors are needed.*

### C. Toy Laser Pointer Hookup
- **Laser Positive (+)** $\rightarrow$ **`5V/VBUS`** (Left Header, Pin 1)
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

## 5. How to Flash and Test

### Step 1: Build & Flash
Connect your ESP32-S3 Super Mini via USB-C and run:
```bash
# Compile and build the binary
pio run

# Flash to the board
pio run --target upload

# Open the serial monitor
pio device monitor
```

> [!IMPORTANT]
> **CRITICAL: Unpair / Forget "Sentinal" on your PC/Phone First!**
> If you have paired Sentinal previously, the host OS has cached the old single-mouse GATT table. Go into Bluetooth Settings on your PC or phone, select **"Remove device" / "Unpair"**, and pair anew. This tells the OS to discover the new composite Keyboard & Mouse reports so arrow keys and reel controls work!

---

## 6. Quick Functional Acceptance Checklist

1. **Boot Test:** Open the serial monitor at 115200 baud (`pio device monitor`). You will see the startup banner displaying pinout, detected MPU6050 address `0x68`, and calibration completion. The **Onboard LED (GPIO 48)** flashes 1x to confirm Mode 1 (`AIR_MOUSE`).
2. **Corrected Cursor Motion:**
   - Tilt UP $\rightarrow$ Cursor moves UP.
   - Tilt DOWN $\rightarrow$ Cursor moves DOWN.
   - Rotate LEFT $\rightarrow$ Cursor moves LEFT.
   - Rotate RIGHT $\rightarrow$ Cursor moves RIGHT.
3. **Stationary Test:** Rest the board on a flat table. The cursor stays completely stationary with zero drift or jitter.
4. **Left / Right Click (Air Mouse Mode):** Tap Button A for left click; tap Button B for right click (held for solid 35ms pulse for guaranteed OS detection).
5. **Motion Pause:** Press and hold Button A for $\ge 700\text{ ms}$. Cursor freezes for wrist repositioning. Long press Button A again to resume.
6. **Mode Switching (Mode Button GPIO 7 OR A+B Simultaneously):**
   - **Switch to Mode 2 (`PRESENTATION`):** Onboard LED flashes twice. Gyro cursor motion pauses (zero shake). Button A sends **Right Arrow** (`KEY_RIGHT_ARROW`), Button B sends **Left Arrow** (`KEY_LEFT_ARROW`). Long hold Button A launches fullscreen (`F5`).
   - **Switch to Mode 3 (`REEL_SCROLLER`):** Onboard LED flashes 3 times. Button A sends **Down Arrow** + Mouse Wheel Down (-5), Button B sends **Up Arrow** + Mouse Wheel Up (+5). Long hold Button A toggles Play/Pause (`SPACE`).
   - **Switch back to Mode 1 (`AIR_MOUSE`):** Onboard LED flashes once. Gyro cursor movement immediately resumes!

---

## 7. iPhone / iOS Reel Scroller Setup (Mandatory Apple Settings)

Unlike PCs and Macs (which naturally accept keyboard arrow keys or mouse wheels to change videos in web browsers), **Apple iOS strictly blocks Bluetooth HID devices from performing touch drag/swipe gestures unless AssistiveTouch is enabled**.

All commercial Bluetooth TikTok/Reels scroller rings (D01, E-Ring, etc.) require this 1-minute setup:

### Step-by-Step iPhone Configuration:
1. **Enable AssistiveTouch:**
   - Open iPhone **Settings** $\rightarrow$ **Accessibility** $\rightarrow$ **Touch** $\rightarrow$ **AssistiveTouch**.
   - Toggle **AssistiveTouch** to **ON** (a small circular floating button will appear on your screen).
2. **Verify Touch Gestures:**
   - In that same AssistiveTouch menu, make sure **Perform Touch Gestures** is **ON**.
3. **Adjust Tracking Sensitivity:**
   - Scroll down to the **Tracking Sensitivity** slider in the same menu.
   - Adjust the turtle-to-rabbit slider to approximately **25% – 35%** (about 1/3 speed). *If set too high or too low, iOS may misinterpret single-reel pagination.*
4. **Positioning the Cursor:**
   - In **Mode 1 (Air Mouse)**, tilt the device so the circular pointer sits roughly in the **middle** of your iPhone screen (not pinned at the very top or bottom status bar).
   - Tap **Button Mode** twice to enter **Mode 3 (`REEL_SCROLLER`)** (LED blinks 3 times).
   - Open **Instagram Reels**, **TikTok**, or **YouTube Shorts**.
   - **Tap Button A:** Instantly flings to the **Next Reel**.
   - **Tap Button B:** Instantly flings to the **Previous Reel**.
   - **Hold Button A ($\ge 700\text{ ms}$):** Double-taps the screen to **Heart / Like** the reel!
