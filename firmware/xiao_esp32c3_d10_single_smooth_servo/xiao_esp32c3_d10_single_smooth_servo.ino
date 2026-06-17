#include <ESP32Servo.h>

// XIAO ESP32C3: D10 has been verified with an LED blink test.
static constexpr int SERVO_PIN = D10;

static constexpr int SERVO_MIN_US = 500;
static constexpr int SERVO_MAX_US = 2400;

static constexpr int SERVO_MIN_DEG = 0;
static constexpr int SERVO_MAX_DEG = 180;
static constexpr int SERVO_CENTER_DEG = 90;

static constexpr int FRAME_DELAY_MS = 18;

Servo servo;
float servoDeg = SERVO_CENTER_DEG;

static float easeInOut(float t) {
  return t < 0.5f ? 2.0f * t * t : 1.0f - powf(-2.0f * t + 2.0f, 2.0f) * 0.5f;
}

static void writeServo(float deg) {
  servoDeg = constrain(deg, SERVO_MIN_DEG, SERVO_MAX_DEG);
  servo.write(static_cast<int>(servoDeg + 0.5f));
}

static void moveSmooth(float targetDeg, int durationMs) {
  const float startDeg = servoDeg;
  const int steps = max(1, durationMs / FRAME_DELAY_MS);

  for (int i = 0; i <= steps; i++) {
    const float t = static_cast<float>(i) / static_cast<float>(steps);
    const float e = easeInOut(t);
    const float nextDeg = startDeg + (targetDeg - startDeg) * e;
    writeServo(nextDeg);
    delay(FRAME_DELAY_MS);
  }
}

static void holdPose(int holdMs) {
  delay(holdMs);
}

void setup() {
  servo.setPeriodHertz(50);
  servo.attach(SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US);

  writeServo(SERVO_CENTER_DEG);
  delay(1500);
}

void loop() {
  moveSmooth(90, 900);
  holdPose(500);

  moveSmooth(25, 1800);
  holdPose(350);

  moveSmooth(155, 2600);
  holdPose(350);

  moveSmooth(90, 1400);
  holdPose(500);

  moveSmooth(40, 1500);
  holdPose(350);

  moveSmooth(140, 2200);
  holdPose(350);

  moveSmooth(90, 1500);
  holdPose(900);
}

