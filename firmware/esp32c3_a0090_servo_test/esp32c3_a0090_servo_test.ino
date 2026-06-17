/*
  ESP32-C3 + 9g A0090 servo bring-up firmware using ESP32Servo.

  Wire the servo signal to A0090_PIN. Power the servo from a separate 5 V
  supply that can handle servo current spikes, and connect ESP32-C3 GND and
  servo GND together. Do not power the servo from the ESP32-C3 3.3 V pin.

  Serial commands:
    HELP
    PING
    ENABLE
    DISABLE
    STOP
    HOME
    LIMIT <min_deg> <max_deg>
    ANGLE <deg>
    PULSE <microseconds>
*/

#include <ESP32Servo.h>

#define A0090_PIN 4
#define A0090_MIN 500
#define A0090_MAX 2400

const long BAUD_RATE = 115200;
const float DEFAULT_MIN_ANGLE_DEG = 45.0;
const float DEFAULT_MAX_ANGLE_DEG = 135.0;
const float DEFAULT_HOME_ANGLE_DEG = 90.0;

Servo a0090;
bool outputEnabled = false;
float minAngleDeg = DEFAULT_MIN_ANGLE_DEG;
float maxAngleDeg = DEFAULT_MAX_ANGLE_DEG;
float currentAngleDeg = DEFAULT_HOME_ANGLE_DEG;
int currentPulseUs = 0;

void enableServoOutput() {
  if (!a0090.attached()) {
    a0090.attach(A0090_PIN, A0090_MIN, A0090_MAX);
  }
}

void disableServoOutput() {
  currentPulseUs = 0;
  a0090.detach();
}

int angleToPulse(float angleDeg) {
  float limitedAngle = constrain(angleDeg, 0.0, 180.0);
  float ratio = limitedAngle / 180.0;
  return A0090_MIN + (int)((A0090_MAX - A0090_MIN) * ratio);
}

void applyAngle(float angleDeg) {
  currentAngleDeg = constrain(angleDeg, minAngleDeg, maxAngleDeg);
  currentPulseUs = angleToPulse(currentAngleDeg);
  if (!outputEnabled) {
    disableServoOutput();
    return;
  }
  enableServoOutput();
  a0090.write(currentAngleDeg);
}

void applyPulse(int pulseUs) {
  currentPulseUs = constrain(pulseUs, A0090_MIN, A0090_MAX);
  currentAngleDeg = -1.0;
  if (!outputEnabled) {
    disableServoOutput();
    return;
  }
  enableServoOutput();
  a0090.writeMicroseconds(currentPulseUs);
}

void printStatus(const char *prefix) {
  Serial.print(prefix);
  Serial.print(",enabled=");
  Serial.print(outputEnabled ? 1 : 0);
  Serial.print(",angle_deg=");
  Serial.print(currentAngleDeg, 1);
  Serial.print(",pulse_us=");
  Serial.print(currentPulseUs);
  Serial.print(",min_deg=");
  Serial.print(minAngleDeg, 1);
  Serial.print(",max_deg=");
  Serial.println(maxAngleDeg, 1);
}

void printHelp() {
  Serial.println("commands: HELP PING ENABLE DISABLE STOP HOME LIMIT <min_deg> <max_deg> ANGLE <deg> PULSE <microseconds>");
}

bool parseTwoFloats(const String &text, float &first, float &second) {
  int splitAt = text.indexOf(' ');
  if (splitAt < 0) {
    return false;
  }
  first = text.substring(0, splitAt).toFloat();
  second = text.substring(splitAt + 1).toFloat();
  return true;
}

void handleCommand(String line) {
  line.trim();
  line.toUpperCase();
  if (line.length() == 0) {
    return;
  }

  if (line == "HELP") {
    printHelp();
    return;
  }

  if (line == "PING") {
    printStatus("pong");
    return;
  }

  if (line == "ENABLE") {
    outputEnabled = true;
    applyAngle(currentAngleDeg);
    printStatus("enabled");
    return;
  }

  if (line == "DISABLE" || line == "STOP") {
    outputEnabled = false;
    disableServoOutput();
    printStatus("disabled");
    return;
  }

  if (line == "HOME") {
    applyAngle(DEFAULT_HOME_ANGLE_DEG);
    printStatus("home");
    return;
  }

  if (line.startsWith("LIMIT ")) {
    float requestedMin = 0.0;
    float requestedMax = 0.0;
    if (!parseTwoFloats(line.substring(6), requestedMin, requestedMax)) {
      Serial.println("error,usage=LIMIT <min_deg> <max_deg>");
      return;
    }
    minAngleDeg = constrain(requestedMin, 0.0, 180.0);
    maxAngleDeg = constrain(requestedMax, 0.0, 180.0);
    if (minAngleDeg > maxAngleDeg) {
      float tmp = minAngleDeg;
      minAngleDeg = maxAngleDeg;
      maxAngleDeg = tmp;
    }
    applyAngle(currentAngleDeg);
    printStatus("limit");
    return;
  }

  if (line.startsWith("ANGLE ")) {
    applyAngle(line.substring(6).toFloat());
    printStatus("angle");
    return;
  }

  if (line.startsWith("PULSE ")) {
    applyPulse(line.substring(6).toInt());
    printStatus("pulse");
    return;
  }

  Serial.print("error,unknown_command=");
  Serial.println(line);
}

void setup() {
  Serial.begin(BAUD_RATE);
  a0090.setPeriodHertz(50);
  disableServoOutput();
  Serial.println("ready,esp32c3_a0090_servo_test");
  printHelp();
  printStatus("boot");
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    handleCommand(line);
  }
}

