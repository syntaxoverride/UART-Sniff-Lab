# PRD: "Wire Tap" — UART Eavesdropping & Injection Lab

## 1. Overview

A hands-on cybersecurity lab where students exploit UART (Universal Asynchronous Receiver-Transmitter) to sniff plaintext credentials and inject spoofed sensor data between two ESP32-WROOM boards. The scenario: a bank vault's vibration detection system is compromised, allowing a thief to drill into the vault while the security gateway reports "all clear."

Two additional protocols (I2C and SPI) are demonstrated on a big screen by the instructor using a Saleae Logic 16 analyzer to provide comparative context.

---

## 2. Learning Objectives

By the end of this lab, students will be able to:

1. Explain what UART is, how TX/RX lines carry data, and why no authentication or encryption exists at the protocol level.
2. Physically tap a UART bus using a spare microcontroller and read plaintext traffic.
3. Inject forged data onto a UART line by replacing a legitimate transmitter.
4. Articulate the real-world implications: firmware extraction, credential theft, and sensor spoofing on commercial hardware.
5. Identify UART headers on unfamiliar PCBs and describe the attacker workflow.

---

## 3. Target Audience

- Undergraduate cybersecurity club members at TAMU-CC
- Mixed experience levels: some comfortable with Arduino, others first-time flashers
- No prior embedded/hardware security experience assumed

---

## 4. Hardware Bill of Materials (Per Student)

| Qty | Item | Role |
|-----|------|------|
| 2 | ESP32-WROOM dev board (e.g., ESP32-DevKitC) | "Victim gateway" + "Attacker/sniffer" |
| 1 | Vibration sensor module (3-pin, e.g., SW-420) | Vault vibration detector |
| 1 | Breadboard (full or half size) | Wiring platform |
| 10+ | Male-to-male jumper wires | Connections |
| 2 | USB-A to Micro-USB cables | Power/programming |
| 1 | USB hub | Dual serial monitor support |
| 1 | Laptop with Arduino IDE or PlatformIO installed | Development environment |

### Shared / Instructor Equipment

| Qty | Item | Role |
|-----|------|------|
| 1 | Saleae Logic 16 analyzer | Instructor demo of all three protocols on projector |
| 1 | Projector / large display | Live Saleae captures, wiring diagram reference |
| 1+ | Extra ESP32 + BME280 (I2C/SPI capable sensor) | Instructor demo boards for I2C and SPI segments |

---

## 5. Software Requirements

| Component | Tool |
|-----------|------|
| IDE | Arduino IDE 2.x (or PlatformIO) |
| Board package | ESP32 by Espressif (Arduino Board Manager) |
| Libraries | None required — vibration sensor uses digitalRead only |
| Saleae software | Logic 2 (instructor machine only) |
| Serial monitor | Arduino Serial Monitor or PuTTY / CoolTerm |

---

## 6. Lab Structure

### Phase 0 — Context Setting (10 min, instructor-led)

**Goal:** Frame *why* serial protocols matter in security.

**Deliverables required:**

- [ ] **Slide deck or talking points** covering:
  - Real-world UART exploitation examples (router root shells, medical device firmware dumps, automotive ECU access)
  - Photo examples of UART headers on commercial PCBs (labeled and unlabeled)
  - Brief comparison table: UART vs. I2C vs. SPI (speed, wiring, addressing, security)
- [ ] **Visual aid: protocol comparison diagram** — a single reference image showing all three protocols side by side with their signal lines, topology, and key characteristics

---

### Phase 1 — "The Honest System" (20 min, hands-on)

**Goal:** Students build a working UART vault sensor system. Nothing malicious yet — just get the thing working.

**Scenario:** First National Bank has a vibration sensor mounted on Vault #7. The sensor detects drilling, impacts, or tampering and reports status to a security gateway over a UART serial link.

**Setup:**
- **ESP32 #1 ("Vault Sensor")** reads vibration data and transmits status over UART2 to ESP32 #2.
- **ESP32 #2 ("Security Gateway")** receives UART data and prints alerts to USB serial monitor. Simulates the bank's monitoring station.

**Deliverables required:**

