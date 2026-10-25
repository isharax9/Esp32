#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <DHT.h>

#include "config.h"
#include "index_html.h"

// ==========================================
// Enumerations & State Machine Variables
// ==========================================
enum PomodoroState {
  STATE_IDLE,
  STATE_WORK,
  STATE_BREAK
};

PomodoroState currentState   = STATE_IDLE;
bool isPaused                = false;
bool autoPauseEnabled        = true;

unsigned long phaseDurationMs = DEFAULT_WORK_DURATION_MS;
unsigned long phaseStartTime  = 0;
unsigned long remainingMs     = DEFAULT_WORK_DURATION_MS;
uint32_t cyclesCompleted      = 0;

// User Presence Tracking
bool isUserPresent           = false;
float currentDistanceCm      = 999.0f;
unsigned long lastDistancePingMs = 0;
unsigned long absenceStartTime   = 0;
unsigned long presenceStartTime  = 0;
bool autoPausedByAbsence     = false;

// DHT Sensor Tracking
float currentTemperature     = -999.0f;
float currentHumidity        = -999.0f;
unsigned long lastDHTReadMs  = 0;

// Brightness & Dynamic Lighting Effects
uint8_t currentBrightness = 255; // 0-255 scale
uint8_t baseR = 0, baseG = 0, baseB = 0; // Stored base color before brightness scaling

enum EffectMode {
  FX_NONE,
  FX_FLASH,
  FX_STROBE,
  FX_FADE,
  FX_SMOOTH
};

EffectMode currentEffect = FX_NONE;
unsigned long lastEffectUpdateMs = 0;
uint16_t effectStep = 0;

// Hardware Peripherals
IRsend irsend(PIN_IR_LED);
IRrecv irrecv(PIN_IR_RECV);
decode_results irResults;
DHT dht(PIN_DHT, DHTTYPE);
WebServer server(80);

// ==========================================
// Ultrasonic Distance Measurement
// ==========================================
float measureDistanceCM() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // 25ms timeout corresponds to approx 4.3 meters
  long duration = pulseIn(PIN_ECHO, HIGH, 25000);
  if (duration == 0) return 999.0f;

  float distance = (duration * 0.0343f) / 2.0f;
  return distance;
}

// Simple moving average to smooth ultrasonic jitter
float getSmoothedDistanceCM() {
  static float runningAvg = 999.0f;
  float raw = measureDistanceCM();
  if (raw > 400.0f) raw = 999.0f;

  if (runningAvg >= 900.0f) {
    runningAvg = raw;
  } else {
    runningAvg = (runningAvg * 0.6f) + (raw * 0.4f);
  }
  return runningAvg;
}

// ==========================================
// Dual RGB Light Output Helpers (Direct Pins)
// ==========================================
// Dynamic pin mappings (calibratable at runtime from Mobile UI)
uint8_t pinRGB1_Red   = PIN_RGB1_RED;
uint8_t pinRGB1_Green = PIN_RGB1_GREEN;
uint8_t pinRGB1_Blue  = PIN_RGB1_BLUE;

uint8_t pinRGB2_Red   = PIN_RGB2_RED;
uint8_t pinRGB2_Green = PIN_RGB2_GREEN;
uint8_t pinRGB2_Blue  = PIN_RGB2_BLUE;

void writeRGBPin(uint8_t pin, uint8_t duty) {
#if RGB_COMMON_ANODE
  analogWrite(pin, 255 - duty);
#else
  analogWrite(pin, duty);
#endif
}

void setRGB1ColorRaw(uint8_t r, uint8_t g, uint8_t b) {
#if ENABLE_DIRECT_RGB
  writeRGBPin(pinRGB1_Red,   r);
  writeRGBPin(pinRGB1_Green, g);
  writeRGBPin(pinRGB1_Blue,  b);
#endif
}

void setRGB2ColorRaw(uint8_t r, uint8_t g, uint8_t b) {
#if ENABLE_DIRECT_RGB
  writeRGBPin(pinRGB2_Red,   r);
  writeRGBPin(pinRGB2_Green, g);
  writeRGBPin(pinRGB2_Blue,  b);
#endif
}

void setRGB1Color(uint8_t r, uint8_t g, uint8_t b) {
  uint8_t sr = ((uint16_t)r * currentBrightness) / 255;
  uint8_t sg = ((uint16_t)g * currentBrightness) / 255;
  uint8_t sb = ((uint16_t)b * currentBrightness) / 255;
  setRGB1ColorRaw(sr, sg, sb);
}

void setRGB2Color(uint8_t r, uint8_t g, uint8_t b) {
  uint8_t sr = ((uint16_t)r * currentBrightness) / 255;
  uint8_t sg = ((uint16_t)g * currentBrightness) / 255;
  uint8_t sb = ((uint16_t)b * currentBrightness) / 255;
  setRGB2ColorRaw(sr, sg, sb);
}

