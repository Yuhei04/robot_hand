#include <ESP32Servo.h>

/*
  Five servo angle control for XIAO ESP32C3.

  Commands over Serial Monitor at 115200 baud:
    a90    : move motor A to 90 degrees
    b120   : move motor B to 120 degrees
    a90 b120 c45 : move multiple motors together
    grab   : run grab motion
    release: run release motion
    c      : center all motors
    s      : print current angles
    ?      : show help

  Set Serial Monitor line ending to "Newline" or "Both NL & CR".
*/

static constexpr int MOTOR_COUNT = 5;

// XIAO ESP32C3: use GPIO numbers, not D numbers.
// Change these to match the actual wiring.
static constexpr int MOTOR_PINS[MOTOR_COUNT] = {
  2,  // motor A
  3,  // motor B
  4,  // motor C
  5,  // motor D
  6   // motor E
};

static constexpr int SERVO_MIN_US = 500;
static constexpr int SERVO_MAX_US = 2400;

static constexpr int MOTOR_MIN_DEG[MOTOR_COUNT] = {0, 0, 0, 0, 0};
static constexpr int MOTOR_MAX_DEG[MOTOR_COUNT] = {180, 180, 180, 180, 180};
static constexpr int MOTOR_CENTER_DEG[MOTOR_COUNT] = {90, 90, 90, 90, 90};
static constexpr int FRAME_DELAY_MS = 20;

static constexpr char MOTOR_NAMES[MOTOR_COUNT] = {'A', 'B', 'C', 'D', 'E'};

struct MotionStep {
  int angle[MOTOR_COUNT];
  int durationMs;
  int holdMs;
};

// Edit these angles to match the robot hand mechanics.
// static const MotionStep GRAB_MOTION[] = {
//   {{180, 180, 180, 0, 30}, 300, 100},
//   {{180, 180, 90, 0, 30}, 700, 100},
//   {{70, 70, 90, 40, 30}, 900, 100},
// };
static const MotionStep GRAB_MOTION[] = {
  {{180, 180, 180, 0, 30}, 300, 100},
  {{180, 180, 90, 0, 30}, 700, 100},
  {{40, 40, 70, 60, 30}, 900, 100},
};

static const MotionStep RELEASE_MOTION[] = {
  {{180, 180, 160, 0, 30}, 900, 100},
  {{180, 180, 180, 0, 30}, 700, 100},
  {{180, 180, 180, 0, 120}, 300, 100},
};

Servo motors[MOTOR_COUNT];
int motorDeg[MOTOR_COUNT] = {
  MOTOR_CENTER_DEG[0],
  MOTOR_CENTER_DEG[1],
  MOTOR_CENTER_DEG[2],
  MOTOR_CENTER_DEG[3],
  MOTOR_CENTER_DEG[4]
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

static void moveAllMotorsSmooth(const int target[MOTOR_COUNT], int durationMs) {
  int start[MOTOR_COUNT];
  for (int i = 0; i < MOTOR_COUNT; i++) {
    start[i] = motorDeg[i];
  }

  int steps = max(1, durationMs / FRAME_DELAY_MS);
  for (int step = 1; step <= steps; step++) {
    for (int i = 0; i < MOTOR_COUNT; i++) {
      long next = start[i] + ((long)(target[i] - start[i]) * step) / steps;
      motorDeg[i] = constrain((int)next, MOTOR_MIN_DEG[i], MOTOR_MAX_DEG[i]);
      motors[i].write(motorDeg[i]);
    }
    delay(FRAME_DELAY_MS);
  }

  printStatus();
}

static void runMotion(const char *name, const MotionStep motion[], int stepCount) {
  Serial.print("motion=");
  Serial.println(name);

  for (int i = 0; i < stepCount; i++) {
    Serial.print("step=");
    Serial.println(i + 1);
    moveAllMotorsSmooth(motion[i].angle, motion[i].durationMs);
    delay(motion[i].holdMs);
  }

  Serial.print("motion_done=");
  Serial.println(name);
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

static void centerAllMotors() {
  for (int i = 0; i < MOTOR_COUNT; i++) {
    motorDeg[i] = MOTOR_CENTER_DEG[i];
  }
  applyAllMotors();
}

static void printHelp() {
  Serial.println();
  Serial.println("Five motor angle control");
  Serial.println("Commands:");
  Serial.println("  a90   : move motor A to 90 degrees");
  Serial.println("  b120  : move motor B to 120 degrees");
  Serial.println("  a90 b120 c45: move multiple motors together");
  Serial.println("  c45   : move motor C to 45 degrees");
  Serial.println("  d135  : move motor D to 135 degrees");
  Serial.println("  e90   : move motor E to 90 degrees");
  Serial.println("  grab  : run grab motion");
  Serial.println("  release/open: run release motion");
  Serial.println("  center: center all motors");
  Serial.println("  s     : print current angles");
  Serial.println("  ?     : show help");
  Serial.println();
}

static void processCommand(String command) {
  command.trim();
  command.toLowerCase();

  if (command.length() == 0) {
    return;
  }

  if (command == "center" || command == "home") {
    centerAllMotors();
    return;
  }

  if (command == "grab" || command == "close") {
    runMotion("grab", GRAB_MOTION, sizeof(GRAB_MOTION) / sizeof(GRAB_MOTION[0]));
    return;
  }

  if (command == "release" || command == "open") {
    runMotion("release", RELEASE_MOTION, sizeof(RELEASE_MOTION) / sizeof(RELEASE_MOTION[0]));
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
  centerAllMotors();
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
