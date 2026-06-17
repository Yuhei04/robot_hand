#include <ESP32Servo.h>

/*
  Three-finger robot hand serial angle control for XIAO ESP32C3.

  This sketch is intended to be driven from the PC camera bridge:
    python scripts/hand_camera_servo_bridge.py --layout three-finger --port <port> --send

  Physical mapping:
    motor A: index finger flexion        open 180, closed 0
    motor B: middle finger flexion       open 180, closed 0
    motor C: thumb base flexion          open 180, closed 50
    motor D: thumb distal flexion        open 0,   closed 180
    motor E: thumb orientation/opposite  open 120, closed 30

  Commands over Serial at 115200 baud:
    a90                 : move motor A to 90 degrees
    a90 b120 c60 d10 e75: move multiple motors together
    open or home        : move all motors to the open pose
    close               : move all motors to the closed pose
    center              : midpoint of calibrated range
    s                   : print current angles
    ?                   : show help
*/

static constexpr int MOTOR_COUNT = 5;

// XIAO ESP32C3: use GPIO numbers, not D aliases.
// Change these to match the actual wiring.
static constexpr int MOTOR_PINS[MOTOR_COUNT] = {
  2,  // motor A: index finger
  3,  // motor B: middle finger
  4,  // motor C: thumb base
  5,  // motor D: thumb distal
  6   // motor E: thumb orientation
};

static constexpr int SERVO_MIN_US = 500;
static constexpr int SERVO_MAX_US = 2400;

static constexpr int MOTOR_OPEN_DEG[MOTOR_COUNT] = {
  180,  // A index open
  180,  // B middle open
  180,  // C thumb base open
  0,    // D thumb distal open
  120   // E thumb orientation open
};

static constexpr int MOTOR_CLOSED_DEG[MOTOR_COUNT] = {
  0,    // A index closed
  0,    // B middle closed
  50,   // C thumb base closed
  180,  // D thumb distal closed
  30    // E thumb orientation closed
};

static constexpr int MOTOR_MIN_DEG[MOTOR_COUNT] = {0, 0, 50, 0, 30};
static constexpr int MOTOR_MAX_DEG[MOTOR_COUNT] = {180, 180, 180, 180, 120};
static constexpr int MOTOR_CENTER_DEG[MOTOR_COUNT] = {90, 90, 115, 90, 75};

static constexpr char MOTOR_NAMES[MOTOR_COUNT] = {'A', 'B', 'C', 'D', 'E'};

Servo motors[MOTOR_COUNT];
int motorDeg[MOTOR_COUNT] = {
  MOTOR_OPEN_DEG[0],
  MOTOR_OPEN_DEG[1],
  MOTOR_OPEN_DEG[2],
  MOTOR_OPEN_DEG[3],
  MOTOR_OPEN_DEG[4]
};

String inputString = "";

static int motorIndexFromAxis(char axis) {
  axis = tolower(axis);
  if (axis < 'a' || axis > 'e') {
    return -1;
  }
  return axis - 'a';
}

static void printStatus() {
  for (int i = 0; i < MOTOR_COUNT; i++) {
    Serial.print("motor");
    Serial.print(MOTOR_NAMES[i]);
    Serial.print("=");
    Serial.print(motorDeg[i]);
    if (i < MOTOR_COUNT - 1) {
      Serial.print(" ");
    }
  }
  Serial.println();
}

static void applyMotor(int index) {
  motorDeg[index] = constrain(
    motorDeg[index],
    MOTOR_MIN_DEG[index],
    MOTOR_MAX_DEG[index]
  );
  motors[index].write(motorDeg[index]);

  Serial.print("motor");
  Serial.print(MOTOR_NAMES[index]);
  Serial.print("=");
  Serial.println(motorDeg[index]);
}

static void applyAllMotors() {
  for (int i = 0; i < MOTOR_COUNT; i++) {
    motorDeg[i] = constrain(motorDeg[i], MOTOR_MIN_DEG[i], MOTOR_MAX_DEG[i]);
    motors[i].write(motorDeg[i]);
  }
  printStatus();
}

