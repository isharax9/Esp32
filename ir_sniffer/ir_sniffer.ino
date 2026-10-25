#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>

// ==========================================
// ESP32 IR Hardware Doctor & Diagnostic Tool
// ==========================================
const uint16_t PIN_IR_RECV = 15;
const uint16_t PIN_IR_LED  = 4;

IRrecv irrecv(PIN_IR_RECV);
IRsend irsend(PIN_IR_LED);
decode_results results;

volatile unsigned long pulseCount = 0;

void IRAM_ATTR onPinChange() {
  pulseCount++;
  digitalWrite(2, !digitalRead(PIN_IR_RECV)); // Onboard Blue LED flashes on any IR pulse
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=======================================================");
  Serial.println("         ESP32 IR Hardware Doctor & Diagnostics       ");
  Serial.println("=======================================================");

  // Setup Hardware
  pinMode(PIN_IR_RECV, INPUT_PULLUP);
  pinMode(2, OUTPUT); // Onboard Blue LED on ESP32
  pinMode(23, OUTPUT); // RGB1 Red
  pinMode(22, OUTPUT); // RGB1 Green
  pinMode(21, OUTPUT); // RGB1 Blue
  digitalWrite(2, LOW);
  digitalWrite(23, HIGH); // Flash RGB white on boot
  digitalWrite(22, HIGH);
  digitalWrite(21, HIGH);
  delay(300);
  digitalWrite(23, LOW);
  digitalWrite(22, LOW);
  digitalWrite(21, LOW);

  attachInterrupt(digitalPinToInterrupt(PIN_IR_RECV), onPinChange, CHANGE);
  irsend.begin();
  irrecv.enableIRIn();

  Serial.println("[SETUP] IR Receiver on GPIO 15 (Active LOW with pull-up)");
  Serial.println("[SETUP] IR Transmitter LED on GPIO 4");
  Serial.println("-------------------------------------------------------");
  Serial.println("DIAGNOSTIC GUIDE:");
  Serial.println("1. Point your physical remote at the sensor & press keys.");
  Serial.println("2. Watch below for Pin Status, Pulse Counts & Hex Codes.");
  Serial.println("-------------------------------------------------------\n");
}

void loop() {
  static unsigned long lastCheck = 0;
  static unsigned long lastPulseSeen = 0;
  unsigned long currentMillis = millis();

  // 1. Check for decoded IR signals
  if (irrecv.decode(&results)) {
    Serial.println("\n>>> [SUCCESS] IR SIGNAL RECEIVED & DECODED! <<<");
    Serial.printf("Protocol : %s\n", typeToString(results.decode_type).c_str());
    Serial.printf("Bit Count: %d bits\n", results.bits);
    Serial.print("Hex Code : 0x");
    serialPrintUint64(results.value, HEX);
    Serial.println();

    if (results.value == 0xF720DF) Serial.println("--> Detected Key: RED (Focus)");
    else if (results.value == 0xF7A05F) Serial.println("--> Detected Key: GREEN (Break)");
    else if (results.value == 0xF7E01F) Serial.println("--> Detected Key: WHITE (Reset)");
    else if (results.value == 0xF7C03F) Serial.println("--> Detected Key: POWER ON");
    else if (results.value == 0xF740BF) Serial.println("--> Detected Key: POWER OFF");

    Serial.println("-------------------------------------------------------");
    irrecv.resume();
  }

  // 2. Periodic Electrical Pin Health Monitor (every 2 seconds)
  if (currentMillis - lastCheck >= 2000) {
    lastCheck = currentMillis;
    int pinLevel = digitalRead(PIN_IR_RECV);
    unsigned long pulses = pulseCount;

    if (pinLevel == LOW && (pulses - lastPulseSeen == 0)) {
      Serial.println("[ELECTRICAL ALERT] GPIO 15 is STUCK LOW!");
      Serial.println("  -> Cause: Wiring inverted! Check GND and VCC/OUT pins on your VS1838B.");
    } else if (pulses > lastPulseSeen) {
      Serial.printf("[ACTIVITY] Received %lu raw electrical pulses on GPIO 15.\n", (pulses - lastPulseSeen));
      lastPulseSeen = pulses;
    } else {
      Serial.printf("[STATUS] GPIO 15 Idle: %s (Normal) | Waiting for remote pulses...\n", pinLevel == HIGH ? "HIGH" : "LOW");
    }
  }

  // 3. Self-Loopback Test every 10 seconds:
  // Briefly pulse the IR LED on GPIO 4 to verify the receiver catches it
  static unsigned long lastSelfTest = 0;
  if (currentMillis - lastSelfTest >= 10000) {
    lastSelfTest = currentMillis;
    Serial.println("[TEST] Emitting 38kHz test pulse from IR LED (GPIO 4)...");
    irsend.sendNEC(0xF720DF, 32);
    delay(50);
  }
}
