# Wire Tap — UART Eavesdropping & Injection Lab

## Student Worksheet

**Islanders Cybersecurity Club — TAMU-CC**

---

### The Scenario

First National Bank uses a vibration sensor mounted on Vault #7 to detect drilling, impacts, or tampering. The sensor reports status to a security monitoring gateway over a UART serial link. Your job today: compromise that link.

### What You'll Need

- 2x ESP32-WROOM boards (one is pre-flashed by the instructor)
- 1x Vibration sensor module (3-pin)
- 1x Breadboard
- Jumper wires (male-to-male)
- 2x USB cables + USB hub
- Laptop with Arduino IDE installed

### Before You Start

Install the ESP32 board package if you haven't already:

1. Open Arduino IDE → File → Preferences
2. In "Additional Board Manager URLs" add:
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. Tools → Board → Board Manager → search "ESP32" → Install **esp32 by Espressif Systems**

No additional libraries needed — the vibration sensor uses digitalRead only.

### Board Setup

- **Board selection:** Tools → Board → ESP32 Dev Module
- **Port:** Tools → Port → select the correct COM/tty port for each board
- **Baud rate for serial monitor:** 115200 (USB debug output)

---

## Phase 1 — The Honest System (20 min)

**Goal:** Build a working vault sensor-to-gateway link over UART and confirm data flows.

### Step 1: Wire the Circuit

Refer to **Wiring Diagram #1** on the projector or in your packet.

| From | Wire Color | To |
|------|------------|----|
| ESP32 #1 — 3.3V | Red | Vibration sensor — VCC |
| ESP32 #1 — GND | Black | Vibration sensor — GND |
| ESP32 #1 — GPIO 14 | Blue | Vibration sensor — SIGNAL |
| ESP32 #1 — GPIO 17 | Yellow | ESP32 #2 — GPIO 16 |
| ESP32 #1 — GND | Black | ESP32 #2 — GND |

```
  ESP32 #1             ESP32 #2
  VAULT SENSOR         SECURITY GATEWAY
  ┌────────┐           ┌────────┐
  │ GPIO 17├───yellow──►GPIO 16 │
  │    GND ├───black───►GND     │
  │ GPIO 14├──┐        │        │
  │   3.3V ├─┐│        └────────┘
  │    GND ├┐││
  └────────┘│││
            │││  ┌──────────┐
            ││└─►│ SIGNAL   │
            │└──►│ VCC      │ VIBRATION
            └───►│ GND      │ SENSOR
                 └──────────┘
```

### Step 2: Flash the Firmware

Your ESP32 #1 is **already pre-flashed** by the instructor. No action needed.

Flash **gateway.ino** to ESP32 #2:
1. Open `gateway.ino` in Arduino IDE
2. Select the correct port for ESP32 #2
3. Click Upload (→ button)
4. Wait for "Done uploading"

### Step 3: Verify the Serial Monitor

1. Open Serial Monitor for ESP32 #2 (Tools → Serial Monitor, 115200 baud)
2. You should see:

```
===================================
  VAULT SECURITY GATEWAY v2.0
  First National Bank - Vault #N
===================================
WiFi AP: VaultN-Security  [STARTED]
Password: firstnational
Dashboard: http://firstnational.local

[SECURE]  #1     VAULT_STATUS:CLEAR VIBRATION:0 EVENTS:0
[SECURE]  #2     VAULT_STATUS:CLEAR VIBRATION:0 EVENTS:0
```

3. **Tap the vibration sensor** — light taps trigger LOW/MEDIUM, sustained shaking (3+ seconds) triggers CRITICAL:

```
**  WARN  #5 **  VAULT_STATUS:MEDIUM VIBRATION:8 EVENTS:1
!!! ALERT #8 !!! VAULT_STATUS:CRITICAL VIBRATION:12 EVENTS:2
```

### Step 4: Connect to the Dashboard

The gateway also runs a WiFi security dashboard that anyone can view:

1. On your phone, connect to WiFi: **VaultN-Security** (where N is your station number)
2. Password: **firstnational**
3. Open a browser and go to: **http://firstnational.local**
4. You should see the First National Bank vault monitoring dashboard
5. Tap the vibration sensor — watch the status change from green CLEAR to orange MEDIUM to red CRITICAL in real time
6. Hit **CLEAR LOG** to reset the event log

### Checkpoint