static bool setMotorAngleFromToken(String token) {
  token.trim();
  token.toLowerCase();

  if (token.length() == 0) {
    return true;
  }

  int index = motorIndexFromAxis(token.charAt(0));
  if (index < 0) {
    Serial.print("Unknown motor: ");
    Serial.println(token);
    return false;
  }

  if (token.length() < 2) {
    Serial.print("Missing angle: ");
    Serial.println(token);
    return false;
  }

  int angle = token.substring(1).toInt();
  motorDeg[index] = constrain(angle, MOTOR_MIN_DEG[index], MOTOR_MAX_DEG[index]);
  return true;
}

static bool processMultiMotorCommand(String command) {
  command.replace(',', ' ');

  bool foundToken = false;
  bool ok = true;

  while (command.length() > 0) {
    command.trim();
    if (command.length() == 0) {
      break;
    }

    int splitAt = command.indexOf(' ');
    String token;
    if (splitAt < 0) {
      token = command;
      command = "";
    } else {
      token = command.substring(0, splitAt);
      command = command.substring(splitAt + 1);
    }

    token.trim();
    if (token.length() == 0) {
      continue;
    }

    foundToken = true;
    if (!setMotorAngleFromToken(token)) {
      ok = false;
    }
  }

  if (foundToken && ok) {
    applyAllMotors();
  }

  return foundToken;
}

static void openAllMotors() {
  for (int i = 0; i < MOTOR_COUNT; i++) {
    motorDeg[i] = MOTOR_OPEN_DEG[i];
  }
  applyAllMotors();
}

static void closeAllMotors() {
  for (int i = 0; i < MOTOR_COUNT; i++) {
    motorDeg[i] = MOTOR_CLOSED_DEG[i];
  }
  applyAllMotors();
}

static void centerAllMotors() {
  for (int i = 0; i < MOTOR_COUNT; i++) {
    motorDeg[i] = MOTOR_CENTER_DEG[i];
  }
  applyAllMotors();
}

static void printHelp() {
  Serial.println();
  Serial.println("Three-finger robot hand serial angle control");
  Serial.println("Mapping:");
  Serial.println("  A index flexion        open 180 closed 0");
  Serial.println("  B middle flexion       open 180 closed 0");
  Serial.println("  C thumb base flexion   open 180 closed 50");
  Serial.println("  D thumb distal flexion open 0 closed 180");
  Serial.println("  E thumb orientation    open 120 closed 30");
  Serial.println("Commands:");
  Serial.println("  a90   : move motor A to 90 degrees");
  Serial.println("  a90 b120 c60 d10 e75: move multiple motors together");
  Serial.println("  open/home: open all motors");
  Serial.println("  close    : close all motors");
  Serial.println("  center   : midpoint of calibrated range");
  Serial.println("  s        : print current angles");
  Serial.println("  ?        : show help");
  Serial.println();
}

static void processCommand(String command) {
  command.trim();
  command.toLowerCase();

  if (command.length() == 0) {
    return;
  }

  if (command == "open" || command == "home") {
    openAllMotors();
    return;
  }

  if (command == "close") {
    closeAllMotors();
    return;
  }

  if (command == "center") {
    centerAllMotors();
    return;
  }

  if (command == "s") {
    printStatus();
    return;
  }

  if (command == "?") {
    printHelp();
    return;
  }

  if (command.indexOf(' ') >= 0 || command.indexOf(',') >= 0) {
    processMultiMotorCommand(command);
    return;
  }

  int index = motorIndexFromAxis(command.charAt(0));
  if (index >= 0) {
    if (command.length() < 2) {
      Serial.println("Missing angle");
      return;
    }

    int angle = command.substring(1).toInt();
    motorDeg[index] = constrain(angle, MOTOR_MIN_DEG[index], MOTOR_MAX_DEG[index]);
    applyMotor(index);
    return;
  }

  Serial.println("Unknown command");
  printHelp();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  for (int i = 0; i < MOTOR_COUNT; i++) {
    motors[i].setPeriodHertz(50);
    motors[i].attach(MOTOR_PINS[i], SERVO_MIN_US, SERVO_MAX_US);
  }

  printHelp();
  openAllMotors();
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      processCommand(inputString);
      inputString = "";
    } else {
      inputString += c;
    }
  }
}
