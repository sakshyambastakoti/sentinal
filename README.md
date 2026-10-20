# Sentinal — 3-in-1 ESP32-S3 Wireless Air Mouse & Controller

[![Release](https://img.shields.io/badge/Release-v1.0.0-blue.svg)](https://github.com/sakshyambastakoti/sentinal/releases)
[![Platform](https://img.shields.io/badge/Platform-ESP32--S3-orange.svg)](https://www.espressif.com/en/products/socs/esp32-s3)
[![Framework](https://img.shields.io/badge/Framework-PlatformIO%20%7C%20Arduino-brightgreen.svg)](https://platformio.org/)
[![Bluetooth](https://img.shields.io/badge/BLE-NimBLE--Arduino-purple.svg)](https://github.com/h2zero/NimBLE-Arduino)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

**Sentinal** is a production-grade, multi-mode handheld Bluetooth Low Energy (BLE) HID controller designed for the **ESP32-S3FH4R2 Super Mini** micro-board and an **MPU6050 6-axis IMU**.

Sentinal operates as a unified composite HID peripheral (Mouse + Keyboard + Consumer Control) seamlessly compatible with **Windows**, **macOS**, **Linux**, **Android**, and **Apple iOS**.

---

## 1. Core Operating Modes

Sentinal features 3 distinct operational modes with instant, non-blocking hardware switching and visual LED blink indication:

```
  ┌─────────────────────────────────────────────────────────────────┐
  │                        SENTINAL MODES                           │
  ├───────────────────┬──────────────────────┬──────────────────────┤
  │ Mode 1 (1 Blink)  │   Mode 2 (2 Blinks)  │   Mode 3 (3 Blinks)  │
  │     AIR MOUSE     │ PRESENTATION REMOTE  │    REEL SCROLLER     │
  │  PC / Mac / Linux │  PowerPoint / Keynote│ Instagram / TikTok   │
  └───────────────────┴──────────────────────┴──────────────────────┘
```

### Mode 1: 6-Axis Gyro Air Mouse (1 LED Blink)
- **High-Precision Cursor Motion:** Powered by a 100 Hz angular velocity pipeline (gyro rate, not unstable accelerometer gravity). Micro-tremor deadzone, first-order IIR smoothing ($\alpha = 0.45$), sub-pixel fractional accumulator, and velocity acceleration.
- **Button A (Tap):** Left-Click.
- **Button B (Tap):** Right-Click.
- **Button A (Hold $\ge 700\text{ ms}$):** Freeze / Reposition Pointer (re-centering without moving on-screen cursor).
- **Button B (Hold $\ge 700\text{ ms}$):** Stationary Zero-Rate Gyro Recalibration.

### Mode 2: Presentation Remote (2 LED Blinks)
- Air mouse cursor movement is paused for steady hand delivery.
- **Button A (Tap):** Next Slide (Side **`RIGHT_ARROW`** key).
- **Button B (Tap):** Previous Slide (Side **`LEFT_ARROW`** key).
- **Button A (Hold $\ge 700\text{ ms}$):** Start / Toggle Fullscreen Presentation (`F5`).
- **Button B (Hold $\ge 700\text{ ms}$):** Stationary Gyro Recalibration.

### Mode 3: Social Reel / Short Video Scroller (3 LED Blinks)
- Pure keyboard shortcut navigation with **zero cursor movement or screen dragging**.
- **Button A (Tap):** Next Video (**`DOWN_ARROW`**).
- **Button B (Tap):** Previous Video (**`UP_ARROW`**).
- **Button A (Hold $\ge 700\text{ ms}$):** Play / Pause Toggle (**`SPACE`**).
- **Button B (Hold $\ge 700\text{ ms}$):** Stationary Gyro Recalibration.

---

## 2. Hardware Pinout & Wiring Specifications

Engineered specifically for the **ESP32-S3FH4R2 Super Mini**:

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

### Complete Connection Table

| Board Pin | Physical Position | Peripheral Target | Logic / Wiring Details |
|:----------|:------------------|:------------------|:-----------------------|
| **5V / VBUS** | Left Header, Pin 1 | **Toy Laser (+)** | Raw 5V bus directly from USB-C. |
| **GND** | Left Header, Pin 2 | **Common Ground** | Star ground: MPU6050 GND, Laser (-), Buttons, and LED Cathodes. |
| **3V3** | Left Header, Pin 3 | **MPU6050 VCC** | Clean 3.3V power from onboard LDO. |
| **GPIO 1** | Left Header, Pin 4 | **MPU6050 SDA** | Hardware I2C SDA (100 kHz standard mode). |
| **GPIO 2** | Left Header, Pin 5 | **MPU6050 SCL** | Hardware I2C SCL (100 kHz standard mode). |
| **GPIO 7** | Right Header, Pin 1 | **Button Mode** | Active-LOW (`INPUT_PULLUP`). Tap to cycle mode, hold to reset. |
| **GPIO 6** | Right Header, Pin 2 | **Button B** | Active-LOW (`INPUT_PULLUP`). Action B / Prev / Recalib. |
| **GPIO 5** | Right Header, Pin 3 | **Button A** | Active-LOW (`INPUT_PULLUP`). Action A / Next / Pause. |
| **GPIO 4** | Right Header, Pin 4 | **Mode LED Anode (+)** | 1x, 2x, 3x flashes on mode changes (via 220Ω to GND). |
| **GPIO 3** | Right Header, Pin 5 | **Status LED Anode (+)** | Bluetooth status: 1 Hz blink = advertising, Solid ON = connected. |
| **GPIO 10** | Right Header, Pin 6 | **Action LED Anode (+)** | Lights up during button presses and clicks. |
| **GPIO 48** | Built-in PCB LED | **Onboard LED** | Active-LOW: Automatically mirrors Status and Action signals. |

---

## 3. Bluetooth Pairing & Device Name

- **Advertised Bluetooth Name:** **`Sentinal`**
- **Manufacturer:** **`Sentinal`**
- **Appearance:** `0x03C0` (Generic HID — enables Composite Mouse + Keyboard HID drivers across Windows, Mac, Android, and iOS)
- **Security:** BLE "Just Works" pairing with persistent bonded encryption keys.

> [!IMPORTANT]
> **CRITICAL FIRST-TIME STEP (Host Bluetooth GATT Cache Invalidation):**
> If you have previously connected Sentinal to your PC, Mac, or phone, your host operating system has cached the old single-device mouse GATT table.
> You **MUST** open your host's Bluetooth settings, click **"Remove device" / "Unpair"** on `Sentinal`, and re-pair fresh. This forces Windows/macOS/Android to enumerate the new composite Keyboard + Mouse descriptor so presentation keys and reel controls function properly!

### Pairing with Host Devices:
1. Power on Sentinal or plug into USB-C.
2. The **Status LED (GPIO 3 / GPIO 48)** will blink steadily at 1 Hz while awaiting connection.
3. On your PC, Mac, Android, or iPhone, open **Bluetooth Settings** and search for devices.
4. Select **`Sentinal`** and click **Pair / Connect**.
5. Once paired, the **Status LED turns solid ON**.

---

## 4. Operating Modes & Controls

Sentinal supports 4 deterministic operating modes. On boot, it starts in **Mode 1 (Presentation Remote)**. When mode changes, the **Onboard LED (GPIO 48)** and **Mode LED (GPIO 4)** flash in bursts (**1x = Presentation, 2x = Air Mouse, 3x = Reels, 4x = Wireless OTA Portal**):

| Feature / Action | Mode 1: Presentation Remote (1 Blink) | Mode 2: Air Mouse (2 Blinks) | Mode 3: Reel Scroller (3 Blinks) | Mode 4: Wireless OTA Portal (4 Blinks) |
|:---|:---|:---|:---|:---|
| **Gyro Cursor Motion** | Paused (Zero shake on slides) | Active (Wrist-driven pointer) | Paused (Zero cursor displacement) | Paused (Zero motion reports) |
| **Button A (Tap)** | Next Slide (`PAGE_DOWN` / Left Click Fallback) | Left Mouse Click | Next Reel (`DOWN_ARROW` + Wheel Down) | Display OTA Network Status |
| **Button B (Tap)** | Previous Slide (`PAGE_UP` / Wheel Up Fallback) | Right Mouse Click | Previous Reel (`UP_ARROW` + Wheel Up) | Display OTA Portal Info |
| **Button A (Hold $\ge 700$ms)** | Start / Resume Slideshow (`F5`) | Toggle Pointer Motion Pause | Play / Pause Video (`SPACE`) | - |
| **Button B (Hold $\ge 700$ms)** | Stationary Gyro Recalibration | Stationary Gyro Recalibration | Stationary Gyro Recalibration | - |
| **Mode Btn (Tap)** | Cycle to Mode 2 (Air Mouse) | Cycle to Mode 3 (Reels) | Cycle to Mode 1 (Presentation) | Exit to Mode 1 (Presentation) |
| **Mode Btn (Hold 3s)** | **Launch Wireless OTA Web Portal** | **Launch Wireless OTA Web Portal** | **Launch Wireless OTA Web Portal** | Return to Mode 1 (Presentation) |

---

## 5. Wireless OTA Updates (Web Portal)

Sentinal includes a dual-partition Over-The-Air (OTA) update system (`min_spiffs.csv` with dual 1.875MB application slots) and an embedded web portal with a minimalist black-and-white interface (zero emojis).

### How to Perform a Wireless Update:
1. **Enter OTA Mode (Mode 4):**
   - **Press and hold Button Mode for 3 seconds** until the **Mode LED flashes 4 times** (or send `'4'` or `'o'` over the USB Serial terminal).
   - Sentinal will immediately start its fallback SoftAP hotspot:
     - **Wi-Fi SSID:** `Sentinal-OTA`
     - **Password:** *(None / Open Network)*
     - **Default Portal IP:** `http://192.168.4.1`
     - **mDNS Host:** `http://sentinal.local`
2. **Connect to Sentinal:**
   - On your laptop, phone, or tablet, connect to the Wi-Fi network **`Sentinal-OTA`**.
   - Open any browser and navigate to **`http://192.168.4.1`** (or `http://sentinal.local`).
3. **Wi-Fi Configuration (Optional Station Mode):**
   - Enter your local Wi-Fi network SSID and Password in the portal and click **CONNECT TO WIFI**.
   - Sentinal saves these credentials permanently to NVS flash and joins your local network, allowing future OTA access directly through your local LAN IP without disconnecting from the internet.
4. **Wireless Firmware Upload:**
   - In PlatformIO, build your updated binary (`pio run`). The compiled firmware is located at `.pio/build/esp32-s3-devkitc-1/firmware.bin`.
   - In the web portal, click **SELECT FIRMWARE (.BIN)** and choose your `.bin` file.
   - Click **UPLOAD FIRMWARE**. A real-time progress bar shows upload progress.
   - Once upload reaches 100%, Sentinal automatically verifies the image hash, switches OTA partitions, and restarts into the new firmware within 6 seconds.
5. **Exit OTA Mode:**
   - Tap **Button Mode** to return directly to Mode 1 (Presentation Remote). The Wi-Fi radio is immediately powered down to conserve battery and eliminate 2.4 GHz RF contention.

---

## 6. Apple iPhone / iOS Setup (AssistiveTouch)

On iOS, Apple requires AssistiveTouch to be enabled for apps like Instagram and TikTok to accept touch scroll gestures:

1. Open iPhone **Settings** $\rightarrow$ **Accessibility** $\rightarrow$ **Touch** $\rightarrow$ **AssistiveTouch**.
2. Toggle **AssistiveTouch** to **ON**.
3. In the same menu, verify **Perform Touch Gestures** is **ON**.
4. Adjust **Tracking Sensitivity** to approximately **30%** (about 1/3 speed).
5. Switch Sentinal to **Mode 3 (Reel Scroller)** by tapping **Button Mode** (or pressing A+B simultaneously) until the LED flashes 3 times.
6. Open Instagram Reels, TikTok, or YouTube Shorts and tap Button A or B to scroll seamlessly!

---

## 7. USB Serial Telemetry & Diagnostics

Sentinal provides continuous diagnostics over hardware USB CDC Serial at **115200 baud**:

| Command Key | Action | Description |
|:------------|:-------|:------------|
| `'1'` | Force Mode 1 | Instantly sets Presentation Remote mode. |
| `'2'` | Force Mode 2 | Instantly sets Air Mouse mode. |
| `'3'` | Force Mode 3 | Instantly sets Reel Scroller mode. |
| `'4'` / `'o'` / `'O'` | Force Mode 4 | Enters Wireless OTA Web Portal mode. |
| `'m'` / `'M'` | Cycle Operating Mode | Programmatically cycles Presentation $\rightarrow$ Air Mouse $\rightarrow$ Reels $\rightarrow$ Presentation. |
| `'a'` / `'A'` | Test Left Click | Sends a Left Mouse Click report over BLE. |
| `'b'` / `'B'` | Test Right Click | Sends a Right Mouse Click report over BLE. |
| `'k'` / `'K'` | Test Key Action | Tests active mode keyboard/wheel action. |
| `'d'` / `'D'` | Toggle Debug Stream | Streams live gyro rates and HID `dx`/`dy` deltas. |
| `'r'` / `'R'` | Stationary Recalibrate | Triggers 2-second zero-rate gyro offset calibration. |
| `'i'` / `'I'` | Invert Both Axes | Toggles inversion for both X and Y simultaneously. |
| `'x'` / `'X'` | Toggle Invert X | Flips horizontal axis direction on the fly. |
| `'y'` / `'Y'` | Toggle Invert Y | Flips vertical axis direction on the fly. |
| `'h'` / `'H'` | Help Menu | Prints firmware status and available serial commands. |

---

## 8. Build & Flash Instructions

### Prerequisites
- [Visual Studio Code](https://code.visualstudio.com/) + [PlatformIO IDE Extension](https://platformio.org/)
- High-quality USB-C data cable connected to the ESP32-S3 Super Mini.

### Compile & Upload:
```bash
# Clone the repository
git clone https://github.com/sakshyambastakoti/sentinal.git
cd sentinal

# Build the firmware
pio run

# Flash to your board (via USB)
pio run --target upload

# Open the serial monitor
pio device monitor
```

---

## 9. Version History

- **v1.0.0 (Current Release):**
  - Official project branding as **Sentinal**.
  - Bluetooth device broadcast name updated to **`Sentinal`**.
  - Fixed MPU6050 physical motion inversion (`INVERT_X = false`, `INVERT_Y = true`).
  - Added dedicated serial commands (`'x'`, `'y'`, `'i'`) for real-time axis fine-tuning.
  - Full support for 4 Operating Modes (Air Mouse, Presentation Remote, Reel Scroller, Wireless OTA Portal).
  - Built-in Wireless Over-The-Air (OTA) web portal with minimalist black-and-white theme, live progress bar, Wi-Fi NVS configuration, and dual 1.875MB application partition slots.
  - Tri-color multi-action LED driver (Mode, Status, Action, and Onboard LED).
  - High-performance NimBLE HID stack with auto-reconnection and composite Mouse/Keyboard/Consumer reports.
