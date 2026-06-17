#include <Servo.h>

/*
  Qiita-style A0090 sweep using Arduino A2 as the servo signal pin.

  Note: A2 is used as a digital output pin here. RC servos do not use analog
  voltage communication; they use timed control pulses from Servo.h.
*/

Servo myservo;

int pos = 0;
int value = 0;

void setup() {
  myservo.attach(A2);
  Serial.begin(9600);
}

void loop() {
  for (pos = 0; pos <= 180; ++pos) {
    myservo.write(pos);
    value = myservo.read();
    Serial.println(value);
    delay(5);
  }

  for (pos = 180; pos >= 0; --pos) {
    myservo.write(pos);
    value = myservo.read();
    Serial.println(value);
    delay(5);
  }
}

