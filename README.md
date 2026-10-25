# ESP32 Smart Pomodoro & Desk Environment System

A contact-free, non-invasive smart desk companion built on an **ESP32 DevKit V1**. It merges **automated focus tracking (Pomodoro technique)**, **chair presence detection via ultrasonic sensing**, **ambient environmental monitoring (DHT11)**, and **wireless IR control of 5V RGB LED light strips** with an interactive local Wi-Fi dashboard.

---

## 1. Key Features

- **Virtual IR Remote Control:** Simulates 38kHz NEC infrared signals to command cheap 5V USB RGB LED strips without cutting wires or adding MOSFETs.
- **Presence-Aware Pomodoro Automation:** Automatically counts down 25-minute focus sprints (turning lights RED) and 5-minute recovery breaks (turning lights GREEN).
- **Intelligent Auto-Pause & Resume:** Ultrasonic sensor tracks chair occupancy. If you walk away from your desk, the timer pauses automatically and resumes when you sit back down.
- **Responsive Dark-Mode Web Dashboard:** Hosted directly on the ESP32 via Wi-Fi (`http://pomodoro.local` or direct IP). Provides live countdown timers, cycle counts, presence distance, temperature, humidity, and an on-screen RGB remote.
- **Ambient Room Telemetry:** Measures temperature (°C) and relative humidity (%) using a DHT11 sensor.
- **Built-in IR Code Sniffer Tool:** Includes a dedicated setup sketch (`ir_sniffer`) to learn hex codes from any Chinese 24-key or 44-key RGB remote.

---

## 2. Hardware Pinout & Circuit Schematic

### Pin Allocation Map

| Component | Pin / Terminal | ESP32 Connection | Notes |
| :--- | :--- | :--- | :--- |
| **HC-SR04** | VCC | 5V / VIN Rail | Power from 5V rail |
| | GND | GND Rail | Common ground |
| | Trig | GPIO 5 | Digital Output (10µs trigger pulse) |
| | Echo | GPIO 18 *(via divider)* | **Must drop 5V to ~3.3V!** (1kΩ + 2kΩ divider) |
| **IR Emitter LED** | Anode (+) | GPIO 4 | In series with a 100Ω – 220Ω current-limiting resistor |
| | Cathode (-) | GND Rail | Shorter pin / flat edge |
| **VS1838B IR Receiver** *(Setup)*| OUT / Signal | GPIO 15 | Used for capturing remote codes |
| | VCC | 3.3V or 5V Rail | Supply voltage |
| | GND | GND Rail | Common ground |
| **DHT11 Sensor** | DATA | GPIO 27 | 10kΩ pull-up to 3.3V (often on module PCB) |
| | VCC | 3.3V or 5V Rail | Power rail |
| | GND | GND Rail | Common ground |
| **RGB Strip 1 (Direct/Wokwi)** | R / G / B / COM | GPIO 23, 22, 21 / GND | 220Ω series resistors on R, G, B |
| **RGB Strip 2 (Direct/Wokwi)** | R / G / B / COM | GPIO 19, 17, 16 / GND | 220Ω series resistors on R, G, B |
| **5V RGB USB Dongles** | IR Sensor | Facing IR LED | Align LED 2–10 cm from the dongle receivers |
| | USB Plug | 5V 2A USB Supply | Powered by external USB charger / host port |

> [!CAUTION]
> **HC-SR04 Echo Pin Voltage Protection:** Standard HC-SR04 sensors output 5V logic pulses on the Echo pin. ESP32 GPIO pins are rated for 3.3V max. Connecting Echo directly can damage the ESP32. Use a simple 2-resistor voltage divider:
> 
> ```
> HC-SR04 Echo Pin ----[ 1kΩ Resistor ]----+----> ESP32 GPIO 18
>                                          |
>                                    [ 2kΩ or 2.2kΩ Resistor ]
>                                          |
>                                         GND
> ```

---

## 3. Breadboard Wiring Schematic

