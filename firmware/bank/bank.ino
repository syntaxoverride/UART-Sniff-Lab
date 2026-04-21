/*
 * bank.ino, "The Bank Board" (White Case)
 *
 * All-in-one vault security system:
 *   - Reads vibration sensor on GPIO 14
 *   - Serves WiFi dashboard at http://firstnational.local
 *   - Transmits sensor data + auth credentials over UART2 TX (GPIO 17)
 *   - Listens on UART2 RX (GPIO 16) — if external data arrives,
 *     it OVERRIDES the local sensor and the dashboard displays
 *     whatever the attacker sends. This is the injection/MITM surface.
 *
 * Wiring:
 *   Vibration sensor VCC    → ESP32 3.3V
 *   Vibration sensor SIGNAL → ESP32 GPIO 14
 *   Vibration sensor GND    → ESP32 GND
 *   ESP32 GPIO 17 (TX2)     → Blue board GPIO 16 (RX2) [student wires this]
 *   ESP32 GPIO 16 (RX2)     ← Blue board GPIO 17 (TX2) [injection/MITM]
 *   ESP32 GND               → Blue board GND           [student wires this]
 */

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

#define VIBRATION_PIN  14
#define UART2_TX       17
#define UART2_RX       16
#define BAUD_RATE      9600

#ifndef VAULT_NUMBER
#define VAULT_NUMBER 1
#endif

String apSSID = "Vault" + String(VAULT_NUMBER) + "-Security";
const char* AP_PASS = "firstnational";

WebServer server(80);
DNSServer dnsServer;

// Alert thresholds — calibrated for typical vibration sensor modules
// Uses raw pulse counting (number of LOW reads in a 200ms window)
#define THRESH_LOW       1    // pulses to trigger LOW
#define THRESH_MEDIUM    5    // pulses to trigger MEDIUM
#define SUSTAINED_SECS   3    // seconds of continuous MEDIUM+ for CRITICAL

// Current vault state (displayed on dashboard)
String vaultStatus    = "WAITING";
int    vibrationCount = 0;
int    eventCount     = 0;
unsigned long msgCount = 0;
unsigned long lastUpdate = 0;
unsigned long sampleWindow = 200;  // shorter window for responsiveness

// Sensor reading state
unsigned long sustainedStart = 0;
bool inSustained = false;

// UART auth state
unsigned long lastAuthTime = 0;
const unsigned long AUTH_INTERVAL = 15000;

// UART RX override — when attacker injects data, dashboard shows it
bool uartOverrideActive = false;
unsigned long lastUartRx = 0;
const unsigned long UART_OVERRIDE_TIMEOUT = 3000;  // Fall back to local sensor
                                                    // if no UART data for 3s

// Data source indicator for dashboard
String dataSource = "LOCAL SENSOR";

// Status history
#define HISTORY_SIZE 20
String history[HISTORY_SIZE];
int historyHead = 0;
int historyCount = 0;

void addHistory(String entry) {
  history[historyHead] = entry;
  historyHead = (historyHead + 1) % HISTORY_SIZE;
  if (historyCount < HISTORY_SIZE) historyCount++;
}

