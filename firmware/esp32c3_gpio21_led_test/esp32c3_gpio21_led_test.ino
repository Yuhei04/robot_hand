#include <Arduino.h>

const int LED_PIN = 21;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.println("GPIO21 LED test start");
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  Serial.println("GPIO21 HIGH");
  delay(500);

  digitalWrite(LED_PIN, LOW);
  Serial.println("GPIO21 LOW");
  delay(500);
}