- [ ] **Wiring diagram #1: "The Honest System"** — clear diagram showing:
  - ESP32 #1 pin connections to vibration sensor (power, ground, signal on GPIO 14)
  - ESP32 #1 TX pin (GPIO 17) to ESP32 #2 RX pin (GPIO 16)
  - Common ground between both boards
  - USB connections to laptop
  - Pin labels with GPIO numbers
  - Color-coded wires (red = power, black = ground, yellow = data/signal)
- [ ] **Firmware: `sensor_node.ino`** — ESP32 #1 sketch that:
  - Reads vibration sensor on GPIO 14 using INPUT_PULLUP
  - Counts vibration pulses over 500ms windows
  - Reports threat level: CLEAR, LOW, MEDIUM, CRITICAL
  - Transmits over UART2 (not USB serial)
  - Also prints to USB serial for student debugging
- [ ] **Firmware: `gateway.ino`** — ESP32 #2 sketch that:
  - Receives UART data
  - Parses status level and formats alerts accordingly
  - Displays with severity formatting (ALERT/WARN/SECURE prefixes)
- [ ] **Student worksheet / checklist** — step-by-step instructions:
  1. Wire the circuit per diagram
  2. Flash gateway.ino to ESP32 #2 (ESP32 #1 is pre-flashed)
  3. Open serial monitor on ESP32 #2 and confirm status readings appear
  4. Tap the vibration sensor to trigger CRITICAL alerts
  5. Checkpoint: raise hand when readings are flowing

---

### Phase 2 — "The Tap" (20 min, hands-on)

**Goal:** Students eavesdrop on the UART line using their attacker ESP32. Vault admin credentials appear.

**The twist:** The pre-flashed ESP32 #1 firmware (sensor_node_v2) also transmits a simulated authentication handshake periodically:

```
[VAULT-SENSOR] Authenticating to security gateway...
[VAULT-SENSOR] DEVICE_ID: VAULT-7-VIBRATION-01
[VAULT-SENSOR] USER: vault_admin
[VAULT-SENSOR] PASS: F1r$tN@t10nal#2026!
[VAULT-SENSOR] AUTH_TOKEN: eyJhbGciOiJIUzI1NiJ9.dmF1bHRfYWRtaW4.v4u1t_s3cr3t
[VAULT-SENSOR] Session established. Monitoring vault...
```

**Setup:**
- Students disconnect nothing. The "honest system" keeps running.
- Students configure their second ESP32 as a passive sniffer:
  - Connect the sniffer's RX pin to the same TX line from ESP32 #1
  - Share ground
  - Run sniffer firmware that dumps raw bytes to USB serial

**Deliverables required:**

- [ ] **Wiring diagram #2: "The Tap"** — shows the original honest system PLUS:
  - Third wire from ESP32 #1 TX to sniffer ESP32 RX
  - Sniffer ESP32 connected via USB to laptop
  - Visual emphasis on how trivial the physical tap is (one wire)
  - Annotation: "This single wire gives you everything"
- [ ] **Firmware: `sensor_node_v2.ino`** — updated ESP32 #1 sketch that:
  - Still reads and transmits real vibration data
  - Every 15 seconds, transmits a simulated authentication handshake with plaintext vault admin credentials
  - Credentials should look realistic enough to produce the "oh shit" moment
- [ ] **Firmware: `sniffer.ino`** — sniffer ESP32 sketch that:
  - Listens on UART RX
  - Dumps all received bytes to USB serial with timestamps
  - Highlights lines containing keywords like "PASS", "TOKEN", "AUTH", "USER"
  - Code should be short (~20 lines of meaningful logic) so students see there's no magic
- [ ] **Student worksheet / instructions:**
  1. Wire the sniffer per diagram #2
  2. Flash sniffer.ino to sniffer ESP32
  3. Open serial monitor on sniffer
  4. Wait for the authentication sequence
  5. Record the captured credentials on the worksheet
  6. Discussion prompt: "What would you need to do this to a real device?"

---

### Phase 3 — "The Heist" (20 min, hands-on)

**Goal:** Students replace the legitimate vault sensor with their attacker ESP32 and send fake "all clear" data to the security gateway while the vault is being drilled.