void setBothRGBColors(uint8_t r, uint8_t g, uint8_t b) {
  baseR = r;
  baseG = g;
  baseB = b;
  uint8_t sr = ((uint16_t)r * currentBrightness) / 255;
  uint8_t sg = ((uint16_t)g * currentBrightness) / 255;
  uint8_t sb = ((uint16_t)b * currentBrightness) / 255;
  setRGB1ColorRaw(sr, sg, sb);
  setRGB2ColorRaw(sr, sg, sb);
}

void updateBrightness(int8_t step) {
  int newB = (int)currentBrightness + (step * 51);
  if (newB < 25) newB = 25;       // Minimum 10%
  if (newB > 255) newB = 255;   // Maximum 100%
  currentBrightness = (uint8_t)newB;
  Serial.printf("[BRIGHTNESS] Level: %d/255 (%d%%)\n", currentBrightness, (currentBrightness * 100) / 255);
  if (currentEffect == FX_NONE) {
    uint8_t sr = ((uint16_t)baseR * currentBrightness) / 255;
    uint8_t sg = ((uint16_t)baseG * currentBrightness) / 255;
    uint8_t sb = ((uint16_t)baseB * currentBrightness) / 255;
    setRGB1ColorRaw(sr, sg, sb);
    setRGB2ColorRaw(sr, sg, sb);
  }
}

// Convert HSV (h: 0-359, s: 0-255, v: 0-255) to RGB (0-255)
void hsvToRgb(uint16_t h, uint8_t s, uint8_t v, uint8_t &r, uint8_t &g, uint8_t &b) {
  if (s == 0) {
    r = g = b = v;
    return;
  }
  uint8_t region = (h % 360) / 60;
  uint16_t remainder = ((h % 360) - (region * 60)) * 6;
  uint8_t p = (v * (255 - s)) >> 8;
  uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
  uint8_t t = (v * (255 - ((s * (1530 - remainder)) >> 8))) >> 8;

  switch (region) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
  }
}

void updateEffects() {
  if (currentEffect == FX_NONE) return;

  unsigned long now = millis();

  if (currentEffect == FX_FLASH) {
    // 7-color jumping cycle every 350ms
    if (now - lastEffectUpdateMs >= 350) {
      lastEffectUpdateMs = now;
      const uint8_t flashColors[7][3] = {
        {255, 0, 0},     // Red
        {0, 255, 0},     // Green
        {0, 0, 255},     // Blue
        {255, 255, 0},   // Yellow
        {0, 255, 255},   // Cyan
        {255, 0, 255},   // Magenta
        {255, 255, 255}  // White
      };
      effectStep = (effectStep + 1) % 7;
      uint8_t r = flashColors[effectStep][0];
      uint8_t g = flashColors[effectStep][1];
      uint8_t b = flashColors[effectStep][2];
      uint8_t sr = ((uint16_t)r * currentBrightness) / 255;
      uint8_t sg = ((uint16_t)g * currentBrightness) / 255;
      uint8_t sb = ((uint16_t)b * currentBrightness) / 255;
      setRGB1ColorRaw(sr, sg, sb);
      setRGB2ColorRaw(sr, sg, sb);
    }
  } else if (currentEffect == FX_STROBE) {
    // Rapid strobe pulse
    if (now - lastEffectUpdateMs >= (effectStep ? 60 : 100)) {
      lastEffectUpdateMs = now;
      effectStep = !effectStep;
      if (effectStep) {
        uint8_t sw = currentBrightness;
        setRGB1ColorRaw(sw, sw, sw);
        setRGB2ColorRaw(sw, sw, sw);
      } else {
        setRGB1ColorRaw(0, 0, 0);
        setRGB2ColorRaw(0, 0, 0);
      }
    }
  } else if (currentEffect == FX_FADE) {
    // Smooth 360-degree rainbow crossfade
    if (now - lastEffectUpdateMs >= 25) {
      lastEffectUpdateMs = now;
      effectStep = (effectStep + 1) % 360;
      uint8_t r, g, b;
      hsvToRgb(effectStep, 255, currentBrightness, r, g, b);
      setRGB1ColorRaw(r, g, b);
      setRGB2ColorRaw(r, g, b);
    }
  } else if (currentEffect == FX_SMOOTH) {
    // Relaxing soft ambient breathing cycle
    if (now - lastEffectUpdateMs >= 35) {
      lastEffectUpdateMs = now;
      effectStep = (effectStep + 1) % 360;
      uint8_t r, g, b;
      hsvToRgb(effectStep, 175, currentBrightness, r, g, b);
      setRGB1ColorRaw(r, g, b);
      setRGB2ColorRaw(r, g, b);
    }
  }
}

// Optical self-interference protection timestamp
unsigned long lastIrTxTimestamp = 0;

// Safely send external IR codes with optical loop protection
void sendExternalIR(uint32_t code) {
#if ENABLE_EXTERNAL_IR_TRANSMIT
  if (code == 0) return;
  lastIrTxTimestamp = millis();
  irrecv.disableIRIn();
  irsend.sendNEC(code, 32);
  delay(80); // Allow optical pulse burst to dissipate
  irrecv.enableIRIn();
#else
  (void)code;
#endif
}