void handleRoot() {
  String statusColor;
  String statusIcon;
  String bgPulse = "";

  if (vaultStatus == "CRITICAL") {
    statusColor = "#ff1744";
    statusIcon = "&#x26A0;";
    bgPulse = "animation: pulse 0.5s infinite alternate;";
  } else if (vaultStatus == "MEDIUM") {
    statusColor = "#ff9100";
    statusIcon = "&#x26A0;";
  } else if (vaultStatus == "LOW") {
    statusColor = "#ffd600";
    statusIcon = "&#x25CF;";
  } else if (vaultStatus == "CLEAR") {
    statusColor = "#00e676";
    statusIcon = "&#x2713;";
  } else {
    statusColor = "#757575";
    statusIcon = "&#x25CF;";
  }

  unsigned long uptime = millis() / 1000;
  unsigned long ago = (millis() - lastUpdate) / 1000;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='2'>";
  html += "<title>Vault #" + String(VAULT_NUMBER) + " Security</title>";
  html += "<style>";
  html += "@keyframes pulse { from {opacity:1;} to {opacity:0.6;} }";
  html += "* { margin:0; padding:0; box-sizing:border-box; }";
  html += "body { background:#0a0a0a; color:#e0e0e0; font-family:'Courier New',monospace; padding:20px; }";
  html += ".header { text-align:center; border-bottom:2px solid #333; padding-bottom:15px; margin-bottom:20px; }";
  html += ".header h1 { font-size:1.4em; color:#90caf9; letter-spacing:2px; }";
  html += ".header h2 { font-size:0.9em; color:#666; margin-top:5px; }";
  html += ".status-box { text-align:center; padding:30px; border:2px solid " + statusColor + "; border-radius:10px; margin:20px 0; " + bgPulse + " }";
  html += ".status-label { font-size:0.8em; color:#888; text-transform:uppercase; letter-spacing:3px; }";
  html += ".status-value { font-size:3em; font-weight:bold; color:" + statusColor + "; margin:10px 0; }";
  html += ".metrics { display:grid; grid-template-columns:1fr 1fr; gap:10px; margin:20px 0; }";
  html += ".metric { background:#1a1a1a; border:1px solid #333; border-radius:8px; padding:15px; text-align:center; }";
  html += ".metric-label { font-size:0.7em; color:#888; text-transform:uppercase; letter-spacing:2px; }";
  html += ".metric-value { font-size:1.8em; font-weight:bold; color:#fff; margin-top:5px; }";
  html += ".log { background:#0d0d0d; border:1px solid #222; border-radius:8px; padding:15px; margin-top:20px; max-height:300px; overflow-y:auto; }";
  html += ".log-title { font-size:0.8em; color:#888; text-transform:uppercase; letter-spacing:2px; margin-bottom:10px; }";
  html += ".log-entry { font-size:0.75em; padding:3px 0; border-bottom:1px solid #1a1a1a; }";
  html += ".log-critical { color:#ff1744; }";
  html += ".log-medium { color:#ff9100; }";
  html += ".log-clear { color:#00e676; }";
  html += "footer { text-align:center; margin-top:20px; font-size:0.7em; color:#444; }";
  html += "</style></head><body>";

  html += "<div class='header'>";
  html += "<h1>FIRST NATIONAL BANK</h1>";
  html += "<h2>Vault #" + String(VAULT_NUMBER) + " Security Monitor</h2>";
  html += "</div>";

  html += "<div class='status-box'>";
  html += "<div class='status-label'>Vault Status</div>";
  html += "<div class='status-value'>" + String(statusIcon) + " " + vaultStatus + "</div>";
  html += "</div>";

  html += "<div class='metrics'>";

  html += "<div class='metric'>";
  html += "<div class='metric-label'>Vibration Level</div>";
  html += "<div class='metric-value'>" + String(vibrationCount) + "</div>";
  html += "</div>";

  html += "<div class='metric'>";
  html += "<div class='metric-label'>Security Events</div>";
  html += "<div class='metric-value'>" + String(eventCount) + "</div>";
  html += "</div>";

  html += "<div class='metric'>";
  html += "<div class='metric-label'>Messages Received</div>";
  html += "<div class='metric-value'>" + String(msgCount) + "</div>";
  html += "</div>";

  html += "<div class='metric'>";
  html += "<div class='metric-label'>Last Update</div>";
  html += "<div class='metric-value'>" + String(ago) + "s</div>";
  html += "</div>";

  html += "</div>";

  html += "<div class='log'>";
  html += "<div style='display:flex;justify-content:space-between;align-items:center;margin-bottom:10px;'>";
  html += "<div class='log-title' style='margin-bottom:0;'>Event Log (newest first)</div>";
  html += "<a href='/clear' style='color:#ff5252;font-size:0.75em;text-decoration:none;border:1px solid #ff5252;padding:4px 10px;border-radius:4px;'>CLEAR LOG</a>";
  html += "</div>";
  for (int i = 0; i < historyCount; i++) {
    int idx = (historyHead - 1 - i + HISTORY_SIZE) % HISTORY_SIZE;
    String cssClass = "log-entry";
    if (history[idx].indexOf("CRITICAL") >= 0) cssClass += " log-critical";
    else if (history[idx].indexOf("MEDIUM") >= 0) cssClass += " log-medium";
    else if (history[idx].indexOf("CLEAR") >= 0) cssClass += " log-clear";
    html += "<div class='" + cssClass + "'>" + history[idx] + "</div>";
  }
  html += "</div>";

  html += "<footer>Uptime: " + String(uptime) + "s | AP: " + apSSID + " | http://firstnational.local</footer>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleApi() {
  String json = "{";
  json += "\"status\":\"" + vaultStatus + "\",";
  json += "\"vibration\":" + String(vibrationCount) + ",";
  json += "\"events\":" + String(eventCount) + ",";
  json += "\"messages\":" + String(msgCount) + ",";
  json += "\"source\":\"" + dataSource + "\",";
  json += "\"uptime\":" + String(millis() / 1000);
  json += "}";
  server.send(200, "application/json", json);
}

