# Wiring Diagrams — "Wire Tap" UART Lab

All diagrams use the ESP32-WROOM DevKitC pinout. GPIO numbers are labeled.
Color coding is consistent across all phases.

```
WIRE COLOR KEY
══════════════
  Red    ━━━  3.3V Power
  Black  ━━━  Ground (GND)
  Yellow ━━━  UART TX
  Green  ━━━  UART RX
  Blue   ━━━  Sensor Signal (vibration)
  Orange ╍╍╍  Attack / Tap wire (dashed in print)
```

---

## Diagram 1: "The Honest System" (Phase 1)

Two ESP32 boards + vibration sensor. Vault sensor reads vibration and sends status to security gateway over UART.

```
                    ┌─── USB to Laptop ───┐       ┌─── USB to Laptop ───┐
                    │   (Serial Monitor    │       │   (Serial Monitor    │
                    │    for debugging)    │       │    for display)      │
                    └────────┬────────────┘       └────────┬────────────┘
                             │                             │
           ┌─────────────────┴──────────────┐  ┌──────────┴──────────────────┐
           │     ESP32 #1 "VAULT SENSOR"    │  │  ESP32 #2 "SECURITY GATEWAY"│
           │                                │  │  First National Bank        │
           │  GPIO 14 ◄── Blue ── VIBRATION │  │  Vault #7                   │
           │                      SIGNAL    │  │                             │
           │  3.3V ──── Red ────► VIBRATION │  │                             │
           │                      VCC       │  │                             │
           │  GND ───── Black ──► VIBRATION │  │                             │
           │                      GND       │  │                             │
           │                                │  │                             │
           │  GPIO 17 (TX2) ═══ Yellow ═══════► GPIO 16 (RX2)               │
           │                                │  │                             │
           │  GND ═════════════ Black ═══════► GND                          │
           │                                │  │                             │
           └────────────────────────────────┘  └─────────────────────────────┘
```

### Pin Connection Table

| From | Wire | To | Purpose |
|------|------|----|---------|
| ESP32 #1 — 3.3V | Red | Vibration VCC | Sensor power |
| ESP32 #1 — GND | Black | Vibration GND | Sensor ground |
| ESP32 #1 — GPIO 14 | Blue | Vibration SIGNAL | Vibration detection (INPUT_PULLUP) |
| ESP32 #1 — GPIO 17 (TX2) | Yellow | ESP32 #2 — GPIO 16 (RX2) | UART data: sensor → gateway |
| ESP32 #1 — GND | Black | ESP32 #2 — GND | Common ground reference |

### Breadboard Layout Guide

```
    ┌─────────────────────────────────────────────────────────────┐
    │                        BREADBOARD                           │
    │                                                             │
    │   ┌──────────┐                          ┌──────────┐        │
    │   │  ESP32    │                          │  ESP32    │        │
    │   │   #1      │                          │   #2      │        │
    │   │  VAULT    │                          │  GATEWAY  │        │
    │   │  SENSOR  ┌┤GPIO 17 ──── yellow ────► ├┐GPIO 16  │        │
    │   │          ├┤GND ──────── black ─────► ├┤GND      │        │
    │   │          ├┤GPIO 14                   │          │        │
    │   │          ├┤3.3V                      │          │        │
    │   │          ├┤GND                       │          │        │
    │   └──────────┘│                          └──────────┘        │
    │               │                                              │
    │               │   ┌───────────┐                              │
    │               │   │ VIBRATION │                              │
    │               │   │  SENSOR   │                              │
    │               │   │ ┌─┬─┬─┐  │                              │
    │               └──►│ S │   │  │                              │
    │          3.3V ──► │ V │   │  │                              │
    │          GND  ──► │ G │   │  │                              │
    │                   └───────────┘                              │
    │                                                              │
    │   S = SIGNAL (blue wire to GPIO 14)                          │
    │   V = VCC    (red wire to 3.3V)                              │
    │   G = GND    (black wire to GND)                             │
    └─────────────────────────────────────────────────────────────┘
```

---

## Diagram 2: "The Tap" (Phase 2)

Everything from Phase 1 stays in place. One additional wire taps the TX line.