// ==========================================
// Pomodoro State Helpers
// ==========================================
void startWorkSession() {
  currentEffect = FX_NONE;
  currentState = STATE_WORK;
  isPaused = false;
  autoPausedByAbsence = false;
  phaseDurationMs = DEFAULT_WORK_DURATION_MS;
  phaseStartTime = millis();
  remainingMs = phaseDurationMs;
  setBothRGBColors(255, 0, 0);
  sendExternalIR(IR_RED);
  Serial.println("[POMODORO] Focus Session Started -> Light: RED (Both Strips)");
}

void startBreakSession() {
  currentEffect = FX_NONE;
  currentState = STATE_BREAK;
  isPaused = false;
  autoPausedByAbsence = false;
  phaseDurationMs = DEFAULT_BREAK_DURATION_MS;
  phaseStartTime = millis();
  remainingMs = phaseDurationMs;
  cyclesCompleted++;
  setBothRGBColors(0, 255, 0);
  sendExternalIR(IR_GREEN);
  Serial.printf("[POMODORO] Break Session Started (Cycle #%lu) -> Light: GREEN (Both Strips)\n", cyclesCompleted);
}

void pauseSession(bool byAbsence = false) {
  if (currentState == STATE_IDLE || isPaused) return;

  currentEffect = FX_NONE;
  unsigned long elapsed = millis() - phaseStartTime;
  if (elapsed < phaseDurationMs) {
    remainingMs = phaseDurationMs - elapsed;
  } else {
    remainingMs = 0;
  }
  isPaused = true;
  autoPausedByAbsence = byAbsence;
  setBothRGBColors(255, 100, 0); // Amber / orange when paused
  Serial.printf("[POMODORO] Session Paused (%s). Remaining: %lu sec\n", 
                byAbsence ? "User Walkaway" : "Manual", remainingMs / 1000UL);
}

void resumeSession() {
  if (currentState == STATE_IDLE || !isPaused) return;

  currentEffect = FX_NONE;
  phaseStartTime = millis() - (phaseDurationMs - remainingMs);
  isPaused = false;
  autoPausedByAbsence = false;

  // Restore appropriate color on resume
  if (currentState == STATE_WORK) {
    setBothRGBColors(255, 0, 0);
    sendExternalIR(IR_RED);
  } else if (currentState == STATE_BREAK) {
    setBothRGBColors(0, 255, 0);
    sendExternalIR(IR_GREEN);
  }
  Serial.println("[POMODORO] Session Resumed");
}

void resetPomodoro() {
  currentEffect = FX_NONE;
  currentState = STATE_IDLE;
  isPaused = false;
  autoPausedByAbsence = false;
  phaseDurationMs = DEFAULT_WORK_DURATION_MS;
  remainingMs = DEFAULT_WORK_DURATION_MS;
  setBothRGBColors(255, 255, 255);
  sendExternalIR(IR_WHITE);
  Serial.println("[POMODORO] Reset to IDLE -> Light: WHITE (Both Strips)");
}

void skipCurrentPhase() {
  if (currentState == STATE_WORK) {
    startBreakSession();
  } else if (currentState == STATE_BREAK) {
    startWorkSession();
  }
}

// ==========================================
// Web Server Request Handlers
// ==========================================
void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleApiStatus() {
  unsigned long curRemaining = remainingMs;
  if (currentState != STATE_IDLE && !isPaused) {
    unsigned long elapsed = millis() - phaseStartTime;
    if (elapsed < phaseDurationMs) {
      curRemaining = (phaseDurationMs - elapsed) / 1000UL;
    } else {
      curRemaining = 0;
    }
  } else {
    curRemaining = remainingMs / 1000UL;
  }

  String stateStr = "IDLE";
  if (currentState == STATE_WORK) stateStr = "WORK";
  else if (currentState == STATE_BREAK) stateStr = "BREAK";

  char jsonBuf[420];
  snprintf(jsonBuf, sizeof(jsonBuf),
    "{\"state\":\"%s\","
    "\"isPaused\":%s,"
    "\"totalSec\":%lu,"
    "\"remainingSec\":%lu,"
    "\"isUserPresent\":%s,"
    "\"distanceCm\":%.1f,"
    "\"autoPauseEnabled\":%s,"
    "\"cyclesCompleted\":%lu,"
    "\"temperature\":%.1f,"
    "\"humidity\":%.1f,"
    "\"brightness\":%d,"
    "\"effect\":%d,"
    "\"s1\":{\"r\":%d,\"g\":%d,\"b\":%d},"
    "\"s2\":{\"r\":%d,\"g\":%d,\"b\":%d}}",
    stateStr.c_str(),
    isPaused ? "true" : "false",
    phaseDurationMs / 1000UL,
    curRemaining,
    isUserPresent ? "true" : "false",
    currentDistanceCm,
    autoPauseEnabled ? "true" : "false",
    cyclesCompleted,
    currentTemperature,
    currentHumidity,
    currentBrightness,
    (int)currentEffect,
    pinRGB1_Red, pinRGB1_Green, pinRGB1_Blue,
    pinRGB2_Red, pinRGB2_Green, pinRGB2_Blue
  );

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", jsonBuf);
}