void sendAuth() {
  Serial2.println("[VAULT-SENSOR] Authenticating to security gateway...");
  delay(300);
  Serial2.println("[VAULT-SENSOR] DEVICE_ID: VAULT-" + String(VAULT_NUMBER) + "-VIBRATION-01");
  delay(200);
  Serial2.println("[VAULT-SENSOR] USER: vault_admin");
  delay(200);
  Serial2.println("[VAULT-SENSOR] PASS: F1r$tN@t10nal#2026!");
  delay(200);
  Serial2.println("[VAULT-SENSOR] AUTH_TOKEN: eyJhbGciOiJIUzI1NiJ9.dmF1bHRfYWRtaW4.v4u1t_s3cr3t");
  delay(300);
  Serial2.println("[VAULT-SENSOR] Session established. Monitoring vault...");
  Serial2.println();
}

// Parse incoming UART data and override dashboard display
void parseUartOverride(String data) {
  if (data.indexOf("VAULT_STATUS:") < 0) return;

  int statusStart = data.indexOf("VAULT_STATUS:") + 13;
  int statusEnd = data.indexOf(' ', statusStart);
  if (statusStart > 12 && statusEnd > statusStart) {
    vaultStatus = data.substring(statusStart, statusEnd);
  }

  int vibStart = data.indexOf("VIBRATION:") + 10;
  int vibEnd = data.indexOf(' ', vibStart);
  if (vibStart > 9 && vibEnd > vibStart) {
    vibrationCount = data.substring(vibStart, vibEnd).toInt();
  }

  int evtStart = data.indexOf("EVENTS:") + 7;
  if (evtStart > 6) {
    eventCount = data.substring(evtStart).toInt();
  }

  msgCount++;
  lastUpdate = millis();
  lastUartRx = millis();
  uartOverrideActive = true;
  dataSource = "UART OVERRIDE";

  addHistory("#" + String(msgCount) + " " + data);
}

