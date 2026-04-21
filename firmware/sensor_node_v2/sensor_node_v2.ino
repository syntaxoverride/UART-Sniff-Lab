/*
 * sensor_node_v2.ino — Phase 2: "The Tap"
 *
 * Same as sensor_node.ino but periodically transmits a simulated
 * authentication handshake with plaintext credentials over UART.
 * This is the firmware the instructor pre-flashes before the lab.
 *
 * Uses edge detection (LOW→HIGH transitions) to filter sensor ringing.
 * Escalates to CRITICAL after sustained vibration (3 seconds).
 *
 * Wiring: identical to sensor_node.ino
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
#define THRESH_LOW       1
#define THRESH_MEDIUM    6
#define SUSTAINED_SECS   3

unsigned long eventCount = 0;
unsigned long sampleWindow = 500;
bool lastPinState = false;
unsigned long sustainedStart = 0;
bool inSustained = false;
unsigned long lastAuthTime = 0;
const unsigned long AUTH_INTERVAL = 15000;

void sendAuth() {
  Serial2.println("[VAULT-SENSOR] Authenticating to security gateway...");
  delay(300);
  Serial2.println("[VAULT-SENSOR] DEVICE_ID: VAULT-7-VIBRATION-01");
  delay(200);
  Serial2.println("[VAULT-SENSOR] USER: vault_admin");
  delay(200);
  Serial2.println("[VAULT-SENSOR] PASS: F1r$tN@t10nal#2026!");
  delay(200);
  Serial2.println("[VAULT-SENSOR] AUTH_TOKEN: eyJhbGciOiJIUzI1NiJ9.dmF1bHRfYWRtaW4.v4u1t_s3cr3t");
  delay(300);
  Serial2.println("[VAULT-SENSOR] Session established. Monitoring vault...");
  Serial2.println();

  Serial.println("[DEBUG] Auth handshake transmitted over UART2");
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(BAUD_RATE, SERIAL_8N1, UART2_RX, UART2_TX);

  pinMode(VIBRATION_PIN, INPUT_PULLUP);

  Serial.println();
  Serial.println("===================================");
  Serial.println("  VAULT SENSOR v2.0");
  Serial.println("  Phase 2: With Authentication");
  Serial.println("===================================");
  Serial.println("Vibration sensor on GPIO 14 | UART2 TX on GPIO 17");
  Serial.println();
  Serial.println("!! This firmware sends plaintext credentials !!");
  Serial.println("!! Students will sniff these from the TX line !!");
  Serial.println();

  delay(1000);
  sendAuth();
  lastAuthTime = millis();
}

void loop() {
  // Periodic re-authentication
  if (millis() - lastAuthTime >= AUTH_INTERVAL) {
    Serial2.println();
    Serial2.println("[VAULT-SENSOR] Session expired. Re-authenticating...");
    delay(500);
    sendAuth();
    lastAuthTime = millis();
  }

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

  String msg = "VAULT_STATUS:" + status +
               " VIBRATION:" + String(transitions) +
               " EVENTS:" + String(eventCount);

  Serial2.println(msg);
  Serial.println("[VAULT] " + msg);

  delay(1);
}