**Setup:**
- Disconnect ESP32 #1 TX from the gateway's RX
- Connect attacker ESP32 TX to the gateway's RX
- Run injection firmware that sends fake CLEAR status with zero vibration

**Deliverables required:**

- [ ] **Wiring diagram #3: "The Heist"** — shows:
  - ESP32 #1 TX wire disconnected (visually "cut" or removed)
  - Attacker ESP32 TX connected to gateway RX in its place
  - Annotation: "The gateway has no way to verify who is talking to it"
- [ ] **Firmware: `injector.ino`** — attacker ESP32 sketch that:
  - Sends constant CLEAR status with VIBRATION:0
  - Sends spoofed authentication using stolen credentials
  - Prints to USB serial what it's actually doing ("drill is running")
  - Students can modify fake values and re-flash
- [ ] **Student worksheet / instructions:**
  1. Disconnect ESP32 #1 TX wire from gateway
  2. Connect attacker ESP32 TX to gateway RX
  3. Flash injector.ino to attacker ESP32
  4. Observe gateway serial monitor — fake "all clear" accepted
  5. Tap the real sensor — it's screaming CRITICAL but nobody hears it
  6. Discussion prompt: "The vault is being drilled open. The security monitor shows CLEAR. How long until someone notices?"

---

### Phase 3.5 (Stretch Goal) — "The Inside Job" (15 min, hands-on)

**Goal:** The attacker ESP32 sits *between* the vault sensor and the gateway — receiving real alerts, suppressing them, and forwarding fake "all clear." The gateway never knows the difference. Third and biggest "oh shit."

**Prerequisites:** Only attempt if Phases 1-3 completed with time to spare. Instructor makes the call.

**Setup:**
- ESP32 #1 ("Vault Sensor") TX connects to attacker ESP32 RX (attacker receives real data)
- Attacker ESP32 TX connects to ESP32 #2 ("Gateway") RX (attacker forwards to gateway)
- Common ground across all three boards
- Attacker ESP32 runs MITM firmware: reads real sensor data, suppresses alerts, retransmits CLEAR

**Deliverables required:**

- [ ] **Wiring diagram #4: "The Inside Job"** — shows:
  - ESP32 #1 TX → Attacker RX (UART1 RX, GPIO 4)
  - Attacker TX (UART2 TX, GPIO 17) → Gateway RX
  - Attacker ESP32 USB connected to laptop for monitoring the intercept
  - All three boards on breadboard with shared ground
  - Annotation: "Real alerts in, fake CLEAR out — the gateway trusts whatever arrives on its RX line"
  - Annotation on attacker board: "Intercept → Suppress → Forward"
- [ ] **Firmware: `mitm.ino`** — attacker ESP32 sketch that:
  - Receives UART data from the real vault sensor on UART1 (GPIO 4 RX)
  - Parses vault status from incoming string
  - Replaces CRITICAL/MEDIUM with CLEAR and zeroes vibration count
  - Retransmits the suppressed data to the gateway on UART2 (GPIO 17 TX)
  - Prints both real and suppressed values to USB serial
  - Logs intercepted credentials without modifying them
- [ ] **Student worksheet / instructions:**
  1. Disconnect the direct TX→RX wire between sensor and gateway
  2. Wire the attacker ESP32 in the middle per diagram #4
  3. Flash mitm.ino to attacker ESP32
  4. Open three serial monitors: sensor (debug), attacker (intercept log), gateway (received data)
  5. Tap the vibration sensor — sensor shows CRITICAL, attacker shows suppression, gateway shows CLEAR
  6. Discussion prompt: "The gateway received valid-looking data from a valid-looking source. How would you detect this attack?"

**Why this is the biggest "oh shit":**
- Phases 2 and 3 are passive sniffing and crude replacement. The gateway at least *stops getting real data* during injection.
- MITM is invisible. Real data still flows. The gateway still works. The alerts just... never arrive.
- The real-world parallel is immediate: the vault is being drilled, the sensor is screaming, and the security monitor shows nothing but green.

---

### Phase 4 — Instructor Demo: I2C and SPI on the Big Screen (15 min)

**Goal:** Show students that UART isn't unique — other embedded protocols have similar trust problems.

