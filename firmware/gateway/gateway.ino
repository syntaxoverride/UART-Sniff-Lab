/*
 * gateway.ino — Vault Security Gateway
 *
 * Receives vibration sensor data over UART and displays alerts on:
 *   1. USB serial monitor (for the student at the laptop)
 *   2. A WiFi dashboard (for everyone in the room on their phones)
 *
 * Creates a WiFi AP: "Vault#-Security" (number set at compile time)
 * Dashboard at: http://firstnational.local (captive DNS)
 *
 * Wiring:
 *   Sensor ESP32 GPIO 17 (TX2) → This ESP32 GPIO 16 (RX2)
 *   Common GND between both boards
 *   This board connected to laptop via USB for serial monitor
 *
 * No external libraries required — uses built-in WiFi + WebServer.
 */

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

#define UART2_TX  17
#define UART2_RX  16
#define BAUD_RATE 9600

// VAULT_NUMBER is set at compile time via build flag: -DVAULT_NUMBER=7
#ifndef VAULT_NUMBER
#define VAULT_NUMBER 0
#endif

String apSSID = "Vault" + String(VAULT_NUMBER) + "-Security";
const char* AP_PASS = "firstnational";

WebServer server(80);
DNSServer dnsServer;

// Current vault state
String vaultStatus    = "WAITING";
int    vibrationCount = 0;
int    eventCount     = 0;
unsigned long msgCount = 0;
unsigned long lastUpdate = 0;

// Status history for the dashboard log
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
  json += "\"uptime\":" + String(millis() / 1000);
  json += "}";
  server.send(200, "application/json", json);
}

void parseVaultData(String data) {
  // Parse: VAULT_STATUS:CLEAR VIBRATION:0 EVENTS:0
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
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(BAUD_RATE, SERIAL_8N1, UART2_RX, UART2_TX);

  // Start WiFi AP
  WiFi.mode(WIFI_AP);
  delay(100);
  bool apStarted = WiFi.softAP(apSSID.c_str(), AP_PASS);
  delay(500);
  IPAddress ip = WiFi.softAPIP();

  // Captive DNS — resolve ALL domains to this AP's IP
  dnsServer.start(53, "*", ip);

  // Web server routes
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
  Serial.println("  VAULT SECURITY GATEWAY v2.0");
  Serial.println("  First National Bank - Vault #" + String(VAULT_NUMBER));
  Serial.println("===================================");
  Serial.println();
  if (apStarted) {
    Serial.println("WiFi AP: " + apSSID + "  [STARTED]");
  } else {
    Serial.println("WiFi AP: " + apSSID + "  [FAILED]");
  }
  Serial.println("Password: " + String(AP_PASS));
  Serial.println("Dashboard: http://firstnational.local");
  Serial.println();
  Serial.println("Listening on UART2 RX (GPIO 16)...");
  Serial.println();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  if (Serial2.available()) {
    String received = Serial2.readStringUntil('\n');
    received.trim();

    // Only process lines with actual data (ignore noise from floating RX pin)
    if (received.length() > 5 && (received.indexOf("VAULT") >= 0 || received.indexOf("[") >= 0)) {
      msgCount++;
      lastUpdate = millis();

      // Parse vault telemetry
      if (received.indexOf("VAULT_STATUS:") >= 0) {
        parseVaultData(received);
        addHistory("#" + String(msgCount) + " " + received);
      }

      // Serial monitor output
      if (received.indexOf("CRITICAL") >= 0) {
        Serial.println("!!! ALERT #" + String(msgCount) + " !!! " + received);
      } else if (received.indexOf("MEDIUM") >= 0) {
        Serial.println("**  WARN  #" + String(msgCount) + " **  " + received);
      } else {
        Serial.println("[SECURE]  #" + String(msgCount) + "     " + received);
      }
    }
  }

  delay(1);  // Yield time for WiFi stack
}
