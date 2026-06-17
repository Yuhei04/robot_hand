# ESP32-C3 GPIO Notes

Use GPIO numbers in Arduino sketches unless the selected board package
explicitly documents a different pin numbering mode. For example:

```cpp
#define A0090_PIN 4
```

means ESP32-C3 `GPIO4`, not necessarily a board label named `D4`.

On Seeed XIAO ESP32C3, Arduino board definitions may provide `D0`, `D1`, ...,
`D10` aliases. If a blink test works with `D10`, use `D10` directly in sketches
for that board instead of guessing the underlying GPIO number.

## Pins to avoid for the first servo test

- `GPIO2`, `GPIO8`, `GPIO9`: ESP32-C3 strapping pins. External circuits on
  these pins can affect boot mode during reset or power-up.
- `GPIO18`, `GPIO19`: commonly used for native USB Serial/JTAG on ESP32-C3.
  Avoid these while USB upload or serial monitor is unstable.

## Better first candidates

Try these GPIOs first for one A0090 signal line:

```text
GPIO0, GPIO1, GPIO3, GPIO4, GPIO5, GPIO6, GPIO7, GPIO10
```

The exact physical header depends on the board. Always match the board's
pinout table to the GPIO number, not just the silkscreen position.

For the current XIAO ESP32C3 setup, `D10` has been confirmed with an LED blink
test. Prefer `D10` for the next servo signal test.

## Quick signal test

Before connecting the servo signal, test the candidate pin with an LED and a
series resistor:

```cpp
const int TEST_PIN = 4;

void setup() {
  pinMode(TEST_PIN, OUTPUT);
}

void loop() {
  digitalWrite(TEST_PIN, HIGH);
  delay(500);
  digitalWrite(TEST_PIN, LOW);
  delay(500);
}
```

If the LED does not blink, the selected number is not the physical pin you are
using, or the board setting is wrong.
