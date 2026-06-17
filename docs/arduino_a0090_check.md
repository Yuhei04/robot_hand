# Arduino A0090 Check

This check follows the Arduino Nano `Servo.h` style from the Qiita reference.

Reference:

```text
https://qiita.com/mix_dvd/items/537b5c41048e98e11105
```

## Sketch

Use:

```text
firmware/arduino_a0090_type_check/arduino_a0090_type_check.ino
```

For the Qiita example without a button, use:

```text
firmware/arduino_a0090_qiita_no_button/arduino_a0090_qiita_no_button.ino
```

For the same sketch using Arduino `A2` as the servo signal pin, use:

```text
firmware/arduino_a0090_qiita_no_button_a2/arduino_a0090_qiita_no_button_a2.ino
```

`A2` is used as a digital output here. RC servos use timed pulses, not analog
voltage communication.

## Wiring

```text
Arduino GND       -> external 5 V GND
Servo red         -> external 5 V +
Servo brown/black -> external 5 V GND
Servo signal      -> Arduino D2
```

Optional button:

```text
Button one side -> Arduino D3
Button other side -> 5 V
```

The sketch uses `pinMode(3, INPUT)` to match the Qiita style. If the button is
not connected, the sketch still cycles through `90, 75, 90, 105` degrees.

## Judgment

- If it moves to `75, 90, 105` and stops, the servo is a 180 degree positional
  servo and the ESP32-C3 side is the problem.
- If it rotates continuously at `90`, the servo is a 360 degree continuous
  rotation type, damaged, or not receiving a valid signal.
- If Arduino also behaves the same as ESP32-C3 with multiple servos, re-check
  servo signal wire color and external power wiring.
