# Wire Tap — UART Eavesdropping & Injection Lab

A hands-on hardware security lab where students exploit UART to sniff plaintext credentials, inject spoofed sensor data, and execute a man-in-the-middle attack on a simulated bank vault security system. Built for the Islanders Cybersecurity Club at Texas A&M University - Corpus Christi.

## The Scenario

First National Bank uses a vibration sensor mounted on Vault #7 to detect drilling, impacts, or tampering. The sensor reports status to a security monitoring gateway over a UART serial link. A WiFi dashboard displays the vault status in real time.

Students compromise this system in four escalating phases — sniffing credentials, injecting fake "all clear" readings while the vault is drilled, and ultimately sitting invisibly between the sensor and the gateway to suppress all alerts.

## Architecture

Two ESP32-WROOM boards per student station:

### White Board (Bank) — Pre-flashed by Instructor
- Reads vibration sensor on GPIO 14
- Serves a live WiFi security dashboard (`http://firstnational.local`)
- Transmits sensor data + plaintext auth credentials over UART2 TX (GPIO 17)
- Accepts incoming data on UART2 RX (GPIO 16) — if external data arrives, it **overrides** the local sensor display, showing whatever the attacker sends on the dashboard

### Blue Board (Attacker) — Flashed by Students
- **Phase 2 (Sniff):** `sniffer.ino` — Taps the white board's TX wire, captures plaintext credentials
- **Phase 3 (Inject):** `injector.ino` — Sends fake CLEAR status to the white board's RX, dashboard shows green while sensor screams
- **Phase 3.5 (MITM):** `mitm.ino` — Sits between sensor output and gateway input, intercepts real alerts, suppresses them, forwards fake CLEAR

## Lab Phases

| Phase | Time | Activity | "Oh Shit" Moment |
|-------|------|----------|------------------|
| 0 | 10 min | Context setting — why serial protocols matter | Real-world UART exploitation examples |
| 1 | 20 min | Build the honest system — sensor + dashboard working | Students see the dashboard respond to vibration |
| 2 | 20 min | Tap the TX wire — one jumper wire + serial monitor | Plaintext vault admin credentials scroll by |
| 3 | 20 min | Inject fake CLEAR — dashboard goes green while drilling | Security monitor shows nothing wrong |
| 3.5 | 15 min | MITM — intercept, suppress, forward (stretch goal) | Real alerts eaten silently, dashboard stays green |
| 4 | 15 min | Instructor demo: I2C and SPI on Saleae (big screen) | Same trust problems, different protocols |
| 5 | 10 min | Debrief and takeaways | Defense discussion, reference cards |

## Hardware Requirements

### Per Student Station
| Qty | Item | Notes |
|-----|------|-------|
| 1 | ESP32-WROOM (white case) | Pre-flashed bank board |
| 1 | ESP32-WROOM (blue case) | Blank attacker board |
| 1 | Vibration sensor module (3-pin) | VCC, SIGNAL, GND |
| 1 | Breadboard | Full or half size |
| 6+ | Male-to-male jumper wires | |
| 2 | USB cables (data, not charge-only) | |
| 1 | USB hub | Dual serial monitor support |

### Instructor Equipment
| Qty | Item | Notes |
|-----|------|-------|
| 1 | Saleae Logic 16 | UART waveform on projector + I2C/SPI demos |
| 1 | Projector / large display | |
| 1 | Extra ESP32 + BME280 | I2C/SPI demo boards |

## Software Requirements

- Arduino IDE 2.x with ESP32 board package (`esp32 by Espressif Systems`)
- No external libraries required — vibration sensor uses `digitalRead` only
- Python 3 with `pyserial` (for `flash_boards.py`)
- Saleae Logic 2 (instructor machine only)

## Wiring

### Phase 1 — The Honest System
```
White Board                Blue Board (not connected yet)
┌──────────┐
│ GPIO 14  │◄── Vibration Sensor SIGNAL
│ 3.3V     │──► Vibration Sensor VCC
│ GND      │──► Vibration Sensor GND
└──────────┘
```
Dashboard works standalone. No wiring between boards.

### Phase 2 — The Tap
```
White GPIO 17 (TX) ──────► Blue GPIO 16 (RX)   [one wire = full access]
White GND ────────────────► Blue GND
```

### Phase 3 — The Heist
```
Blue GPIO 17 (TX) ────────► White GPIO 16 (RX)  [injection]
Blue GND ─────────────────► White GND
```

### Phase 3.5 — The Inside Job
```
White GPIO 17 (TX) ───────► Blue GPIO 4 (RX)    [real data to attacker]
Blue GPIO 17 (TX) ────────► White GPIO 16 (RX)  [fake data to dashboard]
All GND connected together
```