- [ ] Gateway serial monitor shows vault status readings
- [ ] Dashboard loads on phone at firstnational.local
- [ ] Tapping the sensor triggers CRITICAL alerts
- [ ] Leaving the sensor still shows CLEAR

**Raise your hand when alerts are flowing.**

---

## Phase 2 — The Tap (20 min)

**Goal:** Eavesdrop on the UART line with your second ESP32. Discover what's really being transmitted.

### What Changed

Your instructor pre-flashed ESP32 #1 with updated firmware. In addition to vibration status, the sensor now periodically transmits something else over the same UART line. Your job: find out what.

### Step 1: Wire the Sniffer

**Keep all existing wires in place.** Add one new wire:

| From | Wire Color | To |
|------|------------|----|
| ESP32 #1 — GPIO 17 (same TX line) | Orange | Your sniffer ESP32 — GPIO 16 |
| ESP32 #1 — GND | Black | Your sniffer ESP32 — GND |

```
  ESP32 #1
  VAULT SENSOR
  ┌────────┐
  │ GPIO 17├───yellow──► ESP32 #2 (Gateway) GPIO 16
  │        │
  │ GPIO 17├ ╍ ╍orange╍ ╍► YOUR ESP32 (Sniffer) GPIO 16
  │    GND ├ ╍ ╍black╍ ╍ ► YOUR ESP32 (Sniffer) GND
  └────────┘
```

You are tapping the same TX wire. One wire. That's the entire "exploit."

### Step 2: Flash the Sniffer

1. Open `sniffer.ino` in Arduino IDE
2. Select the correct port for your sniffer ESP32
3. Upload

### Step 3: Watch

1. Open Serial Monitor for your sniffer ESP32 (115200 baud)
2. Wait up to 15 seconds
3. Watch what appears

### What Did You Capture?

Record what you see below:

**Device ID:** `_______________________________________________`

**Username:** `_______________________________________________`

**Password:** `_______________________________________________`

**Auth Token:** `_______________________________________________`

### Think About It

> You just captured vault admin credentials using one jumper wire and 20 lines of code. No password cracking. No exploit development. No special hardware. The sensor has been screaming these credentials into the void every 15 seconds, and the only reason nobody heard them before is that nobody was listening.

- [ ] I captured plaintext vault admin credentials from the UART line

---

## Phase 3 — The Heist (20 min)

**Goal:** Replace the vault sensor with your ESP32. Send fake "all clear" to the security gateway while the vault is being drilled.

### Step 1: Rewire

1. **Disconnect** the yellow wire from ESP32 #1 GPIO 17 to ESP32 #2 GPIO 16
2. **Disconnect** the orange sniffer wire
3. **Connect** your attacker ESP32 GPIO 17 (TX2) → ESP32 #2 GPIO 16 (RX2) with the orange wire
4. **Connect** your attacker ESP32 GND → ESP32 #2 GND

```
  ESP32 #1               ESP32 #2 (Security Gateway)
  VAULT SENSOR           First National Bank
  ┌────────┐             ┌────────┐
  │ GPIO 17├──╳ CUT      │ GPIO 16◄──orange── ESP32 #3 GPIO 17
  └────────┘             │    GND ◄──black─── ESP32 #3 GND
                         └────────┘
                                    YOUR ESP32 ("THE THIEF")
```

### Step 2: Flash the Injector

1. Open `injector.ino` in Arduino IDE
2. Select the correct port for your attacker ESP32
3. Upload

### Step 3: Watch the Gateway

Open Serial Monitor for ESP32 #2 (the security gateway). You should see:

```
[SECURE]  #1     VAULT_STATUS:CLEAR VIBRATION:0 EVENTS:0
[SECURE]  #1     VAULT_STATUS:CLEAR VIBRATION:0 EVENTS:0
[SECURE]  #1     VAULT_STATUS:CLEAR VIBRATION:0 EVENTS:0
```

All clear. No alerts. The security guard sees green across the board.

### Step 4: Now Tap the Real Sensor

