# Wire Tap: Instructor Guide

## Pre-Lab Preparation

### Boards to Pre-Flash

Flash **all boards** (white and blue) using `flash_boards.py` before the lab. Students never flash any boards and never open Arduino IDE.

**Flashing with flash_boards.py:**

```
python3 flash_boards.py
```

**White boards (bank):**
- Plug in a white board, press Enter
- Enter vault number (1-20). Each board gets a unique WiFi AP (`Vault1-Security`, `Vault2-Security`, etc.)
- The script compiles `bank.ino` with the vault number, flashes, and verifies
- Label each board after flashing (tape + marker: "BANK BOARD #N, DO NOT REFLASH")
- All boards share WiFi password: `firstnational`
- Dashboard URL: `http://firstnational.local` (captive DNS, any URL works)

**Blue boards (attacker):**
- Plug in a blue board, press Enter
- The script flashes `attacker.ino`, the multi-mode firmware
- No vault number needed
- Label each board after flashing (tape + marker: "ATTACKER BOARD, DO NOT REFLASH")

**Post-flash verification checklist (white boards):**
- [ ] Plug in each white board, open serial monitor at 115200 baud
- [ ] Confirm startup banner shows `VAULT SECURITY GATEWAY v2.0`
- [ ] Confirm WiFi AP `VaultN-Security` appears in phone WiFi list
- [ ] Wire a vibration sensor (3.3V, GND, SIGNAL to GPIO 14)
- [ ] Tap the sensor, confirm CRITICAL appears on serial monitor and dashboard
- [ ] Leave vibration sensor loose or wired (students wire it in Phase 1 if loose)

**Post-flash verification checklist (blue boards):**
- [ ] Plug in each blue board, open serial monitor at 115200 baud
- [ ] Confirm startup banner appears
- [ ] Press the BOOT button and verify mode cycles: SNIFF (1 short blink) then INJECT (1 long blink) then MITM (3 short blinks)
- [ ] Verify wiring instructions appear on serial monitor when modes change

### Student Stations

Each station needs:
- 1x Pre-flashed white board (bank) + vibration sensor (already wired or loose for students to wire)
- 1x Pre-flashed blue board (attacker, multi-mode firmware)
- Breadboard, jumper wires (at least 6 male-to-male)
- USB hub + 2 USB cables
- Printed student worksheet
- Printed reference card

### No Firmware Distribution Needed

All boards are pre-flashed. Students do not need any firmware files, Arduino IDE, or development tools. Students only need a serial monitor application (Arduino Serial Monitor, PuTTY, or CoolTerm) to view output from the boards.

### WiFi Dashboard

Each white board creates a WiFi access point and serves a live security dashboard:

- **SSID:** VaultN-Security (N = station number)
- **Password:** firstnational
- **URL:** http://firstnational.local (captive DNS; any URL works too)
- **Features:** live status with color-coded alerts, vibration level, event log, CLEAR LOG button
- **Dashboard override behavior:** When data containing `VAULT_STATUS:` arrives on the white board's UART2 RX (GPIO 16), the dashboard overrides its local sensor display and shows whatever arrived on RX. After 3 seconds of no RX data, the dashboard falls back to showing local sensor readings.
- **Audience:** observers can connect on their phones to watch the attack unfold in real time

The dashboard override is the mechanism behind the Phase 3 "wait, what?" moment. Students inject fake CLEAR via the blue board, and the dashboard switches from showing real vibration to showing the injected data. The white board's serial monitor still shows real sensor readings, proving the sensor works but the dashboard is fooled.

### Logic Analyzer Setup (Instructor Demo Station)

**Wiring:**
- Analyzer CH0 → White board GPIO 17 (TX)
- Analyzer GND → White board GND
- Can share the TX wire with the blue board's RX using a splitter

**Software configuration:**
1. Open your logic analyzer software (e.g., Logic 2)
2. Set digital sampling rate to at least **1 MS/s**
3. Set capture mode to **Trigger** (continuous) rather than timed, so it runs without stopping
4. Add an **Async Serial** protocol analyzer on CH0:
   - Bit rate: **9600**
   - Data bits: 8
   - Stop bits: 1
   - Parity: None
   - Bit order: LSB first
5. Start the capture. Decoded ASCII text appears above the waveform.

**What the audience sees on the projector:**
- The raw UART waveform: a line that sits HIGH, drops LOW for start bits, toggles through data bits, returns HIGH for stop bits
- Decoded text: `VAULT_STATUS:CLEAR VIBRATION:0 EVENTS:0`
- Every 15 seconds, the auth handshake scrolls through: `USER: vault_admin`, `PASS: F1r$tN@t10nal#2026!`
- When students tap the vibration sensor, the status changes in real time on the waveform

