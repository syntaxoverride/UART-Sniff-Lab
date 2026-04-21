# Wire Tap — Instructor Guide

## Pre-Lab Preparation

### Boards to Pre-Flash

Flash **every** ESP32 #1 ("Vault Sensor") board with `sensor_node_v2.ino` before the lab.
Students never touch ESP32 #1 firmware — they only flash their attacker/sniffer board.

**Flashing checklist:**
- [ ] Install Arduino IDE 2.x with ESP32 board package
- [ ] No external libraries needed (vibration sensor uses digitalRead)
- [ ] Open `sensor_node_v2.ino`
- [ ] Board: ESP32 Dev Module
- [ ] For each ESP32 #1: plug in, select port, upload, verify startup banner appears on serial monitor
- [ ] Label each pre-flashed board (tape + marker: "VAULT SENSOR — DO NOT REFLASH")
- [ ] Wire a vibration sensor to each pre-flashed board (3.3V, GND, SIGNAL → GPIO 14)
- [ ] Test: tap the sensor, confirm CRITICAL appears on serial monitor

### Student Stations

Each station needs:
- 1x Pre-flashed ESP32 #1 + vibration sensor (already wired or loose for students to wire)
- 1x Blank ESP32 (student's attacker board)
- Breadboard, jumper wires (at least 6 male-to-male)
- USB hub + 2 USB cables
- Printed student worksheet
- Printed reference card

### USB Drive or Shared Folder

Distribute firmware files to students. Options:
- Shared network folder
- USB drive passed around
- GitHub repo link
- Pre-loaded on lab machines

Students need: `gateway.ino`, `sniffer.ino`, `injector.ino`, `mitm.ino`
They do NOT need `sensor_node.ino` or `sensor_node_v2.ino` (pre-flashed).

### Batch Flashing with flash_boards.py

Use the included `flash_boards.py` script to pre-flash all boards:

```
python3 flash_boards.py
```

- Plug in a board, press Enter
- Select board type: Gateway (1) or Sensor (2)
- For gateways: enter vault number (1-20) — each gets a unique WiFi AP (Vault1-Security, Vault2-Security, etc.)
- Script compiles, flashes, and verifies each board
- Label each board after flashing (script tells you what to write)
- All gateways share the same WiFi password: `firstnational`

### WiFi Dashboard

Each gateway ESP32 creates a WiFi access point and serves a live security dashboard:

- **SSID:** VaultN-Security (N = station number)
- **Password:** firstnational
- **URL:** http://firstnational.local (captive DNS — any URL works too)
- **Features:** live status with color-coded alerts, vibration level, event log, CLEAR LOG button
- **Audience:** observers can connect on their phones to watch the attack unfold in real time

The dashboard is the key visual for the "oh shit" moment — everyone in the room sees the status go from green CLEAR to red CRITICAL when vibration is detected, then back to green CLEAR when the attacker injects fake data.

### Saleae Setup (Instructor Demo Station)

For Phase 4 demos:
- Saleae Logic 16 connected to demo machine with projector
- Logic 2 software installed
- Demo ESP32 + BME280 wired for I2C demo
- Demo ESP32 + SPI device (flash chip or SD card module) for SPI demo
- Channels pre-labeled in Logic 2 (SDA, SCL, MOSI, MISO, CLK, CS)

---

## Phase 0 — Context Setting (10 min)

### Key Points to Hit

**The scenario:**

"First National Bank has a vault protected by a vibration sensor. If someone drills into the vault, the sensor detects the vibration and alerts the security monitoring station over a serial UART connection. Simple, reliable, proven technology. Today you're going to break it."

**Why serial protocols matter in security:**

- Every embedded device communicates internally over serial buses — UART, I2C, SPI
- These protocols were designed for trusted, on-board communication between chips
- They were never designed to resist an adversary with physical access
- Physical access to these buses = game over for data confidentiality and integrity

**Real-world exploitation examples:**

- **Routers:** Most consumer routers have an unlabeled UART header on the PCB. Connect at 115200 baud → root shell. No password.
- **Medical devices:** Researchers have extracted firmware and credentials from insulin pumps and patient monitors via UART/SPI test points left on production boards.
- **Automotive:** OBD-II ports expose CAN bus (similar trust model). ECU firmware can be read and modified.
- **Security systems:** Alarm panels, access control systems, and vault sensors all use serial protocols internally. Physical access to the wiring = full control.

**Show photos (if available):**
- A router PCB with a 4-pin UART header circled
- A labeled vs. unlabeled test point array
- A logic analyzer connected to a target board

**Protocol comparison (quick overview — detail in Phase 4):**

| | UART | I2C | SPI |
|---|------|-----|-----|
| Wires | 2 (TX, RX) | 2 (SDA, SCL) | 4+ (CLK, MOSI, MISO, CS) |
| Speed | 9600–115200 baud typical | 100kHz–400kHz | 1MHz–80MHz |
| Addressing | None | 7-bit address | Chip Select line |
| Auth/Encryption | None | None | None |
| Topology | Point-to-point | Multi-device shared bus | Controller + peripherals |

**Transition to hands-on:** "In the next 60 minutes, you're going to rob a bank. Well, a simulated one. You'll sniff credentials, suppress alarms, and — if we have time — sit invisibly between the vault sensor and the security gateway, eating every alert while the drill runs. One jumper wire at a time."

---

## Phase 1 — The Honest System (20 min)

### Instructor Role

- Walk the room while students wire up
- Common issues to watch for:
  - TX/RX reversal (TX goes to RX, not TX to TX)
  - Wrong GPIO pins (confirm GPIO 17 → GPIO 16, not physical pin numbers)
  - Vibration sensor wired backwards (VCC and GND swapped — won't damage but won't read)
  - Wrong board selected in Arduino IDE
  - Wrong COM port
- Checkpoint: every station should show vault status readings on the gateway serial monitor before proceeding
- Have students tap the sensor — they should see CRITICAL alerts. This establishes trust in the system before they break it.

### If a Station is Stuck

- "Is your serial monitor set to 115200 baud?"
- "Check that your TX goes to RX — not TX to TX"
- "Are both boards sharing a common GND wire?"
- If vibration sensor always reads CLEAR: check GPIO 14 connection, check sensor orientation

### Transition to Phase 2

Once all stations are receiving data, say something like:

"Great — you've got a working vault security system. The sensor detects vibration, sends the status to the gateway, the gateway displays alerts. The bank is safe. Now I need you to flash a new firmware onto your second ESP32. This one is called `sniffer.ino`. Don't disconnect anything from Phase 1 — just add one more wire."

Do NOT tell them about the credentials yet. Let them discover it.

---

## Phase 2 — The Tap (20 min)

### Instructor Role

- Let students work through the wiring (it's one wire + ground)
- Do NOT reveal what they'll see — the "oh shit" moment depends on discovery
- Walk the room and listen for reactions
- Once credentials appear on sniffers, pause the room for a brief discussion

### The Reveal Moment

After most students have captured the credentials, address the room:

"Raise your hand if you can see a username and password on your screen right now."

(Wait for hands)

"Raise your hand if you needed any special hacking tools to do that."

(No hands)

"One wire. That's all it took. The vault admin password — `F1r$tN@t10nal#2026!` — was sitting on that copper trace the entire time, broadcast in cleartext at 9600 baud. The sensor has been screaming it into the void every 15 seconds, and the only reason nobody heard it before is that nobody was listening."

"Now think about what you could do with those credentials. Log into the security system? Disable the vault sensor remotely? Change the alert thresholds? You didn't hack anything. You just listened."

### If Time is Tight

Skip the individual capture recording on the worksheet. The visual impact of seeing credentials scroll by is the learning — the fill-in-the-blank is secondary.

---

## Phase 3 — The Heist (20 min)

### Instructor Role

- Guide students through the rewiring (disconnect sensor TX, connect attacker TX)
- Key moment: have students tap the real vibration sensor while watching the gateway monitor. The sensor is detecting vibration. The gateway shows CLEAR. The disconnect is visceral.
- Watch for the "oh shit" when students realize the security system is completely blind

### Discussion Prompt (After Most Students Complete)

"The security gateway accepted your fake 'all clear' without blinking. Why? Because UART has no concept of identity, no authentication, no checksums on the source. It processes bytes. Where those bytes originated — a legitimate vault sensor or a $2 ESP32 running 20 lines of code — is indistinguishable at the protocol level."

"The vault is being drilled open right now. The vibration sensor is detecting it — look at ESP32 #1's monitor, it's screaming CRITICAL. But the security guard's screen shows green. All clear. No alerts. The drill keeps running."

"How long until someone notices? Hours? The next morning when they open the vault? After the vault is empty?"

---

## Phase 3.5 — The Inside Job (15 min, stretch)

### Go / No-Go Decision

Run this phase only if:
- At least 70% of stations completed Phase 3 with time remaining
- Students are engaged and want more
- At least 20 minutes remain (15 for MITM + 5 buffer)

If not, skip to Phase 4 and mention MITM conceptually during the demo.

### Instructor Role

- This phase is the most complex wiring — three boards in a chain
- Walk the room actively; common wiring mistake is connecting UART2 TX on the attacker to the sensor instead of the gateway
- Emphasize: "Open three serial monitors. Tap the sensor. The sensor says CRITICAL. The attacker says SUPPRESSED. The gateway says CLEAR."

### The Payoff

"Phase 3 was a sledgehammer — you cut the sensor off and replaced it. An alert operator might notice the readings stopped and restarted. The Inside Job is a scalpel. The data never stopped. The credentials still worked. The sensor is detecting the drill — but the alarm is being eaten before it reaches the gateway. And you control the suppression. Set it to pass through everything except CRITICAL and you're virtually undetectable."

"This is the difference between a smash-and-grab and a sophisticated attack. The gateway has no idea anything is wrong. The data looks normal. The format is correct. The authentication passed. The only thing missing is the truth."

---

## Phase 4 — I2C and SPI Demo (15 min)

### I2C Demo (7 min)

**Setup:** ESP32 controller reading a BME280 sensor over I2C. Saleae capturing SDA/SCL.

**Walk through the Saleae capture on screen:**

1. **Start condition** — SDA drops while SCL is high
2. **Address byte** — 7 bits of device address + R/W bit. "See that `0x76`? That's the BME280's address. Every device on this bus has one. It's like shouting a name in a room — everyone hears it, but only the named device responds."
3. **ACK bit** — the slave pulls SDA low. "That's the sensor saying 'I'm here.'"
4. **Data bytes** — register address, then data
5. **Stop condition** — SDA rises while SCL is high

**Security angle:**

"I2C is a shared bus. Multiple devices on the same two wires. What happens if I put a second device on the bus with the same address? Bus collision. What if I put a device that responds to every address? I can impersonate any sensor on the bus."

"Same story as the vault sensor — no encryption, no authentication. If you can probe SDA and SCL, you can read every byte from every device on the bus."

### SPI Demo (7 min)

**Setup:** ESP32 reading from an SPI flash chip (or SD card in SPI mode). Saleae capturing CLK, MOSI, MISO, CS.

**Walk through the Saleae capture on screen:**

1. **Chip Select (CS)** — pulled low to select the device. "This is the entire authentication mechanism. CS goes low, you're in."
2. **Clock (CLK)** — generated by the controller. Data is valid on rising or falling edge depending on SPI mode.
3. **MOSI** — controller sends a command (e.g., read address 0x000000)
4. **MISO** — device sends back the data at that address

**Security angle:**

"See that chip select line? Pull it low and the flash chip will dump its entire contents to you. Firmware, configuration, credentials, encryption keys — whatever is stored on it. This is how firmware gets extracted in hardware security research."

"Tools like flashrom and the Bus Pirate automate this. Connect four wires to an SPI flash chip on a router's PCB, run `flashrom -r dump.bin`, and you have the firmware. From there, `binwalk` extracts the filesystem, and `grep` finds the hardcoded passwords."

### Tying it Back to the Vault

"Everything you did to the vault sensor today — sniffing, injection, MITM — applies to I2C and SPI too. Different wires, same trust problem. UART was just the easiest one to demonstrate because it takes one wire and no special timing."

**Transition:** "Every protocol you saw today shares the same fundamental problem. They were designed for chips to talk to each other on the same board, millimeters apart, inside a sealed case. Security was someone else's job. When an attacker opens that case and touches those traces, the protocols have nothing left to offer."

---

## Phase 5 — Debrief (10 min)

### Discussion Prompts

**1. "How would you defend against what you just did to the vault?"**

Steer toward:
- **Encryption at the application layer** — even if UART is plaintext, the payload can be encrypted (AES-encrypted status messages)
- **Message authentication codes (MAC)** — sign each message so tampering is detectable
- **Mutual authentication** — challenge-response before transmitting sensitive data
- **Physical security** — potting/epoxy over traces, tamper-evident enclosures, conduit for sensor wiring
- **Heartbeat monitoring** — if the sensor stops sending for even one cycle, trigger an alert
- **Out-of-band verification** — a second sensor on a different channel that cross-checks the first

**2. "Why don't these protocols have encryption built in?"**

- They predate the IoT era — designed in the 1970s-80s for communication between chips on the same PCB
- Adding crypto requires compute overhead, silicon area, and power — expensive on a $0.10 sensor
- The original threat model assumed physical security of the device enclosure
- That assumption breaks down when devices are deployed in the field, sold to consumers, or connected to networks
- The vault sensor was designed to be inside a secured room. Nobody expected someone to access the wiring.

**3. "What's the first thing you'd look for on an unfamiliar PCB?"**

- Unlabeled 4-pin headers (often UART: VCC, TX, RX, GND)
- Test pads labeled TX, RX, SDA, SCL, CLK, MOSI, etc.
- Chips with known datasheets (Google the part number)
- JTAG/SWD headers (usually 10-pin or 20-pin)
- Flash chips (8-pin SOIC packages — often SPI flash)

**4. "Name one real device category where this matters."**

Let students answer. Expect: security systems, alarm panels, medical devices, cars, drones, smart locks, industrial PLCs, ATMs, vending machines. All valid.

### Closing Statement

"Every protocol you exploited today is in the alarm system protecting your dorm, the card reader on the building door, and the ATM down the street. The difference between a vulnerability and a feature is the threat model. When engineers assume physical security, these protocols work fine. When that assumption fails — and it always fails eventually — you get what you saw today. One wire. One serial monitor. The vault is open."

### Hand Out Reference Cards

Distribute the take-home reference cards as students leave.

---

## Troubleshooting Quick Reference

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| Gateway shows nothing | TX/RX wires swapped | TX goes to RX, not TX to TX |
| Vibration sensor always CLEAR | Bad signal connection | Check GPIO 14, verify sensor orientation |
| Vibration sensor always CRITICAL | Sensor too sensitive / ambient noise | Check mounting, reduce vibration on desk |
| "Failed to connect" on upload | Wrong board/port selected | Tools → Board → ESP32 Dev Module; check port |
| Sniffer shows garbage characters | Baud rate mismatch | Board-to-board must be 9600; USB monitor must be 115200 |
| MITM receives but doesn't forward | Wrong TX pin on attacker | Confirm GPIO 17 is TX2, wired to gateway GPIO 16 |
| Serial monitor shows nothing at all | USB cable is charge-only | Try a different USB cable (needs data lines) |
| WiFi AP not showing up | AP failed to start | Check serial monitor for [STARTED] vs [FAILED]; reflash gateway |
| Dashboard won't load | Phone not on vault WiFi | Verify connected to VaultN-Security, not campus WiFi |
| firstnational.local not resolving | DNS issue on some phones | Try http://192.168.4.1 as fallback |
