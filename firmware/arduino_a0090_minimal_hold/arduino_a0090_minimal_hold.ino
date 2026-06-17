#include <Servo.h>

/*
  Minimal Arduino A0090 test.

  This intentionally uses only the default Servo.h behavior:
    - no custom min/max pulse values
    - no button input
    - one signal pin

  If a 180 degree positional servo still rotates continuously with this sketch,
  the problem is not the angle-sweep program.
*/

Servo a0090;

const int SERVO_PIN = 9;

void setup() {
  a0090.attach(SERVO_PIN);
  a0090.write(90);
}

void loop() {
  a0090.write(90);
  delay(3000);
}

