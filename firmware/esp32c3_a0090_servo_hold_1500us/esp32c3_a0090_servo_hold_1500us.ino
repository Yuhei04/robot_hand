#include <ESP32Servo.h>

// ESP32-C3: use GPIO numbers, not board labels.
static constexpr int A0090_PIN = 4;
// static constexpr int A0090_PIN = 5;

static constexpr int SERVO_MIN_US = 500;
static constexpr int SERVO_MAX_US = 2400;

Servo a0090;

void setup() {
  Serial.begin(115200);
  a0090.setPeriodHertz(50);
  a0090.attach(A0090_PIN, SERVO_MIN_US, SERVO_MAX_US);
  a0090.writeMicroseconds(1500);
}

void loop() {
  a0090.writeMicroseconds(1500);
  delay(1000);
}

