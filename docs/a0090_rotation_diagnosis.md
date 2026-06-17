# A0090 Rotation Diagnosis

If the A0090 rotates continuously in one direction when commanded with
`write(75)`, `write(90)`, or `write(105)`, stop the test and remove the servo
from the robot hand mechanism.

## What the behavior means

For a 180 degree positional servo:

- `1500 us` should move to center and stop.
- `1450 us` and `1550 us` should move to nearby angles and stop.
- `1000 us` and `2000 us` should move near the ends and stop.

For a continuous rotation servo:

- `1500 us` is stop or near-stop.
- Below `1500 us` rotates one direction.
- Above `1500 us` rotates the other direction.
- Angle commands such as `write(75)` behave like speed commands, not position
  commands.

## Safe probe

Flash this sketch:

```text
firmware/esp32c3_a0090_servo_pulse_probe/esp32c3_a0090_servo_pulse_probe.ino
```

It only tests `1500 us`, `1450 us`, and `1550 us`.

## How to judge

- If `1500 us` stops and `1450/1550 us` rotate opposite directions, treat this
  A0090 as a continuous rotation servo.
- If every pulse rotates the same direction, check GND, signal pin, board
  selection, and servo condition.
- If the shaft moves to a position and stops at each pulse, it is a positional
  servo and the previous angle range was too wide or mechanically constrained.

## If every pulse rotates the same direction

First, flash this sketch:

```text
firmware/esp32c3_a0090_servo_hold_1500us/esp32c3_a0090_servo_hold_1500us.ino
```

It outputs only `1500 us`.

For a normal positional servo, this should move to center and stop. For a
continuous rotation servo, this should stop or nearly stop. If it still rotates
continuously, do not attach the servo to the robot hand mechanism.

Flash this sketch next:

```text
firmware/esp32c3_a0090_servo_neutral_sweep/esp32c3_a0090_servo_neutral_sweep.ino
```

It holds `1300, 1350, 1400, ..., 1700 us` for 2.5 seconds each.

- If one pulse stops or nearly stops the servo, this is a continuous rotation
  servo with a shifted neutral point.
- If direction reverses across the sweep, this is a continuous rotation servo.
- If every pulse still rotates the same direction at about the same speed, the
  servo is not seeing the intended signal or the servo electronics are damaged.
