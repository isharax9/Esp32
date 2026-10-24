#pragma once
#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Smart Pomodoro & Desk Controller</title>
  <style>
    :root {
      --bg: #0b0f19;
      --card-bg: #151d30;
      --card-border: #1e293b;
      --text-main: #f8fafc;
      --text-muted: #94a3b8;
      --red: #ef4444;
      --red-hover: #dc2626;
      --green: #22c55e;
      --green-hover: #16a34a;
      --blue: #3b82f6;
      --blue-hover: #2563eb;
      --amber: #f59e0b;
      --purple: #a855f7;
      --cyan: #06b6d4;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      background-color: var(--bg);
      color: var(--text-main);
      padding: 16px;
      display: flex;
      flex-direction: column;
      align-items: center;
      min-height: 100vh;
    }
    header {
      width: 100%;
      max-width: 480px;
      margin-bottom: 20px;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    h1 { font-size: 1.25rem; font-weight: 700; letter-spacing: -0.02em; }
    .status-dot {
      display: inline-block;
      width: 10px;
      height: 10px;
      border-radius: 50%;
      background: var(--green);
      box-shadow: 0 0 8px var(--green);
      margin-right: 6px;
    }
    .header-badge {
      font-size: 0.75rem;
      background: #1e293b;
      padding: 4px 10px;
      border-radius: 999px;
      display: flex;
      align-items: center;
      color: var(--text-muted);
    }
    .card {
      width: 100%;
      max-width: 480px;
      background: var(--card-bg);
      border: 1px solid var(--card-border);
      border-radius: 16px;
      padding: 20px;
      margin-bottom: 16px;
      box-shadow: 0 4px 20px rgba(0,0,0,0.3);
    }
    .card-title {
      font-size: 0.85rem;
      text-transform: uppercase;
      letter-spacing: 0.08em;
      color: var(--text-muted);
      margin-bottom: 14px;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    /* Timer Display */
    .timer-center { text-align: center; padding: 10px 0 16px; }
    .timer-badge {
      display: inline-block;
      font-size: 0.8rem;
      font-weight: 700;
      padding: 4px 12px;
      border-radius: 999px;
      text-transform: uppercase;
      letter-spacing: 0.05em;
      margin-bottom: 12px;
    }
    .badge-idle { background: #334155; color: #cbd5e1; }
    .badge-work { background: rgba(239, 68, 68, 0.2); color: var(--red); border: 1px solid var(--red); }
    .badge-break { background: rgba(34, 197, 94, 0.2); color: var(--green); border: 1px solid var(--green); }
    .badge-paused { background: rgba(245, 158, 11, 0.2); color: var(--amber); border: 1px solid var(--amber); }

    .countdown {
      font-size: 3.5rem;
      font-weight: 800;
      font-variant-numeric: tabular-nums;
      letter-spacing: -0.04em;
      margin-bottom: 8px;
    }
    .progress-bar-bg {
      width: 100%;
      height: 8px;
      background: #1e293b;
      border-radius: 999px;
      overflow: hidden;
      margin-bottom: 16px;
    }
    .progress-bar-fill {
      height: 100%;
      width: 0%;
      background: var(--red);
      transition: width 0.4s ease, background 0.4s ease;
    }
    /* Button Layout */
    .btn-row { display: flex; gap: 8px; margin-top: 8px; }
    .btn {
      flex: 1;
      padding: 12px;
      font-size: 0.875rem;
      font-weight: 600;
      border: none;
      border-radius: 10px;
      cursor: pointer;
      color: #fff;
      transition: all 0.15s ease;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 6px;
      outline: none;
    }
    .btn:active { transform: scale(0.98); }
    .btn-primary { background: var(--blue); }
    .btn-primary:hover { background: var(--blue-hover); }
    .btn-danger { background: var(--red); }
    .btn-danger:hover { background: var(--red-hover); }
    .btn-success { background: var(--green); }
    .btn-success:hover { background: var(--green-hover); }
    .btn-warning { background: var(--amber); }
    .btn-secondary { background: #334155; }
    .btn-secondary:hover { background: #475569; }

    /* Environment Metrics */
    .grid-2x2 {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 12px;
    }
    .metric-box {
      background: #0f172a;
      border: 1px solid #1e293b;
      border-radius: 12px;
      padding: 12px;
    }
    .metric-label { font-size: 0.75rem; color: var(--text-muted); margin-bottom: 4px; }
    .metric-val { font-size: 1.25rem; font-weight: 700; }
    .metric-sub { font-size: 0.7rem; color: var(--text-muted); margin-top: 2px; }

    /* Remote Pad */
    .remote-grid {
      display: grid;
      grid-template-columns: repeat(4, 1fr);
      gap: 8px;
    }
    .btn-ir {
      padding: 10px 4px;
      font-size: 0.75rem;
      font-weight: 700;
      border-radius: 8px;
      border: none;
      cursor: pointer;
      color: #fff;
      transition: transform 0.1s;
    }
    .btn-ir:active { transform: scale(0.95); }
    .c-red { background: #dc2626; }
    .c-green { background: #16a34a; }
    .c-blue { background: #2563eb; }
    .c-white { background: #f8fafc; color: #0f172a; }
    .c-orange { background: #ea580c; }
    .c-cyan { background: #0891b2; }
    .c-purple { background: #9333ea; }
    .c-dark { background: #334155; }

    /* Toggle switch */
    .switch-container {
      display: flex;
      align-items: center;
      justify-content: space-between;
      margin-top: 10px;
      padding: 8px 12px;
      background: #0f172a;
      border-radius: 10px;
    }
    .switch-label { font-size: 0.8rem; font-weight: 600; color: #cbd5e1; }
    .switch {
      position: relative;
      display: inline-block;
      width: 44px;
      height: 24px;
    }
    .switch input { opacity: 0; width: 0; height: 0; }
    .slider {
      position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0;
      background-color: #334155;
      transition: .3s;
      border-radius: 24px;
    }
    .slider:before {
      position: absolute; content: ""; height: 18px; width: 18px; left: 3px; bottom: 3px;
      background-color: white;
      transition: .3s;
      border-radius: 50%;
    }
    input:checked + .slider { background-color: var(--blue); }
    input:checked + .slider:before { transform: translateX(20px); }

    /* Toast notification */
    #toast {
      visibility: hidden;
      min-width: 200px;
      background-color: #1e293b;
      color: #f8fafc;
      text-align: center;
      border-radius: 8px;
      padding: 10px 16px;
      position: fixed;
      z-index: 100;
      bottom: 24px;
      font-size: 0.85rem;
      border: 1px solid #334155;
      opacity: 0;
      transition: opacity 0.3s, bottom 0.3s;
    }
    #toast.show {
      visibility: visible;
      opacity: 1;
      bottom: 30px;
    }
  </style>
</head>
<body>

  <header>
    <h1>Desk Assistant</h1>
    <div class="header-badge">
      <span class="status-dot" id="live-dot"></span>
      <span id="conn-status">Connected</span>
    </div>
  </header>

  <!-- Pomodoro Card -->
  <div class="card">
    <div class="card-title">
      <span>Pomodoro Focus Timer</span>
      <span id="cycle-count" style="font-size:0.75rem; color:#64748b;">Cycles: 0</span>
    </div>
    <div class="timer-center">
      <div id="state-badge" class="timer-badge badge-idle">IDLE</div>
      <div id="timer-display" class="countdown">25:00</div>
      <div class="progress-bar-bg">
        <div id="progress-bar" class="progress-bar-fill"></div>
      </div>
      <div class="btn-row">
        <button id="btn-toggle" class="btn btn-primary" onclick="togglePomodoro()">Start Focus</button>
        <button class="btn btn-secondary" onclick="skipPhase()">Skip</button>
        <button class="btn btn-secondary" onclick="resetTimer()">Reset</button>
      </div>
    </div>
  </div>

  <!-- Presence & Environment Card -->
  <div class="card">
    <div class="card-title">
      <span>Desk Presence & Environment</span>
    </div>
    <div class="grid-2x2">
      <div class="metric-box">
        <div class="metric-label">Chair Presence</div>
        <div class="metric-val" id="presence-val" style="color:var(--text-muted);">Away</div>
        <div class="metric-sub" id="distance-val">-- cm</div>
      </div>
      <div class="metric-box">
        <div class="metric-label">Auto-Pause</div>
        <div class="metric-val" id="autopause-status" style="font-size: 1rem; margin-top:3px;">Enabled</div>
        <div class="metric-sub">Pauses on walkaway</div>
      </div>
      <div class="metric-box">
        <div class="metric-label">Room Temperature</div>
        <div class="metric-val" id="temp-val">-- &deg;C</div>
        <div class="metric-sub">DHT11 Sensor</div>
      </div>
      <div class="metric-box">
        <div class="metric-label">Humidity</div>
        <div class="metric-val" id="hum-val">-- %</div>
        <div class="metric-sub">Relative Humidity</div>
      </div>
    </div>
    <div class="switch-container">
      <span class="switch-label">Auto-Pause on User Absence</span>
      <label class="switch">
        <input type="checkbox" id="autopause-toggle" checked onchange="toggleAutoPause(this.checked)">
        <span class="slider"></span>
      </label>
    </div>
  </div>

  <!-- Virtual IR Remote Control Card -->
  <div class="card">
    <div class="card-title">
      <span>Virtual IR Remote (5V RGB Strip)</span>
    </div>
    <div class="remote-grid">
      <button class="btn-ir c-green" onclick="sendIR('on')">ON</button>
      <button class="btn-ir c-dark" onclick="sendIR('bright_up')">BRIGHT +</button>
      <button class="btn-ir c-dark" onclick="sendIR('bright_dn')">BRIGHT -</button>
      <button class="btn-ir c-red" onclick="sendIR('off')">OFF</button>

      <button class="btn-ir c-red" onclick="sendIR('red')">RED</button>
      <button class="btn-ir c-green" onclick="sendIR('green')">GREEN</button>
      <button class="btn-ir c-blue" onclick="sendIR('blue')">BLUE</button>
      <button class="btn-ir c-white" onclick="sendIR('white')">WHITE</button>

      <button class="btn-ir c-orange" onclick="sendIR('orange')">ORANGE</button>
      <button class="btn-ir c-cyan" onclick="sendIR('cyan')">CYAN</button>
      <button class="btn-ir c-purple" onclick="sendIR('purple')">PURPLE</button>
      <button class="btn-ir c-dark" onclick="sendIR('flash')">FLASH</button>
    </div>
  </div>

  <div id="toast">Command sent</div>

  <script>
    let currentState = "IDLE";
    let isPaused = false;

    function formatTime(sec) {
      const m = Math.floor(sec / 60);
      const s = sec % 60;
      return `${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
    }

    function showToast(msg) {
      const toast = document.getElementById("toast");
      toast.innerText = msg;
      toast.className = "show";
      setTimeout(() => { toast.className = toast.className.replace("show", ""); }, 2000);
    }

    async function updateStatus() {
      try {
        const res = await fetch('/api/status');
        if (!res.ok) throw new Error("Status fetch error");
        const data = await res.json();

        // Update Connection Indicator
        document.getElementById("live-dot").style.background = "#22c55e";
        document.getElementById("live-dot").style.boxShadow = "0 0 8px #22c55e";
        document.getElementById("conn-status").innerText = "Connected";

        // Update State & Timer
        currentState = data.state;
        isPaused = data.isPaused;
        const totalSec = data.totalSec || 1500;
        const remainingSec = data.remainingSec || 0;
        const progressPct = Math.max(0, Math.min(100, ((totalSec - remainingSec) / totalSec) * 100));

        document.getElementById("timer-display").innerText = formatTime(remainingSec);
        document.getElementById("progress-bar").style.width = `${progressPct}%`;
        document.getElementById("cycle-count").innerText = `Cycles: ${data.cyclesCompleted || 0}`;

        const badge = document.getElementById("state-badge");
        const btnToggle = document.getElementById("btn-toggle");
        const progressBar = document.getElementById("progress-bar");

        if (currentState === "WORK") {
          if (isPaused) {
            badge.className = "timer-badge badge-paused";
            badge.innerText = "PAUSED (USER AWAY)";
            progressBar.style.background = "var(--amber)";
            btnToggle.innerText = "Resume";
            btnToggle.className = "btn btn-warning";
          } else {
            badge.className = "timer-badge badge-work";
            badge.innerText = "FOCUS SESSION";
            progressBar.style.background = "var(--red)";
            btnToggle.innerText = "Pause";
            btnToggle.className = "btn btn-danger";
          }
        } else if (currentState === "BREAK") {
          badge.className = "timer-badge badge-break";
          badge.innerText = "REST BREAK";
          progressBar.style.background = "var(--green)";
          btnToggle.innerText = "Pause";
          btnToggle.className = "btn btn-success";
        } else {
          badge.className = "timer-badge badge-idle";
          badge.innerText = "IDLE";
          progressBar.style.background = "#334155";
          btnToggle.innerText = "Start Focus";
          btnToggle.className = "btn btn-primary";
        }

        // Presence Update
        const presenceVal = document.getElementById("presence-val");
        if (data.isUserPresent) {
          presenceVal.innerText = "Present";
          presenceVal.style.color = "var(--green)";
        } else {
          presenceVal.innerText = "Away";
          presenceVal.style.color = "var(--text-muted)";
        }
        document.getElementById("distance-val").innerText = (data.distanceCm >= 900) ? "Out of range" : `${data.distanceCm.toFixed(1)} cm`;

        // Auto-pause toggle state sync
        document.getElementById("autopause-toggle").checked = data.autoPauseEnabled;
        document.getElementById("autopause-status").innerText = data.autoPauseEnabled ? "Enabled" : "Disabled";

        // DHT Environment
        document.getElementById("temp-val").innerHTML = (data.temperature > -90) ? `${data.temperature.toFixed(1)} &deg;C` : "-- &deg;C";
        document.getElementById("hum-val").innerText = (data.humidity > -90) ? `${data.humidity.toFixed(0)} %` : "-- %";

      } catch (err) {
        document.getElementById("live-dot").style.background = "#ef4444";
        document.getElementById("live-dot").style.boxShadow = "0 0 8px #ef4444";
        document.getElementById("conn-status").innerText = "Reconnecting...";
      }
    }

    async function sendIR(cmd) {
      try {
        await fetch(`/api/cmd?val=${cmd}`, { method: 'POST' });
        showToast(`Sent IR: ${cmd.toUpperCase()}`);
      } catch (e) {
        showToast("Error sending command");
      }
    }

    async function togglePomodoro() {
      if (currentState === "IDLE") {
        await fetch('/api/pomodoro?action=start', { method: 'POST' });
        showToast("Focus session started");
      } else if (isPaused) {
        await fetch('/api/pomodoro?action=resume', { method: 'POST' });
        showToast("Session resumed");
      } else {
        await fetch('/api/pomodoro?action=pause', { method: 'POST' });
        showToast("Session paused");
      }
      updateStatus();
    }

    async function skipPhase() {
      await fetch('/api/pomodoro?action=skip', { method: 'POST' });
      showToast("Phase skipped");
      updateStatus();
    }

    async function resetTimer() {
      await fetch('/api/pomodoro?action=reset', { method: 'POST' });
      showToast("Timer reset to Idle");
      updateStatus();
    }

    async function toggleAutoPause(enabled) {
      await fetch(`/api/settings?autopause=${enabled ? 1 : 0}`, { method: 'POST' });
      showToast(enabled ? "Auto-pause enabled" : "Auto-pause disabled");
      updateStatus();
    }

    // Polling status loop every 1 second
    setInterval(updateStatus, 1000);
    updateStatus();
  </script>
</body>
</html>
)rawliteral";
