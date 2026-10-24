#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
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

// Hardware Peripherals
IRsend irsend(PIN_IR_LED);
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
// RGB Light Output Helper (IR + Direct Pins)
// ==========================================
void setRGBColor(uint8_t r, uint8_t g, uint8_t b) {
#if ENABLE_DIRECT_RGB
  digitalWrite(PIN_RGB_RED,   r ? HIGH : LOW);
  digitalWrite(PIN_RGB_GREEN, g ? HIGH : LOW);
  digitalWrite(PIN_RGB_BLUE,  b ? HIGH : LOW);
#endif
}

// ==========================================
// Pomodoro State Helpers
// ==========================================
void startWorkSession() {
  currentState = STATE_WORK;
  isPaused = false;
  autoPausedByAbsence = false;
  phaseDurationMs = DEFAULT_WORK_DURATION_MS;
  phaseStartTime = millis();
  remainingMs = phaseDurationMs;
  irsend.sendNEC(IR_RED, 32);
  setRGBColor(255, 0, 0);
  Serial.println("[POMODORO] Focus Session Started -> Light: RED");
}

void startBreakSession() {
  currentState = STATE_BREAK;
  isPaused = false;
  autoPausedByAbsence = false;
  phaseDurationMs = DEFAULT_BREAK_DURATION_MS;
  phaseStartTime = millis();
  remainingMs = phaseDurationMs;
  cyclesCompleted++;
  irsend.sendNEC(IR_GREEN, 32);
  setRGBColor(0, 255, 0);
  Serial.printf("[POMODORO] Break Session Started (Cycle #%u) -> Light: GREEN\n", cyclesCompleted);
}

void pauseSession(bool byAbsence = false) {
  if (currentState == STATE_IDLE || isPaused) return;

  unsigned long elapsed = millis() - phaseStartTime;
  if (elapsed < phaseDurationMs) {
    remainingMs = phaseDurationMs - elapsed;
  } else {
    remainingMs = 0;
  }
  isPaused = true;
  autoPausedByAbsence = byAbsence;
  setRGBColor(255, 100, 0); // Amber / orange when paused
  Serial.printf("[POMODORO] Session Paused (%s). Remaining: %lu sec\n", 
                byAbsence ? "User Walkaway" : "Manual", remainingMs / 1000UL);
}

void resumeSession() {
  if (currentState == STATE_IDLE || !isPaused) return;

  phaseStartTime = millis() - (phaseDurationMs - remainingMs);
  isPaused = false;
  autoPausedByAbsence = false;

  // Restore appropriate color on resume
  if (currentState == STATE_WORK) {
    irsend.sendNEC(IR_RED, 32);
    setRGBColor(255, 0, 0);
  } else if (currentState == STATE_BREAK) {
    irsend.sendNEC(IR_GREEN, 32);
    setRGBColor(0, 255, 0);
  }
  Serial.println("[POMODORO] Session Resumed");
}

void resetPomodoro() {
  currentState = STATE_IDLE;
  isPaused = false;
  autoPausedByAbsence = false;
  phaseDurationMs = DEFAULT_WORK_DURATION_MS;
  remainingMs = DEFAULT_WORK_DURATION_MS;
  irsend.sendNEC(IR_WHITE, 32);
  setRGBColor(255, 255, 255);
  Serial.println("[POMODORO] Reset to IDLE -> Light: WHITE");
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

  char jsonBuf[320];
  snprintf(jsonBuf, sizeof(jsonBuf),
    "{\"state\":\"%s\","
    "\"isPaused\":%s,"
    "\"totalSec\":%lu,"
    "\"remainingSec\":%lu,"
    "\"isUserPresent\":%s,"
    "\"distanceCm\":%.1f,"
    "\"autoPauseEnabled\":%s,"
    "\"cyclesCompleted\":%u,"
    "\"temperature\":%.1f,"
    "\"humidity\":%.1f}",
    stateStr.c_str(),
    isPaused ? "true" : "false",
    phaseDurationMs / 1000UL,
    curRemaining,
    isUserPresent ? "true" : "false",
    currentDistanceCm,
    autoPauseEnabled ? "true" : "false",
    cyclesCompleted,
    currentTemperature,
    currentHumidity
  );

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", jsonBuf);
}

void handleApiCmd() {
  if (server.hasArg("val")) {
    String val = server.arg("val");
    uint32_t code = 0;

    if (val == "on")            { code = IR_POWER_ON;  setRGBColor(255, 255, 255); }
    else if (val == "off")       { code = IR_POWER_OFF; setRGBColor(0, 0, 0); }
    else if (val == "bright_up") { code = IR_BRIGHT_UP; }
    else if (val == "bright_dn") { code = IR_BRIGHT_DN; }
    else if (val == "red")       { code = IR_RED;       setRGBColor(255, 0, 0); }
    else if (val == "green")     { code = IR_GREEN;     setRGBColor(0, 255, 0); }
    else if (val == "blue")      { code = IR_BLUE;      setRGBColor(0, 0, 255); }
    else if (val == "white")     { code = IR_WHITE;     setRGBColor(255, 255, 255); }
    else if (val == "orange")    { code = IR_ORANGE;    setRGBColor(255, 120, 0); }
    else if (val == "cyan")      { code = IR_CYAN;      setRGBColor(0, 255, 255); }
    else if (val == "purple")    { code = IR_PURPLE;    setRGBColor(255, 0, 255); }
    else if (val == "flash")     { code = IR_FLASH; }
    else if (val == "smooth")    { code = IR_SMOOTH; }

    if (code != 0) {
      irsend.sendNEC(code, 32);
      Serial.printf("[IR] Sent 0x%06X (%s)\n", code, val.c_str());
      server.send(200, "application/json", "{\"status\":\"ok\"}");
      return;
    }
  }
  server.send(400, "application/json", "{\"error\":\"invalid command\"}");
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
  pinMode(PIN_RGB_RED, OUTPUT);
  pinMode(PIN_RGB_GREEN, OUTPUT);
  pinMode(PIN_RGB_BLUE, OUTPUT);
  setRGBColor(255, 255, 255); // Default to white
#endif

  // Initialize IR Transmitter
  irsend.begin();

  // Initialize DHT Sensor
  dht.begin();

  // Wi-Fi Connection
  Serial.printf("[WIFI] Connecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint8_t wifiRetries = 0;
  while (WiFi.status() != WL_CONNECTED && wifiRetries < 40) {
    delay(500);
    Serial.print(".");
    wifiRetries++;
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
    Serial.println("\n[WIFI] Connection timed out. Running in standalone mode.");
    Serial.println("[WIFI] Connect to local AP or update credentials in config.h.");
  }

  // Setup WebServer Routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleApiStatus);
  server.on("/api/cmd", HTTP_ANY, handleApiCmd);
  server.on("/api/pomodoro", HTTP_ANY, handleApiPomodoro);
  server.on("/api/settings", HTTP_ANY, handleApiSettings);

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
}