```text
                           +----------------------+
                           |     ESP32 DevKit     |
                           |                      |
[+5V VIN Rail] <-----------+ VIN              GND +-----------> [GND Rail]
[+3.3V Rail]   <-----------+ 3V3                  |
                           |  GPIO 4      GPIO 15 |
                           |  GPIO 5      GPIO 18 |
                           |  GPIO 27             |
                           +----------+-----------+
                                      |
============================== PERIPHERALS ==============================

1. HC-SR04 Ultrasonic Distance Sensor:
   VCC   ----------------------> [+5V Rail]
   GND   ----------------------> [GND Rail]
   Trig  ----------------------> ESP32 GPIO 5
   Echo  ---> [1kΩ] ---> GPIO 18 ---> [2kΩ] ---> [GND Rail]

2. 940nm IR Emitter LED:
   ESP32 GPIO 4 ---> [ 150Ω - 220Ω Resistor ] ---> Anode (+) [IR LED] Cathode (-) ---> [GND Rail]

3. VS1838B IR Receiver Module (Code Sniffer Setup):
   VCC   ----------------------> [+3.3V Rail] or [+5V Rail]
   GND   ----------------------> [GND Rail]
   OUT   ----------------------> ESP32 GPIO 15

4. DHT11 Temperature & Humidity Sensor:
   VCC   ----------------------> [+3.3V Rail]
   GND   ----------------------> [GND Rail]
   DATA  ----------------------> ESP32 GPIO 27  (with 10kΩ pull-up to 3.3V if bare sensor)

5. 5V RGB USB Controller Dongle:
   Align the ESP32's IR LED emitter directly at the IR sensor bulb of the USB dongle.
```

---

## 4. Quick Start & Setup Guide

### Option A: Using Arduino IDE
1. Open Arduino IDE and install the **ESP32 Board Package** via Boards Manager (`esp32` by Espressif Systems).
2. Install the required libraries via **Sketch -> Include Library -> Manage Libraries**:
   - `IRremoteESP8266` (by crankyoldgit)
   - `DHT sensor library` (by Adafruit)
   - `Adafruit Unified Sensor` (by Adafruit)
3. Connect your ESP32 via USB and select board **DOIT ESP32 DEVKIT V1** and your serial port.

