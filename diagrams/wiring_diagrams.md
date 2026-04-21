# Wiring Diagrams: "Wire Tap" UART Lab

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

White board (bank) with vibration sensor, running standalone. No wiring between boards.

```
                    ┌─── USB to Laptop ───┐
                    │   (Serial Monitor    │
                    │    115200 baud)      │
                    └────────┬────────────┘
                             │
           ┌─────────────────┴──────────────┐
           │   WHITE BOARD (BANK)            │
           │   "VAULT SECURITY GATEWAY"      │
           │   Runs bank.ino         │
           │                                 │
           │  GPIO 14 ◄── Blue ── VIBRATION  │
           │                      SIGNAL     │
           │  3.3V ──── Red ────► VIBRATION  │
           │                      VCC        │
           │  GND ───── Black ──► VIBRATION  │
           │                      GND        │
           │                                 │
           │  GPIO 17 (TX2) ── transmitting  │
           │                   sensor data   │
           │                   + credentials │
           │  GPIO 16 (RX2) ── listening     │
           │                   (nothing      │
           │                    connected)   │
           └─────────────────────────────────┘

        BLUE BOARD (ATTACKER)
        Not connected yet. Pre-flashed with attacker.ino.
        Runs multi-mode firmware (SNIFF/INJECT/MITM via BOOT button).
```

### Pin Connection Table

| From | Wire | To | Purpose |
|------|------|------|------|
| White board, 3.3V | Red | Vibration VCC | Sensor power |
| White board, GND | Black | Vibration GND | Sensor ground |
| White board, GPIO 14 | Blue | Vibration SIGNAL | Vibration detection (INPUT_PULLUP) |

### Breadboard Layout Guide

```
    ┌─────────────────────────────────────────────────────────────┐
    │                        BREADBOARD                           │
    │                                                             │
    │   ┌──────────┐                                              │
    │   │  WHITE   │                                              │
    │   │  BOARD   │                                              │
    │   │  (BANK)  │                                              │
    │   │          ├┤GPIO 14                                      │
    │   │          ├┤3.3V                                         │
    │   │          ├┤GND                                          │
    │   └──────────┘│                                             │
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

Dashboard works standalone at http://firstnational.local (WiFi AP: VaultN-Security, password: firstnational). No blue board wiring needed.

---

## Diagram 2: "The Tap" (Phase 2)

Everything from Phase 1 stays in place. One additional wire taps the white board's TX line.
Students press the BOOT button on the blue board to select SNIFF mode (1 short blink every 3 seconds).

```
           ┌─────────────────────────────────┐
           │   WHITE BOARD (BANK)            │
           │   Runs bank.ino         │
           │                                 │
           │  GPIO 14 ◄── Blue ── VIBRATION  │
           │  3.3V ──── Red ────► VIBRATION  │
           │  GND ───── Black ──► VIBRATION  │
           │                                 │
           │  GPIO 17 (TX2) ══╦══════════════╪═══ transmitting sensor data
           │                  ║              │     + credentials at 9600 baud
           │  GND ══════╦═════╩══════════════╪═══
           │            ║                    │
           └────────────║────────────────────┘
                        ║
                ┌───────║─────────────────────────┐
                │       ║   BLUE BOARD (ATTACKER)  │
                │       ║   SNIFF mode (attacker.ino)       │
                │       ║                         │
                │       ╠══ Orange ══► GPIO 16    │
                │       ║             (RX2)       │
                │       ║                         │  ┌─── USB to Laptop ───┐
                │       ╚══ Black ═══► GND        │  │ (Serial Monitor:   │
                │                                 │──┤  watch vault admin  │
                │                                 │  │  credentials appear │
                └─────────────────────────────────┘  │  in plaintext!)     │
                                                     └────────────────────┘

    ┌──────────────────────────────────────────────────────────┐
    │                                                          │
    │   ONE ORANGE WIRE GIVES YOU EVERYTHING.                  │
    │                                                          │
    │   No tools. No exploits. No authentication to bypass.    │
    │   Just a wire and a serial monitor.                      │
    │                                                          │
    └──────────────────────────────────────────────────────────┘
