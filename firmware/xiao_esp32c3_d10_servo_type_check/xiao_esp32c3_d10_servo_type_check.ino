#include <ESP32Servo.h>

/*
  XIAO ESP32C3 + A0090 servo type check using D10.

  Use this after confirming that the LED blink example works on D10.

  Wiring:
    Servo red          -> external 5 V +
    Servo brown/black  -> external 5 V GND
    XIAO GND          -> external 5 V GND
    Servo signal      -> D10
*/

const int SERVO_PIN = D10;

const int SERVO_MIN_US = 500;
const int SERVO_MAX_US = 2400;

Servo a0090;

void holdPulse(const char *label, int pulseUs, int holdMs) {
  Serial.print(label);
  Serial.print(",pulse_us=");
  Serial.println(pulseUs);
  a0090.writeMicroseconds(pulseUs);
  delay(holdMs);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  a0090.setPeriodHertz(50);
  a0090.attach(SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US);

  Serial.println("XIAO ESP32C3 D10 A0090 type check");
  holdPulse("neutral_start", 1500, 4000);
}

void loop() {
  holdPulse("neutral", 1500, 4000);
  holdPulse("slightly_low", 1450, 3000);
  holdPulse("neutral", 1500, 3000);
  holdPulse("slightly_high", 1550, 3000);
  holdPulse("neutral", 1500, 3000);
  holdPulse("low", 1200, 3000);
  holdPulse("neutral", 1500, 3000);
  holdPulse("high", 1800, 3000);
  holdPulse("neutral", 1500, 5000);
}

