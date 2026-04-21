/*
 * calibrate.ino — Raw vibration sensor sampling
 *
 * Prints both raw pulse count and transition count
 * every 500ms so we can see what the sensor actually produces.
 */

#define VIBRATION_PIN 14

bool lastState = false;

void setup() {
  Serial.begin(115200);
  pinMode(VIBRATION_PIN, INPUT_PULLUP);

  Serial.println();
  Serial.println("VIBRATION CALIBRATION");
  Serial.println("Sampling every 500ms");
  Serial.println("pulses = raw HIGH count | transitions = LOW->HIGH edges");
  Serial.println("=============================================");
}

void loop() {
  unsigned long pulses = 0;
  unsigned long transitions = 0;
  unsigned long start = millis();

  while (millis() - start < 500) {
    bool current = digitalRead(VIBRATION_PIN) == HIGH;

    if (current) pulses++;
    if (current && !lastState) transitions++;

    lastState = current;
    delay(2);  // ~500 samples per window
  }

  if (pulses > 0 || transitions > 0) {
    Serial.println("pulses=" + String(pulses) + "  transitions=" + String(transitions));
  } else {
    Serial.println("--- quiet ---");
  }
}
