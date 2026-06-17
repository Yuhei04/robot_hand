/*
  ESP32-C3 + A0090 servo pin sweep test.

  Use this when the servo has holding torque but does not move and serial
  monitor is unavailable. The sketch outputs the same servo signal on several
  common ESP32-C3 GPIOs so a board-label/GPIO-number mismatch can be found.

  Connect the servo signal wire to one candidate pin at a time. Keep the servo
  horn disconnected from the mechanism while testing.

  Candidate GPIOs: 0, 1, 3, 4, 5, 6, 7, 10.

  GPIO2, GPIO8, and GPIO9 are strapping pins on ESP32-C3. GPIO18/GPIO19 are
  commonly used for USB Serial/JTAG. Avoid those pins for the first servo test.
*/

#include <Arduino.h>

#if defined(ESP_ARDUINO_VERSION_MAJOR)
#include <esp_arduino_version.h>
#endif

const int SERVO_PINS[] = {0, 1, 3, 4, 5, 6, 7, 10};
const int SERVO_PIN_COUNT = sizeof(SERVO_PINS) / sizeof(SERVO_PINS[0]);

const int SERVO_FREQ_HZ = 50;
const int SERVO_RESOLUTION_BITS = 16;

uint32_t pulseToDuty(int pulseUs) {
  const uint32_t maxDuty = (1UL << SERVO_RESOLUTION_BITS) - 1;
  const uint32_t periodUs = 1000000UL / SERVO_FREQ_HZ;
  return (uint32_t)((uint64_t)pulseUs * maxDuty / periodUs);
}

void setupPwm() {
  for (int i = 0; i < SERVO_PIN_COUNT; i++) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(SERVO_PINS[i], SERVO_FREQ_HZ, SERVO_RESOLUTION_BITS);
#else
    ledcSetup(i, SERVO_FREQ_HZ, SERVO_RESOLUTION_BITS);
    ledcAttachPin(SERVO_PINS[i], i);
#endif
  }
}

void writeAllServoPins(int pulseUs) {
  uint32_t duty = pulseToDuty(pulseUs);
  for (int i = 0; i < SERVO_PIN_COUNT; i++) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(SERVO_PINS[i], duty);
#else
    ledcWrite(i, duty);
#endif
  }
}

void setup() {
  setupPwm();
}

void loop() {
  writeAllServoPins(1500);
  delay(1200);

  writeAllServoPins(1000);
  delay(1200);

  writeAllServoPins(2000);
  delay(1200);
}
