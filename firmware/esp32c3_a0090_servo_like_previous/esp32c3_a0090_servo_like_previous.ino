#include <ESP32Servo.h>

// ESP32-C3: use GPIO numbers, not board labels such as D4.
// Try GPIO4 first. If it does not move, try GPIO5 next.
static constexpr int A0090_PIN = 4;
// static constexpr int A0090_PIN = 5;

static constexpr int A0090_MIN_US = 500;
static constexpr int A0090_MAX_US = 2400;

// Start narrow to avoid hitting the robot hand mechanism.
static constexpr int CENTER_DEG = 90;
static constexpr int LOW_DEG = 75;
static constexpr int HIGH_DEG = 105;
static constexpr int FRAME_DELAY_MS = 18;

Servo a0090;
float currentDeg = CENTER_DEG;

static float easeInOut(float t) {
  return t < 0.5f ? 2.0f * t * t : 1.0f - powf(-2.0f * t + 2.0f, 2.0f) * 0.5f;
}

static void writeServo(float deg) {
  currentDeg = constrain(deg, LOW_DEG, HIGH_DEG);
  a0090.write(static_cast<int>(currentDeg + 0.5f));
}

static void moveSmooth(float targetDeg, int durationMs) {
  const float startDeg = currentDeg;
  const int steps = max(1, durationMs / FRAME_DELAY_MS);

  for (int i = 0; i <= steps; i++) {
    const float t = static_cast<float>(i) / static_cast<float>(steps);
    const float e = easeInOut(t);
    writeServo(startDeg + (targetDeg - startDeg) * e);
    delay(FRAME_DELAY_MS);
  }
}

void setup() {
  Serial.begin(115200);
  a0090.setPeriodHertz(50);
  a0090.attach(A0090_PIN, A0090_MIN_US, A0090_MAX_US);

  writeServo(CENTER_DEG);
  delay(1500);
}

void loop() {
  moveSmooth(CENTER_DEG, 800);
  delay(500);

  moveSmooth(LOW_DEG, 1200);
  delay(500);

  moveSmooth(HIGH_DEG, 1800);
  delay(500);

  moveSmooth(CENTER_DEG, 1200);
  delay(900);
}

