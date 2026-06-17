#include <Arduino.h>

#if defined(ESP_ARDUINO_VERSION_MAJOR)
#include <esp_arduino_version.h>
#endif

/*
  A0090 type check for ESP32-C3 using LEDC directly.

  This sketch does not use Servo.h or ESP32Servo.h.

  Wiring:
    Servo red          -> external 5 V +
    Servo brown/black  -> external 5 V GND
    ESP32-C3 GND      -> external 5 V GND
    Servo signal      -> SERVO_PIN
*/

static constexpr int SERVO_PIN = 4;
// static constexpr int SERVO_PIN = 5;

static constexpr int SERVO_FREQ_HZ = 50;
static constexpr int SERVO_RESOLUTION_BITS = 16;
static constexpr int SERVO_CHANNEL = 0;

static uint32_t pulseToDuty(int pulseUs) {
  const uint32_t maxDuty = (1UL << SERVO_RESOLUTION_BITS) - 1;
  const uint32_t periodUs = 1000000UL / SERVO_FREQ_HZ;
  return (uint32_t)((uint64_t)pulseUs * maxDuty / periodUs);
}

static void writePulse(int pulseUs) {
  uint32_t duty = pulseToDuty(pulseUs);
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(SERVO_PIN, duty);
#else
  ledcWrite(SERVO_CHANNEL, duty);
#endif
}

static void setupServoPwm() {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(SERVO_PIN, SERVO_FREQ_HZ, SERVO_RESOLUTION_BITS);
#else
  ledcSetup(SERVO_CHANNEL, SERVO_FREQ_HZ, SERVO_RESOLUTION_BITS);
  ledcAttachPin(SERVO_PIN, SERVO_CHANNEL);
#endif
}

static void holdPulse(const char *label, int pulseUs, int holdMs) {
  Serial.print(label);
  Serial.print(",pulse_us=");
  Serial.println(pulseUs);
  writePulse(pulseUs);
  delay(holdMs);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  setupServoPwm();
  Serial.println("A0090 servo type check with LEDC");
  Serial.println("No Servo.h / ESP32Servo.h dependency.");

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