**Deliverables required:**

- [ ] **I2C demo setup** — two ESP32s on an I2C bus with a BME280:
  - Saleae capturing SDA/SCL
  - Show a clean transaction on screen, walk through start bit, address, ACK, data, stop
  - Show what happens when a second device responds to the same address (bus collision)
  - Live Saleae capture projected
- [ ] **SPI demo setup** — ESP32 talking to an SPI device:
  - Saleae capturing CLK, MOSI, MISO, CS
  - Show that pulling CS low is the only "authentication"
  - Show a flash chip read and ask: "What if this chip holds firmware?"
  - Live Saleae capture projected
- [ ] **Visual aid: "Attack Surface Map"** — a diagram showing a hypothetical IoT device PCB with labeled UART, I2C, SPI buses and annotations pointing to what an attacker targets on each
- [ ] **Talking points document** — key points to hit for each demo, including:
  - What makes each protocol similar to UART (no auth, no encryption)
  - What makes each protocol different (addressing, shared bus, chip select)
  - Real-world exploitation examples for each

---

### Phase 5 — Debrief & Takeaways (10 min)

**Deliverables required:**

- [ ] **Debrief discussion guide** with prompts:
  - "How would you defend against what you just did?"
  - "Why don't these protocols have encryption built in?"
  - "What's the first thing you'd look for on an unfamiliar PCB?"
  - Introduce concepts: encrypted serial, hardware security modules, secure boot, physical tamper detection
- [ ] **Take-home reference card** (single page, printable) containing:
  - Protocol comparison table (UART/I2C/SPI — pins, speed, topology, security)
  - Common UART baud rates
  - Pin identification tips for unknown PCBs
  - Recommended tools and further reading (Saleae, Bus Pirate, JTAGulator, flashrom)

---

## 7. Deliverables — Full Inventory

### Diagrams & Visuals

| ID | Deliverable | Format | Description |
|----|------------|--------|-------------|
| D1 | Protocol Comparison Diagram | PNG/SVG image or printable PDF | Side-by-side visual: UART, I2C, SPI signal lines, topology, characteristics |
| D2 | Wiring Diagram #1: Honest System | PNG/SVG image or printable PDF | ESP32 #1 + vibration sensor + ESP32 #2 gateway, pin labels, color-coded wires |
| D3 | Wiring Diagram #2: The Tap | PNG/SVG image or printable PDF | Honest system + sniffer ESP32 tap wire, annotation callout |
| D4 | Wiring Diagram #3: The Heist | PNG/SVG image or printable PDF | Gateway + attacker ESP32 replacing sensor, disconnected wire shown |
| D5 | Wiring Diagram #4: The Inside Job | PNG/SVG image or printable PDF | Sensor → Attacker → Gateway chain, three boards, intercept annotations |
| D6 | IoT Attack Surface Map | PNG/SVG image or printable PDF | Hypothetical PCB with UART/I2C/SPI attack points labeled |

### Firmware

| ID | Deliverable | Language | Description |
|----|------------|----------|-------------|
| F1 | `sensor_node.ino` | Arduino C++ | Phase 1 — reads vibration sensor, transmits vault status over UART |
| F2 | `gateway.ino` | Arduino C++ | Phase 1 — receives UART data, displays formatted security alerts |
| F3 | `sensor_node_v2.ino` | Arduino C++ | Phase 2 — adds simulated auth handshake with vault admin credentials |
| F4 | `sniffer.ino` | Arduino C++ | Phase 2 — passive UART listener, highlights captured credentials |
| F5 | `injector.ino` | Arduino C++ | Phase 3 — sends fake CLEAR status while vault is drilled |
| F6 | `mitm.ino` | Arduino C++ | Phase 3.5 (stretch) — intercepts real alerts, suppresses them, forwards CLEAR |

### Documents

| ID | Deliverable | Format | Description |
|----|------------|--------|-------------|
| W1 | Student Worksheet | PDF/printed handout | Step-by-step for all phases, checkpoints, discussion prompts |
| W2 | Instructor Talking Points | Markdown/PDF | Phase 0 framing + Phase 4 demo narration + Phase 5 debrief guide |
| W3 | Take-Home Reference Card | Single-page PDF | Protocol comparison, baud rates, pin ID tips, tool list |

