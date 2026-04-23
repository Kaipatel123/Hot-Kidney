#include <Arduino.h>

#include <HX711.h>

// XIAO ESP32-C3 wiring: DT->D2, SCK->D3, VCC->3.3V/5V, GND->GND
const uint8_t HX711_DT  = D2;
const uint8_t HX711_SCK = D3;

// HX711 gain=128, AVDD=5V:
//   FS differential = ±20 mV → ±2^23 counts
//   → 419,430 counts/mV
// Deltran at 5V excitation: 25 µV/mmHg → ~10,486 counts/mmHg
// Use ONLY as a sanity-check estimate until calibrated.
const float COUNTS_PER_MMHG_NOMINAL = 10486.0f;

HX711 scale;
long zero_offset = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);

  Serial.println(F("HX711 + Deltran DPT-100 bring-up"));
  Serial.println(F("Open pressure port to atmosphere before tare."));
  Serial.println(F("Serial commands: t=tare"));

  scale.begin(HX711_DT, HX711_SCK);
  scale.set_gain(128);

  // Discard warm-up samples (regulator + PGA settling)

// Flush settling transient before tare
for (int i = 0; i < 20; i++) {
  while (!scale.is_ready()) { delay(5); }
  scale.read();
}
delay(500);
zero_offset = scale.read_average(30);



  Serial.println(F("Taring..."));
  
  Serial.print(F("Zero offset (raw counts): "));
  Serial.println(zero_offset);
  Serial.println(F("Streaming..."));
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 't') {
      Serial.println(F("Re-taring..."));
      zero_offset = scale.read_average(20);
      Serial.print(F("Zero offset: "));
      Serial.println(zero_offset);
    }
  }

  if (scale.is_ready()) {
    long raw   = scale.read();
    long delta = raw - zero_offset;
    float mmHg_est = delta / COUNTS_PER_MMHG_NOMINAL;

    Serial.print(F("raw="));        Serial.print(raw);
    Serial.print(F("\tdelta="));    Serial.print(delta);
    Serial.print(F("\t~mmHg="));    Serial.println(mmHg_est, 2);
  }
}