**Optional Phase 2 use:** Run the logic analyzer on the projector during Phase 2 while students sniff with their blue boards. The audience sees the raw waveform while students see decoded text on their serial monitors. Two perspectives on the same attack.

**For Phase 4 I2C/SPI demos:**
- Demo ESP32 + BME280 wired for I2C demo
- Demo ESP32 + SPI device (flash chip or SD card module) for SPI demo
- Pre-label channels in the analyzer software (SDA, SCL, MOSI, MISO, CLK, CS)

---

## Phase 0: Context Setting (10 min)

### Key Points to Hit

**The scenario:**

"First National Bank has a vault protected by a vibration sensor. If someone drills into the vault, the sensor detects the vibration and alerts the security monitoring station over a serial UART connection. Simple, reliable, proven technology. Today you're going to break it."

**Why serial protocols matter in security:**

- Every embedded device communicates internally over serial buses: UART, I2C, SPI
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

**Protocol comparison (quick overview; detail in Phase 4):**

| | UART | I2C | SPI |
|------|------|------|------|
| Wires | 2 (TX, RX) | 2 (SDA, SCL) | 4+ (CLK, MOSI, MISO, CS) |
| Speed | 9600–115200 baud typical | 100kHz–400kHz | 1MHz–80MHz |
| Addressing | None | 7-bit address | Chip Select line |
| Auth/Encryption | None | None | None |
| Topology | Point-to-point | Multi-device shared bus | Controller + peripherals |

**Transition to hands-on:** "In the next 60 minutes, you're going to rob a bank. Well, a simulated one. You'll sniff credentials, suppress alarms, and, if we have time, sit invisibly between the vault sensor and the security gateway, eating every alert while the drill runs. One jumper wire at a time."

---

## Phase 1: The Honest System (20 min)

### Instructor Role

