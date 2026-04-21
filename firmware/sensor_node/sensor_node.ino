/*
 * sensor_node.ino — Phase 1: "The Honest System"
 *
 * Bank vault vibration sensor. Monitors a vibration sensor and
 * transmits status over UART2 to the vault security gateway.
 *
 * Uses edge detection (LOW→HIGH transitions) to filter sensor ringing.
 * Escalates to CRITICAL after sustained vibration (3 seconds).
 *
 * Wiring:
 *   Vibration sensor VCC    → ESP32 3.3V
 *   Vibration sensor SIGNAL → ESP32 GPIO 14
 *   Vibration sensor GND    → ESP32 GND
 *   ESP32 GPIO 17 (TX2) → Gateway ESP32 GPIO 16 (RX2)
 *   ESP32 GND → Gateway ESP32 GND
 *
 * No external libraries required.
 */

#define VIBRATION_PIN  14
#define UART2_TX       17
#define UART2_RX       16
#define BAUD_RATE      9600

// Alert thresholds — calibrated for typical vibration sensor modules
#define THRESH_LOW       1    // transitions to trigger LOW (light brush)
#define THRESH_MEDIUM    6    // transitions to trigger MEDIUM (tap)
#define SUSTAINED_SECS   3    // seconds of continuous MEDIUM+ to escalate to CRITICAL

unsigned long eventCount = 0;
unsigned long sampleWindow = 500;
bool lastPinState = false;
unsigned long sustainedStart = 0;
bool inSustained = false;

void setup() {
  Serial.begin(115200);
  Serial2.begin(BAUD_RATE, SERIAL_8N1, UART2_RX, UART2_TX);

  pinMode(VIBRATION_PIN, INPUT_PULLUP);

  Serial.println();
  Serial.println("===================================");
  Serial.println("  VAULT SENSOR v1.0");
  Serial.println("  Phase 1: The Honest System");
  Serial.println("===================================");
  Serial.println("Vibration sensor on GPIO 14 | UART2 TX on GPIO 17");
  Serial.println("Monitoring vault integrity...");
  Serial.println();
}

void loop() {
  // Count rising-edge transitions over the sample window
  unsigned long transitions = 0;
  unsigned long start = millis();

  while (millis() - start < sampleWindow) {
    bool currentState = digitalRead(VIBRATION_PIN) == HIGH;
    if (currentState && !lastPinState) {
      transitions++;
    }
    lastPinState = currentState;
    delay(5);
  }

  // Determine threat level
  String status;
  if (transitions < THRESH_LOW) {
    status = "CLEAR";
    inSustained = false;
  } else if (transitions < THRESH_MEDIUM) {
    status = "LOW";
    inSustained = false;
  } else {
    // MEDIUM or above — check for sustained vibration
    if (!inSustained) {
      sustainedStart = millis();
      inSustained = true;
    }

    if (millis() - sustainedStart >= SUSTAINED_SECS * 1000UL) {
      status = "CRITICAL";
      eventCount++;
    } else {
      status = "MEDIUM";
    }
  }

  // Build message
  String msg = "VAULT_STATUS:" + status +
               " VIBRATION:" + String(transitions) +
               " EVENTS:" + String(eventCount);

  Serial2.println(msg);
  Serial.println("[VAULT] " + msg);

  delay(1);
}