```

### Additional Connections for Phase 2

| From | Wire | To | Purpose |
|------|------|------|------|
| White board, GPIO 17 (TX2) | Orange (tap) | Blue board, GPIO 16 (RX2) | Passive eavesdrop on TX line |
| White board, GND | Black | Blue board, GND | Common ground for sniffer |

**All Phase 1 wiring remains unchanged.**

---

## Diagram 3: "The Heist" (Phase 3)

The blue board injects fake CLEAR data into the white board's RX. The dashboard overrides its local sensor display and shows whatever the blue board sends, even while the real sensor detects vibration.

```
           ┌─────────────────────────────────┐
           │   WHITE BOARD (BANK)            │
           │   Runs bank.ino         │
           │                                 │
           │  GPIO 14 ◄── Blue ── VIBRATION  │
           │  3.3V ──── Red ────► VIBRATION  │
           │  GND ───── Black ──► VIBRATION  │
           │                                 │
           │  GPIO 17 (TX2) ── still sending │
           │     real data, but nobody is    │
           │     listening to it             │
           │                                 │
           │  GPIO 16 (RX2) ◄═══════════════╪═══ Orange ═══
           │                                 │             ║
           │  GND ══════════════════════════╪═══ Black ═══║═
           │                                 │             ║ ║
           └─────────────────────────────────┘             ║ ║
                                                           ║ ║
                ┌──────────────────────────────┐           ║ ║
                │  BLUE BOARD (ATTACKER)       │           ║ ║
                │  INJECT mode (attacker.ino)           │           ║ ║
                │                              │           ║ ║
                │  GPIO 17 (TX2) ══════════════╪═══════════╝ ║
                │                              │             ║
                │  GND ════════════════════════╪═════════════╝
                │                              │
                └──────────────────────────────┘
                        │
                 ┌──────┴──────┐
                 │ USB Laptop  │
                 │ (attacker   │
                 │  monitor:   │
                 │  "drill is  │
                 │   running") │
                 └─────────────┘

    ┌──────────────────────────────────────────────────────────┐
    │                                                          │
    │   THE VAULT IS BEING DRILLED.                            │
    │   THE DASHBOARD SHOWS "CLEAR."                           │
    │                                                          │
    │   The white board's dashboard overrides its local sensor │
    │   display whenever data arrives on RX. The sensor still  │
    │   detects vibration (check white board serial monitor),  │
    │   but the dashboard shows whatever the blue board sends. │
    │                                                          │
    │   After 3 seconds of no RX data, the dashboard falls    │
    │   back to showing local sensor readings.                 │
    │                                                          │
    └──────────────────────────────────────────────────────────┘
```

### Connection Changes from Phase 2 to Phase 3

| Action | Wire | Notes |
|------|------|------|
| REMOVE | Orange wire (white TX to blue RX) | Sniffer no longer needed |
| ADD | Orange wire: blue GPIO 17 (TX2) to white GPIO 16 (RX2) | Attacker injects data into dashboard |
| KEEP | Black wire: blue GND to white GND | Common ground |

---

## Diagram 4: "The Inside Job" (Phase 3.5, Stretch)

The blue board sits between the white board's TX and RX. Real sensor alerts flow into the blue board, get suppressed, and fake CLEAR flows out to the white board's RX. The dashboard sees normal-looking traffic. The vault is being drilled and nobody knows.

```
    ┌──────────────────┐     ┌──────────────────────┐
    │  WHITE BOARD     │     │  BLUE BOARD           │
    │  (BANK)          │     │  (ATTACKER)           │
    │                  │     │  MITM mode (attacker.ino)        │
    │  VIBRATION──►G14 │     │                       │
    │                  │     │                       │
    │  GPIO 17 (TX2) ══╪═══►│═ GPIO 4 (UART1 RX)   │
    │                  │     │                       │
    │  GPIO 16 (RX2) ◄═╪════╪═ GPIO 17 (UART2 TX)  │
    │                  │     │                       │
    │  GND ════════════╪════╪═ GND                  │
    │                  │     │                       │
    └──────────────────┘     └──────────────────────┘
           │                          │
    ┌──────┴──────┐           ┌───────┴──────┐
    │ USB Laptop  │           │ USB Laptop   │
    │ (white      │           │ (intercept   │
    │  serial:    │           │  log: see    │
    │  CRITICAL!) │           │  REAL vs     │
    │             │           │  SUPPRESSED) │
    └─────────────┘           └──────────────┘

                    DATA FLOW
                    ─────────
      Real alerts          Suppressed alerts
    WHITE TX ════════► BLUE ═════════════════► WHITE RX
    CRITICAL          intercepts,              CLEAR
    VIBRATION:42      suppresses,              VIBRATION:0
                      forwards

    ┌──────────────────────────────────────────────────────────┐
    │                                                          │
    │   THE SENSOR IS SCREAMING. THE DASHBOARD HEARS SILENCE.  │
    │                                                          │
    │   Intercept, Suppress, Forward                           │
    │                                                          │
    │   The drill is running. The alarm is being eaten.        │
    │   How would you detect it from the dashboard alone?      │
    │                                                          │
    └──────────────────────────────────────────────────────────┘
```

### Pin Connection Table (Phase 3.5)

| From | Wire | To | Purpose |
|------|------|------|------|
| White board, GPIO 17 (TX2) | Yellow | Blue board, GPIO 4 (UART1 RX) | Real sensor data to attacker |
| Blue board, GPIO 17 (UART2 TX) | Orange | White board, GPIO 16 (RX2) | Suppressed data to dashboard |
| White board, GND | Black | Blue board, GND | Common ground |

### Rewiring from Phase 3 to Phase 3.5

| Action | Wire | Notes |
|------|------|------|
| KEEP | Orange wire: blue GPIO 17 (TX2) to white GPIO 16 (RX2) | Attacker still feeds dashboard |
| ADD | Yellow wire: white GPIO 17 (TX2) to blue GPIO 4 | Sensor output now feeds attacker |
| KEEP | Black GND connection between both boards | Shared ground |

---

## Quick Reference: All GPIO Assignments

| Board | GPIO | Function | Used In |
|------|------|------|------|
| White (Bank) | GPIO 14 | Vibration sensor SIGNAL (INPUT_PULLUP) | All phases |
| White (Bank) | GPIO 17 | UART2 TX (sends sensor data + credentials) | All phases |
| White (Bank) | GPIO 16 | UART2 RX (receives data; overrides dashboard display) | Phases 3, 3.5 |
| Blue (Attacker) | GPIO 16 | UART2 RX (sniff/receive) | Phase 2 |
| Blue (Attacker) | GPIO 17 | UART2 TX (inject/forward) | Phases 3, 3.5 |
| Blue (Attacker) | GPIO 4 | UART1 RX (receive from white TX) | Phase 3.5 only |