void handleApiCmd() {
  if (server.hasArg("val")) {
    String val = server.arg("val");
    String stripTarget = server.hasArg("strip") ? server.arg("strip") : "all";
    uint32_t code = 0;

    auto applyColor = [&](uint8_t r, uint8_t g, uint8_t b) {
      if (stripTarget == "1") setRGB1Color(r, g, b);
      else if (stripTarget == "2") setRGB2Color(r, g, b);
      else setBothRGBColors(r, g, b);
    };

    if (val == "on")            { currentEffect = FX_NONE; code = IR_POWER_ON;  applyColor(255, 255, 255); }
    else if (val == "off")       { currentEffect = FX_NONE; code = IR_POWER_OFF; applyColor(0, 0, 0); }
    else if (val == "bright_up") { code = IR_BRIGHT_UP; updateBrightness(+1); }
    else if (val == "bright_dn") { code = IR_BRIGHT_DN; updateBrightness(-1); }
    else if (val == "brightness" && server.hasArg("level")) {
      currentBrightness = constrain(server.arg("level").toInt(), 10, 255);
      if (currentEffect == FX_NONE) {
        applyColor(baseR, baseG, baseB);
      }
      server.send(200, "application/json", "{\"status\":\"ok\"}");
      return;
    }
    else if (val == "red")       { currentEffect = FX_NONE; code = IR_RED;       applyColor(255, 0, 0); }
    else if (val == "green")     { currentEffect = FX_NONE; code = IR_GREEN;     applyColor(0, 255, 0); }
    else if (val == "blue")      { currentEffect = FX_NONE; code = IR_BLUE;      applyColor(0, 0, 255); }
    else if (val == "white")     { currentEffect = FX_NONE; code = IR_WHITE;     applyColor(255, 255, 255); }
    else if (val == "orange")    { currentEffect = FX_NONE; code = IR_ORANGE;    applyColor(255, 80, 0); }
    else if (val == "pea_green") { currentEffect = FX_NONE; code = IR_PEA_GREEN; applyColor(40, 255, 20); }
    else if (val == "dark_blue") { currentEffect = FX_NONE; code = IR_DARK_BLUE; applyColor(0, 50, 255); }
    else if (val == "dark_orange") { currentEffect = FX_NONE; code = IR_DARK_ORANGE; applyColor(255, 35, 0); }
    else if (val == "cyan")      { currentEffect = FX_NONE; code = IR_CYAN;      applyColor(0, 255, 255); }
    else if (val == "purple")    { currentEffect = FX_NONE; code = IR_PURPLE;    applyColor(150, 0, 255); }
    else if (val == "brown" || val == "magenta") { currentEffect = FX_NONE; code = IR_BROWN; applyColor(180, 0, 100); }
    else if (val == "light_orange" || val == "amber") { currentEffect = FX_NONE; code = IR_LIGHT_ORANGE; applyColor(255, 140, 0); }
    else if (val == "light_blue" || val == "sky_blue") { currentEffect = FX_NONE; code = IR_LIGHT_BLUE; applyColor(0, 160, 255); }
    else if (val == "pink")      { currentEffect = FX_NONE; code = IR_PINK;      applyColor(255, 40, 160); }
    else if (val == "yellow")    { currentEffect = FX_NONE; code = IR_YELLOW;    applyColor(255, 220, 0); }
    else if (val == "ice_blue")  { currentEffect = FX_NONE; code = IR_SKY_BLUE;  applyColor(50, 220, 255); }
    else if (val == "flash")     { currentEffect = FX_FLASH; effectStep = 0; code = IR_FLASH; }
    else if (val == "strobe")    { currentEffect = FX_STROBE; effectStep = 0; code = IR_STROBE; }
    else if (val == "fade")      { currentEffect = FX_FADE; effectStep = 0; code = IR_FADE; }
    else if (val == "smooth")    { currentEffect = FX_SMOOTH; effectStep = 0; code = IR_SMOOTH; }

    if (code != 0) {
      sendExternalIR(code);
      Serial.printf("[IR] Sent 0x%06lX (%s) [Strip: %s]\n", (unsigned long)code, val.c_str(), stripTarget.c_str());
      server.send(200, "application/json", "{\"status\":\"ok\"}");
      return;
    }
  }
  server.send(400, "application/json", "{\"error\":\"invalid command\"}");
}

