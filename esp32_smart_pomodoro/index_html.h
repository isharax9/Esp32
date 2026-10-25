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
  <meta name="theme-color" content="#070a12">
  <title>Desk Assistant & Pomodoro</title>
  <style>
    :root {
      --bg: #070a12;
      --card-bg: rgba(19, 25, 41, 0.78);
      --card-border: rgba(255, 255, 255, 0.08);
      --card-highlight: rgba(255, 255, 255, 0.04);
      --text: #f8fafc;
      --text-dim: #94a3b8;
      --text-sub: #64748b;
      --red: #ef4444;
      --red-glow: rgba(239, 68, 68, 0.35);
      --green: #10b981;
      --green-glow: rgba(16, 185, 129, 0.35);
      --blue: #3b82f6;
      --blue-glow: rgba(59, 130, 246, 0.35);
      --amber: #f59e0b;
      --amber-glow: rgba(245, 158, 11, 0.35);
      --purple: #8b5cf6;
      --cyan: #06b6d4;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; -webkit-tap-highlight-color: transparent; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "SF Pro Display", "Segoe UI", Roboto, sans-serif;
      background: radial-gradient(circle at 50% -10%, #17223b 0%, #080c16 55%, #05070c 100%);
      color: var(--text);
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      padding-bottom: 96px;
      user-select: none;
      -webkit-font-smoothing: antialiased;
    }

    /* Sticky Frosted Header */
    header {
      padding: 14px 18px 12px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      background: rgba(7, 10, 18, 0.85);
      border-bottom: 1px solid var(--card-border);
      position: sticky;
      top: 0;
      z-index: 50;
      backdrop-filter: blur(20px);
      -webkit-backdrop-filter: blur(20px);
    }
    .brand {
      display: flex;
      align-items: center;
      gap: 10px;
      font-weight: 800;
      font-size: 1.05rem;
      letter-spacing: -0.02em;
    }
    .brand-glow-dot {
      width: 10px;
      height: 10px;
      border-radius: 50%;
      background: var(--red);
      box-shadow: 0 0 12px var(--red);
      animation: pulseBrand 2.5s infinite;
    }
    @keyframes pulseBrand {
      0%, 100% { transform: scale(1); opacity: 1; }
      50% { transform: scale(1.15); opacity: 0.7; }
    }
    .header-actions {
      display: flex;
      align-items: center;
      gap: 8px;
    }

    /* Screen Keep-Awake Pill Button */
    .wake-pill {
      display: flex;
      align-items: center;
      gap: 6px;
      padding: 6px 12px;
      border-radius: 999px;
      font-size: 0.74rem;
      font-weight: 700;
      cursor: pointer;
      border: 1px solid rgba(255, 255, 255, 0.12);
      background: rgba(255, 255, 255, 0.05);
      color: var(--text-dim);
      transition: all 0.25s ease;
    }
    .wake-pill svg { width: 14px; height: 14px; stroke-width: 2.2; transition: all 0.25s; }
    .wake-pill.active {
      background: rgba(245, 158, 11, 0.16);
      color: #fbbf24;
      border-color: rgba(245, 158, 11, 0.5);
      box-shadow: 0 0 14px rgba(245, 158, 11, 0.3);
    }
    .wake-pill.active svg {
      stroke: #fbbf24;
      filter: drop-shadow(0 0 4px #fbbf24);
      transform: rotate(20deg);
    }

    .status-badge {
      font-size: 0.72rem;
      font-weight: 700;
      padding: 5px 10px;
      border-radius: 999px;
      background: rgba(16, 185, 129, 0.12);
      color: var(--green);
      border: 1px solid rgba(16, 185, 129, 0.25);
      display: flex;
      align-items: center;
      gap: 6px;
    }
    .status-dot {
      width: 6px;
      height: 6px;
      border-radius: 50%;
      background: var(--green);
      box-shadow: 0 0 6px var(--green);
    }

    /* Main Container & Glass Cards */
    main {
      padding: 16px;
      max-width: 480px;
      width: 100%;
      margin: 0 auto;
    }
    .tab-pane { display: none; }
    .tab-pane.active {
      display: block;
      animation: tabFadeIn 0.25s cubic-bezier(0.16, 1, 0.3, 1);
    }
    @keyframes tabFadeIn {
      from { opacity: 0; transform: translateY(6px); }
      to { opacity: 1; transform: translateY(0); }
    }

    .card {
      background: var(--card-bg);
      border: 1px solid var(--card-border);
      border-radius: 24px;
      padding: 20px;
      margin-bottom: 16px;
      box-shadow: 0 12px 36px -4px rgba(0, 0, 0, 0.45), inset 0 1px 0 var(--card-highlight);
      backdrop-filter: blur(20px);
      -webkit-backdrop-filter: blur(20px);
      position: relative;
      overflow: hidden;
    }
    .card-head {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 16px;
    }
    .card-title {
      font-size: 0.78rem;
      font-weight: 800;
      letter-spacing: 0.06em;
      text-transform: uppercase;
      color: var(--text-dim);
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .card-title svg { stroke: var(--text-dim); }

    /* Circular Pomodoro Ring */
    .timer-wrap {
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      padding: 8px 0 14px;
    }
    .ring-container {
      position: relative;
      width: 230px;
      height: 230px;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    .ring-svg {
      transform: rotate(-90deg);
      width: 230px;
      height: 230px;
    }
    .ring-bg {
      fill: none;
      stroke: rgba(255, 255, 255, 0.05);
      stroke-width: 11;
    }
    .ring-progress {
      fill: none;
      stroke: var(--red);
      stroke-width: 11;
      stroke-linecap: round;
      stroke-dasharray: 628;
      stroke-dashoffset: 0;
      transition: stroke-dashoffset 0.6s cubic-bezier(0.16, 1, 0.3, 1), stroke 0.4s ease;
      filter: drop-shadow(0 0 12px var(--red-glow));
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
      letter-spacing: 0.08em;
      padding: 4px 12px;
      border-radius: 999px;
      text-transform: uppercase;
      margin-bottom: 6px;
      transition: all 0.3s;
    }
    .phase-idle {
      background: rgba(148, 163, 184, 0.14);
      color: #cbd5e1;
      border: 1px solid rgba(148, 163, 184, 0.25);
    }
    .phase-work {
      background: rgba(239, 68, 68, 0.16);
      color: #fca5a5;
      border: 1px solid rgba(239, 68, 68, 0.4);
      box-shadow: 0 0 14px var(--red-glow);
    }
    .phase-break {
      background: rgba(16, 185, 129, 0.16);
      color: #6ee7b7;
      border: 1px solid rgba(16, 185, 129, 0.4);
      box-shadow: 0 0 14px var(--green-glow);
    }
    .phase-pause {
      background: rgba(245, 158, 11, 0.16);
      color: #fcd34d;
      border: 1px solid rgba(245, 158, 11, 0.4);
      box-shadow: 0 0 14px var(--amber-glow);
    }

    .timer-digits {
      font-size: 3.6rem;
      font-weight: 800;
      letter-spacing: -0.04em;
      line-height: 1;
      font-variant-numeric: tabular-nums;
      text-shadow: 0 2px 14px rgba(0, 0, 0, 0.6);
    }
    .timer-sub {
      font-size: 0.76rem;
      color: var(--text-dim);
      margin-top: 8px;
      font-weight: 600;
    }

    /* Buttons & Controls */
    .btn-grid {
      display: flex;
      gap: 10px;
      width: 100%;
      margin-top: 14px;
    }
    .btn {
      flex: 1;
      min-height: 50px;
      padding: 12px 14px;
      border-radius: 16px;
      border: none;
      font-size: 0.88rem;
      font-weight: 700;
      color: #fff;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 7px;
      cursor: pointer;
      box-shadow: 0 4px 16px rgba(0, 0, 0, 0.35);
      transition: transform 0.1s ease, box-shadow 0.2s ease, opacity 0.2s ease;
    }
    .btn:active {
      transform: scale(0.96);
      box-shadow: 0 2px 8px rgba(0, 0, 0, 0.3);
    }
    .btn-red {
      background: linear-gradient(135deg, #ef4444, #dc2626);
      box-shadow: 0 4px 20px rgba(239, 68, 68, 0.35);
    }
    .btn-green {
      background: linear-gradient(135deg, #10b981, #059669);
      box-shadow: 0 4px 20px rgba(16, 185, 129, 0.35);
    }
    .btn-amber {
      background: linear-gradient(135deg, #f59e0b, #d97706);
      box-shadow: 0 4px 20px rgba(245, 158, 11, 0.35);
    }
    .btn-dark {
      background: rgba(30, 41, 59, 0.8);
      border: 1px solid rgba(255, 255, 255, 0.08);
      color: #e2e8f0;
    }
    .btn-outline {
      background: transparent;
      border: 1px solid rgba(255, 255, 255, 0.14);
      color: #cbd5e1;
    }

    /* Screen Lock Card in Pomodoro View */
    .wake-card-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 14px 16px;
      border-radius: 16px;
      background: rgba(15, 23, 42, 0.6);
      border: 1px solid var(--card-border);
      margin-top: 14px;
    }
    .wake-card-info {
      display: flex;
      flex-direction: column;
      gap: 2px;
    }
    .wake-card-title {
      font-size: 0.84rem;
      font-weight: 700;
      display: flex;
      align-items: center;
      gap: 6px;
    }
    .wake-card-desc {
      font-size: 0.72rem;
      color: var(--text-dim);
    }
    .wake-toggle-btn {
      padding: 8px 16px;
      border-radius: 12px;
      font-size: 0.78rem;
      font-weight: 800;
      border: 1px solid rgba(255, 255, 255, 0.12);
      background: rgba(255, 255, 255, 0.05);
      color: var(--text-dim);
      cursor: pointer;
      transition: all 0.2s;
    }
    .wake-toggle-btn.active {
      background: linear-gradient(135deg, #f59e0b, #d97706);
      color: #fff;
      border-color: #f59e0b;
      box-shadow: 0 2px 14px rgba(245, 158, 11, 0.4);
    }

    /* Presence Bar Visual */
    .presence-meter-wrap {
      margin-top: 10px;
    }
    .presence-bar-bg {
      height: 8px;
      background: rgba(255, 255, 255, 0.06);
      border-radius: 999px;
      overflow: hidden;
      position: relative;
    }
    .presence-bar-fill {
      height: 100%;
      background: linear-gradient(90deg, #10b981, #3b82f6);
      border-radius: 999px;
      width: 0%;
      transition: width 0.4s ease, background 0.3s;
    }
    .presence-ticks {
      display: flex;
      justify-content: space-between;
      margin-top: 4px;
      font-size: 0.66rem;
      color: var(--text-sub);
      font-weight: 600;
    }

    /* Target Strip Selector */
    .strip-selector {
      display: flex;
      background: rgba(10, 15, 28, 0.8);
      border-radius: 14px;
      padding: 4px;
      gap: 4px;
      margin-bottom: 16px;
      border: 1px solid var(--card-border);
    }
    .strip-tab {
      flex: 1;
      padding: 11px;
      border-radius: 10px;
      border: none;
      background: none;
      color: var(--text-dim);
      font-size: 0.82rem;
      font-weight: 700;
      cursor: pointer;
      transition: all 0.2s cubic-bezier(0.16, 1, 0.3, 1);
    }
    .strip-tab.active {
      background: var(--blue);
      color: #fff;
      box-shadow: 0 4px 14px rgba(59, 130, 246, 0.4);
    }

    /* 16-Color Palette Grid */
    .color-grid {
      display: grid;
      grid-template-columns: repeat(4, 1fr);
      gap: 9px;
      margin-top: 12px;
    }
    .color-chip {
      height: 52px;
      border-radius: 14px;
      border: 1px solid rgba(255, 255, 255, 0.12);
      font-size: 0.72rem;
      font-weight: 700;
      color: #fff;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      cursor: pointer;
      transition: transform 0.1s ease, box-shadow 0.2s ease;
      box-shadow: 0 3px 10px rgba(0, 0, 0, 0.35);
      text-shadow: 0 1px 3px rgba(0, 0, 0, 0.9);
      position: relative;
    }
    .color-chip:active { transform: scale(0.92); }
    .color-chip::after {
      content: "";
      position: absolute;
      inset: 0;
      border-radius: 13px;
      background: linear-gradient(180deg, rgba(255, 255, 255, 0.25) 0%, rgba(255, 255, 255, 0) 50%);
      pointer-events: none;
    }

    /* Dynamic Effects Grid */
    .fx-grid {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 10px;
      margin-top: 12px;
    }
    .fx-btn {
      padding: 14px;
      border-radius: 16px;
      border: 1px solid var(--card-border);
      background: rgba(15, 23, 42, 0.65);
      color: var(--text);
      font-size: 0.86rem;
      font-weight: 700;
      display: flex;
      align-items: center;
      gap: 10px;
      cursor: pointer;
      transition: all 0.2s ease;
    }
    .fx-btn:active { transform: scale(0.97); }
    .fx-btn.active {
      border-color: var(--blue);
      background: rgba(59, 130, 246, 0.18);
      color: #93c5fd;
      box-shadow: 0 0 18px rgba(59, 130, 246, 0.35);
    }

    /* Brightness Slider */
    .slider-wrap {
      display: flex;
      align-items: center;
      gap: 12px;
      background: rgba(11, 16, 30, 0.7);
      padding: 14px 16px;
      border-radius: 16px;
      margin-top: 12px;
      border: 1px solid var(--card-border);
    }
    .slider-wrap input[type=range] {
      flex: 1;
      height: 8px;
      border-radius: 999px;
      background: linear-gradient(90deg, #1e293b, var(--blue));
      outline: none;
      -webkit-appearance: none;
    }
    .slider-wrap input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 26px;
      height: 26px;
      border-radius: 50%;
      background: #ffffff;
      box-shadow: 0 0 14px var(--blue);
      cursor: pointer;
    }
    .slider-val {
      font-size: 0.88rem;
      font-weight: 800;
      width: 44px;
      text-align: right;
      color: var(--blue);
    }

    /* Environment Grid */
    .env-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
    .env-card {
      background: rgba(11, 16, 30, 0.7);
      border: 1px solid var(--card-border);
      border-radius: 18px;
      padding: 16px;
    }
    .env-label {
      font-size: 0.72rem;
      color: var(--text-dim);
      font-weight: 700;
      text-transform: uppercase;
      margin-bottom: 6px;
      display: flex;
      align-items: center;
      gap: 5px;
    }
    .env-val {
      font-size: 1.65rem;
      font-weight: 800;
      letter-spacing: -0.03em;
    }
    .env-sub {
      font-size: 0.72rem;
      color: var(--text-dim);
      margin-top: 4px;
      font-weight: 600;
    }

    /* Modern Toggle Row */
    .switch-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 14px 16px;
      background: rgba(11, 16, 30, 0.7);
      border-radius: 16px;
      margin-top: 14px;
      border: 1px solid var(--card-border);
    }
    .switch-text { font-size: 0.84rem; font-weight: 700; }
    .switch-sub { font-size: 0.7rem; color: var(--text-dim); display: block; margin-top: 2px; }
    .switch { position: relative; width: 48px; height: 28px; }
    .switch input { opacity: 0; width: 0; height: 0; }
    .slider {
      position: absolute; cursor: pointer; inset: 0;
      background-color: #334155; transition: .25s; border-radius: 28px;
    }
    .slider:before {
      position: absolute; content: ""; height: 22px; width: 22px; left: 3px; bottom: 3px;
      background-color: white; transition: .25s; border-radius: 50%;
      box-shadow: 0 2px 6px rgba(0,0,0,0.4);
    }
    input:checked + .slider { background-color: var(--green); }
    input:checked + .slider:before { transform: translateX(20px); }

    /* Pin Calibrator & Sniffer Boxes */
    .pin-box {
      background: rgba(11, 16, 30, 0.7);
      border-radius: 16px;
      padding: 14px 16px;
      margin-bottom: 12px;
      border: 1px solid var(--card-border);
    }
    .pin-title {
      font-size: 0.84rem;
      font-weight: 800;
      margin-bottom: 8px;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .pin-badges { display: flex; gap: 6px; margin-bottom: 10px; font-size: 0.74rem; font-weight: 700; }
    .p-badge { padding: 4px 9px; border-radius: 8px; background: rgba(30, 41, 59, 0.8); }
    .p-red { color: #f87171; border: 1px solid rgba(239, 68, 68, 0.35); }
    .p-green { color: #4ade80; border: 1px solid rgba(34, 197, 94, 0.35); }
    .p-blue { color: #60a5fa; border: 1px solid rgba(59, 130, 246, 0.35); }

    .cal-btn-row { display: flex; gap: 7px; margin-top: 7px; }
    .cal-btn {
      flex: 1; padding: 10px 4px; font-size: 0.76rem; font-weight: 700; border-radius: 10px;
      border: 1px solid rgba(255,255,255,0.08); background: rgba(30, 41, 59, 0.8); color: #fff; cursor: pointer;
      transition: transform 0.1s;
    }
    .cal-btn:active { transform: scale(0.95); }

    /* Bottom Frosted Navigation Dock */
    .tab-bar {
      position: fixed;
      bottom: 0;
      left: 0;
      right: 0;
      height: 74px;
      background: rgba(7, 10, 18, 0.88);
      border-top: 1px solid var(--card-border);
      display: flex;
      justify-content: space-around;
      align-items: center;
      z-index: 100;
      padding-bottom: env(safe-area-inset-bottom);
      backdrop-filter: blur(24px);
      -webkit-backdrop-filter: blur(24px);
    }
    .tab-btn {
      background: none;
      border: none;
      color: var(--text-sub);
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 5px;
      font-size: 0.72rem;
      font-weight: 700;
      cursor: pointer;
      padding: 8px 14px;
      border-radius: 14px;
      transition: all 0.2s cubic-bezier(0.16, 1, 0.3, 1);
    }
    .tab-btn svg { width: 22px; height: 22px; stroke-width: 2.2; transition: all 0.2s; }
    .tab-btn.active { color: var(--blue); }
    .tab-btn.active svg {
      stroke: var(--blue);
      filter: drop-shadow(0 0 8px rgba(59, 130, 246, 0.6));
      transform: translateY(-2px);
    }

    /* Floating Toast */
    #toast {
      position: fixed;
      bottom: 86px;
      left: 50%;
      transform: translateX(-50%) translateY(20px);
      background: rgba(15, 23, 42, 0.94);
      border: 1px solid rgba(255, 255, 255, 0.16);
      color: #fff;
      padding: 10px 22px;
      border-radius: 999px;
      font-size: 0.84rem;
      font-weight: 700;
      opacity: 0;
      pointer-events: none;
      transition: all 0.25s cubic-bezier(0.16, 1, 0.3, 1);
      z-index: 200;
      box-shadow: 0 8px 24px rgba(0, 0, 0, 0.6);
      backdrop-filter: blur(12px);
      display: flex;
      align-items: center;
      gap: 6px;
    }
    #toast.show { opacity: 1; transform: translateX(-50%) translateY(0); }
  </style>
</head>
<body>

  <!-- Top Mobile Bar -->
  <header>
    <div class="brand">
      <div class="brand-glow-dot"></div>
      <span>Desk Assistant</span>
    </div>
    <div class="header-actions">
      <!-- Screen Keep-Awake Button -->
      <button id="header-wake-btn" class="wake-pill" onclick="toggleWakeLock()" title="Prevent Screen from Locking">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor"><circle cx="12" cy="12" r="5"/><path d="M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42"/></svg>
        <span id="header-wake-txt">Keep Awake</span>
      </button>

      <div class="status-badge">
        <span class="status-dot" id="live-dot"></span>
        <span id="conn-status">Online</span>
      </div>
    </div>
  </header>

  <!-- Main Viewports -->
  <main>

    <!-- 1. POMODORO VIEW -->
    <div id="tab-timer" class="tab-pane active">
      <div class="card">
        <div class="card-head">
          <div class="card-title">
            <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>
            Focus Clock
          </div>
          <span id="cycle-badge" style="font-size:0.75rem; font-weight:800; color:var(--amber);">🍅 0 Done</span>
        </div>

        <div class="timer-wrap">
          <div class="ring-container">
            <svg class="ring-svg">
              <circle class="ring-bg" cx="115" cy="115" r="98"></circle>
              <circle id="ring-arc" class="ring-progress" cx="115" cy="115" r="98"></circle>
            </svg>
            <div class="ring-text">
              <div id="phase-pill" class="timer-phase-pill phase-idle">IDLE</div>
              <div id="timer-digits" class="timer-digits">25:00</div>
              <div id="timer-hint" class="timer-sub">Ready to start focus session</div>
            </div>
          </div>
        </div>

        <div class="btn-grid">
          <button id="btn-primary-action" class="btn btn-red" onclick="handlePrimaryAction()">Start Focus</button>
          <button class="btn btn-dark" onclick="skipPhase()">Skip</button>
          <button class="btn btn-outline" onclick="resetPomodoro()">Reset</button>
        </div>

        <!-- Quick Keep-Awake Option in Timer Card -->
        <div class="wake-card-row">
          <div class="wake-card-info">
            <div class="wake-card-title">
              <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="5" y="2" width="14" height="20" rx="2" ry="2"/><line x1="12" y1="18" x2="12.01" y2="18"/></svg>
              Prevent Screen Sleep
            </div>
            <div class="wake-card-desc" id="wake-card-status">Display stays awake while timer is running</div>
          </div>
          <button id="card-wake-btn" class="wake-toggle-btn" onclick="toggleWakeLock()">OFF</button>
        </div>
      </div>

      <!-- Ultrasonic Presence Indicator -->
      <div class="card">
        <div class="card-head">
          <div class="card-title">
            <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/><circle cx="12" cy="7" r="4"/></svg>
            Desk Presence Radar
          </div>
          <span id="presence-badge" style="font-size:0.75rem; font-weight:800; color:var(--text-sub);">Checking...</span>
        </div>
        <div class="env-grid">
          <div class="env-card">
            <div class="env-label">Chair Proximity</div>
            <div id="presence-text" class="env-val" style="color:var(--text-sub);">Away</div>
            <div id="distance-text" class="env-sub">-- cm away</div>
          </div>
          <div class="env-card">
            <div class="env-label">Auto-Pause</div>
            <div id="autopause-label" class="env-val" style="color:var(--green); font-size:1.2rem;">Active</div>
            <div class="env-sub">Pauses on walkaway</div>
          </div>
        </div>

        <!-- Visual Distance Bar -->
        <div class="presence-meter-wrap">
          <div class="presence-bar-bg">
            <div id="presence-bar" class="presence-bar-fill"></div>
          </div>
          <div class="presence-ticks">
            <span>0 cm</span>
            <span style="color:var(--green);">75 cm (Present limit)</span>
            <span>150+ cm</span>
          </div>
        </div>

        <div class="switch-row">
          <div>
            <span class="switch-text">Auto-Pause on Absence</span>
            <span class="switch-sub">Automatically pauses timer when you leave your desk</span>
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
            <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="5"/><path d="M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42"/></svg>
            Dual Desk Strip Lighting
          </div>
        </div>

        <!-- Target Strip Switcher -->
        <div class="strip-selector">
          <button id="tab-strip-all" class="strip-tab active" onclick="setStrip('all')">Both Strips</button>
          <button id="tab-strip-1" class="strip-tab" onclick="setStrip('1')">Strip 1 (Left)</button>
          <button id="tab-strip-2" class="strip-tab" onclick="setStrip('2')">Strip 2 (Right)</button>
        </div>

        <!-- Master Power Buttons -->
        <div class="btn-grid" style="margin-top:0; margin-bottom:14px;">
          <button class="btn btn-green" onclick="sendCmd('on')">⚡ Power ON</button>
          <button class="btn btn-outline" style="background:rgba(255,255,255,0.08); color:#f8fafc; font-weight:800;" onclick="sendCmd('white')">⚪ Pure White</button>
          <button class="btn btn-red" onclick="sendCmd('off')">⭕ Power OFF</button>
        </div>

        <!-- Touch Brightness Slider -->
        <div class="card-title" style="margin-top:6px;">Brightness Level</div>
        <div class="slider-wrap">
          <span style="font-size:0.8rem; color:var(--text-sub); font-weight:700;">10%</span>
          <input type="range" id="bright-range" min="25" max="255" value="255" oninput="onBrightnessInput(this.value)" onchange="onBrightnessChange(this.value)">
          <span id="bright-txt" class="slider-val">100%</span>
        </div>

        <!-- 16-Color Mobile Palette Grid -->
        <div class="card-title" style="margin-top:16px;">16 Static RGB Colors</div>
        <div class="color-grid">
          <!-- Row 1 -->
          <div class="color-chip" style="background:#ef4444;" onclick="sendCmd('red')">RED</div>
          <div class="color-chip" style="background:#10b981;" onclick="sendCmd('green')">GREEN</div>
          <div class="color-chip" style="background:#3b82f6;" onclick="sendCmd('blue')">BLUE</div>
          <div class="color-chip" style="background:#ffffff; color:#0f172a; text-shadow:none; font-weight:800;" onclick="sendCmd('white')">WHITE</div>

          <!-- Row 2 -->
          <div class="color-chip" style="background:#f97316;" onclick="sendCmd('orange')">ORANGE</div>
          <div class="color-chip" style="background:#22c55e;" onclick="sendCmd('pea_green')">LIME</div>
          <div class="color-chip" style="background:#2563eb;" onclick="sendCmd('dark_blue')">COBALT</div>
          <div class="color-chip" style="background:#06b6d4;" onclick="sendCmd('cyan')">CYAN</div>

          <!-- Row 3 -->
          <div class="color-chip" style="background:#ea580c;" onclick="sendCmd('dark_orange')">AMBER</div>
          <div class="color-chip" style="background:#14b8a6;" onclick="sendCmd('ice_blue')">MINT</div>
          <div class="color-chip" style="background:#a855f7;" onclick="sendCmd('purple')">PURPLE</div>
          <div class="color-chip" style="background:#d946ef;" onclick="sendCmd('magenta')">MAGENTA</div>

          <!-- Row 4 -->
          <div class="color-chip" style="background:#eab308; color:#0f172a; text-shadow:none;" onclick="sendCmd('yellow')">YELLOW</div>
          <div class="color-chip" style="background:#0ea5e9;" onclick="sendCmd('light_blue')">SKY BLUE</div>
          <div class="color-chip" style="background:#ec4899;" onclick="sendCmd('pink')">PINK</div>
          <div class="color-chip" style="background:#fb923c;" onclick="sendCmd('light_orange')">CORAL</div>
        </div>

        <!-- Dynamic Lighting Effects -->
        <div class="card-title" style="margin-top:18px;">Dynamic Light Effects</div>
        <div class="fx-grid">
          <button id="fx-flash" class="fx-btn" onclick="sendCmd('flash')">
            <span style="font-size:1.3rem;">⚡</span>
            <div><div>FLASH</div><div style="font-size:0.68rem; color:var(--text-dim);">7-Color Jump</div></div>
          </button>
          <button id="fx-strobe" class="fx-btn" onclick="sendCmd('strobe')">
            <span style="font-size:1.3rem;">✨</span>
            <div><div>STROBE</div><div style="font-size:0.68rem; color:var(--text-dim);">Pure Pulse</div></div>
          </button>
          <button id="fx-fade" class="fx-btn" onclick="sendCmd('fade')">
            <span style="font-size:1.3rem;">🌈</span>
            <div><div>FADE</div><div style="font-size:0.68rem; color:var(--text-dim);">Rainbow Cycle</div></div>
          </button>
          <button id="fx-smooth" class="fx-btn" onclick="sendCmd('smooth')">
            <span style="font-size:1.3rem;">🌊</span>
            <div><div>SMOOTH</div><div style="font-size:0.68rem; color:var(--text-dim);">Ambient Breath</div></div>
          </button>
        </div>
      </div>
    </div>

    <!-- 3. ENVIRONMENT & CLIMATE VIEW -->
    <div id="tab-weather" class="tab-pane">
      <div class="card">
        <div class="card-head">
          <div class="card-title">
            <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M14 14.76V3.5a2.5 2.5 0 0 0-5 0v11.26a4.5 4.5 0 1 0 5 0z"/></svg>
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
          <p id="climate-summary" style="font-size:0.8rem; color:var(--text-dim); line-height:1.45;">
            Continuous environmental monitoring via onboard DHT sensor. Keeps track of room temperature and comfort thresholds.
          </p>
        </div>
      </div>
    </div>

    <!-- 4. PIN SETUP & CALIBRATOR VIEW -->
    <div id="tab-cal" class="tab-pane">
      <div class="card">
        <div class="card-head">
          <div class="card-title">
            <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M14.7 6.3a1 1 0 0 0 0 1.4l1.6 1.6a1 1 0 0 0 1.4 0l3.77-3.77a6 6 0 0 1-7.94 7.94l-6.91 6.91a2.12 2.12 0 0 1-3-3l6.91-6.91a6 6 0 0 1 7.94-7.94l-3.76 3.76z"/></svg>
            Hardware Pin Calibrator
          </div>
        </div>
        <p style="font-size:0.75rem; color:var(--text-dim); margin-bottom:12px; line-height:1.4;">
          Test channels individually or swap mismatched pins in real-time right from your phone.
        </p>

        <!-- Strip 1 Calibrator -->
        <div class="pin-box">
          <div class="pin-title">
            <span>Strip 1 Channels</span>
            <span style="color:var(--text-sub); font-size:0.72rem;">Desk Left</span>
          </div>
          <div class="pin-badges">
            <span id="p1-r" class="p-badge p-red">R: GPIO 23</span>
            <span id="p1-g" class="p-badge p-green">G: GPIO 22</span>
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
            <span style="color:var(--text-sub); font-size:0.72rem;">Desk Right</span>
          </div>
          <div class="pin-badges">
            <span id="p2-r" class="p-badge p-red">R: D19</span>
            <span id="p2-g" class="p-badge p-green">G: TX2</span>
            <span id="p2-b" class="p-badge p-blue">B: RX2</span>
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
            <span style="color:var(--green); font-size:0.72rem; font-weight:700;">● Active</span>
          </div>
          <p style="font-size:0.72rem; color:var(--text-dim); margin-bottom:8px;">
            Point your remote at the VS1838B sensor and press any button to inspect live NEC codes:
          </p>
          <div style="display:flex; align-items:center; justify-content:space-between; background:rgba(7, 10, 18, 0.7); padding:12px 14px; border-radius:12px; border:1px solid var(--card-border);">
            <div>
              <div style="font-size:0.65rem; color:var(--text-sub); text-transform:uppercase; font-weight:800;">Received Hex</div>
              <div id="remote-hex" style="font-family:ui-monospace, monospace; font-size:1.15rem; font-weight:800; color:var(--text); letter-spacing:1px;">0x000000</div>
            </div>
            <div style="text-align:right;">
              <div style="font-size:0.65rem; color:var(--text-sub); text-transform:uppercase; font-weight:800;">Identified Button</div>
              <div id="remote-name" style="font-size:0.86rem; font-weight:800; color:var(--blue);">Ready for input</div>
            </div>
          </div>
        </div>

        <!-- Ultrasonic Sensor Hardware Guidance -->
        <div style="margin-top:14px; padding:12px 14px; border-radius:14px; background:rgba(59, 130, 246, 0.08); border:1px solid rgba(59, 130, 246, 0.25); font-size:0.75rem; color:#93c5fd; line-height:1.5;">
          📡 <b>Ultrasonic Sensor Pin Guide:</b><br>
          • <b>VCC:</b> Must connect to <b>5V / VIN rail</b><br>
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

  <!-- Hidden Minimal Silent Video for older iOS NoSleep fallback -->
  <video id="nosleep-video" playsinline muted loop style="position:fixed;width:1px;height:1px;opacity:0.01;pointer-events:none;z-index:-1;">
    <source src="data:video/mp4;base64,AAAAHGZ0eXBtcDQyAAAAAG1wNDJpc29tYXZjMQAAADhmoW92AAAAbG12aGQAAAAA10..." type="video/mp4">
  </video>

  <script>
    let currentState = "IDLE";
    let isPaused = false;
    let targetStrip = "all";
    const ringCircumference = 2 * Math.PI * 98; // ~615.75 px

    // ==========================================
    // Screen Lock / Keep-Awake Engine
    // ==========================================
    let wakeLockSentinel = null;
    let wakeLockDesired = false;

    function updateWakeLockUI(isActive) {
      const headerBtn = document.getElementById("header-wake-btn");
      const headerTxt = document.getElementById("header-wake-txt");
      const cardBtn = document.getElementById("card-wake-btn");
      const cardStatus = document.getElementById("wake-card-status");

      if (isActive) {
        if (headerBtn) headerBtn.classList.add("active");
        if (headerTxt) headerTxt.innerText = "Awake ON";
        if (cardBtn) {
          cardBtn.classList.add("active");
          cardBtn.innerText = "ON";
        }
        if (cardStatus) cardStatus.innerText = "Screen lock prevented • Display will stay on";
      } else {
        if (headerBtn) headerBtn.classList.remove("active");
        if (headerTxt) headerTxt.innerText = "Keep Awake";
        if (cardBtn) {
          cardBtn.classList.remove("active");
          cardBtn.innerText = "OFF";
        }
        if (cardStatus) cardStatus.innerText = "Display stays awake while timer is running";
      }
    }

    async function requestWakeLock() {
      // 1. Try Native Standard Screen Wake Lock API
      if ('wakeLock' in navigator) {
        try {
          wakeLockSentinel = await navigator.wakeLock.request('screen');
          wakeLockSentinel.addEventListener('release', () => {
            if (!wakeLockDesired) {
              updateWakeLockUI(false);
            }
          });
          updateWakeLockUI(true);
          return true;
        } catch (err) {
          console.warn("Native WakeLock request error:", err);
        }
      }

      // 2. Fallback: Silent Loop Video for older Safari/WebViews
      try {
        const vid = document.getElementById("nosleep-video");
        if (vid) {
          const p = vid.play();
          if (p !== undefined) {
            await p;
          }
          updateWakeLockUI(true);
          return true;
        }
      } catch (vidErr) {
        console.warn("Video fallback error:", vidErr);
      }

      updateWakeLockUI(false);
      return false;
    }

    async function releaseWakeLock() {
      wakeLockDesired = false;
      if (wakeLockSentinel) {
        try {
          await wakeLockSentinel.release();
        } catch (e) {}
        wakeLockSentinel = null;
      }
      try {
        const vid = document.getElementById("nosleep-video");
        if (vid) vid.pause();
      } catch (e) {}
      updateWakeLockUI(false);
    }

    async function toggleWakeLock() {
      hapticTap();
      if (wakeLockDesired) {
        await releaseWakeLock();
        showToast("Screen Lock: Auto-Sleep Restored");
      } else {
        wakeLockDesired = true;
        const ok = await requestWakeLock();
        if (ok) {
          showToast("Screen Awake: Active (No Sleep) ☀️");
        } else {
          showToast("Keep-Awake requires user tap or unsupported");
        }
      }
    }

    // Auto re-acquire wake lock if user switches back to this tab
    document.addEventListener("visibilitychange", async () => {
      if (wakeLockDesired && document.visibilityState === "visible") {
        await requestWakeLock();
      }
    });

    // ==========================================
    // UI Navigation & Haptic Feedback
    // ==========================================
    function hapticTap() {
      if (navigator.vibrate) {
        navigator.vibrate(12);
      }
    }

    function switchTab(tab) {
      hapticTap();
      document.querySelectorAll('.tab-pane').forEach(el => el.classList.remove('active'));
      document.querySelectorAll('.tab-btn').forEach(el => el.classList.remove('active'));
      document.getElementById('tab-' + tab).classList.add('active');
      document.getElementById('nav-' + tab).classList.add('active');
    }

    function setStrip(strip) {
      hapticTap();
      targetStrip = strip;
      document.querySelectorAll('.strip-tab').forEach(el => el.classList.remove('active'));
      document.getElementById('tab-strip-' + strip).classList.add('active');
      const label = strip === 'all' ? 'Both Strips' : (strip === '1' ? 'Strip 1 (Left)' : 'Strip 2 (Right)');
      showToast(`Targeting: ${label}`);
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

    // ==========================================
    // Backend API Interaction
    // ==========================================
    async function sendCmd(val) {
      hapticTap();
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
      hapticTap();
      try {
        await fetch(`/api/cmd?val=brightness&level=${val}&strip=${targetStrip}`, { method: 'POST' });
        showToast(`Brightness: ${Math.round((val / 255) * 100)}%`);
      } catch (e) {}
    }

    async function handlePrimaryAction() {
      hapticTap();
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
      hapticTap();
      await fetch('/api/pomodoro?action=skip', { method: 'POST' });
      showToast("Phase skipped ⏭");
      updateStatus();
    }

    async function resetPomodoro() {
      hapticTap();
      await fetch('/api/pomodoro?action=reset', { method: 'POST' });
      showToast("Reset to Idle ⏹");
      updateStatus();
    }

    async function toggleAutoPause(enabled) {
      hapticTap();
      await fetch(`/api/settings?autopause=${enabled ? 1 : 0}`, { method: 'POST' });
      showToast(enabled ? "Auto-pause Enabled" : "Auto-pause Disabled");
      updateStatus();
    }

    async function testChannel(strip, ch) {
      hapticTap();
      try {
        await fetch(`/api/calibrate?strip=${strip}&test=${ch}`, { method: 'POST' });
        showToast(`Testing ${ch.toUpperCase()} on Strip ${strip}`);
      } catch (e) {}
    }

    async function swapPins(strip, action) {
      hapticTap();
      try {
        await fetch(`/api/calibrate?strip=${strip}&action=${action}`, { method: 'POST' });
        showToast(`Swapped pins on Strip ${strip}`);
        updateStatus();
      } catch (e) {}
    }

    // ==========================================
    // Live Real-Time Telemetry Polling
    // ==========================================
    async function updateStatus() {
      try {
        const res = await fetch('/api/status');
        if (!res.ok) throw new Error();
        const d = await res.json();

        // Connection
        document.getElementById("live-dot").style.background = "#10b981";
        document.getElementById("conn-status").innerText = "Online";

        currentState = d.state;
        isPaused = d.isPaused;

        // Timer Display
        const total = d.totalSec || 1500;
        const rem = d.remainingSec || 0;
        document.getElementById("timer-digits").innerText = formatTime(rem);
        document.getElementById("cycle-badge").innerText = `🍅 ${d.cyclesCompleted || 0} Done`;

        // Ring Progress
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
            pill.innerText = "PAUSED (AWAY)";
            hint.innerText = "Step in front of desk to resume";
            arc.style.stroke = "var(--amber)";
            arc.style.filter = "drop-shadow(0 0 12px var(--amber-glow))";
            btn.className = "btn btn-amber";
            btn.innerText = "Resume Focus";
          } else {
            pill.className = "timer-phase-pill phase-work";
            pill.innerText = "FOCUS SESSION";
            hint.innerText = "Stay in the flow zone";
            arc.style.stroke = "var(--red)";
            arc.style.filter = "drop-shadow(0 0 12px var(--red-glow))";
            btn.className = "btn btn-amber";
            btn.innerText = "Pause Focus";
          }
        } else if (currentState === "BREAK") {
          pill.className = "timer-phase-pill phase-break";
          pill.innerText = "REST BREAK";
          hint.innerText = "Relax, stretch, and hydrate";
          arc.style.stroke = "var(--green)";
          arc.style.filter = "drop-shadow(0 0 12px var(--green-glow))";
          btn.className = "btn btn-green";
          btn.innerText = "Pause Rest";
        } else {
          pill.className = "timer-phase-pill phase-idle";
          pill.innerText = "IDLE";
          hint.innerText = "Ready to start a 25m Focus";
          arc.style.stroke = "rgba(255,255,255,0.15)";
          arc.style.filter = "none";
          btn.className = "btn btn-red";
          btn.innerText = "Start Focus";
        }

        // Presence Radar & Distance Meter
        const pBadge = document.getElementById("presence-badge");
        const pTxt = document.getElementById("presence-text");
        const dTxt = document.getElementById("distance-text");
        const pBar = document.getElementById("presence-bar");

        if (d.isUserPresent) {
          pBadge.innerText = "Present";
          pBadge.style.color = "var(--green)";
          pTxt.innerText = "At Desk";
          pTxt.style.color = "var(--green)";
        } else {
          pBadge.innerText = "Away";
          pBadge.style.color = "var(--text-sub)";
          pTxt.innerText = "Away";
          pTxt.style.color = "var(--text-sub)";
        }

        if (d.distanceCm >= 900) {
          dTxt.innerText = "Out of range";
          if (pBar) pBar.style.width = "0%";
        } else {
          dTxt.innerText = `${d.distanceCm.toFixed(1)} cm away`;
          if (pBar) {
            // Map 0 - 150cm to 100% - 0% width (closer = fuller bar)
            const norm = Math.max(0, Math.min(100, Math.round(((150 - d.distanceCm) / 150) * 100)));
            pBar.style.width = `${norm}%`;
            pBar.style.background = (d.distanceCm < 75) 
              ? "linear-gradient(90deg, #10b981, #3b82f6)" 
              : "linear-gradient(90deg, #64748b, #475569)";
          }
        }

        // Auto pause
        document.getElementById("autopause-chk").checked = d.autoPauseEnabled;
        document.getElementById("autopause-label").innerText = d.autoPauseEnabled ? "Active" : "Disabled";
        document.getElementById("autopause-label").style.color = d.autoPauseEnabled ? "var(--green)" : "var(--text-sub)";

        // Environment & Climate
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

        // Dynamic Pin Status
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

        // Live Remote Sniffer
        if (d.lastIrHex && d.lastIrHex !== "0x000000") {
          const rh = document.getElementById("remote-hex");
          const rn = document.getElementById("remote-name");
          if (rh) rh.innerText = d.lastIrHex;
          if (rn) {
            rn.innerText = d.lastIrName;
            rn.style.color = (d.lastIrName === "UNMAPPED") ? "#ef4444" : "#10b981";
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