#### Step 4.1: Sniffing Your Remote Codes (Optional)
If your 5V RGB strip uses a custom remote:
1. Open [`ir_sniffer/ir_sniffer.ino`](file:///Users/mac_knight141/myProjects/Esp32/ir_sniffer/ir_sniffer.ino).
2. Upload the sketch and open the **Serial Monitor** at **115200 baud**.
3. Point your remote at the VS1838B sensor on GPIO 15 and press buttons (`ON`, `OFF`, `RED`, `GREEN`, etc.).
4. Copy the detected 32-bit HEX values into `esp32_smart_pomodoro/config.h`.

#### Step 4.2: Flashing the Main Application
1. Open [`esp32_smart_pomodoro/esp32_smart_pomodoro.ino`](file:///Users/mac_knight141/myProjects/Esp32/esp32_smart_pomodoro/esp32_smart_pomodoro.ino).
2. Open [`esp32_smart_pomodoro/config.h`](file:///Users/mac_knight141/myProjects/Esp32/esp32_smart_pomodoro/config.h) and set your local Wi-Fi SSID and password:
   ```cpp
   const char* WIFI_SSID     = "Your_WiFi_Network";
   const char* WIFI_PASSWORD = "Your_WiFi_Password";
   ```
3. Upload the sketch.
4. Open the Serial Monitor at **115200 baud** to see the assigned IP address:
   ```text
   [WIFI] Connected Successfully!
   [WIFI] IP Address: http://192.168.1.120
   [mDNS] Responder started: http://pomodoro.local
   ```
5. Open your browser on any phone, tablet, or laptop connected to the same Wi-Fi network and visit `http://pomodoro.local` or the printed IP address.

---

### Option B: Using PlatformIO
1. Open the project folder in VS Code with the PlatformIO extension installed.
2. Update Wi-Fi credentials in `esp32_smart_pomodoro/config.h`.
3. To upload the main Pomodoro application:
   ```bash
   pio run -e esp32_smart_pomodoro --target upload
   pio device monitor -b 115200
   ```
4. To upload the IR sniffer tool:
   ```bash
   pio run -e ir_sniffer --target upload
   pio device monitor -b 115200
   ```

---

## 5. Standard 24-Key NEC Hex Reference

Most generic 5V USB RGB strip controllers adhere to standard Chinese 32-bit NEC encoding:

| Button | Hex Code (32-bit NEC) | Action / Light Effect |
| :--- | :--- | :--- |
| **Power ON** | `0xF7C03F` | Turn on strip |
| **Power OFF** | `0xF740BF` | Turn off strip |
| **Bright +** | `0xF700FF` | Increase brightness |
| **Bright -** | `0xF7807F` | Decrease brightness |
| **RED** | `0xF720DF` | Work session active color |
| **GREEN** | `0xF7A05F` | Break session active color |
| **BLUE** | `0xF7609F` | Deep focus / manual color |
| **WHITE** | `0xF7E01F` | Neutral desk illumination / reset |
| **ORANGE** | `0xF710EF` | Warm desk accent |
| **CYAN** | `0xF7B04F` | Cool blue accent |
| **PURPLE** | `0xF7708F` | Creative focus |
| **FLASH** | `0xF7D02F` | Strobe mode |
| **SMOOTH** | `0xF7C837` | Color gradient cycle |

---

## 6. Web Dashboard REST API

The ESP32 runs a lightweight REST API that you can also integrate into Home Assistant, Stream Deck, or terminal scripts:

| Endpoint | Method | Parameters | Description |
| :--- | :--- | :--- | :--- |
| `/api/status` | `GET` | *None* | Returns JSON with current state, timer, presence, distance, and temperature/humidity. |
| `/api/pomodoro` | `POST` | `action=start\|pause\|resume\|skip\|reset` | Controls the Pomodoro state machine. |
| `/api/cmd` | `POST` | `val=on\|off\|red\|green\|blue\|white\|...` | Triggers IR LED pulses for remote control. |
| `/api/settings` | `POST` | `autopause=1\|0` | Enables or disables presence-based automatic timer pausing. |

### Sample JSON Status Response
```json
{
  "state": "WORK",
  "isPaused": false,
  "totalSec": 1500,
  "remainingSec": 1342,
  "isUserPresent": true,
  "distanceCm": 54.2,
  "autoPauseEnabled": true,
  "cyclesCompleted": 2,
  "temperature": 23.8,
  "humidity": 48.0
}
```

---

## 7. Android Companion App (Git Submodule)

A native Android app built with **Jetpack Compose (Material 3)** and **Kotlin** with a minimum requirement of **Android 8.0 (API Level 26)** is integrated as a submodule in `android_app/` ([Repository Link](https://github.com/isharax9/esp32-smart-pomodoro-android)).

### Key App Features
- **Real-Time Countdown Dial:** Live synchronized timer with focus/break states and haptic background notifications.
- **Desk Telemetry:** Real-time chair presence (At Desk vs Away), ultrasonic distance gauge, and DHT11 room climate readings.
- **Virtual 24-Key RGB Controller:** Color swatches, brightness slider (10%–100%), dual-strip targeting, and dynamic lighting effects (Flash, Strobe, Fade, Smooth).
- **Fast Wi-Fi Connection:** Connects over the ESP32's SoftAP hotspot (`192.168.4.1`) or local network mDNS (`pomodoro.local`) with zero firmware modifications.

### Cloning with Submodules
To clone this project along with the Android app:
```bash
git clone --recurse-submodules https://github.com/isharax9/Esp32.git
```
Or if already cloned:
```bash
git submodule update --init --recursive
```

---

## 8. Project File Structure

```text
Esp32/
├── platformio.ini                     # Dual-environment PlatformIO config
├── README.md                          # Full project documentation & wiring
├── android_app/                       # [Submodule] Native Jetpack Compose Android App (minSdk 26)
├── esp32_smart_pomodoro/
│   ├── esp32_smart_pomodoro.ino       # Core firmware: Pomodoro + Wi-Fi + IR + Presence
│   ├── config.h                       # Wi-Fi credentials, pinouts, timing constants, IR codes
│   └── index_html.h                   # Modern dark-mode dashboard (embedded PROGMEM HTML/JS)
└── ir_sniffer/
    └── ir_sniffer.ino                 # Diagnostic sniffer to capture custom remote hex codes
```