void handleApiCalibrate() {
  String strip = server.hasArg("strip") ? server.arg("strip") : "1";

  if (server.hasArg("action")) {
    String action = server.arg("action");
    if (strip == "1") {
      if (action == "swap_rg") std::swap(pinRGB1_Red, pinRGB1_Green);
      else if (action == "swap_gb") std::swap(pinRGB1_Green, pinRGB1_Blue);
      else if (action == "swap_rb") std::swap(pinRGB1_Red, pinRGB1_Blue);
    } else if (strip == "2") {
      if (action == "swap_rg") std::swap(pinRGB2_Red, pinRGB2_Green);
      else if (action == "swap_gb") std::swap(pinRGB2_Green, pinRGB2_Blue);
      else if (action == "swap_rb") std::swap(pinRGB2_Red, pinRGB2_Blue);
      else if (action == "set_green_d5") { pinRGB2_Green = 5; }
      else if (action == "set_green_d17") { pinRGB2_Green = 17; }
    }
    pinMode(pinRGB1_Red, OUTPUT);
    pinMode(pinRGB1_Green, OUTPUT);
    pinMode(pinRGB1_Blue, OUTPUT);
    pinMode(pinRGB2_Red, OUTPUT);
    pinMode(pinRGB2_Green, OUTPUT);
    pinMode(pinRGB2_Blue, OUTPUT);
    
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", "{\"status\":\"ok\"}");
    return;
  }

  if (server.hasArg("test")) {
    String test = server.arg("test");
    currentEffect = FX_NONE;
    if (test == "r") {
      if (strip == "1") { setRGB1ColorRaw(255, 0, 0); setRGB2ColorRaw(0, 0, 0); }
      else if (strip == "2") { setRGB1ColorRaw(0, 0, 0); setRGB2ColorRaw(255, 0, 0); }
      else { setBothRGBColors(255, 0, 0); }
    } else if (test == "g") {
      if (strip == "1") { setRGB1ColorRaw(0, 255, 0); setRGB2ColorRaw(0, 0, 0); }
      else if (strip == "2") { setRGB1ColorRaw(0, 0, 0); setRGB2ColorRaw(0, 255, 0); } // Calibrated Green!
      else { setBothRGBColors(0, 255, 0); }
    } else if (test == "b") {
      if (strip == "1") { setRGB1ColorRaw(0, 0, 255); setRGB2ColorRaw(0, 0, 0); }
      else if (strip == "2") { setRGB1ColorRaw(0, 0, 0); setRGB2ColorRaw(0, 0, 255); }
      else { setBothRGBColors(0, 0, 255); }
    } else if (test == "d5") {
      pinMode(5, OUTPUT);
      writeRGBPin(5, 255);
    } else if (test == "d18") {
      pinMode(18, OUTPUT);
      writeRGBPin(18, 255);
    } else if (test == "off") {
      setBothRGBColors(0, 0, 0);
      writeRGBPin(5, 0);
      writeRGBPin(18, 0);
    }
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", "{\"status\":\"ok\"}");
    return;
  }

  server.send(400, "application/json", "{\"error\":\"missing param\"}");
}

void handleApiPomodoro() {
  if (server.hasArg("action")) {
    String action = server.arg("action");
    if (action == "start") {
      startWorkSession();
    } else if (action == "pause") {
      pauseSession(false);
    } else if (action == "resume") {
      resumeSession();
    } else if (action == "reset") {
      resetPomodoro();
    } else if (action == "skip") {
      skipCurrentPhase();
    }
    server.send(200, "application/json", "{\"status\":\"ok\"}");
    return;
  }
  server.send(400, "application/json", "{\"error\":\"invalid action\"}");
}

void handleApiSettings() {
  if (server.hasArg("autopause")) {
    autoPauseEnabled = (server.arg("autopause") == "1" || server.arg("autopause") == "true");
    Serial.printf("[SETTINGS] Auto-pause set to: %s\n", autoPauseEnabled ? "ENABLED" : "DISABLED");
    server.send(200, "application/json", "{\"status\":\"ok\"}");
    return;
  }
  server.send(400, "application/json", "{\"error\":\"invalid setting\"}");
}