The real vault sensor (ESP32 #1) is still running. Tap the vibration sensor. Look at ESP32 #1's serial monitor — it's screaming CRITICAL. Look at the gateway — still CLEAR.

**The drill is running. The alarm is silent. Nobody knows.**

### Step 5: Check Your Attacker Monitor

Open the serial monitor on your attacker ESP32:

```
[HEIST] Sent: VAULT_STATUS:CLEAR VIBRATION:0 EVENTS:0  (drill is running)
[HEIST] Sent: VAULT_STATUS:CLEAR VIBRATION:0 EVENTS:0  (drill is running)
```

### Think About It

> The security gateway accepted your fake "all clear" without hesitation. No error. No warning. No "authentication failed." UART has no concept of identity — it accepts bytes from whatever is electrically connected to the RX pin.
>
> The vault is being drilled open. The vibration sensor is detecting it. But the security monitor shows green. How long until someone notices? Hours? Days? After the vault is empty?

- [ ] I sent fake CLEAR data that the security gateway accepted
- [ ] I confirmed the real sensor detected vibration while the gateway showed nothing

---

## Phase 3.5 — The Inside Job (Stretch Goal, 15 min)

**Goal:** Sit between the vault sensor and gateway. Receive real CRITICAL alerts, suppress them, forward fake CLEAR. The gateway never knows.

### Step 1: Rewire

This phase uses all three ESP32 boards in a chain:

```
  VAULT SENSOR (ESP32 #1)   THE INSIDE MAN (Your ESP32)    SEC. GATEWAY (ESP32 #2)
  ┌────────────┐             ┌────────────┐                ┌────────────┐
  │ GPIO 17 TX ├──yellow────►│ GPIO 4  RX │                │            │
  │            │             │            │                │            │
  │            │             │ GPIO 17 TX ├───orange──────►│ GPIO 16 RX │
  │            │             │            │                │            │
  │ GND ───────┼──black─────►│ GND ───────┼───black──────►│ GND        │
  └────────────┘             └────────────┘                └────────────┘
```

| From | Wire | To |
|------|------|----|
| ESP32 #1 — GPIO 17 (TX2) | Yellow | Your ESP32 — GPIO 4 (UART1 RX) |
| Your ESP32 — GPIO 17 (UART2 TX) | Orange | ESP32 #2 — GPIO 16 (RX2) |
| All boards — GND | Black | Connected together |

### Step 2: Flash the MITM Firmware

1. Open `mitm.ino` in Arduino IDE
2. Upload to your attacker ESP32

### Step 3: Open Three Serial Monitors

You need three serial monitors running simultaneously:

1. **ESP32 #1 (Vault Sensor)** — shows real vibration readings (CRITICAL when tapped)
2. **Your ESP32 (Inside Man)** — shows intercepted data + what was suppressed
3. **ESP32 #2 (Security Gateway)** — shows what it actually received (CLEAR)

Tap the vibration sensor. Compare all three monitors.

### Sample Output (Attacker Monitor)

```
[MITM] #1 Intercepted:
  RAW: VAULT_STATUS:CRITICAL VIBRATION:42 EVENTS:1
  REAL:    STATUS:CRITICAL VIBRATION:42
  SENDING: STATUS:CLEAR VIBRATION:0
  ^^^^ ALERT SUPPRESSED ^^^^

[MITM] #2 Intercepted:
  RAW: [VAULT-SENSOR] PASS: F1r$tN@t10nal#2026!
  >>>> CREDENTIAL CAPTURED <<<<
  FORWARDED: (unmodified)
```

### Think About It

> Phase 3 was a sledgehammer — you cut the sensor off and replaced it. An alert operator might notice the gap. The Inside Job is a scalpel. The data never stops flowing. The credentials still work. The sensor detects the drill. The alarm is eaten before it arrives.
>
> How would you detect this attack from the gateway side alone?

- [ ] I observed real CRITICAL alerts being silently suppressed to CLEAR
- [ ] I captured credentials that were forwarded without modification
- [ ] I can explain why this is harder to detect than simple injection

---

## Debrief Questions

After the instructor demos, discuss with your table:

1. **Defense:** How would you protect a UART connection against what you just did?
   - Encryption? Authentication? Physical tamper detection?
   - Why don't embedded protocols have these built in?

2. **Real world:** What's the first thing you'd look for on an unfamiliar PCB?
   - Hint: unlabeled 4-pin headers, test pads, silkscreen labels like TX/RX/GND

3. **Scale:** UART is point-to-point. I2C is a shared bus with multiple devices. SPI uses chip-select lines. How do the attack surfaces differ?

4. **So what?** Name one real device category where this attack has consequences.
   - Security systems, medical devices, industrial controllers, automotive ECUs, smart locks...

---

## Cleanup Checklist

- [ ] Disconnect all jumper wires
- [ ] Return borrowed equipment
- [ ] Keep your reference card
