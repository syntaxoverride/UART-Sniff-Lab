# Wire Tap: UART Eavesdropping & Injection Lab

<p align="center">
  <a href="#"><img src="https://img.shields.io/badge/C++-00599C?style=flat&logo=cplusplus&logoColor=white" alt="C++"></a>
  <a href="#"><img src="https://img.shields.io/badge/Python-3776AB?style=flat&logo=python&logoColor=white" alt="Python"></a>
  <a href="#"><img src="https://img.shields.io/badge/ESP32-E7352C?style=flat&logo=espressif&logoColor=white" alt="ESP32"></a>
  <a href="#"><img src="https://img.shields.io/badge/Arduino-00878F?style=flat&logo=arduino&logoColor=white" alt="Arduino"></a>
  <a href="#"><img src="https://img.shields.io/badge/UART-Serial-orange?style=flat" alt="UART"></a>
  <a href="#"><img src="https://img.shields.io/badge/Hardware-Security-red?style=flat" alt="Hardware Security"></a>
  <a href="#"><img src="https://img.shields.io/badge/IoT-Security-blueviolet?style=flat" alt="IoT"></a>
  <a href="#"><img src="https://img.shields.io/badge/Embedded-Systems-green?style=flat" alt="Embedded Systems"></a>
  <a href="#"><img src="https://img.shields.io/badge/Logic%20Analyzer-black?style=flat" alt="Logic Analyzer"></a>
  <a href="#"><img src="https://img.shields.io/badge/License-Educational-blue?style=flat" alt="License"></a>
</p>

A hands-on hardware security lab where students exploit UART to sniff plaintext credentials, inject spoofed sensor data, and execute a man-in-the-middle attack on a simulated bank vault security system. Built for the undergraduate cybersecurity students.

## The Scenario

First National Bank uses a vibration sensor mounted on Vault #7 to detect drilling, impacts, or tampering. A single ESP32 board reads the sensor, runs a WiFi security dashboard, and transmits sensor data plus plaintext credentials over UART. The dashboard displays vault status in real time.

Students compromise the system in four escalating phases: sniffing credentials, injecting fake "all clear" readings that override the dashboard while the vault is drilled, and ultimately sitting invisibly between the bank board's TX and RX to suppress all alerts.

## Architecture

Two ESP32-WROOM boards per student station:

### White Board (Bank), Pre-flashed by Instructor
- Runs `bank.ino`, a single firmware that handles everything
- Reads vibration sensor on GPIO 14
- Serves a live WiFi security dashboard (`http://firstnational.local`) via captive DNS on the `VaultN-Security` AP
- Transmits sensor data + plaintext auth credentials over UART2 TX (GPIO 17)
- Listens on UART2 RX (GPIO 16). If incoming data contains `VAULT_STATUS:`, the dashboard **overrides** the local sensor display and shows whatever arrived on RX. Falls back to local sensor data after 3 seconds of no UART RX input.

<img width="540" height="774" alt="target board simulating a bank vault" src="https://github.com/user-attachments/assets/77fe1699-a963-4220-bf4f-45d9985738c8" />


### Blue Board (Attacker), Pre-flashed by Instructor
- Runs `attacker.ino`, a single multi-mode firmware pre-flashed by the instructor. Students never flash any boards.
- The BOOT button cycles through three modes: SNIFF, INJECT, MITM.
- LED patterns indicate the active mode:
  - 1 short blink every 3 seconds = SNIFF
  - 1 long blink every 3 seconds = INJECT
  - 3 short blinks every 3 seconds = MITM
- The serial monitor displays wiring instructions each time the mode changes.
<img width="328" height="303" alt="attack board" src="https://github.com/user-attachments/assets/bc633e13-4eed-4845-bd0c-0fa229fef657" />

- **Phase 2 (SNIFF, 1 short blink):** Wire blue RX (GPIO 16) to white TX (GPIO 17), shared GND. Students capture plaintext credentials on serial monitor.
- **Phase 3 (INJECT, 1 long blink):** Wire blue TX (GPIO 17) to white RX (GPIO 16), shared GND. Dashboard on the white board overrides to show fake CLEAR while the sensor still detects vibration.
- **Phase 3.5 (MITM, 3 short blinks):** Wire white TX (GPIO 17) to blue GPIO 4, blue TX (GPIO 17) to white RX (GPIO 16), shared GND. Blue board intercepts real alerts, suppresses them, and forwards CLEAR.

## Lab Phases

| Phase | Time | Activity | "Wait, What?" Moment |
|------|------|------|------|
| 0 | 10 min | Context setting: why serial protocols matter | Real-world UART exploitation examples |
| 1 | 20 min | Connect to white board WiFi, view dashboard, tap sensor | Students see the dashboard respond to vibration |
| 2 | 20 min | Tap the TX wire, one jumper wire + serial monitor | Plaintext vault admin credentials scroll by |
| 3 | 20 min | Inject fake CLEAR into white RX, dashboard overrides to green | Security monitor shows nothing wrong while sensor screams |
| 3.5 | 15 min | MITM: intercept, suppress, forward (stretch goal) | Real alerts eaten silently, dashboard stays green |
| 4 | 15 min | Instructor demo: I2C and SPI on Logic Analyzer (big screen) | Same trust problems, different protocols |
| 5 | 10 min | Debrief and takeaways | Defense discussion, reference cards |

## Hardware Requirements