// ==========================================
// Physical 24-Key IR Remote Handler
// ==========================================
void handlePhysicalIRCommand(uint32_t val) {
  // Reject optical reflections from internal IR transmissions
  if (millis() - lastIrTxTimestamp < 400) return;

  static unsigned long lastRemoteTime = 0;
  if (millis() - lastRemoteTime < 200) return; // Debounce rapid repeats
  lastRemoteTime = millis();

  Serial.printf("[REMOTE] Received IR Hex: 0x%06lX\n", (unsigned long)val);

  switch (val) {
    // Row 1
    case IR_POWER_ON:
      currentEffect = FX_NONE;
      setBothRGBColors(255, 255, 255);
      Serial.println("[REMOTE] -> Lights ON (White)");
      break;
    case IR_POWER_OFF:
      currentEffect = FX_NONE;
      setBothRGBColors(0, 0, 0);
      Serial.println("[REMOTE] -> Lights OFF");
      break;
    case IR_BRIGHT_UP:
      updateBrightness(+1);
      break;
    case IR_BRIGHT_DN:
      updateBrightness(-1);
      break;

    // Row 2
    case IR_RED:
      startWorkSession();
      break;
    case IR_GREEN:
      startBreakSession();
      break;
    case IR_BLUE:
      currentEffect = FX_NONE;
      setBothRGBColors(0, 0, 255);
      Serial.println("[REMOTE] -> BLUE");
      break;
    case IR_WHITE:
      resetPomodoro();
      setBothRGBColors(255, 255, 255);
      Serial.println("[REMOTE] -> WHITE");
      break;

    // Row 3
    case IR_ORANGE:
      currentEffect = FX_NONE;
      setBothRGBColors(255, 80, 0);
      Serial.println("[REMOTE] -> ORANGE");
      break;
    case IR_PEA_GREEN:
      currentEffect = FX_NONE;
      setBothRGBColors(40, 255, 20);
      Serial.println("[REMOTE] -> PEA GREEN");
      break;
    case IR_DARK_BLUE:
      currentEffect = FX_NONE;
      setBothRGBColors(0, 50, 255);
      Serial.println("[REMOTE] -> DARK BLUE");
      break;
    case IR_FLASH:
      currentEffect = FX_FLASH;
      effectStep = 0;
      Serial.println("[REMOTE] -> Effect: FLASH (7-color jump)");
      break;

    // Row 4
    case IR_DARK_ORANGE:
      currentEffect = FX_NONE;
      setBothRGBColors(255, 35, 0);
      Serial.println("[REMOTE] -> DARK ORANGE");
      break;
    case IR_CYAN:
      currentEffect = FX_NONE;
      setBothRGBColors(0, 255, 255);
      Serial.println("[REMOTE] -> CYAN");
      break;
    case IR_BROWN:
      currentEffect = FX_NONE;
      setBothRGBColors(180, 0, 100);
      Serial.println("[REMOTE] -> MAGENTA/ROSE");
      break;
    case IR_STROBE:
      currentEffect = FX_STROBE;
      effectStep = 0;
      Serial.println("[REMOTE] -> Effect: STROBE (White pulse)");
      break;

    // Row 5
    case IR_LIGHT_ORANGE:
      currentEffect = FX_NONE;
      setBothRGBColors(255, 140, 0);
      Serial.println("[REMOTE] -> AMBER / WARM YELLOW");
      break;
    case IR_LIGHT_BLUE:
      currentEffect = FX_NONE;
      setBothRGBColors(0, 160, 255);
      Serial.println("[REMOTE] -> SKY BLUE");
      break;
    case IR_PINK:
      currentEffect = FX_NONE;
      setBothRGBColors(255, 40, 160);
      Serial.println("[REMOTE] -> PINK");
      break;
    case IR_FADE:
      currentEffect = FX_FADE;
      effectStep = 0;
      Serial.println("[REMOTE] -> Effect: FADE (Rainbow crossfade)");
      break;

    // Row 6
    case IR_YELLOW:
      currentEffect = FX_NONE;
      setBothRGBColors(255, 220, 0);
      Serial.println("[REMOTE] -> YELLOW");
      break;
    case IR_SKY_BLUE:
      currentEffect = FX_NONE;
      setBothRGBColors(50, 220, 255);
      Serial.println("[REMOTE] -> ICE BLUE");
      break;
    case IR_PURPLE:
      currentEffect = FX_NONE;
      setBothRGBColors(150, 0, 255);
      Serial.println("[REMOTE] -> PURPLE");
      break;
    case IR_SMOOTH:
      currentEffect = FX_SMOOTH;
      effectStep = 0;
      Serial.println("[REMOTE] -> Effect: SMOOTH (Ambient breath)");
      break;

    default:
      Serial.printf("[REMOTE] Unmapped key: 0x%06lX\n", (unsigned long)val);
      break;
  }
}

// ==========================================
// Setup
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n\n=========================================");
  Serial.println("  ESP32 Smart Pomodoro & Desk Assistant  ");
  Serial.println("=========================================");

  // GPIO Pin Modes
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

#if ENABLE_DIRECT_RGB
  pinMode(pinRGB1_Red, OUTPUT);
  pinMode(pinRGB1_Green, OUTPUT);
  pinMode(pinRGB1_Blue, OUTPUT);

  pinMode(pinRGB2_Red, OUTPUT);
  pinMode(pinRGB2_Green, OUTPUT);
  pinMode(pinRGB2_Blue, OUTPUT);

  setBothRGBColors(0, 0, 0); // Start with LEDs OFF so user can clearly see button responses
#endif

  // Initialize IR Transmitter & Receiver
  irsend.begin();
  irrecv.enableIRIn();
  Serial.println("[IR] VS1838B IR Receiver active on GPIO 15");

  // Initialize DHT Sensor
  dht.begin();

  // Wi-Fi Connection
  if (String(WIFI_SSID) != "YOUR_WIFI_SSID" && strlen(WIFI_SSID) > 0) {
    Serial.printf("[WIFI] Connecting to %s", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint8_t wifiRetries = 0;
    while (WiFi.status() != WL_CONNECTED && wifiRetries < 30) {
      delay(500);
      Serial.print(".");
      wifiRetries++;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WIFI] Connected Successfully!");
    Serial.print("[WIFI] IP Address: http://");
    Serial.println(WiFi.localIP());

#if ENABLE_MDNS
    if (MDNS.begin(MDNS_HOSTNAME)) {
      Serial.printf("[mDNS] Responder started: http://%s.local\n", MDNS_HOSTNAME);
    }
#endif
  } else {
    Serial.println("\n[WIFI] Could not connect to configured Wi-Fi.");
    Serial.println("[WIFI] Starting fallback Access Point: 'Desk-Pomodoro' (Password: '12345678')");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("Desk-Pomodoro", "12345678");
    Serial.print("[WIFI] Hotspot URL: http://");
    Serial.println(WiFi.softAPIP());
  }

  // Setup WebServer Routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleApiStatus);
  server.on("/api/cmd", HTTP_ANY, handleApiCmd);
  server.on("/api/pomodoro", HTTP_ANY, handleApiPomodoro);
  server.on("/api/settings", HTTP_ANY, handleApiSettings);
  server.on("/api/calibrate", HTTP_ANY, handleApiCalibrate);

  server.begin();
  Serial.println("[HTTP] Web dashboard ready on port 80");
}

