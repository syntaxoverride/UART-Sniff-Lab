# Wire Tap: UART Eavesdropping & Injection Lab

## Student Worksheet

**Cybersecurity Club**

---

### The Scenario

First National Bank uses a vibration sensor mounted on Vault #7 to detect drilling, impacts, or tampering. A single ESP32 board (the white board) reads the sensor, runs a WiFi security dashboard, and transmits data over UART. Your job today: compromise that UART link.

### What You'll Need

- 2x ESP32-WROOM boards: white case (bank board) + blue case (attacker board), both pre-flashed by the instructor
- 1x Vibration sensor module (3-pin)
- 1x Breadboard
- Jumper wires (male-to-male)
- 2x USB cables + USB hub
- Laptop with a serial monitor application (Arduino Serial Monitor, PuTTY, or CoolTerm)

### Before You Start

Both boards are already flashed with firmware. No installation or setup is needed beyond a serial monitor application.

- **Serial monitor baud rate:** 115200
- **Blue board modes:** Press the BOOT button to cycle through SNIFF, INJECT, and MITM modes. The LED blink pattern tells you which mode is active:
  - 1 short blink every 3 seconds = SNIFF
  - 1 long blink every 3 seconds = INJECT
  - 3 short blinks every 3 seconds = MITM
- Each time you switch modes, the serial monitor displays wiring instructions for that mode.

---

## Phase 1: The Honest System (20 min)

**Goal:** See the bank board working standalone. Connect to its WiFi dashboard and confirm the vibration sensor triggers alerts.

### Step 1: Wire the Vibration Sensor

The white board (bank) is **already pre-flashed** by the instructor. Wire the vibration sensor only:

| From | Wire Color | To |
|------|------|------|
| White board, 3.3V | Red | Vibration sensor, VCC |
| White board, GND | Black | Vibration sensor, GND |
| White board, GPIO 14 | Blue | Vibration sensor, SIGNAL |

```
  WHITE BOARD (BANK)
  ┌────────┐
  │ GPIO 14├──┐
  │   3.3V ├─┐│
  │    GND ├┐││
  └────────┘│││
            │││  ┌──────────┐
            ││└─►│ SIGNAL   │
            │└──►│ VCC      │ VIBRATION
            └───►│ GND      │ SENSOR
                 └──────────┘
```

No wiring between boards in Phase 1. The blue board stays disconnected.

### Step 2: Power Up and Verify Serial Monitor

1. Plug the white board into USB
2. Open Serial Monitor (Tools → Serial Monitor, 115200 baud)
3. You should see:

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

4. **Tap the vibration sensor.** Light taps trigger LOW/MEDIUM, sustained shaking (3+ seconds) triggers CRITICAL:

```
**  WARN  #5 **  VAULT_STATUS:MEDIUM VIBRATION:8 EVENTS:1
!!! ALERT #8 !!! VAULT_STATUS:CRITICAL VIBRATION:12 EVENTS:2
```

### Step 3: Connect to the Dashboard

The white board runs a WiFi security dashboard:

1. On your phone, connect to WiFi: **VaultN-Security** (where N is your station number)
2. Password: **firstnational**
3. Open a browser and go to: **http://firstnational.local**
4. You should see the First National Bank vault monitoring dashboard
5. Tap the vibration sensor and watch the status change from green CLEAR to orange MEDIUM to red CRITICAL in real time
6. Hit **CLEAR LOG** to reset the event log

### Checkpoint

- [ ] White board serial monitor shows vault status readings
- [ ] Dashboard loads on phone at firstnational.local
- [ ] Tapping the sensor triggers CRITICAL alerts on the dashboard
- [ ] Leaving the sensor still shows CLEAR

**Raise your hand when alerts are flowing.**

---

## Phase 2: The Tap (20 min)

**Goal:** Eavesdrop on the white board's UART TX line using the blue board. Discover what's really being transmitted.

### What's Happening

The white board transmits sensor data over UART2 TX (GPIO 17). In addition to vibration status, the firmware periodically transmits something else over that same TX line. Your job: find out what.

### Step 1: Set the Blue Board to SNIFF Mode

1. Plug the blue board into USB
2. Open a serial monitor for the blue board (115200 baud)
3. Press the BOOT button until the LED shows **1 short blink every 3 seconds** (SNIFF mode)
4. The serial monitor displays wiring instructions for SNIFF mode

### Step 2: Wire the Sniffer

**Keep all sensor wires from Phase 1 in place.** Add two new wires to connect the blue board:

| From | Wire Color | To |
|------|------|------|
| White board, GPIO 17 (TX) | Orange | Blue board, GPIO 16 (RX) |
| White board, GND | Black | Blue board, GND |

```
  WHITE BOARD (BANK)
  ┌────────┐
  │ GPIO 17├ ╍ ╍orange╍ ╍► BLUE BOARD (SNIFFER) GPIO 16
  │    GND ├ ╍ ╍black╍ ╍ ► BLUE BOARD (SNIFFER) GND
  └────────┘
```

You are tapping the TX wire. One wire. That's the entire "exploit."

### Step 3: Watch

