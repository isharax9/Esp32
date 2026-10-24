#include <Arduino.h>
#include <IRrecv.h>
#include <IRutils.h>

// ==========================================
// VS1838B IR Sniffer & Code Mapper
// ==========================================
// Pin connection:
// OUT / Signal -> ESP32 GPIO 15
// VCC          -> 3.3V or 5V Rail
// GND          -> GND Rail

const uint16_t RECV_PIN = 15;
IRrecv irrecv(RECV_PIN);
decode_results results;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=======================================================");
  Serial.println("         ESP32 IR Remote Code Sniffer Tool            ");
  Serial.println("=======================================================");
  Serial.println("1. Point your 24-key RGB remote at the VS1838B sensor.");
  Serial.println("2. Press buttons one by one (ON, OFF, Colors, etc.).");
  Serial.println("3. Copy the detected 32-bit HEX values into config.h.\n");

  irrecv.enableIRIn(); // Start the receiver
  Serial.println("[READY] Listening for IR pulses on GPIO 15...\n");
}

void loop() {
  if (irrecv.decode(&results)) {
    // Filter out 0x0 or repeat codes (0xFFFFFFFF in NEC)
    if (results.value != 0 && results.value != 0xFFFFFFFF) {
      Serial.println("-------------------------------------------------------");
      Serial.print("Protocol : ");
      Serial.println(typeToString(results.decode_type));

      Serial.print("Bit Count: ");
      Serial.println(results.bits);

      Serial.print("Raw Hex  : 0x");
      serialPrintUint64(results.value, HEX);
      Serial.println();

      // Formatted C++ snippet ready for copy-pasting into config.h
      Serial.print("C++ Line : const uint32_t IR_KEY = 0x");
      serialPrintUint64(results.value, HEX);
      Serial.println(";");
      Serial.println("-------------------------------------------------------\n");
    } else if (results.value == 0xFFFFFFFF) {
      Serial.println("[REPEAT] Button held down (NEC repeat signal).");
    }

    irrecv.resume(); // Receive the next value
  }
}