// ==========================================
// Main Loop
// ==========================================
void loop() {
  server.handleClient();

  unsigned long currentMillis = millis();

  // 1. Periodic Ultrasonic Distance & Presence Check
  if (currentMillis - lastDistancePingMs >= PRESENCE_CHECK_INTERVAL_MS) {
    lastDistancePingMs = currentMillis;
    currentDistanceCm = getSmoothedDistanceCM();

    bool nowDetected = (currentDistanceCm < PRESENCE_THRESHOLD_CM);

    if (nowDetected) {
      absenceStartTime = 0;
      if (presenceStartTime == 0) presenceStartTime = currentMillis;

      // Check if user has returned and stayed present past debounce
      if (!isUserPresent && (currentMillis - presenceStartTime >= RETURN_DEBOUNCE_MS)) {
        isUserPresent = true;
        Serial.printf("[PRESENCE] User RETURNED (Distance: %.1f cm)\n", currentDistanceCm);

        // Auto-resume if session was auto-paused due to walkaway
        if (autoPauseEnabled && autoPausedByAbsence && isPaused) {
          resumeSession();
        }
      }
    } else {
      presenceStartTime = 0;
      if (absenceStartTime == 0) absenceStartTime = currentMillis;

      // Check if user has left desk past debounce interval
      if (isUserPresent && (currentMillis - absenceStartTime >= ABSENCE_DEBOUNCE_MS)) {
        isUserPresent = false;
        Serial.printf("[PRESENCE] User LEFT desk (Distance: %.1f cm)\n", currentDistanceCm);

        // Auto-pause active work session on walkaway
        if (autoPauseEnabled && currentState == STATE_WORK && !isPaused) {
          pauseSession(true);
        }
      }
    }
  }

  // 2. Periodic DHT11 Sensor Reading
  if (currentMillis - lastDHTReadMs >= DHT_READ_INTERVAL_MS) {
    lastDHTReadMs = currentMillis;
    float h = dht.readHumidity();
    float t = dht.readTemperature(); // Celsius

    if (!isnan(h) && !isnan(t)) {
      currentHumidity = h;
      currentTemperature = t;
    } else {
      // DHT not connected or read failure
      currentHumidity = -999.0f;
      currentTemperature = -999.0f;
    }
  }

  // 3. Pomodoro Session Countdown Automation
  if (currentState != STATE_IDLE && !isPaused) {
    if (currentMillis - phaseStartTime >= phaseDurationMs) {
      if (currentState == STATE_WORK) {
        startBreakSession();
      } else if (currentState == STATE_BREAK) {
        startWorkSession();
      }
    }
  }

  // 4. Update Dynamic Lighting Effects (Flash, Strobe, Fade, Smooth)
  updateEffects();

  // 5. Physical IR Remote Decoding
  if (irrecv.decode(&irResults)) {
    uint32_t val = (uint32_t)irResults.value;
    if (val != 0 && val != 0xFFFFFFFF) {
      handlePhysicalIRCommand(val);
    }
    irrecv.resume();
  }

  // 6. Serial Console Commands for Rapid Pin & Effect Testing
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'r' || c == 'R') {
      currentEffect = FX_NONE;
      Serial.println("[SERIAL] Set RED (Both Strips)");
      setBothRGBColors(255, 0, 0);
    } else if (c == 'g' || c == 'G') {
      currentEffect = FX_NONE;
      Serial.println("[SERIAL] Set GREEN (Both Strips)");
      setBothRGBColors(0, 255, 0);
    } else if (c == 'b' || c == 'B') {
      currentEffect = FX_NONE;
      Serial.println("[SERIAL] Set BLUE (Both Strips)");
      setBothRGBColors(0, 0, 255);
    } else if (c == 'w' || c == 'W') {
      currentEffect = FX_NONE;
      Serial.println("[SERIAL] Set WHITE (Both Strips)");
      setBothRGBColors(255, 255, 255);
    } else if (c == 'o' || c == 'O') {
      currentEffect = FX_NONE;
      Serial.println("[SERIAL] Set OFF (Both Strips)");
      setBothRGBColors(0, 0, 0);
      writeRGBPin(5, 0);
      writeRGBPin(18, 0);
    } else if (c == '1') {
      currentEffect = FX_NONE;
      Serial.printf("[SERIAL TEST] Strip 1 RED ONLY (GPIO %d)\n", pinRGB1_Red);
      setRGB1ColorRaw(255, 0, 0); setRGB2ColorRaw(0, 0, 0);
    } else if (c == '2') {
      currentEffect = FX_NONE;
      Serial.printf("[SERIAL TEST] Strip 1 GREEN ONLY (GPIO %d)\n", pinRGB1_Green);
      setRGB1ColorRaw(0, 255, 0); setRGB2ColorRaw(0, 0, 0);
    } else if (c == '3') {
      currentEffect = FX_NONE;
      Serial.printf("[SERIAL TEST] Strip 1 BLUE ONLY (GPIO %d)\n", pinRGB1_Blue);
      setRGB1ColorRaw(0, 0, 255); setRGB2ColorRaw(0, 0, 0);
    } else if (c == '4') {
      currentEffect = FX_NONE;
      Serial.printf("[SERIAL TEST] Strip 2 RED ONLY (GPIO %d - D19)\n", pinRGB2_Red);
      setRGB1ColorRaw(0, 0, 0); setRGB2ColorRaw(255, 0, 0);
    } else if (c == '5') {
      currentEffect = FX_NONE;
      Serial.printf("[SERIAL TEST] Strip 2 GREEN ONLY (GPIO %d - TX2)\n", pinRGB2_Green);
      setRGB1ColorRaw(0, 0, 0); setRGB2ColorRaw(0, 255, 0);
    } else if (c == '6') {
      currentEffect = FX_NONE;
      Serial.printf("[SERIAL TEST] Strip 2 BLUE ONLY (GPIO %d - RX2)\n", pinRGB2_Blue);
      setRGB1ColorRaw(0, 0, 0); setRGB2ColorRaw(0, 0, 255);
    } else if (c == '7') {
      currentEffect = FX_NONE;
      Serial.println("[SERIAL TEST] Testing GPIO 5 (D5 / HC-SR04 Trig) -> Pulling LOW (LED ON if miswired here)");
      pinMode(5, OUTPUT);
      writeRGBPin(5, 255);
    } else if (c == '8') {
      currentEffect = FX_NONE;
      Serial.println("[SERIAL TEST] Testing GPIO 18 (D18 / HC-SR04 Echo) -> Pulling LOW (LED ON if miswired here)");
      pinMode(18, OUTPUT);
      writeRGBPin(18, 255);
    } else if (c == 'x' || c == 'X') {
      std::swap(pinRGB2_Red, pinRGB2_Green);
      Serial.printf("[SERIAL CALIBRATION] Swapped Strip 2 Red & Green -> Red: %d, Green: %d\n", pinRGB2_Red, pinRGB2_Green);
    } else if (c == 'y' || c == 'Y') {
      std::swap(pinRGB2_Green, pinRGB2_Blue);
      Serial.printf("[SERIAL CALIBRATION] Swapped Strip 2 Green & Blue -> Green: %d, Blue: %d\n", pinRGB2_Green, pinRGB2_Blue);
    } else if (c == 't' || c == 'T') {
      currentEffect = FX_NONE;
      Serial.println("==================================================");
      Serial.println("[DIAGNOSTIC] Walking Strip 2 Channels (Watch LED 2):");
      Serial.printf(" 1/3: Lighting Strip 2 RED (GPIO %d) for 2.5s...\n", pinRGB2_Red);
      setBothRGBColors(0, 0, 0);
      setRGB2ColorRaw(255, 0, 0);
      delay(2500);
      Serial.printf(" 2/3: Lighting Strip 2 GREEN (GPIO %d) for 2.5s...\n", pinRGB2_Green);
      setBothRGBColors(0, 0, 0);
      setRGB2ColorRaw(0, 255, 0);
      delay(2500);
      Serial.printf(" 3/3: Lighting Strip 2 BLUE (GPIO %d) for 2.5s...\n", pinRGB2_Blue);
      setBothRGBColors(0, 0, 0);
      setRGB2ColorRaw(0, 0, 255);
      delay(2500);
      Serial.println("[DIAGNOSTIC] Done walking test. Turning OFF.");
      setBothRGBColors(0, 0, 0);
      Serial.println("==================================================");
    } else if (c == '+' || c == '=') {
      updateBrightness(+1);
    } else if (c == '-' || c == '_') {
      updateBrightness(-1);
    } else if (c == 'f' || c == 'F') {
      currentEffect = FX_FLASH;
      effectStep = 0;
      Serial.println("[SERIAL] Effect: FLASH");
    } else if (c == 's' || c == 'S') {
      currentEffect = FX_STROBE;
      effectStep = 0;
      Serial.println("[SERIAL] Effect: STROBE");
    } else if (c == 'd' || c == 'D') {
      currentEffect = FX_FADE;
      effectStep = 0;
      Serial.println("[SERIAL] Effect: FADE");
    } else if (c == 'm' || c == 'M') {
      currentEffect = FX_SMOOTH;
      effectStep = 0;
      Serial.println("[SERIAL] Effect: SMOOTH");
    }
  }
}