1. Wait up to 15 seconds
2. Watch what appears on the blue board's serial monitor

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

## Phase 3: The Heist (20 min)

**Goal:** Inject fake "all clear" data into the white board's RX. The dashboard overrides its local sensor display and shows whatever the blue board sends, even while the real sensor detects vibration.

### Step 1: Switch to INJECT Mode

1. Press the BOOT button on the blue board until the LED shows **1 long blink every 3 seconds** (INJECT mode)
2. The serial monitor displays wiring instructions for INJECT mode

### Step 2: Rewire

1. **Remove** the orange sniffer wire from Phase 2
2. **Connect** blue board GPIO 17 (TX) to white board GPIO 16 (RX) with the orange wire
3. **Verify** blue board GND is connected to white board GND (should already be from Phase 2)

```
  WHITE BOARD (BANK)                  BLUE BOARD ("THE THIEF")
  ┌────────┐                          ┌────────┐
  │ GPIO 16◄──────────orange──────────┤GPIO 17 │
  │    GND ◄──────────black───────────┤GND     │
  └────────┘                          └────────┘
```

### Step 3: Watch the Dashboard

Open the dashboard on your phone (http://firstnational.local). You should see:

```
VAULT_STATUS: CLEAR    VIBRATION: 0
```

All clear. No alerts. The security guard sees green across the board.

### Step 4: Now Tap the Real Sensor

The vibration sensor is still connected to the white board. Tap it. Open the white board's serial monitor: vibration detected, CRITICAL alerts. Look at the dashboard: still CLEAR.

**The drill is running. The alarm is silent. Nobody knows.**

The white board's dashboard overrides its local sensor display whenever data arrives on RX. After 3 seconds of no RX data, the dashboard falls back to showing local sensor readings.

### Step 5: Check Your Attacker Monitor

Open the serial monitor on the blue board:

```
[HEIST] Sent: VAULT_STATUS:CLEAR VIBRATION:0 EVENTS:0  (drill is running)
[HEIST] Sent: VAULT_STATUS:CLEAR VIBRATION:0 EVENTS:0  (drill is running)
```

### Think About It

> The dashboard accepted your fake "all clear" without hesitation. No error. No warning. No "authentication failed." UART has no concept of identity; the white board accepts bytes from whatever is electrically connected to its RX pin.
>
> The vault is being drilled open. The vibration sensor is detecting it. The white board's serial monitor shows CRITICAL. But the dashboard shows green. How long until someone notices? Hours? Days? After the vault is empty?

- [ ] I sent fake CLEAR data that the dashboard accepted
- [ ] I confirmed the real sensor detected vibration (serial monitor) while the dashboard showed CLEAR

---

## Phase 3.5: The Inside Job (Stretch Goal, 15 min)

**Goal:** Sit between the white board's TX and RX. Intercept real sensor data on the blue board, suppress CRITICAL alerts, and forward fake CLEAR to the dashboard. The dashboard never knows.

### Step 1: Switch to MITM Mode

1. Press the BOOT button on the blue board until the LED shows **3 short blinks every 3 seconds** (MITM mode)
2. The serial monitor displays wiring instructions for MITM mode

### Step 2: Rewire

Phase 3.5 uses both boards with the blue board sitting in the middle of the white board's TX/RX path:

```
  WHITE BOARD (BANK)         BLUE BOARD ("THE INSIDE MAN")
  ┌────────────┐             ┌────────────┐
  │ GPIO 17 TX ├──yellow────►│ GPIO 4  RX │  (blue receives real sensor data)
  │            │             │            │
  │ GPIO 16 RX ◄──orange────┤ GPIO 17 TX │  (blue forwards fake CLEAR to dashboard)
  │            │             │            │
  │ GND ───────┼──black─────►│ GND        │
  └────────────┘             └────────────┘
```

| From | Wire | To |
|------|------|------|
| White board, GPIO 17 (TX) | Yellow | Blue board, GPIO 4 (UART1 RX) |
| Blue board, GPIO 17 (UART2 TX) | Orange | White board, GPIO 16 (RX) |
| Both boards, GND | Black | Connected together |

### Step 3: Open Two Serial Monitors + Dashboard

You need two serial monitors and the dashboard:

1. **White board serial monitor**: shows real vibration readings (CRITICAL when tapped)
2. **Blue board serial monitor**: shows intercepted data + what was suppressed
3. **Dashboard on phone**: shows what the dashboard actually displays (CLEAR)

Tap the vibration sensor. Compare all three.

### Sample Output (Blue Board Monitor)

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

> Phase 3 was a sledgehammer; you overrode the dashboard with fake data, but the white board's serial monitor still showed real vibration. An alert operator checking serial output might notice. The Inside Job is a scalpel. The blue board intercepts the real TX output and replaces it before it reaches the dashboard's RX. The data never stops flowing. The credentials still work. The sensor detects the drill. The alarm is eaten before it arrives.
>
> How would you detect this attack from the dashboard alone?

- [ ] I observed real CRITICAL alerts being silently suppressed to CLEAR
- [ ] I captured credentials that were forwarded without modification
- [ ] I can explain why interception is harder to detect than simple injection

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