---

## 8. Wiring Diagram Specifications

All wiring diagrams must:

- Use actual ESP32-WROOM DevKitC pinout with GPIO numbers labeled
- Use consistent color coding across all diagrams:
  - **Red** = 3.3V power
  - **Black** = Ground (GND)
  - **Yellow** = UART TX lines
  - **Green** = UART RX lines
  - **Blue** = Sensor signal line (vibration sensor)
  - **Orange** = Tap/attack wires (dashed in print)
- Show breadboard layout (not just schematic — students need to see where wires physically go)
- Include a small legend/key on each diagram
- Mark which USB port connects to which serial monitor instance
- Be printable in grayscale (use line styles — dashed for attack wires — in addition to color)

---

## 9. Firmware Specifications

All firmware must:

- Compile under Arduino IDE 2.x with ESP32 board package
- Use only `HardwareSerial` (no SoftwareSerial) for UART communication
- Use **UART2** (GPIO 16 RX, GPIO 17 TX) for board-to-board communication, reserving UART0 (USB) for debug/monitor output
- Target baud rate: **9600** for board-to-board UART (slow enough to be readable in real-time on a serial monitor, and to allow the ESP32 sniffer to keep up without drops)
- Include clear comments explaining each section (students may read the code)
- Print a startup banner on USB serial identifying the firmware name and version
- Avoid blocking delays longer than 2 seconds (keep the serial monitor active and responsive)
- Require no external library dependencies — vibration sensor uses digitalRead only
- Vibration sensor pin: **GPIO 14** with **INPUT_PULLUP**

---

## 10. Constraints & Assumptions

- **Time budget:** ~95 minutes total (10 + 20 + 20 + 20 + 15 + 10, with buffer). Stretch MITM phase adds ~15 min if time allows.
- **No soldering** — everything is breadboard and jumper wire
- **Students may not have Saleae units** — all hands-on phases use ESP32-based sniffing only
- **USB hubs available** — students have USB hubs to run two serial monitors simultaneously
- **ESP32 #1 boards pre-flashed by instructor** — sensor_node_v2.ino is loaded before the lab begins. Students only flash the sniffer/attacker ESP32 themselves. This saves ~15 minutes of setup and eliminates "my board won't flash" troubleshooting.
- **Vibration sensor:** 3-pin breakout module (VCC, SIGNAL, GND). No external pull-up needed — firmware uses INPUT_PULLUP. Sensor triggers HIGH on vibration.

---

## 11. Success Criteria

The lab is successful if:

1. Every student captures plaintext vault admin credentials from a UART line they physically tapped
2. Every student sends at least one spoofed "all clear" value that the security gateway accepts
3. Students can articulate *why* UART has no built-in security (it predates the need — designed for trusted, internal board-to-board communication)
4. At least one student says "oh shit" (or equivalent) out loud during Phase 2

---

## 12. Resolved Decisions

| # | Question | Decision |
|---|----------|----------|
| 1 | Sensor type | **Vibration sensor (3-pin module)** on GPIO 14 with INPUT_PULLUP. Triggers HIGH on vibration. |
| 2 | Pre-flash vs. live flash | **Pre-flash ESP32 #1** — instructor loads sensor_node_v2.ino before the lab. Students only flash the attacker/sniffer board. |
| 3 | USB hub availability | **Confirmed** — students have USB hubs for dual serial monitors. |
| 4 | MITM stretch goal | **Yes** — included as Phase 3.5. Instructor decides at runtime whether time permits. |
| 5 | Library dependencies | **None** — vibration sensor uses digitalRead, no external libraries needed. |

## 13. Remaining Open Questions

All resolved. Summary of pin decisions:

- **UART2:** GPIO 16 (RX) / GPIO 17 (TX) — confirmed available on boards in inventory
- **Vibration sensor:** GPIO 14, INPUT_PULLUP, triggers HIGH on vibration
- **MITM UART allocation:** UART1 remapped to GPIO 4 (RX from sensor), UART2 TX on GPIO 17 (forward to gateway). UART0 reserved for USB debug. No conflicts with flash pins.
