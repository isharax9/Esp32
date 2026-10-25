#pragma once
#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
  <meta name="apple-mobile-web-app-capable" content="yes">
  <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
  <meta name="theme-color" content="#090d16">
  <title>Desk Assistant Mobile</title>
  <style>
    :root {
      --bg: #090d16;
      --card-bg: rgba(22, 28, 45, 0.85);
      --card-border: rgba(255, 255, 255, 0.08);
      --text: #f8fafc;
      --muted: #94a3b8;
      --red: #ef4444;
      --green: #22c55e;
      --blue: #3b82f6;
      --amber: #f59e0b;
      --purple: #a855f7;
      --cyan: #06b6d4;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; -webkit-tap-highlight-color: transparent; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      background-color: var(--bg);
      color: var(--text);
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      padding-bottom: 90px;
      user-select: none;
    }
    header {
      padding: 16px 20px 12px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      background: rgba(9, 13, 22, 0.95);
      border-bottom: 1px solid var(--card-border);
      position: sticky;
      top: 0;
      z-index: 50;
      backdrop-filter: blur(10px);
    }
    .brand { display: flex; align-items: center; gap: 8px; font-weight: 800; font-size: 1.1rem; letter-spacing: -0.02em; }
    .brand-icon { width: 14px; height: 14px; border-radius: 50%; background: var(--red); box-shadow: 0 0 10px var(--red); }
    .status-badge {
      font-size: 0.72rem;
      font-weight: 600;
      padding: 4px 10px;
      border-radius: 999px;
      background: rgba(34, 197, 94, 0.15);
      color: var(--green);
      border: 1px solid rgba(34, 197, 94, 0.3);
      display: flex;
      align-items: center;
      gap: 5px;
    }
    .status-dot { width: 7px; height: 7px; border-radius: 50%; background: var(--green); }
    
    /* Navigation Bar */
    .tab-bar {
      position: fixed;
      bottom: 0;
      left: 0;
      right: 0;
      height: 70px;
      background: rgba(15, 23, 42, 0.96);
      border-top: 1px solid var(--card-border);
      display: flex;
      justify-content: space-around;
      align-items: center;
      z-index: 100;
      padding-bottom: env(safe-area-inset-bottom);
      backdrop-filter: blur(16px);
    }
    .tab-btn {
      background: none;
      border: none;
      color: var(--muted);
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 4px;
      font-size: 0.7rem;
      font-weight: 600;
      cursor: pointer;
      padding: 6px 12px;
      border-radius: 12px;
      transition: all 0.2s;
    }
    .tab-btn svg { width: 22px; height: 22px; stroke-width: 2; }
    .tab-btn.active { color: var(--blue); }
    .tab-btn.active svg { stroke: var(--blue); filter: drop-shadow(0 0 8px rgba(59, 130, 246, 0.5)); }

    /* Content Area */
    main { padding: 16px; max-width: 480px; width: 100%; margin: 0 auto; }
    .tab-pane { display: none; }
    .tab-pane.active { display: block; animation: fadeIn 0.2s ease-out; }
    @keyframes fadeIn { from { opacity: 0; transform: translateY(4px); } to { opacity: 1; transform: translateY(0); } }

    .card {
      background: var(--card-bg);
      border: 1px solid var(--card-border);
      border-radius: 20px;
      padding: 18px;
      margin-bottom: 14px;
      box-shadow: 0 8px 24px rgba(0, 0, 0, 0.35);
      backdrop-filter: blur(12px);
    }
    .card-head {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 14px;
    }
    .card-title {
      font-size: 0.8rem;
      font-weight: 700;
      letter-spacing: 0.05em;
      text-transform: uppercase;
      color: var(--muted);
      display: flex;
      align-items: center;
      gap: 6px;
    }

    /* Pomodoro Timer Center */
    .timer-wrap {
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      padding: 10px 0;
    }
    .ring-container {
      position: relative;
      width: 220px;
      height: 220px;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    .ring-svg { transform: rotate(-90deg); width: 220px; height: 220px; }
    .ring-bg { fill: none; stroke: rgba(255, 255, 255, 0.06); stroke-width: 10; }
    .ring-progress {
      fill: none;
      stroke: var(--red);
      stroke-width: 10;
      stroke-linecap: round;
      stroke-dasharray: 628;
      stroke-dashoffset: 0;
      transition: stroke-dashoffset 0.5s ease, stroke 0.4s ease;
    }
    .ring-text {
      position: absolute;
      text-align: center;
      display: flex;
      flex-direction: column;
      align-items: center;
    }
    .timer-phase-pill {
      font-size: 0.72rem;
      font-weight: 800;
      letter-spacing: 0.06em;
      padding: 4px 12px;
      border-radius: 999px;
      text-transform: uppercase;
      margin-bottom: 6px;
    }
    .phase-idle { background: rgba(148, 163, 184, 0.2); color: #cbd5e1; }
    .phase-work { background: rgba(239, 68, 68, 0.2); color: var(--red); border: 1px solid rgba(239, 68, 68, 0.4); }
    .phase-break { background: rgba(34, 197, 94, 0.2); color: var(--green); border: 1px solid rgba(34, 197, 94, 0.4); }
    .phase-pause { background: rgba(245, 158, 11, 0.2); color: var(--amber); border: 1px solid rgba(245, 158, 11, 0.4); }

    .timer-digits {
      font-size: 3.4rem;
      font-weight: 800;
      letter-spacing: -0.04em;
      line-height: 1;
      font-variant-numeric: tabular-nums;
    }
    .timer-sub { font-size: 0.75rem; color: var(--muted); margin-top: 6px; font-weight: 600; }

    /* Touch Action Buttons */
    .btn-grid { display: flex; gap: 10px; width: 100%; margin-top: 16px; }
    .btn {
      flex: 1;
      min-height: 48px;
      padding: 12px;
      border-radius: 14px;
      border: none;
      font-size: 0.88rem;
      font-weight: 700;
      color: #fff;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 6px;
      cursor: pointer;
      box-shadow: 0 4px 12px rgba(0,0,0,0.25);
      transition: transform 0.1s, opacity 0.2s;
    }
    .btn:active { transform: scale(0.96); }
    .btn-red { background: linear-gradient(135deg, #ef4444, #dc2626); }
    .btn-green { background: linear-gradient(135deg, #22c55e, #16a34a); }
    .btn-blue { background: linear-gradient(135deg, #3b82f6, #2563eb); }
    .btn-amber { background: linear-gradient(135deg, #f59e0b, #d97706); }
    .btn-dark { background: #1e293b; color: #cbd5e1; }
    .btn-outline { background: transparent; border: 1px solid rgba(255,255,255,0.15); color: #cbd5e1; }

    /* Segment Strip Selector */
    .strip-selector {
      display: flex;
      background: #0f172a;
      border-radius: 12px;
      padding: 4px;
      gap: 4px;
      margin-bottom: 14px;
    }
    .strip-tab {
      flex: 1;
      padding: 10px;
      border-radius: 9px;
      border: none;
      background: none;
      color: var(--muted);
      font-size: 0.8rem;
      font-weight: 700;
      cursor: pointer;
      transition: all 0.2s;
    }
    .strip-tab.active { background: var(--blue); color: #fff; box-shadow: 0 2px 8px rgba(59, 130, 246, 0.4); }

    /* 16-Color Palette Grid */
    .color-grid {
      display: grid;
      grid-template-columns: repeat(4, 1fr);
      gap: 8px;
      margin-top: 10px;
    }
    .color-chip {
      height: 48px;
      border-radius: 12px;
      border: 1px solid rgba(255,255,255,0.12);
      font-size: 0.72rem;
      font-weight: 700;
      color: #fff;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      cursor: pointer;
      transition: transform 0.1s, box-shadow 0.2s;
      box-shadow: 0 2px 8px rgba(0,0,0,0.3);
      text-shadow: 0 1px 2px rgba(0,0,0,0.8);
    }
    .color-chip:active { transform: scale(0.92); }

    /* Dynamic FX Modes Grid */
    .fx-grid {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 10px;
      margin-top: 10px;
    }
    .fx-btn {
      padding: 14px;
      border-radius: 14px;
      border: 1px solid var(--card-border);
      background: rgba(15, 23, 42, 0.8);
      color: var(--text);
      font-size: 0.84rem;
      font-weight: 700;
      display: flex;
      align-items: center;
      gap: 8px;
      cursor: pointer;
      transition: all 0.2s;
    }
    .fx-btn:active { transform: scale(0.97); }
    .fx-btn.active {
      border-color: var(--blue);
      background: rgba(59, 130, 246, 0.15);
      color: #93c5fd;
      box-shadow: 0 0 16px rgba(59, 130, 246, 0.3);
    }

    /* Brightness Slider */
    .slider-wrap {
      display: flex;
      align-items: center;
      gap: 12px;
      background: #0f172a;
      padding: 12px 14px;
      border-radius: 14px;
      margin-top: 10px;
    }
    .slider-wrap input[type=range] {
      flex: 1;
      height: 8px;
      border-radius: 999px;
      background: #334155;
      outline: none;
      -webkit-appearance: none;
    }
    .slider-wrap input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 24px;
      height: 24px;
      border-radius: 50%;
      background: var(--blue);
      box-shadow: 0 0 10px var(--blue);
      cursor: pointer;
    }
    .slider-val { font-size: 0.85rem; font-weight: 700; width: 44px; text-align: right; color: var(--blue); }

    /* Environment Grid */
    .env-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
    .env-card {
      background: #0f172a;
      border: 1px solid var(--card-border);
      border-radius: 16px;
      padding: 14px;
    }
    .env-label { font-size: 0.72rem; color: var(--muted); font-weight: 600; text-transform: uppercase; margin-bottom: 4px; }
    .env-val { font-size: 1.5rem; font-weight: 800; letter-spacing: -0.02em; }
    .env-sub { font-size: 0.72rem; color: var(--muted); margin-top: 4px; font-weight: 500; }

    /* Toggle Switch */
    .switch-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 12px 14px;
      background: #0f172a;
      border-radius: 14px;
      margin-top: 12px;
    }
    .switch-text { font-size: 0.82rem; font-weight: 600; }
    .switch-sub { font-size: 0.7rem; color: var(--muted); display: block; margin-top: 2px; }
    .switch { position: relative; width: 46px; height: 26px; }
    .switch input { opacity: 0; width: 0; height: 0; }
    .slider {
      position: absolute; cursor: pointer; inset: 0;
      background-color: #334155; transition: .25s; border-radius: 26px;
    }
    .slider:before {
      position: absolute; content: ""; height: 20px; width: 20px; left: 3px; bottom: 3px;
      background-color: white; transition: .25s; border-radius: 50%;
    }
    input:checked + .slider { background-color: var(--blue); }
    input:checked + .slider:before { transform: translateX(20px); }

    /* Pin Calibrator */
    .pin-box {
      background: #0f172a;
      border-radius: 14px;
      padding: 12px 14px;
      margin-bottom: 10px;
    }
    .pin-title { font-size: 0.82rem; font-weight: 700; margin-bottom: 6px; display: flex; justify-content: space-between; }
    .pin-badges { display: flex; gap: 6px; margin-bottom: 10px; font-size: 0.75rem; font-weight: 700; }
    .p-badge { padding: 4px 8px; border-radius: 6px; background: #1e293b; }
    .p-red { color: #f87171; border: 1px solid rgba(239, 68, 68, 0.3); }
    .p-green { color: #4ade80; border: 1px solid rgba(34, 197, 94, 0.3); }
    .p-blue { color: #60a5fa; border: 1px solid rgba(59, 130, 246, 0.3); }

    .cal-btn-row { display: flex; gap: 6px; margin-top: 6px; }
    .cal-btn {
      flex: 1; padding: 9px 4px; font-size: 0.75rem; font-weight: 700; border-radius: 9px;
      border: 1px solid rgba(255,255,255,0.1); background: #1e293b; color: #fff; cursor: pointer;
    }
    .cal-btn:active { transform: scale(0.95); }

    /* Toast */
    #toast {
      position: fixed;
      bottom: 80px;
      left: 50%;
      transform: translateX(-50%) translateY(20px);
      background: rgba(15, 23, 42, 0.95);
      border: 1px solid rgba(255, 255, 255, 0.15);
      color: #fff;
      padding: 10px 20px;
      border-radius: 999px;
      font-size: 0.82rem;
      font-weight: 600;
      opacity: 0;
      pointer-events: none;
      transition: all 0.25s ease;
      z-index: 200;
      box-shadow: 0 4px 20px rgba(0,0,0,0.5);
    }
    #toast.show { opacity: 1; transform: translateX(-50%) translateY(0); }
  </style>
</head>
<body>

  <!-- Top Mobile Bar -->
  <header>
    <div class="brand">
      <div class="brand-icon"></div>
      <span>Desk Assistant</span>
    </div>
    <div class="status-badge">
      <span class="status-dot" id="live-dot"></span>
      <span id="conn-status">Online</span>
    </div>
  </header>

  <!-- Main Viewports -->
  <main>

    <!-- 1. POMODORO VIEW -->
    <div id="tab-timer" class="tab-pane active">
      <div class="card">
        <div class="card-head">
          <div class="card-title">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>
            Pomodoro Focus Clock
          </div>
          <span id="cycle-badge" style="font-size:0.75rem; font-weight:700; color:var(--amber);">🍅 0 Done</span>
        </div>

        <div class="timer-wrap">
          <div class="ring-container">
            <svg class="ring-svg">
              <circle class="ring-bg" cx="110" cy="110" r="95"></circle>
              <circle id="ring-arc" class="ring-progress" cx="110" cy="110" r="95"></circle>
            </svg>
            <div class="ring-text">
              <div id="phase-pill" class="timer-phase-pill phase-idle">IDLE</div>
              <div id="timer-digits" class="timer-digits">25:00</div>
              <div id="timer-hint" class="timer-sub">Ready to Focus</div>
            </div>
          </div>
        </div>

        <div class="btn-grid">
          <button id="btn-primary-action" class="btn btn-red" onclick="handlePrimaryAction()">Start Focus</button>
          <button class="btn btn-dark" onclick="skipPhase()">Skip</button>
          <button class="btn btn-outline" onclick="resetPomodoro()">Reset</button>
        </div>
      </div>

      <!-- Ultrasonic Presence Indicator -->
      <div class="card">
        <div class="card-head">
          <div class="card-title">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/><circle cx="12" cy="7" r="4"/></svg>
            Desk Presence Detection
          </div>
          <span id="presence-badge" style="font-size:0.75rem; font-weight:700; color:var(--muted);">Checking...</span>
        </div>
        <div class="env-grid">
          <div class="env-card">
            <div class="env-label">Chair State</div>
            <div id="presence-text" class="env-val" style="color:var(--muted);">Away</div>
            <div id="distance-text" class="env-sub">-- cm away</div>
          </div>
          <div class="env-card">
            <div class="env-label">Auto-Pause</div>
            <div id="autopause-label" class="env-val" style="color:var(--green); font-size:1.15rem;">Active</div>
            <div class="env-sub">Pauses on walkaway</div>
          </div>
        </div>
        <div class="switch-row">
          <div>
            <span class="switch-text">Auto-Pause on Absence</span>
            <span class="switch-sub">Timer pauses when you leave your desk</span>
          </div>
          <label class="switch">
            <input type="checkbox" id="autopause-chk" checked onchange="toggleAutoPause(this.checked)">
            <span class="slider"></span>
          </label>
        </div>
      </div>
    </div>

    <!-- 2. LIGHTING STUDIO VIEW -->
    <div id="tab-lights" class="tab-pane">
      <div class="card">
        <div class="card-head">
          <div class="card-title">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="5"/><path d="M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42"/></svg>
            Dual Desk Strip Lighting
          </div>
        </div>

        <!-- Target Strip Switcher -->
        <div class="strip-selector">
          <button id="tab-strip-all" class="strip-tab active" onclick="setStrip('all')">Both Strips</button>
          <button id="tab-strip-1" class="strip-tab" onclick="setStrip('1')">Strip 1</button>
          <button id="tab-strip-2" class="strip-tab" onclick="setStrip('2')">Strip 2</button>
        </div>

        <!-- Master Power Buttons -->
        <div class="btn-grid" style="margin-top:0; margin-bottom:12px;">
          <button class="btn btn-dark" style="background:#22c55e; color:#fff;" onclick="sendCmd('on')">⚡ Power ON</button>
          <button class="btn btn-outline" style="background:#f8fafc; color:#0f172a; font-weight:800;" onclick="sendCmd('white')">⚪ Pure White</button>
          <button class="btn btn-dark" style="background:#ef4444; color:#fff;" onclick="sendCmd('off')">⭕ Power OFF</button>
        </div>

        <!-- Touch Brightness Slider -->
        <div class="card-title" style="margin-top:8px;">Brightness Level</div>
        <div class="slider-wrap">
          <span style="font-size:0.8rem; color:var(--muted); font-weight:700;">10%</span>
          <input type="range" id="bright-range" min="25" max="255" value="255" oninput="onBrightnessInput(this.value)" onchange="onBrightnessChange(this.value)">
          <span id="bright-txt" class="slider-val">100%</span>
        </div>

        <!-- 16-Color Mobile Palette Grid -->
        <div class="card-title" style="margin-top:16px;">16 Static RGB Colors</div>
        <div class="color-grid">
          <!-- Row 1 -->
          <div class="color-chip" style="background:#ef4444;" onclick="sendCmd('red')">RED</div>
          <div class="color-chip" style="background:#22c55e;" onclick="sendCmd('green')">GREEN</div>
          <div class="color-chip" style="background:#3b82f6;" onclick="sendCmd('blue')">BLUE</div>
          <div class="color-chip" style="background:#f8fafc; color:#0f172a;" onclick="sendCmd('white')">WHITE</div>

          <!-- Row 2 -->
          <div class="color-chip" style="background:#ea580c;" onclick="sendCmd('orange')">ORANGE</div>
          <div class="color-chip" style="background:#10b981;" onclick="sendCmd('pea_green')">PEA GRN</div>
          <div class="color-chip" style="background:#1d4ed8;" onclick="sendCmd('dark_blue')">DK BLUE</div>
          <div class="color-chip" style="background:#c2410c;" onclick="sendCmd('dark_orange')">DK ORG</div>

          <!-- Row 3 -->
          <div class="color-chip" style="background:#06b6d4;" onclick="sendCmd('cyan')">CYAN</div>
          <div class="color-chip" style="background:#be185d;" onclick="sendCmd('brown')">MAGENTA</div>
          <div class="color-chip" style="background:#f59e0b;" onclick="sendCmd('light_orange')">AMBER</div>
          <div class="color-chip" style="background:#38bdf8; color:#0f172a;" onclick="sendCmd('light_blue')">SKY BLUE</div>

          <!-- Row 4 -->
          <div class="color-chip" style="background:#ec4899;" onclick="sendCmd('pink')">PINK</div>
          <div class="color-chip" style="background:#eab308; color:#0f172a;" onclick="sendCmd('yellow')">YELLOW</div>
          <div class="color-chip" style="background:#67e8f9; color:#0f172a;" onclick="sendCmd('ice_blue')">ICE BLUE</div>
          <div class="color-chip" style="background:#a855f7;" onclick="sendCmd('purple')">PURPLE</div>
        </div>

        <!-- 4 Dynamic Lighting Modes -->
        <div class="card-title" style="margin-top:16px;">Dynamic Lighting Modes</div>
        <div class="fx-grid">
          <button id="fx-flash" class="fx-btn" onclick="sendCmd('flash')">
            <span style="font-size:1.2rem;">⚡</span>
            <div><div>FLASH</div><div style="font-size:0.65rem; color:var(--muted);">7-Color Jump</div></div>
          </button>
          <button id="fx-strobe" class="fx-btn" onclick="sendCmd('strobe')">
            <span style="font-size:1.2rem;">🚨</span>
            <div><div>STROBE</div><div style="font-size:0.65rem; color:var(--muted);">White Pulse</div></div>
          </button>
          <button id="fx-fade" class="fx-btn" onclick="sendCmd('fade')">
            <span style="font-size:1.2rem;">🌈</span>
            <div><div>FADE</div><div style="font-size:0.65rem; color:var(--muted);">Rainbow Cycle</div></div>
          </button>
          <button id="fx-smooth" class="fx-btn" onclick="sendCmd('smooth')">
            <span style="font-size:1.2rem;">🌊</span>
            <div><div>SMOOTH</div><div style="font-size:0.65rem; color:var(--muted);">Ambient Breath</div></div>
          </button>
        </div>
      </div>
    </div>

    <!-- 3. ENVIRONMENT & WEATHER VIEW -->
    <div id="tab-weather" class="tab-pane">
      <div class="card">
        <div class="card-head">
          <div class="card-title">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M14 14.76V3.5a2.5 2.5 0 0 0-5 0v11.26a4.5 4.5 0 1 0 5 0z"/></svg>
            Room Climate & Sensors
          </div>
        </div>
        <div class="env-grid">
          <div class="env-card">
            <div class="env-label">Temperature</div>
            <div id="temp-display" class="env-val">-- &deg;C</div>
            <div id="temp-comfort" class="env-sub" style="color:var(--green);">Checking...</div>
          </div>
          <div class="env-card">
            <div class="env-label">Humidity</div>
            <div id="hum-display" class="env-val">-- %</div>
            <div id="hum-comfort" class="env-sub" style="color:var(--cyan);">Checking...</div>
          </div>
        </div>

        <div class="pin-box" style="margin-top:14px;">
          <div class="pin-title">Desk Climate Insights</div>
          <p id="climate-summary" style="font-size:0.8rem; color:var(--muted); line-height:1.4;">
            Connecting to onboard DHT sensor for live temperature and ambient humidity telemetry.
          </p>
        </div>
      </div>
    </div>

    <!-- 4. PIN SETUP & CALIBRATOR VIEW -->
    <div id="tab-cal" class="tab-pane">
      <div class="card">
        <div class="card-head">
          <div class="card-title">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M14.7 6.3a1 1 0 0 0 0 1.4l1.6 1.6a1 1 0 0 0 1.4 0l3.77-3.77a6 6 0 0 1-7.94 7.94l-6.91 6.91a2.12 2.12 0 0 1-3-3l6.91-6.91a6 6 0 0 1 7.94-7.94l-3.76 3.76z"/></svg>
            Hardware Pin Calibrator
          </div>
        </div>
        <p style="font-size:0.75rem; color:var(--muted); margin-bottom:12px; line-height:1.4;">
          Test channels individually or swap mismatched pins in real-time right from your phone.
        </p>

        <!-- Strip 1 Calibrator -->
        <div class="pin-box">
          <div class="pin-title">
            <span>Strip 1 Channels</span>
            <span style="color:var(--muted); font-size:0.72rem;">Desk Left</span>
          </div>
          <div class="pin-badges">
            <span id="p1-r" class="p-badge p-red">R: GPIO 22</span>
            <span id="p1-g" class="p-badge p-green">G: GPIO 23</span>
            <span id="p1-b" class="p-badge p-blue">B: GPIO 21</span>
          </div>
          <div class="cal-btn-row">
            <button class="cal-btn" style="color:#ef4444;" onclick="testChannel('1','r')">🔴 Test Red</button>
            <button class="cal-btn" style="color:#22c55e;" onclick="testChannel('1','g')">🟢 Test Green</button>
            <button class="cal-btn" style="color:#3b82f6;" onclick="testChannel('1','b')">🔵 Test Blue</button>
          </div>
          <div class="cal-btn-row">
            <button class="cal-btn" onclick="swapPins('1','swap_rg')">🔄 Swap R & G</button>
            <button class="cal-btn" onclick="swapPins('1','swap_gb')">🔄 Swap G & B</button>
          </div>
        </div>

        <!-- Strip 2 Calibrator -->
        <div class="pin-box">
          <div class="pin-title">
            <span>Strip 2 Channels</span>
            <span style="color:var(--muted); font-size:0.72rem;">Desk Right</span>
          </div>
          <div class="pin-badges">
            <span id="p2-r" class="p-badge p-red">R: D19</span>
            <span id="p2-g" class="p-badge p-green">G: TX2 (GPIO 17)</span>
            <span id="p2-b" class="p-badge p-blue">B: RX2 (GPIO 16)</span>
          </div>
          <div class="cal-btn-row">
            <button class="cal-btn" style="color:#ef4444;" onclick="testChannel('2','r')">🔴 Test Red (D19)</button>
            <button class="cal-btn" style="color:#22c55e;" onclick="testChannel('2','g')">🟢 Test Green (TX2)</button>
            <button class="cal-btn" style="color:#3b82f6;" onclick="testChannel('2','b')">🔵 Test Blue (RX2)</button>
          </div>
          <div class="cal-btn-row">
            <button class="cal-btn" onclick="swapPins('2','swap_rg')">🔄 Swap R & G</button>
            <button class="cal-btn" onclick="swapPins('2','swap_gb')">🔄 Swap G & B</button>
            <button class="cal-btn" onclick="swapPins('2','swap_rb')">🔄 Swap R & B</button>
          </div>
        </div>

        <button class="btn btn-dark" style="width:100%; margin-top:4px;" onclick="testChannel('all','off')">Turn Off Test Lights</button>

        <!-- Live Physical Remote Signal Sniffer Card -->
        <div class="pin-box" style="margin-top:14px;">
          <div class="pin-title">
            <span>Live 24-Key Remote Sniffer</span>
            <span style="color:var(--green); font-size:0.72rem; font-weight:700;">● Listening</span>
          </div>
          <p style="font-size:0.72rem; color:var(--muted); margin-bottom:8px;">
            Point your remote at the VS1838B sensor and press any button to inspect live codes:
          </p>
          <div style="display:flex; align-items:center; justify-content:space-between; background:var(--card-bg); padding:10px 14px; border-radius:10px; border:1px solid var(--border);">
            <div>
              <div style="font-size:0.65rem; color:var(--muted); text-transform:uppercase; font-weight:700;">Received Hex</div>
              <div id="remote-hex" style="font-family:monospace; font-size:1.1rem; font-weight:800; color:var(--text); letter-spacing:1px;">0x000000</div>
            </div>
            <div style="text-align:right;">
              <div style="font-size:0.65rem; color:var(--muted); text-transform:uppercase; font-weight:700;">Identified Button</div>
              <div id="remote-name" style="font-size:0.85rem; font-weight:800; color:var(--accent);">Ready for input</div>
            </div>
          </div>
        </div>

        <!-- Ultrasonic Sensor Hardware Guidance -->
        <div style="margin-top:14px; padding:12px 14px; border-radius:12px; background:rgba(59, 130, 246, 0.08); border:1px solid rgba(59, 130, 246, 0.25); font-size:0.75rem; color:#93c5fd; line-height:1.5;">
          📡 <b>Ultrasonic Sensor Wiring Check:</b><br>
          • <b>VCC:</b> Must connect to <b>5V / VIN rail</b> (HC-SR04 requires 5V to ping)<br>
          • <b>GND:</b> Common GND rail<br>
          • <b>Trig:</b> ESP32 pin <b>D5</b><br>
          • <b>Echo:</b> ESP32 pin <b>D18</b> (via 1k/2k voltage divider)
        </div>
      </div>
    </div>

  </main>

  <!-- Bottom Navigation Bar -->
  <nav class="tab-bar">
    <button id="nav-timer" class="tab-btn active" onclick="switchTab('timer')">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>
      <span>Pomodoro</span>
    </button>
    <button id="nav-lights" class="tab-btn" onclick="switchTab('lights')">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor"><circle cx="12" cy="12" r="5"/><path d="M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42"/></svg>
      <span>Lighting</span>
    </button>
    <button id="nav-weather" class="tab-btn" onclick="switchTab('weather')">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor"><path d="M14 14.76V3.5a2.5 2.5 0 0 0-5 0v11.26a4.5 4.5 0 1 0 5 0z"/></svg>
      <span>Climate</span>
    </button>
    <button id="nav-cal" class="tab-btn" onclick="switchTab('cal')">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor"><path d="M14.7 6.3a1 1 0 0 0 0 1.4l1.6 1.6a1 1 0 0 0 1.4 0l3.77-3.77a6 6 0 0 1-7.94 7.94l-6.91 6.91a2.12 2.12 0 0 1-3-3l6.91-6.91a6 6 0 0 1 7.94-7.94l-3.76 3.76z"/></svg>
      <span>Pins</span>
    </button>
  </nav>

  <div id="toast">Command sent</div>

  <script>
    let currentState = "IDLE";
    let isPaused = false;
    let targetStrip = "all";
    const ringCircumference = 2 * Math.PI * 95; // ~596.9 px

    function switchTab(tab) {
      document.querySelectorAll('.tab-pane').forEach(el => el.classList.remove('active'));
      document.querySelectorAll('.tab-btn').forEach(el => el.classList.remove('active'));
      document.getElementById('tab-' + tab).classList.add('active');
      document.getElementById('nav-' + tab).classList.add('active');
    }

    function setStrip(strip) {
      targetStrip = strip;
      document.querySelectorAll('.strip-tab').forEach(el => el.classList.remove('active'));
      document.getElementById('tab-strip-' + strip).classList.add('active');
      showToast(`Targeting: ${strip === 'all' ? 'Both Strips' : 'Strip ' + strip}`);
    }

    function showToast(msg) {
      const t = document.getElementById("toast");
      t.innerText = msg;
      t.className = "show";
      setTimeout(() => { t.className = ""; }, 1800);
    }

    function formatTime(sec) {
      const m = Math.floor(sec / 60);
      const s = sec % 60;
      return `${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
    }

    async function sendCmd(val) {
      try {
        await fetch(`/api/cmd?val=${val}&strip=${targetStrip}`, { method: 'POST' });
        showToast(`Sent: ${val.toUpperCase()}`);
        updateStatus();
      } catch (e) {
        showToast("Error sending command");
      }
    }

    function onBrightnessInput(val) {
      const pct = Math.round((val / 255) * 100);
      document.getElementById('bright-txt').innerText = `${pct}%`;
    }

    async function onBrightnessChange(val) {
      try {
        await fetch(`/api/cmd?val=brightness&level=${val}&strip=${targetStrip}`, { method: 'POST' });
        showToast(`Brightness: ${Math.round((val / 255) * 100)}%`);
      } catch (e) {}
    }

    async function handlePrimaryAction() {
      if (currentState === "IDLE") {
        await fetch('/api/pomodoro?action=start', { method: 'POST' });
        showToast("Focus session started! 🍅");
      } else if (isPaused) {
        await fetch('/api/pomodoro?action=resume', { method: 'POST' });
        showToast("Session resumed ▶");
      } else {
        await fetch('/api/pomodoro?action=pause', { method: 'POST' });
        showToast("Session paused ⏸");
      }
      updateStatus();
    }

    async function skipPhase() {
      await fetch('/api/pomodoro?action=skip', { method: 'POST' });
      showToast("Phase skipped ⏭");
      updateStatus();
    }

    async function resetPomodoro() {
      await fetch('/api/pomodoro?action=reset', { method: 'POST' });
      showToast("Reset to Idle ⏹");
      updateStatus();
    }

    async function toggleAutoPause(enabled) {
      await fetch(`/api/settings?autopause=${enabled ? 1 : 0}`, { method: 'POST' });
      showToast(enabled ? "Auto-pause Enabled" : "Auto-pause Disabled");
      updateStatus();
    }

    async function testChannel(strip, ch) {
      try {
        await fetch(`/api/calibrate?strip=${strip}&test=${ch}`, { method: 'POST' });
        showToast(`Testing ${ch.toUpperCase()} on Strip ${strip}`);
      } catch (e) {}
    }

    async function swapPins(strip, action) {
      try {
        await fetch(`/api/calibrate?strip=${strip}&action=${action}`, { method: 'POST' });
        showToast(`Swapped pins on Strip ${strip}`);
        updateStatus();
      } catch (e) {}
    }

    async function updateStatus() {
      try {
        const res = await fetch('/api/status');
        if (!res.ok) throw new Error();
        const d = await res.json();

        // Connection
        document.getElementById("live-dot").style.background = "#22c55e";
        document.getElementById("conn-status").innerText = "Online";

        currentState = d.state;
        isPaused = d.isPaused;

        // Timer Display
        const total = d.totalSec || 1500;
        const rem = d.remainingSec || 0;
        document.getElementById("timer-digits").innerText = formatTime(rem);
        document.getElementById("cycle-badge").innerText = `🍅 ${d.cyclesCompleted || 0} Done`;

        // Ring progress
        const pct = Math.max(0, Math.min(1, (total - rem) / total));
        const offset = ringCircumference * (1 - pct);
        const arc = document.getElementById("ring-arc");
        arc.style.strokeDasharray = ringCircumference;
        arc.style.strokeDashoffset = offset;

        const pill = document.getElementById("phase-pill");
        const hint = document.getElementById("timer-hint");
        const btn = document.getElementById("btn-primary-action");

        if (currentState === "WORK") {
          if (isPaused) {
            pill.className = "timer-phase-pill phase-pause";
            pill.innerText = "PAUSED (USER AWAY)";
            hint.innerText = "Step in front of desk to resume";
            arc.style.stroke = "var(--amber)";
            btn.className = "btn btn-amber";
            btn.innerText = "Resume Focus";
          } else {
            pill.className = "timer-phase-pill phase-work";
            pill.innerText = "FOCUS SESSION";
            hint.innerText = "Focus on your deep work";
            arc.style.stroke = "var(--red)";
            btn.className = "btn btn-amber";
            btn.innerText = "Pause Focus";
          }
        } else if (currentState === "BREAK") {
          pill.className = "timer-phase-pill phase-break";
          pill.innerText = "REST BREAK";
          hint.innerText = "Relax, stretch, and hydrate";
          arc.style.stroke = "var(--green)";
          btn.className = "btn btn-green";
          btn.innerText = "Pause Rest";
        } else {
          pill.className = "timer-phase-pill phase-idle";
          pill.innerText = "IDLE";
          hint.innerText = "Ready to start a 25m Focus";
          arc.style.stroke = "rgba(255,255,255,0.15)";
          btn.className = "btn btn-red";
          btn.innerText = "Start Focus";
        }

        // Presence
        const pBadge = document.getElementById("presence-badge");
        const pTxt = document.getElementById("presence-text");
        const dTxt = document.getElementById("distance-text");
        if (d.isUserPresent) {
          pBadge.innerText = "Present";
          pBadge.style.color = "var(--green)";
          pTxt.innerText = "At Desk";
          pTxt.style.color = "var(--green)";
        } else {
          pBadge.innerText = "Away";
          pBadge.style.color = "var(--muted)";
          pTxt.innerText = "Away";
          pTxt.style.color = "var(--muted)";
        }
        dTxt.innerText = (d.distanceCm >= 900) ? "Out of range" : `${d.distanceCm.toFixed(1)} cm away`;

        // Auto pause
        document.getElementById("autopause-chk").checked = d.autoPauseEnabled;
        document.getElementById("autopause-label").innerText = d.autoPauseEnabled ? "Active" : "Disabled";
        document.getElementById("autopause-label").style.color = d.autoPauseEnabled ? "var(--green)" : "var(--muted)";

        // Environment
        if (d.temperature > -90) {
          document.getElementById("temp-display").innerHTML = `${d.temperature.toFixed(1)} &deg;C`;
          const tComp = (d.temperature < 20) ? "A bit cool" : (d.temperature > 27 ? "Warm room" : "Ideal room temp");
          document.getElementById("temp-comfort").innerText = tComp;
        }
        if (d.humidity > -90) {
          document.getElementById("hum-display").innerText = `${d.humidity.toFixed(0)} %`;
          const hComp = (d.humidity < 35) ? "Dry air" : (d.humidity > 65 ? "Humid air" : "Comfortable humidity");
          document.getElementById("hum-comfort").innerText = hComp;
        }

        // Effect Mode Indicators
        const fxMap = { 1: "fx-flash", 2: "fx-strobe", 3: "fx-fade", 4: "fx-smooth" };
        document.querySelectorAll('.fx-btn').forEach(b => b.classList.remove('active'));
        if (fxMap[d.effect]) {
          const act = document.getElementById(fxMap[d.effect]);
          if (act) act.classList.add('active');
        }

        // Dynamic Pins
        if (d.s1) {
          document.getElementById("p1-r").innerText = `R: GPIO ${d.s1.r}`;
          document.getElementById("p1-g").innerText = `G: GPIO ${d.s1.g}`;
          document.getElementById("p1-b").innerText = `B: GPIO ${d.s1.b}`;
        }
        if (d.s2) {
          document.getElementById("p2-r").innerText = `R: GPIO ${d.s2.r}`;
          document.getElementById("p2-g").innerText = `G: GPIO ${d.s2.g}`;
          document.getElementById("p2-b").innerText = `B: GPIO ${d.s2.b}`;
        }

        // Live Remote Signal
        if (d.lastIrHex && d.lastIrHex !== "0x000000") {
          const rh = document.getElementById("remote-hex");
          const rn = document.getElementById("remote-name");
          if (rh) rh.innerText = d.lastIrHex;
          if (rn) {
            rn.innerText = d.lastIrName;
            rn.style.color = (d.lastIrName === "UNMAPPED") ? "#ef4444" : "#22c55e";
          }
        }

      } catch (err) {
        document.getElementById("live-dot").style.background = "#ef4444";
        document.getElementById("conn-status").innerText = "Reconnecting...";
      }
    }

    setInterval(updateStatus, 1000);
    updateStatus();
  </script>
</body>
</html>
)rawliteral";