```
           ┌─────────────────────────────────┐  ┌─────────────────────────────┐
           │     ESP32 #1 "VAULT SENSOR"     │  │  ESP32 #2 "SECURITY GATEWAY"│
           │     (pre-flashed v2 firmware)   │  │                             │
           │                                 │  │                             │
           │  GPIO 14 ◄── Blue ── VIBRATION  │  │                             │
           │  3.3V ──── Red ────► VIBRATION  │  │                             │
           │  GND ───── Black ──► VIBRATION  │  │                             │
           │                                 │  │                             │
           │  GPIO 17 (TX2) ══╦══ Yellow ══════► GPIO 16 (RX2)               │
           │                  ║              │  │                             │
           │  GND ══════╦═════╩══ Black ═══════► GND                         │
           │            ║                    │  │                             │
           └────────────║────────────────────┘  └─────────────────────────────┘
                        ║
                ┌───────║─────────────────────────┐
                │       ║   ESP32 #3 "SNIFFER"    │
                │       ║   (attacker board)      │
                │       ║                         │
                │       ╠══ Orange ══► GPIO 16    │
                │       ║             (RX2)       │
                │       ║                         │  ┌─── USB to Laptop ───┐
                │       ╚══ Black ═══► GND        │  │ (Serial Monitor —   │
                │                                 │──┤  watch vault admin  │
                │                                 │  │  credentials appear │
                └─────────────────────────────────┘  │  in plaintext!)     │
                                                     └────────────────────┘

    ┌──────────────────────────────────────────────────────────┐
    │                                                          │
    │   THIS SINGLE ORANGE WIRE GIVES YOU EVERYTHING.          │
    │                                                          │
    │   No tools. No exploits. No authentication to bypass.    │
    │   Just a wire and a serial monitor.                      │
    │                                                          │
    └──────────────────────────────────────────────────────────┘
```

### Additional Connections for Phase 2

| From | Wire | To | Purpose |
|------|------|----|---------|
| ESP32 #1 — GPIO 17 (TX2) | Orange (tap) | ESP32 #3 — GPIO 16 (RX2) | Passive eavesdrop on TX line |
| ESP32 #1 — GND | Black | ESP32 #3 — GND | Common ground for sniffer |

**All Phase 1 wiring remains unchanged.**

---

## Diagram 3: "The Heist" (Phase 3)

The sensor's TX wire to the gateway is disconnected. The attacker board takes its place, sending constant "CLEAR" while the vault is drilled.

```
           ┌─────────────────────────────────┐  ┌─────────────────────────────┐
           │     ESP32 #1 "VAULT SENSOR"     │  │  ESP32 #2 "SECURITY GATEWAY"│
           │     (still running — ignored)   │  │                             │
           │                                 │  │                             │
           │  GPIO 14 ◄── Blue ── VIBRATION  │  │                             │
           │  3.3V ──── Red ────► VIBRATION  │  │                             │
           │  GND ───── Black ──► VIBRATION  │  │                             │
           │                                 │  │                             │
           │  GPIO 17 (TX2) ──╳  DISCONNECTED│  │                             │
           │                                 │  │                             │
           │  GND ══════════════ Black ═══════► GND                          │
           │                                 │  │                             │
           └─────────────────────────────────┘  └──────────────┬──────────────┘
                                                               │
                                                               │
                ┌──────────────────────────────┐               │
                │  ESP32 #3 "THE THIEF"        │               │
                │  (running injector.ino)      │               │
                │                              │               │
                │  GPIO 17 (TX2) ═══ Orange ═══════════════════╪══► GPIO 16
                │                              │               │    (RX2)
                │  GND ═════════════ Black  ═══════════════════╪══► GND
                │                              │               │
                └──────────────────────────────┘               │
                        │                                      │
                 ┌──────┴──────┐                        ┌──────┴──────┐
                 │ USB Laptop  │                        │ USB Laptop  │
                 │ (attacker   │                        │ (gateway    │
                 │  monitor —  │                        │  monitor —  │
                 │  "drill is  │                        │  shows      │
                 │   running") │                        │  CLEAR!)    │
                 └─────────────┘                        └─────────────┘

    ┌──────────────────────────────────────────────────────────┐
    │                                                          │
    │   THE VAULT IS BEING DRILLED.                            │
    │   THE SECURITY MONITOR SHOWS "CLEAR."                    │
    │                                                          │
    │   Same baud rate + same format = accepted.               │
    │   The real sensor is still running. Still detecting.     │
    │   Nobody is listening to it anymore.                     │
    │                                                          │
    └──────────────────────────────────────────────────────────┘
```

### Connection Changes from Phase 2 → Phase 3