### Per Student Station
| Qty | Item | Notes |
|------|------|------|
| 1 | ESP32-WROOM (white case) | Pre-flashed bank board (sensor + dashboard + UART) |
| 1 | ESP32-WROOM (blue case) | Pre-flashed attacker board (multi-mode, BOOT button cycles modes) |
| 1 | Vibration sensor module (3-pin) | VCC, SIGNAL, GND |
| 1 | Breadboard | Full or half size |
| 6+ | Male-to-male jumper wires | |
| 2 | USB cables (data, not charge-only) | |
| 1 | USB hub | Dual serial monitor support |

### Instructor Equipment
| Qty | Item | Notes |
|------|------|------|
| 1 | logic analyzer | UART waveform on projector + I2C/SPI demos |
| 1 | Projector / large display | |
| 1 | Extra ESP32 + BME280 | I2C/SPI demo boards |

## Software Requirements

- Python 3 with `pyserial` (for `flash_boards.py`, instructor use only)
- Arduino IDE 2.x with ESP32 board package (instructor use only, for compiling firmware)
- No external libraries required; vibration sensor uses `digitalRead` only
- logic analyzer software (instructor machine only)
- Students need only a serial monitor (Arduino Serial Monitor, PuTTY, or CoolTerm). Students never open Arduino IDE.

## Wiring

### Phase 1: The Honest System
```
White Board                Blue Board (not connected yet)
┌──────────┐
│ GPIO 14  │◄── Vibration Sensor SIGNAL
│ 3.3V     │──► Vibration Sensor VCC
│ GND      │──► Vibration Sensor GND
└──────────┘
```
Dashboard works standalone. No wiring between boards.

### Phase 2: The Tap
```
White GPIO 17 (TX) ──────► Blue GPIO 16 (RX)   [one wire = full access]
White GND ────────────────► Blue GND
```

### Phase 3: The Heist
```
Blue GPIO 17 (TX) ────────► White GPIO 16 (RX)  [injection overrides dashboard]
Blue GND ─────────────────► White GND
```
The white board's dashboard switches from showing local sensor data to showing whatever arrives on RX. Students can tap the sensor and verify on the white board's serial monitor that vibration is still detected, but the dashboard shows CLEAR.

### Phase 3.5: The Inside Job
```
White GPIO 17 (TX) ───────► Blue GPIO 4 (RX)    [real data to attacker]
Blue GPIO 17 (TX) ────────► White GPIO 16 (RX)  [fake data to dashboard]
All GND connected together
```
Blue board intercepts real sensor output, suppresses alerts, and forwards CLEAR to the white board's RX. The dashboard stays green while the sensor screams.

### Logic Analyzer (Instructor, Projector)
```
Analyzer CH0 ───────────────► White GPIO 17 (TX)  [tap the same TX wire]
Analyzer GND ───────────────► White GND
```
The logic analyzer is always passive; it observes, never interferes. Can share the TX wire with the blue board's RX using a splitter. Set capture mode to Trigger (continuous), add an Async Serial analyzer at 9600 baud, and the audience sees decoded UART traffic in real time on the projector.

## GPIO Pin Reference

| Board | GPIO | Function |
|------|------|------|
| White (Bank) | GPIO 14 | Vibration sensor SIGNAL (INPUT_PULLUP) |
| White (Bank) | GPIO 17 | UART2 TX: sends data (sniff target) |
| White (Bank) | GPIO 16 | UART2 RX: receives data (injection target, overrides dashboard) |
| Blue (Attacker) | GPIO 16 | UART2 RX: listens (sniff/MITM receive) |
| Blue (Attacker) | GPIO 17 | UART2 TX: sends (inject/MITM forward) |
| Blue (Attacker) | GPIO 4 | UART1 RX: receives from sensor (MITM only) |

## Batch Flashing

Pre-flash **all boards** (white and blue) before the lab using `flash_boards.py`:

```bash
python3 flash_boards.py
```

The script handles both board types:

- **White boards (bank):** Plug in a board, press Enter. Enter vault number (1-20). Each board gets a unique WiFi AP (`Vault1-Security`, `Vault2-Security`, etc.). All boards share WiFi password: `firstnational`. Dashboard URL: `http://firstnational.local` (captive DNS, any URL works).
- **Blue boards (attacker):** Plug in a board, press Enter. The script flashes `attacker.ino`, the multi-mode firmware. No vault number needed.

Students never flash any boards. Both board types arrive at the lab ready to use.

## Logic Analyzer Settings

For capturing UART traffic on the projector:

| Setting | Value |
|------|------|
| Sampling rate | 1 MS/s or higher (digital) |
| Capture mode | Trigger (continuous), not timed |
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
├── flash_boards.py                         # Batch flashing script for both board types
├── firmware/
│   ├── bank/bank.ino       # Bank board (white case): sensor + dashboard + UART
│   └── attacker/attacker.ino               # Attacker board (blue case): multi-mode (SNIFF/INJECT/MITM)
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

The lab simulates attacks that map directly to real-world embedded system compromises:

- **ICS/SCADA:** Sensors in power plants and water treatment facilities use unencrypted serial protocols (Modbus RTU, RS-485). Stuxnet manipulated sensor feedback so operators saw normal readings while centrifuges were destroyed.
- **Building Security:** Alarm panels and vault sensors often use serial buses between components. Physical access to the wiring between sensor and panel allows injection.
- **Automotive:** CAN bus operates on the same trust model. ECUs broadcast messages with no authentication.
- **Medical Devices:** Researchers have demonstrated intercepting and modifying dosage data on insulin pumps via unencrypted serial links.

## License

Educational use. Built for the undergraduate cybersecurity students.
