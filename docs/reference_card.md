# Serial Protocol Quick Reference — Islanders Cybersecurity Club

## Protocol Comparison

```
┌──────────────┬───────────────────┬───────────────────┬───────────────────┐
│              │       UART        │       I2C         │       SPI         │
├──────────────┼───────────────────┼───────────────────┼───────────────────┤
│ Signal Lines │ TX, RX            │ SDA, SCL          │ CLK, MOSI, MISO,  │
│              │                   │                   │ CS (per device)    │
├──────────────┼───────────────────┼───────────────────┼───────────────────┤
│ Typical Speed│ 9600–115200 baud  │ 100kHz–400kHz     │ 1–80 MHz          │
├──────────────┼───────────────────┼───────────────────┼───────────────────┤
│ Topology     │ Point-to-point    │ Multi-device bus   │ Controller +      │
│              │ (1:1)             │ (shared SDA/SCL)  │ peripherals (1:N) │
├──────────────┼───────────────────┼───────────────────┼───────────────────┤
│ Addressing   │ None              │ 7-bit device addr │ Chip Select (CS)  │
│              │                   │ (up to 127)       │ line per device   │
├──────────────┼───────────────────┼───────────────────┼───────────────────┤
│ Duplex       │ Full (TX + RX)    │ Half (shared SDA) │ Full (MOSI + MISO)│
├──────────────┼───────────────────┼───────────────────┼───────────────────┤
│ Clock        │ No clock line —   │ SCL from          │ CLK from          │
│              │ agreed baud rate  │ controller        │ controller        │
├──────────────┼───────────────────┼───────────────────┼───────────────────┤
│ Auth/Crypto  │ NONE              │ NONE              │ NONE              │
├──────────────┼───────────────────┼───────────────────┼───────────────────┤
│ Attack       │ Sniff TX/RX,      │ Bus sniff, device │ CS hijack,        │
│ Surface      │ inject, MITM      │ impersonation     │ flash chip dump   │
└──────────────┴───────────────────┴───────────────────┴───────────────────┘
```

## Common UART Baud Rates

```
  9600    ← Most common default for sensors/debug
  19200
  38400
  57600
  115200  ← Most common for debug consoles / root shells
  230400
  460800
  921600
```

If you get garbage on a serial monitor, you probably have the wrong baud rate.
Try 115200 first, then 9600. Cycle through the rest if neither works.

## Identifying UART on an Unknown PCB

```
What to look for:
┌──────────────────────────────────────────────────────────────────┐
│                                                                  │
│  1. 4-pin headers (often unlabeled):  ○ ○ ○ ○                   │
│     Typical order: VCC  TX  RX  GND                              │
│                                                                  │
│  2. Test pads labeled TX, RX, GND                                │
│                                                                  │
│  3. Single row of 3–4 pads near the edge of the board            │
│                                                                  │
│  4. Silkscreen labels: UART, CONSOLE, DEBUG, J1, JP1             │
│                                                                  │
│  How to identify pins without labels:                            │
│  • GND: continuity to any ground pad / USB shield                │
│  • VCC: reads 3.3V or 5V with multimeter                        │
│  • TX:  fluctuates on multimeter (device sending data)           │
│  • RX:  steady voltage (waiting for input)                       │
│                                                                  │
│  Or use a JTAGulator / Bus Pirate to auto-detect.                │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

## Tools for Hardware Hacking

| Tool | What It Does | Price Range |
|------|-------------|-------------|
| **Saleae Logic Analyzer** | Captures and decodes UART/I2C/SPI/etc. | $170–500 |
| **Bus Pirate** | Talks UART/I2C/SPI/JTAG from a terminal | ~$35 |
| **JTAGulator** | Auto-detects JTAG and UART pinouts | ~$150 |
| **flashrom** | Reads/writes SPI flash chips | Free (software) |
| **binwalk** | Extracts filesystems from firmware dumps | Free (software) |
| **PuTTY / minicom** | Serial terminal for UART connections | Free (software) |
| **Multimeter** | Identifies VCC, GND, signal pins | $20–50 |
| **Logic 2** | Saleae's protocol analyzer software | Free (software) |

## ESP32-WROOM GPIO Quick Reference

```
          ┌───────────────┐
          │   ESP32-WROOM │
          │   DevKitC     │
     3.3V │○             ○│ VIN (5V)
    GPIO0 │○             ○│ GND
  (UART1) │○ GPIO 4      ○│ GPIO 16 (UART2 RX)
          │○             ○│ GPIO 17 (UART2 TX)
          │○             ○│ GPIO 5
          │○             ○│ GPIO 18
          │○             ○│ GPIO 19
          │○             ○│ GPIO 21
  (UART0) │○ GPIO 1 (TX) ○│ GPIO 22
  (UART0) │○ GPIO 3 (RX) ○│ GPIO 23
          │○             ○│
      GND │○             ○│ GND
          └───────────────┘

  UART0: GPIO 1 (TX), GPIO 3 (RX) — USB serial (don't use for board-to-board)
  UART2: GPIO 17 (TX), GPIO 16 (RX) — free for board-to-board communication
  UART1: Defaults to flash pins — remap via Serial1.begin(baud, config, RX, TX)
```

## Further Reading

- **OWASP IoT Top 10** — owasp.org/www-project-internet-of-things
- **Wrong Baud** (blog) — wrongbaud.github.io — hardware CTF writeups
- **Flashback** (tool) — github.com/SecureThingz — firmware analysis
- **UART exploitation talk** — search "Exploiting UART" on YouTube
- **The Hardware Hacking Handbook** — Colin O'Flynn (No Starch Press)
