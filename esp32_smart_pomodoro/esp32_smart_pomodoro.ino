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
uint8_t baseR = 255, baseG = 255, baseB = 255; // Stored base color before brightness scaling

enum EffectMode {
  FX_NONE   = 0,
  FX_FLASH  = 1,
  FX_STROBE = 2,
  FX_FADE   = 3,
  FX_SMOOTH = 4
};

enum StrobeMode {
  STROBE_COLOR   = 0, // Strobes user's chosen active color (Red, Blue, Cyan, etc.)
  STROBE_RGB     = 1, // Cycles: Red flash -> Green flash -> Blue flash
  STROBE_RAINBOW = 2, // Cycles through 7 vibrant rainbow colors on each pulse
  STROBE_WHITE   = 3  // Crisp moonlight pure white strobe
};

EffectMode currentEffect     = FX_NONE;
StrobeMode currentStrobeMode = STROBE_COLOR;
unsigned long lastEffectUpdateMs = 0;
uint16_t effectStep = 0;

// Smooth Dynamic Transitions State
float tgtR1 = 0.0f, tgtG1 = 0.0f, tgtB1 = 0.0f;
float tgtR2 = 0.0f, tgtG2 = 0.0f, tgtB2 = 0.0f;
float curR1 = 0.0f, curG1 = 0.0f, curB1 = 0.0f;
float curR2 = 0.0f, curG2 = 0.0f, curB2 = 0.0f;
bool inTransition = false;

// Effect Animation Steppers
float fadeHue = 0.0f;
float smoothAngle = 0.0f;
uint8_t flashColorIndex = 0;
unsigned long flashStepStartMs = 0;
uint8_t strobeColorIndex = 0;

// Hardware Peripherals
IRsend irsend(PIN_IR_LED);
IRrecv irrecv(PIN_IR_RECV);
decode_results irResults;
DHT dht(PIN_DHT, DHTTYPE);
WebServer server(80);

// Live Telemetry for Physical Remote Sniffing
uint32_t lastReceivedIrHex = 0;
String lastReceivedIrName  = "None";

// ==========================================
// Ultrasonic Distance Measurement
// ==========================================
float measureDistanceCM() {
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(4);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // 30ms timeout corresponds to approx 5.1 meters
  long duration = pulseIn(PIN_ECHO, HIGH, 30000);
  if (duration == 0) return 999.0f;

  float distance = (duration * 0.0343f) / 2.0f;
  return distance;
}

