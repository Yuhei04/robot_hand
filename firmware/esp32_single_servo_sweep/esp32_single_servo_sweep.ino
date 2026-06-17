#include <ESP32Servo.h>

/*
  Single servo version of the ESP32Servo multi-servo example.

  Wiring:
    Servo red          -> external 5 V +
    Servo brown/black  -> external 5 V GND
    ESP32 GND         -> external 5 V GND
    Servo signal      -> SERVO_PIN
*/

Servo servo1;

// For ESP32-C3, use GPIO numbers. Change this to the pin you wired.
#if defined(CONFIG_IDF_TARGET_ESP32C3)
const int SERVO_PIN = 4;
#else
const int SERVO_PIN = 15;
#endif

const int MIN_US = 1000;
const int MAX_US = 2000;
const int STEP_DELAY_MS = 15;

void setup() {
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  Serial.begin(115200);
  servo1.setPeriodHertz(50);
  servo1.attach(SERVO_PIN, MIN_US, MAX_US);
}

void loop() {
  for (int pos = 0; pos <= 180; pos += 1) {
    servo1.write(pos);
    Serial.println(pos);
    delay(STEP_DELAY_MS);
  }

  for (int pos = 180; pos >= 0; pos -= 1) {
    servo1.write(pos);
    Serial.println(pos);
    delay(STEP_DELAY_MS);
  }

  delay(1000);
}

