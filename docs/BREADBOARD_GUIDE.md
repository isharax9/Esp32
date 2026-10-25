# ESP32 Dual-RGB Desk Assistant Breadboard Guide

An interactive, visual version of this guide is available at [breadboard_guide.html](../breadboard_guide.html). You can open it directly in any web browser.

---

## 📌 Complete Pinout Table

### 1. RGB Strip 1 (Left Desk Light) — Common Anode
| Pin Name | Function | ESP32 Pin / Rail | Wire Color | Resistor |
| :--- | :--- | :--- | :--- | :--- |
| **Pin 1** | Red Channel | **GPIO 23** (D23) | 🔴 Red | 220Ω |
| **Pin 2** | Anode COM (+) | **+3.3V Rail** | 🟠 Orange | 220Ω |
| **Pin 3** | Green Channel | **GPIO 22** (D22) | 🟢 Green | 220Ω |
| **Pin 4** | Blue Channel | **GPIO 21** (D21) | 🔵 Blue | 220Ω |

---

### 2. RGB Strip 2 (Right Desk Light) — Common Anode
| Pin Name | Function | ESP32 Pin / Rail | Wire Color | Resistor |
| :--- | :--- | :--- | :--- | :--- |
| **Pin 1** | Red Channel | **GPIO 19** (D19) | 🔴 Red | 220Ω |
| **Pin 2** | Anode COM (+) | **+3.3V Rail** | 🟠 Orange | 220Ω |
| **Pin 3** | Green Channel | **GPIO 17** (TX2) | 🟢 Green | 220Ω |
| **Pin 4** | Blue Channel | **GPIO 16** (RX2) | 🔵 Blue | 220Ω |

---

### 3. HC-SR04 Ultrasonic Distance Sensor
| Sensor Pin | ESP32 Pin / Rail | Notes |
| :--- | :--- | :--- |
| **VCC** | **5V / VIN Rail** | HC-SR04 requires 5V to emit acoustic ping |
| **GND** | **GND Rail** | Common Ground |
| **TRIG** | **GPIO 5** (D5) | 10µs trigger pulse |
| **ECHO** | **GPIO 18** (D18) | Use 1kΩ / 2kΩ voltage divider to step 5V down to 3.3V |

---

### 4. Infrared (IR) Communication
| Component | Pin | ESP32 Pin / Rail | Notes |
| :--- | :--- | :--- | :--- |
| **VS1838B IR Receiver** | OUT (Data) | **GPIO 15** (D15) | Decodes 24-key NEC remote |
| | VCC | **+3.3V Rail** | |
| | GND | **GND Rail** | |
| **940nm IR LED Emitter** | Anode (+) | **GPIO 4** (via 220Ω) | Transmit to external commercial bulbs |
| | Cathode (-) | **GND Rail** | |

---

### 5. DHT11 / DHT22 Climate Sensor
| Sensor Pin | ESP32 Pin / Rail | Notes |
| :--- | :--- | :--- |
| **VCC** | **+3.3V Rail** | |
| **DATA** | **GPIO 27** (D27) | 10kΩ pull-up to 3.3V if using bare 4-pin DHT sensor |
| **GND** | **GND Rail** | |

---

## 🔍 Verification & Testing
- Open the mobile web dashboard at `http://192.168.4.1` on Wi-Fi `Desk-Pomodoro`.
- Navigate to the **⚙️ Pins** tab to test each individual R, G, and B channel independently.
- To inspect physical remote buttons, point your remote at the VS1838B receiver and check the **Live 24-Key Remote Sniffer** card.