### Saleae Logic Analyzer
```
Saleae CH0 ───────────────► White GPIO 17 (TX)  [tap the same TX wire]
Saleae GND ───────────────► White GND
```
Saleae is always passive — it observes, never interferes. Can share the TX wire with the blue board's RX using a splitter.

## GPIO Pin Reference

| Board | GPIO | Function |
|-------|------|----------|
| White (Bank) | GPIO 14 | Vibration sensor SIGNAL (INPUT_PULLUP) |
| White (Bank) | GPIO 17 | UART2 TX — sends data (sniff target) |
| White (Bank) | GPIO 16 | UART2 RX — receives data (injection target) |
| Blue (Attacker) | GPIO 16 | UART2 RX — listens (sniff/MITM receive) |
| Blue (Attacker) | GPIO 17 | UART2 TX — sends (inject/MITM forward) |
| Blue (Attacker) | GPIO 4 | UART1 RX — receives from sensor (MITM only) |

## Batch Flashing

Pre-flash all white (bank) boards before the lab:

```bash
python3 flash_boards.py
```

- Plug in a board, press Enter
- Enter vault number (1–20) — each board gets a unique WiFi AP (`Vault1-Security`, `Vault2-Security`, etc.)
- Script compiles with the vault number, flashes, and verifies
- All boards share WiFi password: `firstnational`
- Dashboard URL: `http://firstnational.local` (captive DNS — any URL works)

Blue (attacker) boards ship blank — students flash them during the lab.

## Saleae Logic Analyzer Settings

For capturing UART traffic on the projector:

| Setting | Value |
|---------|-------|
| Sampling rate | 1 MS/s or higher (digital) |
| Capture duration | 20+ seconds |
| Analyzer | Async Serial |
| Channel | Whichever is wired to GPIO 17 |
| Bit rate | 9600 |
| Data bits | 8 |
| Stop bits | 1 |
| Parity | None |
| Bit order | LSB first |

## Project Structure

```
UART-Sniff-Lab/
├── README.md
├── PRD.md                                  # Product requirements document
├── flash_boards.py                         # Batch flashing script for bank boards
├── firmware/
│   ├── gateway_test/gateway_test.ino       # Bank board — sensor + dashboard + UART
│   ├── sniffer/sniffer.ino                 # Phase 2 — passive UART listener
│   ├── injector/injector.ino               # Phase 3 — fake CLEAR injection
│   ├── mitm/mitm.ino                       # Phase 3.5 — intercept and suppress
│   ├── calibrate/calibrate.ino             # Utility — raw sensor pulse counter
│   ├── gateway/gateway.ino                 # Standalone UART gateway (reference)
│   ├── sensor_node/sensor_node.ino         # Standalone sensor (reference)
│   └── sensor_node_v2/sensor_node_v2.ino   # Standalone sensor + auth (reference)
├── diagrams/
│   ├── wiring_diagrams.md                  # All phase wiring with ASCII diagrams
│   └── protocol_comparison.md              # UART vs I2C vs SPI + attack surface map
└── docs/
    ├── student_worksheet.md                # Step-by-step handout with checkboxes
    ├── instructor_guide.md                 # Talking points, pacing, troubleshooting
    └── reference_card.md                   # Take-home: protocol table, tools, tips
```

## Credentials (Simulated)

The following credentials are transmitted in plaintext over UART for educational purposes:

```
DEVICE_ID:  VAULT-N-VIBRATION-01
USER:       vault_admin
PASS:       F1r$tN@t10nal#2026!
AUTH_TOKEN:  eyJhbGciOiJIUzI1NiJ9.dmF1bHRfYWRtaW4.v4u1t_s3cr3t
```

These are not real credentials. They exist solely to demonstrate why plaintext serial communication is a security risk.

## Real-World Relevance

This lab simulates attacks that map directly to real-world embedded system compromises:

- **ICS/SCADA:** Sensors in power plants and water treatment facilities communicate over unencrypted serial protocols (Modbus RTU, RS-485). Stuxnet manipulated sensor feedback so operators saw normal readings while centrifuges were destroyed.
- **Building Security:** Alarm panels and vault sensors often use serial buses between components. Physical access to the wiring between sensor and panel allows injection.
- **Automotive:** CAN bus operates on the same trust model — ECUs broadcast messages with no authentication.
- **Medical Devices:** Researchers have demonstrated intercepting and modifying dosage data on insulin pumps via unencrypted serial links.

## License

Educational use. Built for the Islanders Cybersecurity Club at Texas A&M University - Corpus Christi.
