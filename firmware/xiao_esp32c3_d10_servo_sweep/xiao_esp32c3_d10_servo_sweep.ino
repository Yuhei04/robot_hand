#include <ESP32Servo.h>

/*
  XIAO ESP32C3 + A0090 simple sweep using D10.
*/

const int SERVO_PIN = D10;
const int SERVO_MIN_US = 500;
const int SERVO_MAX_US = 2400;
const int STEP_DELAY_MS = 15;

Servo a0090;

void setup() {
  Serial.begin(115200);
  a0090.setPeriodHertz(50);
  a0090.attach(SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US);
}

void loop() {
  for (int pos = 0; pos <= 180; pos++) {
    a0090.write(pos);
    Serial.println(pos);
    delay(STEP_DELAY_MS);
  }

  for (int pos = 180; pos >= 0; pos--) {
    a0090.write(pos);
    Serial.println(pos);
    delay(STEP_DELAY_MS);
  }

  delay(1000);
}

