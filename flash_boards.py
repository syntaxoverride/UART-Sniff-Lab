#!/usr/bin/env python3
"""
flash_boards.py — Batch flashing tool for Wire Tap UART Lab

Flashes both board types:
  - Bank Board (white case): sensor + dashboard + UART, unique Vault#-Security AP
  - Attacker Board (blue case): multi-mode firmware (SNIFF/INJECT/MITM via button)

Usage:
  python3 flash_boards.py

The script will:
  1. Detect the connected ESP32 serial port
  2. Ask which board type to flash (bank or attacker)
  3. For bank boards: ask for the vault number (1-20)
  4. Compile, flash, and verify
  5. Loop for the next board
"""

import subprocess
import sys
import os
import glob
import time
import serial

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
FQBN = "esp32:esp32:esp32"

FIRMWARE = {
    "bank": os.path.join(SCRIPT_DIR, "firmware", "bank", "bank.ino"),
    "attacker": os.path.join(SCRIPT_DIR, "firmware", "attacker", "attacker.ino"),
}


def find_serial_port():
    """Find connected ESP32 serial port."""
    patterns = ["/dev/cu.usbserial-*", "/dev/cu.wchusbserial-*",
                "/dev/cu.SLAB_USBtoUART*", "/dev/ttyUSB*", "/dev/ttyACM*"]
    ports = []
    for pattern in patterns:
        ports.extend(glob.glob(pattern))

    if not ports:
        return None
    if len(ports) == 1:
        return ports[0]

    print("\nMultiple ports detected:")
    for i, p in enumerate(ports, 1):
        print(f"  {i}. {p}")
    while True:
        choice = input("Select port number: ").strip()
        if choice.isdigit() and 1 <= int(choice) <= len(ports):
            return ports[int(choice) - 1]
        print("Invalid selection.")


def compile_and_flash(firmware_path, port, vault_number=None):
    """Compile and flash firmware to the board."""
    compile_cmd = ["arduino-cli", "compile", "--fqbn", FQBN]

    if vault_number is not None:
        compile_cmd += ["--build-property",
                        f"build.extra_flags=-DVAULT_NUMBER={vault_number}"]

    compile_cmd.append(firmware_path)

    print("\n  Compiling...", end=" ", flush=True)
    result = subprocess.run(compile_cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print("FAILED")
        print(result.stderr)
        return False
    print("OK")

    upload_cmd = ["arduino-cli", "upload", "--fqbn", FQBN,
                  "--port", port, firmware_path]

    print("  Flashing...", end=" ", flush=True)
    result = subprocess.run(upload_cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print("FAILED")
        print(result.stderr)
        return False
    print("OK")

    return True


def verify_flash(port, expected_text):
    """Read serial output and verify the startup banner."""
    print("  Verifying...", end=" ", flush=True)
    try:
        ser = serial.Serial(port, 115200, timeout=2)
        time.sleep(0.5)

        ser.setDTR(False)
        time.sleep(0.1)
        ser.setDTR(True)
        time.sleep(2)
        ser.reset_input_buffer()

        output = ""
        end = time.time() + 5
        while time.time() < end:
            raw = ser.readline()
            if raw:
                line = raw.decode("utf-8", errors="ignore").strip()
                output += line + "\n"
                if expected_text in line:
                    ser.close()
                    print("OK")
                    return True

        ser.close()

        if expected_text in output:
            print("OK")
            return True
        else:
            print("WARNING - banner not found")
            print(f"  Expected to see: {expected_text}")
            return False

    except Exception as e:
        print(f"FAILED ({e})")
        return False


def print_banner():
    print()
    print("=" * 50)
    print("  WIRE TAP LAB — Board Flasher")
    print("  Cybersecurity Club")
    print("=" * 50)


def print_summary(flashed):
    """Print summary of all flashed boards."""
    if not flashed:
        return
    print()
    print("=" * 50)
    print("  FLASH SUMMARY")
    print("=" * 50)
    for entry in flashed:
        print(f"  {entry}")
    print(f"\n  Total: {len(flashed)} boards flashed")
    print("=" * 50)


def main():
    print_banner()
    flashed = []

    while True:
        print()
        print("-" * 50)
        print("  Plug in ONE board and press Enter")
        print("  (or type 'q' to quit)")
        print("-" * 50)

        user_input = input("> ").strip().lower()
        if user_input in ("q", "quit", "exit"):
            break

        port = find_serial_port()
        if not port:
            print("  ERROR: No ESP32 detected. Check USB cable.")
            continue

        print(f"  Detected: {port}")

        print()
        print("  Which board?")
        print("    1. Bank     (white case, sensor + dashboard)")
        print("    2. Attacker (blue case, SNIFF/INJECT/MITM)")
        print()
        board_type = input("  Select (1 or 2): ").strip()

        if board_type == "1":
            while True:
                vault_num = input("  Vault number (1-20): ").strip()
                if vault_num.isdigit() and 1 <= int(vault_num) <= 20:
                    vault_num = int(vault_num)
                    break
                print("  Enter a number between 1 and 20.")

            print()
            print(f"  Flashing BANK BOARD — Vault#{vault_num}-Security")

            success = compile_and_flash(FIRMWARE["bank"], port,
                                        vault_number=vault_num)

            if success:
                verify_flash(port, f"Vault #{vault_num}")
                label = f"Bank #{vault_num:>2d}  WiFi: Vault{vault_num}-Security  (white case)"
                flashed.append(label)
                print()
                print(f"  >>> DONE: Vault{vault_num}-Security <<<")
                print(f"  >>> WiFi pass: firstnational <<<")
                print(f"  >>> Put in WHITE case <<<")
                print(f"  >>> Label: BANK #{vault_num} <<<")

        elif board_type == "2":
            print()
            print("  Flashing ATTACKER BOARD")

            success = compile_and_flash(FIRMWARE["attacker"], port)

            if success:
                verify_flash(port, "ATTACKER BOARD")
                flashed.append("Attacker  (blue case)")
                print()
                print("  >>> DONE: Attacker board ready <<<")
                print("  >>> Put in BLUE case <<<")
                print("  >>> BOOT button cycles: SNIFF → INJECT → MITM <<<")

        else:
            print("  Invalid selection.")
            continue

    print_summary(flashed)
    print("\nDone. Happy hacking.")


if __name__ == "__main__":
    main()
