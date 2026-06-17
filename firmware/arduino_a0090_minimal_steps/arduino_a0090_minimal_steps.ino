#include <Servo.h>

/*
  Minimal Arduino A0090 step test using default Servo.h timing.
*/

Servo a0090;

const int SERVO_PIN = 9;

void setup() {
  a0090.attach(SERVO_PIN);
  a0090.write(90);
  delay(3000);
}

void loop() {
  a0090.write(90);
  delay(2000);

  a0090.write(80);
  delay(2000);

  a0090.write(100);
  delay(2000);
}

