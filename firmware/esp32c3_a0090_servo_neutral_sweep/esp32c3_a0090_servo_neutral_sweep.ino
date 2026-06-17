#include <ESP32Servo.h>

// ESP32-C3: use GPIO numbers, not board labels.
static constexpr int A0090_PIN = 4;
// static constexpr int A0090_PIN = 5;

static constexpr int SERVO_MIN_US = 500;
static constexpr int SERVO_MAX_US = 2400;
static constexpr int STEP_HOLD_MS = 2500;

Servo a0090;

static void holdPulse(int pulseUs) {
  a0090.writeMicroseconds(pulseUs);
  delay(STEP_HOLD_MS);
}

void setup() {
  Serial.begin(115200);
  a0090.setPeriodHertz(50);
  a0090.attach(A0090_PIN, SERVO_MIN_US, SERVO_MAX_US);
}

void loop() {
  // For a continuous rotation servo, one of these values may stop or nearly
  // stop the output shaft. If every value rotates the same direction, suspect
  // signal wiring, board/pin selection, or servo damage.
  holdPulse(1300);
  holdPulse(1350);
  holdPulse(1400);
  holdPulse(1450);
  holdPulse(1500);
  holdPulse(1550);
  holdPulse(1600);
  holdPulse(1650);
  holdPulse(1700);
}