void readLocalSensor() {
  // Count state changes (any transition) over the sample window
  // Works regardless of sensor polarity or resting state
  unsigned long pulses = 0;
  bool lastState = digitalRead(VIBRATION_PIN);
  unsigned long start = millis();

  while (millis() - start < sampleWindow) {
    bool currentState = digitalRead(VIBRATION_PIN);
    if (currentState != lastState) {
      pulses++;
      lastState = currentState;
    }
    delayMicroseconds(500);  // sample at ~2kHz
  }

  // Determine threat level from local sensor
  if (pulses < THRESH_LOW) {
    vaultStatus = "CLEAR";
    inSustained = false;
  } else if (pulses < THRESH_MEDIUM) {
    vaultStatus = "LOW";
    inSustained = false;
  } else {
    if (!inSustained) {
      sustainedStart = millis();
      inSustained = true;
    }
    if (millis() - sustainedStart >= SUSTAINED_SECS * 1000UL) {
      vaultStatus = "CRITICAL";
      eventCount++;
    } else {
      vaultStatus = "MEDIUM";
    }
  }

  vibrationCount = pulses;
  msgCount++;
  lastUpdate = millis();
  dataSource = "LOCAL SENSOR";

  String msg = "VAULT_STATUS:" + vaultStatus +
               " VIBRATION:" + String(pulses) +
               " EVENTS:" + String(eventCount);

  addHistory("#" + String(msgCount) + " " + msg);

  // Send over UART2 TX — this is the wire students will tap
  Serial2.println(msg);

  // USB debug output (includes raw pin for debugging)
  Serial.println("[VAULT] " + msg + " RAW:" + String(digitalRead(VIBRATION_PIN)));
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(BAUD_RATE, SERIAL_8N1, UART2_RX, UART2_TX);
  pinMode(VIBRATION_PIN, INPUT_PULLUP);

  WiFi.mode(WIFI_AP);
  delay(100);
  bool apStarted = WiFi.softAP(apSSID.c_str(), AP_PASS);
  delay(500);
  IPAddress ip = WiFi.softAPIP();

  dnsServer.start(53, "*", ip);

  server.on("/", handleRoot);
  server.on("/api", handleApi);
  server.on("/clear", []() {
    historyHead = 0;
    historyCount = 0;
    eventCount = 0;
    msgCount = 0;
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "Redirecting...");
  });
  server.begin();

  Serial.println();
  Serial.println("===================================");
  Serial.println("  FIRST NATIONAL BANK v1.0");
  Serial.println("  Vault #" + String(VAULT_NUMBER) + " Security System");
  Serial.println("===================================");
  Serial.println();
  Serial.println("WiFi AP: " + apSSID + (apStarted ? "  [STARTED]" : "  [FAILED]"));
  Serial.println("Password: " + String(AP_PASS));
  Serial.println("Dashboard: http://firstnational.local");
  Serial.println("Vibration sensor on GPIO 14");
  Serial.println("UART TX on GPIO 17 (sniff target)");
  Serial.println("UART RX on GPIO 16 (injection target)");
  Serial.println();

  // Initial auth handshake over UART
  delay(1000);
  sendAuth();
  lastAuthTime = millis();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  // Periodic re-authentication over UART TX (only when not overridden)
  if (!uartOverrideActive && millis() - lastAuthTime >= AUTH_INTERVAL) {
    Serial2.println();
    Serial2.println("[VAULT-SENSOR] Session expired. Re-authenticating...");
    delay(500);
    sendAuth();
    lastAuthTime = millis();
  }

  // ALWAYS check UART RX first — attacker injection takes priority
  while (Serial2.available()) {
    String uartData = Serial2.readStringUntil('\n');
    uartData.trim();
    if (uartData.length() > 5 && uartData.indexOf("VAULT_STATUS:") >= 0) {
      parseUartOverride(uartData);
      Serial.println("[OVERRIDE] Dashboard hijacked: " + uartData);
    }
  }

  // If UART override is active, skip local sensor entirely
  if (uartOverrideActive) {
    // Check if override has timed out (no UART data for 3s)
    if (millis() - lastUartRx > UART_OVERRIDE_TIMEOUT) {
      uartOverrideActive = false;
      dataSource = "LOCAL SENSOR";
      Serial.println("[OVERRIDE] UART timeout — returning to local sensor");
    }
    delay(10);
    return;
  }

  // Normal operation — read local sensor and transmit over UART TX
  readLocalSensor();

  delay(1);
}
