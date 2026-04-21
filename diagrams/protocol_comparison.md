# Protocol Comparison Diagram

Visual reference for instructor presentation. Shows UART, I2C, and SPI side by side.

---

## Signal Line Topology

```
  UART — Point-to-Point (2 wires, no clock)
  ════════════════════════════════════════════

      Device A                  Device B
    ┌──────────┐              ┌──────────┐
    │       TX ├─────────────►│ RX       │
    │          │              │          │
    │       RX │◄─────────────┤ TX       │
    └──────────┘              └──────────┘

    • No clock — both sides must agree on baud rate
    • Full duplex — TX and RX are independent
    • No addressing — one sender, one receiver
    • No bus arbitration — nothing to arbitrate



  I2C — Shared Bus (2 wires + pull-ups)
  ════════════════════════════════════════════

         VCC
          │
         ┌┴┐  ┌┴┐   Pull-up resistors (4.7kΩ)
         │ │  │ │
         └┬┘  └┬┘
          │    │
    ──────┼────┼──────────────────────────────── SDA (data)
    ──────┼────┼──────────────────────────────── SCL (clock)
          │    │
    ┌─────┴────┴──┐  ┌──────────┐  ┌──────────┐
    │ Controller  │  │ Device   │  │ Device   │
    │ (master)    │  │ 0x76     │  │ 0x3C     │
    └─────────────┘  └──────────┘  └──────────┘

    • Clock provided by controller on SCL
    • Shared data line (SDA) — half duplex
    • 7-bit addressing — up to 127 devices
    • Any device on the bus sees ALL traffic
    • Pull-ups required (open-drain outputs)



  SPI — Controller + Peripherals (4+ wires)
  ════════════════════════════════════════════

    ┌─────────────┐
    │ Controller  │        ┌──────────┐
    │             │        │ Device 1 │
    │   CLK  ─────┼────────┤ CLK      │
    │   MOSI ─────┼────────┤ MOSI     │
    │   MISO ◄────┼────────┤ MISO     │
    │   CS0  ─────┼────────┤ CS       │
    │             │        └──────────┘
    │             │
    │             │        ┌──────────┐
    │             │        │ Device 2 │
    │   CLK  ─────┼────────┤ CLK      │
    │   MOSI ─────┼────────┤ MOSI     │
    │   MISO ◄────┼────────┤ MISO     │
    │   CS1  ─────┼────────┤ CS       │
    │             │        └──────────┘
    └─────────────┘

    • Dedicated clock line from controller
    • Full duplex — MOSI and MISO are separate
    • No addressing — Chip Select (CS) line per device
    • Fastest of the three (1–80+ MHz)
    • Each new device adds one more CS wire
```

---

## Attack Surface Comparison

```
┌─────────────────────────────────────────────────────────────────────┐
│                     ATTACK SURFACE MAP                              │
│                                                                     │
│  UART                                                               │
│  ─────                                                              │
│  ► Sniff TX line → plaintext data, credentials, debug output        │
│  ► Inject on RX line → send commands, fake data                     │
│  ► MITM (intercept + modify + forward) → invisible tampering        │
│  ► Connect to debug header → root shell on many devices             │
│                                                                     │
│  I2C                                                                │
│  ─────                                                              │
│  ► Probe SDA/SCL → read ALL traffic from ALL devices on the bus     │
│  ► Inject responses → impersonate any device by matching address    │
│  ► Address collision → DoS by responding to legitimate addresses    │
│  ► Replay captured transactions → replay valid commands             │
│                                                                     │
│  SPI                                                                │
│  ─────                                                              │
│  ► Pull CS low → access any SPI peripheral directly                 │
│  ► Read flash chip → extract firmware, keys, configurations         │
│  ► Write flash chip → plant backdoor firmware                       │
│  ► Probe MISO during operation → sniff device responses             │
│                                                                     │
│  Common to ALL three:                                               │
│  • No authentication at the protocol level                          │
│  • No encryption at the protocol level                              │
│  • Physical access = full access                                    │
│  • Designed for trusted, on-board communication                     │
│  • Security was never in scope                                      │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Hypothetical IoT Device — Attack Points

```
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│                 Vault Security Controller                        │
│                 ════════════════════════                         │
│                                                                 │
│    ┌────────┐         ┌──────────────┐        ┌────────────┐    │
│    │ WiFi   │◄──SPI──►│     Main     │◄─UART─►│ Debug Port │    │
│    │ Module │         │     MCU      │        │ (4 pins)   │    │
│    │        │         │              │        │ ○ ○ ○ ○    │    │
│    └────────┘         │              │        └────────────┘    │
│        ▲              │              │             ▲             │
│        │              └──────┬───────┘             │             │
│   [SPI flash            │         │          [UART sniff        │
│    contains             │         │           → root shell      │
│    WiFi creds           │         │           → vault admin     │
│    & firmware]       I2C bus   I2C bus        credentials]      │
│                         │         │                             │
│                    ┌────┴──┐ ┌────┴──┐                          │
│                    │Vibra- │ │Door   │                          │
│                    │tion   │ │Contact│                          │
│                    │0x76   │ │0x48   │                          │
│                    └───────┘ └───────┘                          │
│                         ▲         ▲                             │
│                         │         │                             │
│                    [I2C bus sniff → read all sensor data]        │
│                    [I2C inject → spoof "door closed" status]    │
│                                                                 │
│    ┌──────────┐                                                 │
│    │ Config   │◄──SPI──── [Read chip → extract config,          │
│    │ Flash    │            encryption keys, stored creds]        │
│    │ (SPI)    │                                                  │
│    └──────────┘                                                 │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

    Attacker's playbook:
    1. Find the UART debug header → get a shell, steal credentials
    2. Probe I2C bus → sniff/spoof sensor data (vibration, door contacts)
    3. Dump SPI flash → extract firmware and encryption keys
    4. The vault is open.
```
