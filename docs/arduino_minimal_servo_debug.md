# Arduino Minimal Servo Debug

If the ESP32-C3 and Arduino both produce continuous rotation, remove every
extra variable from the program.

## 1. Hold center only

Flash:

```text
firmware/arduino_a0090_minimal_hold/arduino_a0090_minimal_hold.ino
```

Wiring:

```text
Arduino D9        -> servo signal
Arduino GND       -> external 5 V GND
Servo red         -> external 5 V +
Servo brown/black -> external 5 V GND
```

Expected for a 180 degree positional servo:

```text
move to center and stop
```

If it rotates continuously with only `write(90)`, the sweep program is not the
cause.

## 2. Small steps

Flash:

```text
firmware/arduino_a0090_minimal_steps/arduino_a0090_minimal_steps.ino
```

Expected:

```text
90 -> 80 -> 100, stopping at each position
```

## If it still rotates

- Verify the signal wire color. It should be yellow, orange, or white.
- Remove the button circuit completely.
- Use Arduino D9 exactly as written, not D2.
- Keep external 5 V GND and Arduino GND common.
- Try a known SG90 positional servo if available.
- If a known positional servo also rotates continuously, inspect the wiring and
  power supply before changing code again.