| Action | Wire | Notes |
|--------|------|-------|
| REMOVE | Yellow wire (ESP32 #1 TX → ESP32 #2 RX) | Disconnect sensor from gateway |
| REMOVE | Orange wire (ESP32 #1 TX → ESP32 #3 RX) | Sniffer no longer needed |
| ADD | Orange wire: ESP32 #3 GPIO 17 (TX2) → ESP32 #2 GPIO 16 (RX2) | Attacker replaces sensor |
| ADD | Black wire: ESP32 #3 GND → ESP32 #2 GND | Common ground |

---

## Diagram 4: "The Inside Job" (Phase 3.5 — Stretch)

The attacker sits between sensor and gateway. Receives real CRITICAL alerts, suppresses them, forwards fake CLEAR. The gateway sees normal-looking traffic. The vault is being drilled and nobody knows.

```
    ┌──────────────────┐     ┌──────────────────────┐     ┌──────────────────┐
    │  ESP32 #1        │     │  ESP32 #3             │     │  ESP32 #2        │
    │  "VAULT SENSOR"  │     │  "THE INSIDE MAN"     │     │  "SEC. GATEWAY"  │
    │                  │     │                       │     │                  │
    │  VIBRATION──►G14 │     │                       │     │                  │
    │                  │     │                       │     │                  │
    │  GPIO 17 (TX2) ══╪═══►│═ GPIO 4 (UART1 RX)   │     │                  │
    │                  │     │                       │     │                  │
    │                  │     │  GPIO 17 (UART2 TX) ══╪═══►│═ GPIO 16 (RX2)  │
    │                  │     │                       │     │                  │
    │  GND ════════════╪════╪═══════════════════════╪════╪═ GND             │
    │                  │     │                       │     │                  │
    └──────────────────┘     └──────────────────────┘     └──────────────────┘
           │                          │                          │
    ┌──────┴──────┐           ┌───────┴──────┐           ┌──────┴──────┐
    │ USB Laptop  │           │ USB Laptop   │           │ USB Laptop  │
    │ (sensor     │           │ (intercept   │           │ (gateway    │
    │  debug —    │           │  log — see   │           │  display —  │
    │  CRITICAL!) │           │  REAL vs     │           │  shows      │
    │             │           │  SUPPRESSED) │           │  CLEAR!)    │
    └─────────────┘           └──────────────┘           └─────────────┘

                    DATA FLOW
                    ─────────
      Real alerts          Suppressed alerts
    SENSOR ════════► MITM ════════════════════► GATEWAY
    CRITICAL          intercepts,                CLEAR
    VIBRATION:42      suppresses,                VIBRATION:0
                      forwards

    ┌──────────────────────────────────────────────────────────┐
    │                                                          │
    │   THE SENSOR IS SCREAMING. THE GATEWAY HEARS SILENCE.    │
    │                                                          │
    │   Intercept → Suppress → Forward                         │
    │                                                          │
    │   The drill is running. The alarm is being eaten.        │
    │   How would you detect this?                             │
    │                                                          │
    └──────────────────────────────────────────────────────────┘
```

### Pin Connection Table (Phase 3.5)

| From | Wire | To | Purpose |
|------|------|----|---------|
| ESP32 #1 — GPIO 17 (TX2) | Yellow | ESP32 #3 — GPIO 4 (UART1 RX) | Real sensor data to attacker |
| ESP32 #3 — GPIO 17 (UART2 TX) | Orange | ESP32 #2 — GPIO 16 (RX2) | Suppressed data to gateway |
| ESP32 #1 — GND | Black | ESP32 #3 — GND | Common ground |
| ESP32 #3 — GND | Black | ESP32 #2 — GND | Common ground |

### Rewiring from Phase 3 → Phase 3.5

| Action | Wire | Notes |
|--------|------|-------|
| REMOVE | Orange wire (ESP32 #3 TX → ESP32 #2 RX) | Will re-route through MITM |
| ADD | Yellow wire: ESP32 #1 GPIO 17 (TX2) → ESP32 #3 GPIO 4 | Sensor feeds attacker |
| ADD | Orange wire: ESP32 #3 GPIO 17 (TX2) → ESP32 #2 GPIO 16 (RX2) | Attacker feeds gateway |
| KEEP | All GND connections between all three boards | Shared ground bus |

---

## Quick Reference: All GPIO Assignments

| Board | GPIO | Function | Used In |
|-------|------|----------|---------|
| ESP32 #1 (Vault Sensor) | GPIO 14 | Vibration sensor SIGNAL (INPUT_PULLUP) | All phases |
| ESP32 #1 (Vault Sensor) | GPIO 17 | UART2 TX (sends data) | All phases |
| ESP32 #2 (Security Gateway) | GPIO 16 | UART2 RX (receives data) | All phases |
| ESP32 #3 (Attacker) | GPIO 16 | UART2 RX (sniff/receive) | Phases 2, 3 |
| ESP32 #3 (Attacker) | GPIO 17 | UART2 TX (inject/forward) | Phases 3, 3.5 |
| ESP32 #3 (Attacker) | GPIO 4 | UART1 RX (receive from sensor) | Phase 3.5 only |
