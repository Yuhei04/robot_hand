#include <ESP32Servo.h>
// XIAO ESP32S3 Sense: use GPIO numbers, not D numbers.
// Start with D3/GPIO4 and D4/GPIO5 because they worked during motor testing.
static constexpr int PAN_PIN = 4;
static constexpr int TILT_PIN = 5;
static constexpr int SG90_MIN_US = 500;
static constexpr int SG90_MAX_US = 2400;
static constexpr int STEP_DEG = 5;
static constexpr int PAN_MIN_DEG = 0;
static constexpr int PAN_MAX_DEG = 180;
static constexpr int PAN_CENTER_DEG = 90;
static constexpr int TILT_MIN_DEG = 0;
static constexpr int TILT_MAX_DEG = 180;
static constexpr int TILT_CENTER_DEG = 135;
Servo panServo;
Servo tiltServo;
int panDeg = PAN_CENTER_DEG;
int tiltDeg = TILT_CENTER_DEG;
static void applyServos() {
  panDeg = constrain(panDeg, PAN_MIN_DEG, PAN_MAX_DEG);
  tiltDeg = constrain(tiltDeg, TILT_MIN_DEG, TILT_MAX_DEG);
  panServo.write(panDeg);
  tiltServo.write(tiltDeg);
  Serial.print("pan=");
  Serial.print(panDeg);
  Serial.print(" tilt=");
  Serial.println(tiltDeg);
}
static void printHelp() {
  Serial.println();
  Serial.println("Pan/Tilt calibration");
  Serial.println("Commands:");
  Serial.println("  a: pan left");
  Serial.println("  d: pan right");
  Serial.println("  w: tilt up");
  Serial.println("  s: tilt down");
  Serial.println("  c: center both");
  Serial.println("  p: print current angles");
  Serial.println();
}
void setup() {
  Serial.begin(115200);
  delay(1000);
  panServo.setPeriodHertz(50);
  tiltServo.setPeriodHertz(50);
  panServo.attach(PAN_PIN, SG90_MIN_US, SG90_MAX_US);
  tiltServo.attach(TILT_PIN, SG90_MIN_US, SG90_MAX_US);
  printHelp();
  applyServos();
}
void loop() {
  if (!Serial.available()) {
    return;
  }
  const char command = Serial.read();
  switch (command) {
    case 'a':
      panDeg -= STEP_DEG;
      applyServos();
      break;
    case 'd':
      panDeg += STEP_DEG;
      applyServos();
      break;
    case 'w':
      tiltDeg += STEP_DEG;
      applyServos();
      break;
    case 's':
      tiltDeg -= STEP_DEG;
      applyServos();
      break;
    case 'c':
      panDeg = PAN_CENTER_DEG;
      tiltDeg = TILT_CENTER_DEG;
      applyServos();
      break;
    case 'p':
      applyServos();
      break;
    case '\n':
    case '\r':
      break;
    default:
      printHelp();
      break;
  }
}