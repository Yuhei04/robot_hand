#include <ESP32Servo.h>

// ESP32-C3: use GPIO numbers, not board labels.
static constexpr int A0090_PIN = 4;
// static constexpr int A0090_PIN = 5;

static constexpr int SERVO_MIN_US = 500;
static constexpr int SERVO_MAX_US = 2400;

Servo a0090;

static void holdPulse(int pulseUs, int holdMs) {
  a0090.writeMicroseconds(pulseUs);
  delay(holdMs);
}

void setup() {
  Serial.begin(115200);
  a0090.setPeriodHertz(50);
  a0090.attach(A0090_PIN, SERVO_MIN_US, SERVO_MAX_US);

  // Start at neutral. A positional 180 degree servo should move to center and
  // stop. A continuous rotation servo should stop or nearly stop here.
  holdPulse(1500, 4000);
}

void loop() {
  holdPulse(1500, 3000);
  holdPulse(1450, 2500);
  holdPulse(1500, 3000);
  holdPulse(1550, 2500);
}

