#include <Servo.h>

Servo myservo;

int pos = 0;
int value = 0;

void setup() {
  myservo.attach(2);
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

