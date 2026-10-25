#pragma once
#include <Arduino.h>

// ==========================================
// Wi-Fi Configuration
// ==========================================
// Replace with your local Wi-Fi credentials
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Optional: Fallback mDNS hostname (e.g., http://pomodoro.local)
#define ENABLE_MDNS true
const char* MDNS_HOSTNAME = "pomodoro";

// ==========================================
// Hardware Pin Definitions
// ==========================================
// ESP32 DevKit V1 Pin Assignments
const uint16_t PIN_IR_LED   = 4;   // IR Emitter LED (via 100R-220R resistor to Anode)
const uint16_t PIN_IR_RECV  = 15;  // VS1838B IR Receiver (listening to physical remote)
const uint8_t  PIN_TRIG     = 5;   // HC-SR04 Ultrasonic Trigger
const uint8_t  PIN_ECHO     = 18;  // HC-SR04 Ultrasonic Echo (use voltage divider from 5V!)
const uint8_t  PIN_DHT      = 27;  // DHT11 Data pin (with 10k pull-up if bare sensor)
#define DHTTYPE DHT11              // DHT 11 or DHT 22

// Optional: Direct 4-Pin RGB LED pins for 2 Strips (Calibrated to match hardware)
#define ENABLE_DIRECT_RGB true

// Set to true for Common Anode (longest pin to 3.3V/5V), false for Common Cathode (longest pin to GND)
#define RGB_COMMON_ANODE true

// Enable external IR transmitter LED (GPIO 4) to blast commands to external commercial light bulbs.
// Keep FALSE when using direct breadboard RGB LEDs to prevent optical self-interference with VS1838B!
#define ENABLE_EXTERNAL_IR_TRANSMIT false

// RGB Strip 1: Physical breadboard guide wiring (Red on D23, Green on D22, Blue on D21)
const uint8_t PIN_RGB1_RED   = 23;  // Red pin (D23)
const uint8_t PIN_RGB1_GREEN = 22;  // Green pin (D22)
const uint8_t PIN_RGB1_BLUE  = 21;  // Blue pin (D21)

// RGB Strip 2: Pins matching ESP32 DevKit V1 silk-screen
const uint8_t PIN_RGB2_RED   = 19;  // Red pin (D19)
const uint8_t PIN_RGB2_GREEN = 17;  // Green pin (TX2 / GPIO 17)
const uint8_t PIN_RGB2_BLUE  = 16;  // Blue pin (RX2 / GPIO 16)

// ==========================================
// Complete 24-Key RGB IR Remote Codes (NEC 32-bit)
// ==========================================
// Row 1
const uint32_t IR_BRIGHT_UP  = 0xF700FF;
const uint32_t IR_BRIGHT_DN  = 0xF7807F;
const uint32_t IR_POWER_OFF  = 0xF740BF;
const uint32_t IR_POWER_ON   = 0xF7C03F;

// Row 2
const uint32_t IR_RED        = 0xF720DF;
const uint32_t IR_GREEN      = 0xF7A05F;
const uint32_t IR_BLUE       = 0xF7609F;
const uint32_t IR_WHITE      = 0xF7E01F;

// Row 3
const uint32_t IR_ORANGE     = 0xF710EF;
const uint32_t IR_PEA_GREEN  = 0xF7906F;
const uint32_t IR_DARK_BLUE  = 0xF750AF;
const uint32_t IR_FLASH      = 0xF7D02F;

// Row 4
const uint32_t IR_DARK_ORANGE= 0xF730CF;
const uint32_t IR_CYAN       = 0xF7B04F;
const uint32_t IR_BROWN      = 0xF7708F;
const uint32_t IR_STROBE     = 0xF7F00F;

// Row 5
const uint32_t IR_LIGHT_ORANGE=0xF708F7;
const uint32_t IR_LIGHT_BLUE = 0xF78877;
const uint32_t IR_PINK       = 0xF748B7;
const uint32_t IR_FADE       = 0xF7C837;

// Row 6
const uint32_t IR_YELLOW     = 0xF728D7;
const uint32_t IR_SKY_BLUE   = 0xF7A857;
const uint32_t IR_PURPLE     = 0xF76897;
const uint32_t IR_SMOOTH     = 0xF7E817;

// ==========================================
// Pomodoro & Automation Timing Parameters
// ==========================================
// Durations in milliseconds
const unsigned long DEFAULT_WORK_DURATION_MS  = 25UL * 60UL * 1000UL; // 25 minutes
const unsigned long DEFAULT_BREAK_DURATION_MS = 5UL  * 60UL * 1000UL; // 5 minutes

// Ultrasonic Presence Parameters
const float PRESENCE_THRESHOLD_CM             = 75.0f; // Max chair-to-desk distance considered "present"
const unsigned long PRESENCE_CHECK_INTERVAL_MS= 800UL; // How frequently to ping distance (ms)
const unsigned long ABSENCE_DEBOUNCE_MS       = 8000UL;// Must be absent for 8s continuously before pausing
const unsigned long RETURN_DEBOUNCE_MS        = 1500UL;// Must be present for 1.5s before auto-resuming

// DHT Sensor Interval
const unsigned long DHT_READ_INTERVAL_MS      = 3000UL;// Read DHT every 3 seconds