- Walk the room while students wire the vibration sensor to the white board and connect to the dashboard
- No wiring between boards in Phase 1. The white board works standalone.
- Common issues to watch for:
  - Vibration sensor wired backwards (VCC and GND swapped; won't damage but won't read)
  - Phone not connecting to VaultN-Security WiFi (wrong network, wrong password)
  - Dashboard not loading (try http://192.168.4.1 as fallback if firstnational.local won't resolve)
- Checkpoint: every station should see the dashboard respond to vibration before proceeding
- Have students tap the sensor; they should see CRITICAL alerts on the dashboard. Seeing the alerts establishes trust in the system before they break it.

### If a Station is Stuck

- "Is the white board's serial monitor set to 115200 baud?"
- "Is your phone connected to VaultN-Security, not campus WiFi?"
- "Check that the vibration sensor SIGNAL wire goes to GPIO 14"
- If vibration sensor always reads CLEAR: check GPIO 14 connection, check sensor orientation

### Transition to Phase 2

Once all stations have a working dashboard, say something like:

"Great, you've got a working vault security system. The sensor detects vibration, the dashboard shows alerts. The bank is safe. Now grab the blue board, plug it into USB, and press the BOOT button until you see one short blink every 3 seconds. That's SNIFF mode. The serial monitor will show you exactly what to wire. Keep everything from Phase 1 in place. Just add one wire from the white board to the blue board."

Do NOT tell them about the credentials yet. Let them discover it.

---

## Phase 2: The Tap (20 min)

### Instructor Role

- Students press the BOOT button on the blue board to enter SNIFF mode (1 short blink every 3 seconds)
- The serial monitor shows wiring instructions. Students wire the blue board's RX (GPIO 16) to the white board's TX (GPIO 17), plus shared GND.
- Let students work through the wiring (it's one wire + ground)
- Do NOT reveal what they'll see. The "wait, what?" moment depends on discovery
- Walk the room and listen for reactions
- Once credentials appear on the blue board's serial monitor, pause the room for a brief discussion

### The Reveal Moment

After most students have captured the credentials, address the room:

"Raise your hand if you can see a username and password on your screen right now."

(Wait for hands)

"Raise your hand if you needed any special hacking tools to do that."

(No hands)

"One wire. That's all it took. The vault admin password, `F1r$tN@t10nal#2026!`, was sitting on that copper trace the entire time, broadcast in cleartext at 9600 baud. The sensor has been screaming it into the void every 15 seconds, and the only reason nobody heard it before is that nobody was listening."

"Now think about what you could do with those credentials. Log into the security system? Disable the vault sensor remotely? Change the alert thresholds? You didn't hack anything. You just listened."

### If Time is Tight

Skip the individual capture recording on the worksheet. The visual impact of seeing credentials scroll by is the learning; the fill-in-the-blank is secondary.

---

## Phase 3: The Heist (20 min)

### Instructor Role

- Students press the BOOT button on the blue board to enter INJECT mode (1 long blink every 3 seconds). The serial monitor shows wiring instructions.
- Guide students through the rewiring: remove the sniffer wire, wire blue TX (GPIO 17) to white RX (GPIO 16)
- Have students tap the real vibration sensor while watching the dashboard. The white board's serial monitor shows CRITICAL. The dashboard shows CLEAR. The disconnect is visceral.
- Watch for the "wait, what?" when students realize the dashboard is showing injected data while the sensor screams

### Discussion Prompt (After Most Students Complete)

"The dashboard accepted your fake 'all clear' without blinking. Why? Because UART has no concept of identity, no authentication, no checksums on the source. The white board accepts bytes from whatever is electrically connected to its RX pin. Where those bytes originated, whether a legitimate source or a $2 ESP32 running 20 lines of code, is indistinguishable at the protocol level."

"The vault is being drilled open right now. The vibration sensor is detecting it. Look at the white board's serial monitor. CRITICAL alerts are firing. But the dashboard shows green. All clear. No alerts. The drill keeps running."

"How long until someone notices? Hours? The next morning when they open the vault? After the vault is empty?"

---

## Phase 3.5: The Inside Job (15 min, stretch)

### Go / No-Go Decision

Run this phase only if:
- At least 70% of stations completed Phase 3 with time remaining
- Students are engaged and want more
- At least 20 minutes remain (15 for MITM + 5 buffer)

If not, skip to Phase 4 and mention MITM conceptually during the demo.

### Instructor Role

- Students press the BOOT button on the blue board to enter MITM mode (3 short blinks every 3 seconds). The serial monitor shows wiring instructions.
- Walk the room actively; common wiring mistake is connecting blue TX to the wrong pin on the white board
- Correct wiring: white TX (GPIO 17) to blue GPIO 4 (UART1 RX), blue TX (GPIO 17) to white RX (GPIO 16)
- Emphasize: "Open two serial monitors plus the dashboard. Tap the sensor. The white board's serial says CRITICAL. The blue board says SUPPRESSED. The dashboard says CLEAR."

### The Payoff

"Phase 3 was a sledgehammer; you overrode the dashboard, but the white board's serial monitor still showed real vibration. An alert operator checking serial output might notice. The Inside Job is a scalpel. The blue board intercepts the real TX output and replaces it before it reaches the dashboard's RX. The sensor is detecting the drill, but the alarm is being eaten before it arrives. And you control the suppression. Set it to pass through everything except CRITICAL and you're virtually undetectable."

"The dashboard has no idea anything is wrong. The data looks normal. The format is correct. The authentication passed. The only thing missing is the truth."

---

## Phase 4: I2C and SPI Demo (15 min)

### I2C Demo (7 min)

**Setup:** ESP32 controller reading a BME280 sensor over I2C. Logic Analyzer capturing SDA/SCL.

**Walk through the logic analyzer capture on screen:**

1. **Start condition**: SDA drops while SCL is high
2. **Address byte**: 7 bits of device address + R/W bit. "See that `0x76`? That's the BME280's address. Every device on this bus has one. It's like shouting a name in a room; everyone hears it, but only the named device responds."
3. **ACK bit**: the slave pulls SDA low. "That's the sensor saying 'I'm here.'"
4. **Data bytes**: register address, then data
5. **Stop condition**: SDA rises while SCL is high

**Security angle:**

"I2C is a shared bus. Multiple devices on the same two wires. What happens if I put a second device on the bus with the same address? Bus collision. What if I put a device that responds to every address? I can impersonate any sensor on the bus."

"Same story as the vault sensor: no encryption, no authentication. If you can probe SDA and SCL, you can read every byte from every device on the bus."

### SPI Demo (7 min)

**Setup:** ESP32 reading from an SPI flash chip (or SD card in SPI mode). Logic Analyzer capturing CLK, MOSI, MISO, CS.

**Walk through the logic analyzer capture on screen:**

1. **Chip Select (CS)**: pulled low to select the device. "CS is the entire authentication mechanism. CS goes low, you're in."
2. **Clock (CLK)**: generated by the controller. Data is valid on rising or falling edge depending on SPI mode.
3. **MOSI**: controller sends a command (e.g., read address 0x000000)
4. **MISO**: device sends back the data at that address

**Security angle:**

"See that chip select line? Pull it low and the flash chip will dump its entire contents to you. Firmware, configuration, credentials, encryption keys, whatever is stored on it. That's how firmware gets extracted in hardware security research."

"Tools like flashrom and the Bus Pirate automate this. Connect four wires to an SPI flash chip on a router's PCB, run `flashrom -r dump.bin`, and you have the firmware. From there, `binwalk` extracts the filesystem, and `grep` finds the hardcoded passwords."

### Tying it Back to the Vault

"Everything you did to the vault sensor today (sniffing, injection, MITM) applies to I2C and SPI too. Different wires, same trust problem. UART was just the easiest one to demonstrate because it takes one wire and no special timing."

**Transition:** "Every protocol you saw today shares the same fundamental problem. They were designed for chips to talk to each other on the same board, millimeters apart, inside a sealed case. Security was someone else's job. When an attacker opens that case and touches those traces, the protocols have nothing left to offer."

---

## Phase 5: Debrief (10 min)

### Discussion Prompts

**1. "How would you defend against what you just did to the vault?"**

Steer toward:
- **Encryption at the application layer**: even if UART is plaintext, the payload can be encrypted (AES-encrypted status messages)
- **Message authentication codes (MAC)**: sign each message so tampering is detectable
- **Mutual authentication**: challenge-response before transmitting sensitive data
- **Physical security**: potting/epoxy over traces, tamper-evident enclosures, conduit for sensor wiring
- **Heartbeat monitoring**: if the sensor stops sending for even one cycle, trigger an alert
- **Out-of-band verification**: a second sensor on a different channel that cross-checks the first

**2. "Why don't these protocols have encryption built in?"**

- They predate the IoT era, designed in the 1970s-80s for communication between chips on the same PCB
- Adding crypto requires compute overhead, silicon area, and power; all expensive on a $0.10 sensor
- The original threat model assumed physical security of the device enclosure
- That assumption breaks down when devices are deployed in the field, sold to consumers, or connected to networks
- The vault sensor was designed to be inside a secured room. Nobody expected someone to access the wiring.

**3. "What's the first thing you'd look for on an unfamiliar PCB?"**

- Unlabeled 4-pin headers (often UART: VCC, TX, RX, GND)
- Test pads labeled TX, RX, SDA, SCL, CLK, MOSI, etc.
- Chips with known datasheets (Google the part number)
- JTAG/SWD headers (usually 10-pin or 20-pin)
- Flash chips (8-pin SOIC packages, often SPI flash)

**4. "Name one real device category where this matters."**

Let students answer. Expect: security systems, alarm panels, medical devices, cars, drones, smart locks, industrial PLCs, ATMs, vending machines. All valid.

### Closing Statement

"Every protocol you exploited today is in the alarm system protecting your dorm, the card reader on the building door, and the ATM down the street. The difference between a vulnerability and a feature is the threat model. When engineers assume physical security, these protocols work fine. When that assumption fails, and it always fails eventually, you get what you saw today. One wire. One serial monitor. The vault is open."

### Hand Out Reference Cards

Distribute the take-home reference cards as students leave.

---

## Troubleshooting Quick Reference

| Symptom | Likely Cause | Fix |
|------|------|------|
| Sniffer shows nothing | TX/RX wires swapped | White TX (GPIO 17) goes to blue RX (GPIO 16), not TX to TX |
| Vibration sensor always CLEAR | Bad signal connection | Check GPIO 14, verify sensor orientation |
| Vibration sensor always CRITICAL | Sensor too sensitive / ambient noise | Check mounting, reduce vibration on desk |
| Blue board not responding to BOOT button | USB cable may be charge-only | Try a different USB cable (needs data lines) |
| Sniffer shows garbage characters | Baud rate mismatch | Board-to-board must be 9600; USB monitor must be 115200 |
| MITM receives but doesn't forward | Wrong TX pin on blue board | Confirm blue GPIO 17 (TX2) is wired to white GPIO 16 (RX) |
| Serial monitor shows nothing at all | USB cable is charge-only | Try a different USB cable (needs data lines) |
| WiFi AP not showing up | AP failed to start | Check serial monitor for [STARTED] vs [FAILED]; reflash white board |
| Dashboard won't load | Phone not on vault WiFi | Verify connected to VaultN-Security, not campus WiFi |
| firstnational.local not resolving | DNS issue on some phones | Try http://192.168.4.1 as fallback |