// Simple moving average to smooth ultrasonic jitter
float getSmoothedDistanceCM() {
  static float runningAvg = 999.0f;
  float raw = measureDistanceCM();
  if (raw > 400.0f) raw = 999.0f;

  if (raw >= 900.0f) {
    runningAvg = (runningAvg * 0.7f) + (raw * 0.3f);
  } else if (runningAvg >= 900.0f) {
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
  tgtR1 = ((uint16_t)r * currentBrightness) / 255.0f;
  tgtG1 = ((uint16_t)g * currentBrightness) / 255.0f;
  tgtB1 = ((uint16_t)b * currentBrightness) / 255.0f;
  inTransition = true;
}

void setRGB2Color(uint8_t r, uint8_t g, uint8_t b) {
  tgtR2 = ((uint16_t)r * currentBrightness) / 255.0f;
  tgtG2 = ((uint16_t)g * currentBrightness) / 255.0f;
  tgtB2 = ((uint16_t)b * currentBrightness) / 255.0f;
  inTransition = true;
}

void setBothRGBColors(uint8_t r, uint8_t g, uint8_t b) {
  baseR = r;
  baseG = g;
  baseB = b;
  tgtR1 = ((uint16_t)r * currentBrightness) / 255.0f;
  tgtG1 = ((uint16_t)g * currentBrightness) / 255.0f;
  tgtB1 = ((uint16_t)b * currentBrightness) / 255.0f;
  tgtR2 = ((uint16_t)r * currentBrightness) / 255.0f;
  tgtG2 = ((uint16_t)g * currentBrightness) / 255.0f;
  tgtB2 = ((uint16_t)b * currentBrightness) / 255.0f;
  inTransition = true;
}

void setImmediateBothRGB(uint8_t r, uint8_t g, uint8_t b) {
  baseR = r; baseG = g; baseB = b;
  curR1 = tgtR1 = ((uint16_t)r * currentBrightness) / 255.0f;
  curG1 = tgtG1 = ((uint16_t)g * currentBrightness) / 255.0f;
  curB1 = tgtB1 = ((uint16_t)b * currentBrightness) / 255.0f;
  curR2 = tgtR2 = ((uint16_t)r * currentBrightness) / 255.0f;
  curG2 = tgtG2 = ((uint16_t)g * currentBrightness) / 255.0f;
  curB2 = tgtB2 = ((uint16_t)b * currentBrightness) / 255.0f;
  inTransition = false;
  setRGB1ColorRaw((uint8_t)round(curR1), (uint8_t)round(curG1), (uint8_t)round(curB1));
  setRGB2ColorRaw((uint8_t)round(curR2), (uint8_t)round(curG2), (uint8_t)round(curB2));
}

// 100 FPS Smooth Color Crossfade Engine for Static Lighting
void updateColorTransitions() {
  if (currentEffect != FX_NONE && !inTransition) return;

  static unsigned long lastTransMs = 0;
  unsigned long now = millis();
  if (now - lastTransMs < 10) return;
  lastTransMs = now;

  if (currentEffect == FX_NONE) {
    const float alpha = 0.12f; // Smooth exponential approach (~250ms feel)
    auto stepChannel = [](float &cur, float tgt, float a) {
      float diff = tgt - cur;
      if (fabs(diff) < 0.5f) {
        cur = tgt;
      } else {
        cur += diff * a;
      }
    };

    stepChannel(curR1, tgtR1, alpha);
    stepChannel(curG1, tgtG1, alpha);
    stepChannel(curB1, tgtB1, alpha);

    stepChannel(curR2, tgtR2, alpha);
    stepChannel(curG2, tgtG2, alpha);
    stepChannel(curB2, tgtB2, alpha);

    setRGB1ColorRaw((uint8_t)round(curR1), (uint8_t)round(curG1), (uint8_t)round(curB1));
    setRGB2ColorRaw((uint8_t)round(curR2), (uint8_t)round(curG2), (uint8_t)round(curB2));

    if (curR1 == tgtR1 && curG1 == tgtG1 && curB1 == tgtB1 &&
        curR2 == tgtR2 && curG2 == tgtG2 && curB2 == tgtB2) {
      inTransition = false;
    }
  }
}

void updateBrightness(int8_t step) {
  int newB = (int)currentBrightness + (step * 51);
  if (newB < 25) newB = 25;     // Minimum 10%
  if (newB > 255) newB = 255; // Maximum 100%
  currentBrightness = (uint8_t)newB;
  Serial.printf("[BRIGHTNESS] Level: %d/255 (%d%%)\n", currentBrightness, (currentBrightness * 100) / 255);
  if (currentEffect == FX_NONE) {
    setBothRGBColors(baseR, baseG, baseB);
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

const uint8_t FLASH_PALETTE[7][3] = {
  {255, 0, 0},     // Red
  {255, 120, 0},   // Amber / Orange
  {255, 230, 0},   // Warm Yellow
  {0, 255, 0},     // Pure Green
  {0, 220, 255},   // Cyan / Sky
  {0, 30, 255},    // Deep Blue
  {220, 0, 255}    // Magenta / Violet
};

// ==========================================
// Dynamic Lighting Effects Engine
// ==========================================
void updateEffects() {
  if (currentEffect == FX_NONE) return;
  unsigned long now = millis();

  if (currentEffect == FX_FLASH) {
    // Smooth 7-color jumping cycle with cosine crossfade transitions
    if (now - lastEffectUpdateMs >= 15) { // 66 FPS smooth animation
      lastEffectUpdateMs = now;
      if (flashStepStartMs == 0) flashStepStartMs = now;
      unsigned long elapsed = now - flashStepStartMs;

      uint8_t fromIdx = flashColorIndex;
      uint8_t toIdx = (flashColorIndex + 1) % 7;

      float ease = 1.0f;
      if (elapsed < 340) {
        float p = (float)elapsed / 340.0f;
        ease = (1.0f - cos(p * 3.14159265f)) * 0.5f; // Smooth ease-in-out
      }

      float r = FLASH_PALETTE[fromIdx][0] + (FLASH_PALETTE[toIdx][0] - FLASH_PALETTE[fromIdx][0]) * ease;
      float g = FLASH_PALETTE[fromIdx][1] + (FLASH_PALETTE[toIdx][1] - FLASH_PALETTE[fromIdx][1]) * ease;
      float b = FLASH_PALETTE[fromIdx][2] + (FLASH_PALETTE[toIdx][2] - FLASH_PALETTE[fromIdx][2]) * ease;

      uint8_t sr = (uint8_t)round((r * currentBrightness) / 255.0f);
      uint8_t sg = (uint8_t)round((g * currentBrightness) / 255.0f);
      uint8_t sb = (uint8_t)round((b * currentBrightness) / 255.0f);

      curR1 = curR2 = sr;
      curG1 = curG2 = sg;
      curB1 = curB2 = sb;
      setRGB1ColorRaw(sr, sg, sb);
      setRGB2ColorRaw(sr, sg, sb);

      if (elapsed >= 560) {
        flashColorIndex = toIdx;
        flashStepStartMs = now;
      }
    }
  }
  else if (currentEffect == FX_STROBE) {
    // Selectable Strobe Mode: Chosen Color, RGB Cycle, Rainbow Cycle, or White
    static bool strobeState = false;
    unsigned long pulseTime = strobeState ? 55 : 95; // 55ms ON / 95ms OFF

    if (now - lastEffectUpdateMs >= pulseTime) {
      lastEffectUpdateMs = now;
      strobeState = !strobeState;

      if (strobeState) {
        uint8_t r = 255, g = 255, b = 255;
        if (currentStrobeMode == STROBE_COLOR) {
          // Strobes user's selected active color
          if (baseR == 0 && baseG == 0 && baseB == 0) {
            r = 255; g = 255; b = 255;
          } else {
            r = baseR; g = baseG; b = baseB;
          }
        } else if (currentStrobeMode == STROBE_RGB) {
          // Cycles Red -> Green -> Blue on every pulse
          strobeColorIndex = (strobeColorIndex + 1) % 3;
          r = (strobeColorIndex == 0) ? 255 : 0;
          g = (strobeColorIndex == 1) ? 255 : 0;
          b = (strobeColorIndex == 2) ? 255 : 0;
        } else if (currentStrobeMode == STROBE_RAINBOW) {
          // Cycles vibrant rainbow colors on every pulse
          strobeColorIndex = (strobeColorIndex + 1) % 7;
          r = FLASH_PALETTE[strobeColorIndex][0];
          g = FLASH_PALETTE[strobeColorIndex][1];
          b = FLASH_PALETTE[strobeColorIndex][2];
        } else {
          // STROBE_WHITE
          r = 255; g = 255; b = 255;
        }

        uint8_t sr = (uint8_t)round(((uint16_t)r * currentBrightness) / 255.0f);
        uint8_t sg = (uint8_t)round(((uint16_t)g * currentBrightness) / 255.0f);
        uint8_t sb = (uint8_t)round(((uint16_t)b * currentBrightness) / 255.0f);
        setRGB1ColorRaw(sr, sg, sb);
        setRGB2ColorRaw(sr, sg, sb);
      } else {
        setRGB1ColorRaw(0, 0, 0);
        setRGB2ColorRaw(0, 0, 0);
      }
    }
  }
  else if (currentEffect == FX_FADE) {
    // Ultra-smooth continuous 360-degree rainbow flow at 50 FPS
    if (now - lastEffectUpdateMs >= 20) {
      lastEffectUpdateMs = now;
      fadeHue += 0.55f;
      if (fadeHue >= 360.0f) fadeHue -= 360.0f;

      uint8_t r, g, b;
      hsvToRgb((uint16_t)fadeHue, 255, currentBrightness, r, g, b);
      curR1 = curR2 = r;
      curG1 = curG2 = g;
      curB1 = curB2 = b;
      setRGB1ColorRaw(r, g, b);
      setRGB2ColorRaw(r, g, b);
    }
  }
  else if (currentEffect == FX_SMOOTH) {
    // Ambient breathing mood flow with smooth sinusoidal intensity curve
    if (now - lastEffectUpdateMs >= 20) {
      lastEffectUpdateMs = now;
      smoothAngle += 0.045f;
      if (smoothAngle >= 6.2831853f) smoothAngle -= 6.2831853f;

      float breath = sin(smoothAngle) * 0.425f + 0.575f; // Smooth pulse from 15% to 100%

      uint8_t r, g, b;
      if (baseR == 0 && baseG == 0 && baseB == 0) {
        float hue = fmod(smoothAngle * 35.0f, 360.0f);
        hsvToRgb((uint16_t)hue, 180, (uint8_t)round(currentBrightness * breath), r, g, b);
      } else {
        r = (uint8_t)round(baseR * breath * currentBrightness / 255.0f);
        g = (uint8_t)round(baseG * breath * currentBrightness / 255.0f);
        b = (uint8_t)round(baseB * breath * currentBrightness / 255.0f);
      }
      curR1 = curR2 = r;
      curG1 = curG2 = g;
      curB1 = curB2 = b;
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

  char jsonBuf[512];
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
    "\"strobeMode\":%d,"
    "\"lastIrHex\":\"0x%06lX\","
    "\"lastIrName\":\"%s\","
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
    (int)currentStrobeMode,
    (unsigned long)lastReceivedIrHex,
    lastReceivedIrName.c_str(),
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
    bool keepStrobe = (server.hasArg("strobe") && server.arg("strobe") == "1");

    auto applyColor = [&](uint8_t r, uint8_t g, uint8_t b) {
      baseR = r;
      baseG = g;
      baseB = b;
      if (keepStrobe) {
        currentStrobeMode = STROBE_COLOR;
        currentEffect = FX_STROBE;
      } else {
        currentEffect = FX_NONE;
        if (stripTarget == "1") setRGB1Color(r, g, b);
        else if (stripTarget == "2") setRGB2Color(r, g, b);
        else setBothRGBColors(r, g, b);
      }
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
    else if (val == "red")       { code = IR_RED;       applyColor(255, 0, 0); }
    else if (val == "green")     { code = IR_GREEN;     applyColor(0, 255, 0); }
    else if (val == "blue")      { code = IR_BLUE;      applyColor(0, 0, 255); }
    else if (val == "white")     { code = IR_WHITE;     applyColor(255, 255, 255); }
    else if (val == "orange")    { code = IR_ORANGE;    applyColor(255, 80, 0); }
    else if (val == "pea_green") { code = IR_PEA_GREEN; applyColor(40, 255, 20); }
    else if (val == "dark_blue") { code = IR_DARK_BLUE; applyColor(0, 50, 255); }
    else if (val == "dark_orange") { code = IR_DARK_ORANGE; applyColor(255, 35, 0); }
    else if (val == "cyan")      { code = IR_CYAN;      applyColor(0, 255, 255); }
    else if (val == "purple")    { code = IR_PURPLE;    applyColor(150, 0, 255); }
    else if (val == "brown" || val == "magenta") { code = IR_BROWN; applyColor(180, 0, 100); }
    else if (val == "light_orange" || val == "amber") { code = IR_LIGHT_ORANGE; applyColor(255, 140, 0); }
    else if (val == "light_blue" || val == "sky_blue") { code = IR_LIGHT_BLUE; applyColor(0, 160, 255); }
    else if (val == "pink")      { code = IR_PINK;      applyColor(255, 40, 160); }
    else if (val == "yellow")    { code = IR_YELLOW;    applyColor(255, 220, 0); }
    else if (val == "ice_blue")  { code = IR_SKY_BLUE;  applyColor(50, 220, 255); }
    else if (val == "flash")     { currentEffect = FX_FLASH; flashStepStartMs = millis(); code = IR_FLASH; }
    else if (val == "strobe")    {
      currentEffect = FX_STROBE;
      code = IR_STROBE;
      if (server.hasArg("mode")) {
        String m = server.arg("mode");
        if (m == "rgb") currentStrobeMode = STROBE_RGB;
        else if (m == "rainbow") currentStrobeMode = STROBE_RAINBOW;
        else if (m == "white") currentStrobeMode = STROBE_WHITE;
        else currentStrobeMode = STROBE_COLOR;
      }
    }
    else if (val == "strobe_mode") {
      currentEffect = FX_STROBE;
      if (server.hasArg("mode")) {
        String m = server.arg("mode");
        if (m == "rgb") currentStrobeMode = STROBE_RGB;
        else if (m == "rainbow") currentStrobeMode = STROBE_RAINBOW;
        else if (m == "white") currentStrobeMode = STROBE_WHITE;
        else currentStrobeMode = STROBE_COLOR;
      }
      server.send(200, "application/json", "{\"status\":\"ok\"}");
      return;
    }
    else if (val == "fade")      { currentEffect = FX_FADE; fadeHue = 0.0f; code = IR_FADE; }
    else if (val == "smooth")    { currentEffect = FX_SMOOTH; smoothAngle = 0.0f; code = IR_SMOOTH; }

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
    } else if (test == "off") {
      setBothRGBColors(0, 0, 0);
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
  if (millis() - lastRemoteTime < 150) return; // Debounce rapid repeats
  lastRemoteTime = millis();

  lastReceivedIrHex = val;
  Serial.printf("[REMOTE] Received IR Hex: 0x%06lX\n", (unsigned long)val);

  auto setLightOrStrobe = [&](uint8_t r, uint8_t g, uint8_t b, const char* name) {
    if (currentEffect == FX_STROBE) {
      baseR = r; baseG = g; baseB = b;
      currentStrobeMode = STROBE_COLOR;
      lastReceivedIrName = String("STROBE (") + name + ")";
      Serial.printf("[REMOTE] -> %s\n", lastReceivedIrName.c_str());
    } else {
      currentEffect = FX_NONE;
      setBothRGBColors(r, g, b);
      lastReceivedIrName = name;
      Serial.printf("[REMOTE] -> %s\n", name);
    }
  };

  switch (val) {
    // Row 1
    case IR_POWER_ON:
      currentEffect = FX_NONE;
      setBothRGBColors(255, 255, 255);
      lastReceivedIrName = "POWER ON (White)";
      Serial.println("[REMOTE] -> Lights ON (White)");
      break;
    case IR_POWER_OFF:
      currentEffect = FX_NONE;
      setBothRGBColors(0, 0, 0);
      lastReceivedIrName = "POWER OFF";
      Serial.println("[REMOTE] -> Lights OFF");
      break;
    case IR_BRIGHT_UP:
      updateBrightness(+1);
      lastReceivedIrName = "BRIGHTNESS UP";
      break;
    case IR_BRIGHT_DN:
      updateBrightness(-1);
      lastReceivedIrName = "BRIGHTNESS DOWN";
      break;

    // Row 2
    case IR_RED:        setLightOrStrobe(255, 0, 0, "RED"); break;
    case IR_GREEN:      setLightOrStrobe(0, 255, 0, "GREEN"); break;
    case IR_BLUE:       setLightOrStrobe(0, 0, 255, "BLUE"); break;
    case IR_WHITE:      setLightOrStrobe(255, 255, 255, "WHITE"); break;

    // Row 3
    case IR_ORANGE:     setLightOrStrobe(255, 80, 0, "ORANGE"); break;
    case IR_PEA_GREEN:  setLightOrStrobe(40, 255, 20, "PEA GREEN"); break;
    case IR_DARK_BLUE:  setLightOrStrobe(0, 50, 255, "DARK BLUE"); break;
    case IR_FLASH:
      currentEffect = FX_FLASH;
      flashStepStartMs = millis();
      lastReceivedIrName = "FLASH (7-Color Flow)";
      Serial.println("[REMOTE] -> Effect: FLASH (Smooth 7-color flow)");
      break;

    // Row 4
    case IR_DARK_ORANGE: setLightOrStrobe(255, 35, 0, "DARK ORANGE"); break;
    case IR_CYAN:        setLightOrStrobe(0, 255, 255, "CYAN"); break;
    case IR_BROWN:       setLightOrStrobe(180, 0, 100, "MAGENTA/ROSE"); break;
    case IR_STROBE:
      if (currentEffect == FX_STROBE) {
        currentStrobeMode = (StrobeMode)((currentStrobeMode + 1) % 4);
      } else {
        currentEffect = FX_STROBE;
      }
      switch (currentStrobeMode) {
        case STROBE_COLOR:   lastReceivedIrName = "STROBE (Color)"; break;
        case STROBE_RGB:     lastReceivedIrName = "STROBE (RGB Cycle)"; break;
        case STROBE_RAINBOW: lastReceivedIrName = "STROBE (Rainbow)"; break;
        case STROBE_WHITE:   lastReceivedIrName = "STROBE (White Pulse)"; break;
      }
      Serial.printf("[REMOTE] -> %s\n", lastReceivedIrName.c_str());
      break;

    // Row 5
    case IR_LIGHT_ORANGE: setLightOrStrobe(255, 140, 0, "AMBER"); break;
    case IR_LIGHT_BLUE:   setLightOrStrobe(0, 160, 255, "SKY BLUE"); break;
    case IR_PINK:         setLightOrStrobe(255, 40, 160, "PINK"); break;
    case IR_FADE:
      currentEffect = FX_FADE;
      fadeHue = 0.0f;
      lastReceivedIrName = "FADE (Rainbow Flow)";
      Serial.println("[REMOTE] -> Effect: FADE (Rainbow wave)");
      break;

    // Row 6
    case IR_YELLOW:       setLightOrStrobe(255, 220, 0, "YELLOW"); break;
    case IR_SKY_BLUE:     setLightOrStrobe(50, 220, 255, "ICE BLUE"); break;
    case IR_PURPLE:       setLightOrStrobe(150, 0, 255, "PURPLE"); break;
    case IR_SMOOTH:
      currentEffect = FX_SMOOTH;
      smoothAngle = 0.0f;
      lastReceivedIrName = "SMOOTH (Ambient Breath)";
      Serial.println("[REMOTE] -> Effect: SMOOTH (Ambient breath)");
      break;

    default:
      lastReceivedIrName = "UNMAPPED";
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

  // 4. Update Dynamic Lighting Effects & Smooth Color Transitions
  if (currentEffect != FX_NONE) {
    updateEffects();
  } else {
    updateColorTransitions();
  }

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
    if (c == 0 || c == '\n' || c == '\r') return; // Ignore null / newline
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
    } else if (c == 'u' || c == 'U') {
      Serial.println("\n--- [ULTRASONIC LIVE DIAGNOSTIC] ---");
      for (int i = 0; i < 5; i++) {
        pinMode(PIN_ECHO, INPUT);
        int echoBefore = digitalRead(PIN_ECHO);
        digitalWrite(PIN_TRIG, LOW);
        delayMicroseconds(4);
        digitalWrite(PIN_TRIG, HIGH);
        delayMicroseconds(10);
        digitalWrite(PIN_TRIG, LOW);
        long dur = pulseIn(PIN_ECHO, HIGH, 30000);
        float d = (dur == 0) ? 999.0f : (dur * 0.0343f) / 2.0f;
        Serial.printf(" Ping #%d: Echo Idle=%d | Duration=%lu us | Distance=%.1f cm\n", i + 1, echoBefore, dur, d);
        delay(120);
      }
      Serial.println("------------------------------------");
    } else if (c == 'i' || c == 'I') {
      Serial.println("\n--- [IR RECEIVER DIAGNOSTIC] ---");
      Serial.printf(" GPIO 15 pin level: %s (Idle should be HIGH)\n", 
                    digitalRead(PIN_IR_RECV) == HIGH ? "HIGH (Normal)" : "LOW (Stuck/Inverted!)");
      Serial.printf(" Last Decoded IR Hex: 0x%06lX (%s)\n", (unsigned long)lastReceivedIrHex, lastReceivedIrName.c_str());
      Serial.println("---------------------------------");
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
    } else if (c == 'x' || c == 'X') {
      std::swap(pinRGB2_Red, pinRGB2_Green);
      Serial.printf("[SERIAL CALIBRATION] Swapped Strip 2 Red & Green -> Red: %d, Green: %d\n", pinRGB2_Red, pinRGB2_Green);
    } else if (c == 'y' || c == 'Y') {
      std::swap(pinRGB2_Green, pinRGB2_Blue);
      Serial.printf("[SERIAL CALIBRATION] Swapped Strip 2 Green & Blue -> Green: %d, Blue: %d\n", pinRGB2_Green, pinRGB2_Blue);
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
