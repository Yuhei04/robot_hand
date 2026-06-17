# robot_hand

Bring-up workspace for the robot hand hardware prototype.

The current prototype uses 9g A0090 servos with an ESP32-C3 controller. The
first milestone is to test one servo at a time with strict angle limits, then
move toward calibrated finger control and coordinated grasp motions.

## Repository layout

- `config/robot_hand.example.yaml`: hardware configuration template.
- `docs/motor_bringup_checklist.md`: safety and test checklist for motor tests.
- `docs/esp32c3_gpio_notes.md`: ESP32-C3 GPIO numbering and pin selection notes.
- `docs/control_roadmap.md`: staged path from motor bring-up to hand control.
- `firmware/esp32c3_a0090_servo_test/`: minimal ESP32-C3 servo test firmware.
- `scripts/hand_landmark_camera.py`: camera-based 21-point hand skeleton detector.
- `scripts/serial_motor_logger.py`: serial command sender and CSV logger.

## Suggested first test

1. Copy `config/robot_hand.example.yaml` to a working config file and fill in
   the actual servo pin, supply, and linkage details.
2. Power the servo from a separate 5 V supply and connect ESP32-C3 GND and
   servo GND together. Do not power the servo from the ESP32-C3 3.3 V pin.
3. Disconnect the tendon/linkage from the output horn if possible.
4. Flash `firmware/esp32c3_a0090_servo_test/esp32c3_a0090_servo_test.ino`
   after checking the `SERVO_PIN` constant.
5. Start with `LIMIT 80 100`, then test small moves such as `ANGLE 90`,
   `ANGLE 85`, `ANGLE 95`, and `STOP`.

If it does not move, first flash
`firmware/esp32c3_a0090_servo_smoke_test/esp32c3_a0090_servo_smoke_test.ino`.
That sketch moves automatically without serial commands, so it is useful for
checking wiring, power, and the selected GPIO. This smoke test uses
`ESP32Servo.h`, `attach(A0090_PIN, A0090_MIN, A0090_MAX)`, and `write(angle)`.
In Arduino IDE, install the `ESP32Servo` library before compiling.

There is also a smoother test based on the previous pan/tilt project:
`firmware/esp32c3_a0090_servo_like_previous/esp32c3_a0090_servo_like_previous.ino`.
It uses GPIO numbers directly, `ESP32Servo`, 500-2400 us pulses, and a narrow
75-105 degree range.

If the servo rotates continuously instead of moving to an angle and stopping,
use `firmware/esp32c3_a0090_servo_pulse_probe/esp32c3_a0090_servo_pulse_probe.ino`
and see `docs/a0090_rotation_diagnosis.md`.
If `1500 us` also rotates, use
`firmware/esp32c3_a0090_servo_neutral_sweep/esp32c3_a0090_servo_neutral_sweep.ino`
to search for a shifted neutral point.

To identify whether an A0090 is a 180 degree positional servo or a 360 degree
continuous rotation servo, use
`firmware/esp32c3_a0090_servo_type_check/esp32c3_a0090_servo_type_check.ino`.
If Arduino IDE reports that `Servo.h` is incompatible with ESP32, use the
library-free LEDC version:
`firmware/esp32c3_a0090_servo_type_check_ledc/esp32c3_a0090_servo_type_check_ledc.ino`.

For an Arduino Nano/Uno/Mega cross-check based on the Qiita `Servo.h` example,
use `firmware/arduino_a0090_type_check/arduino_a0090_type_check.ino`.
The same Qiita-style sweep without a button is
`firmware/arduino_a0090_qiita_no_button/arduino_a0090_qiita_no_button.ino`.
The A2-pin version is
`firmware/arduino_a0090_qiita_no_button_a2/arduino_a0090_qiita_no_button_a2.ino`.
If that still rotates continuously, use
`firmware/arduino_a0090_minimal_hold/arduino_a0090_minimal_hold.ino` and
`firmware/arduino_a0090_minimal_steps/arduino_a0090_minimal_steps.ino` to remove
button input and custom pulse ranges from the diagnosis.

For a one-servo ESP32 version of the ESP32Servo multi-servo example, use
`firmware/esp32_single_servo_sweep/esp32_single_servo_sweep.ino`.

For a GPIO21 LED blink test on ESP32-C3, use
`firmware/esp32c3_gpio21_led_test/esp32c3_gpio21_led_test.ino`.

For Seeed XIAO ESP32C3 after confirming that `D10` blinks an LED, use
`firmware/xiao_esp32c3_d10_servo_type_check/xiao_esp32c3_d10_servo_type_check.ino`
for the A0090 type check or
`firmware/xiao_esp32c3_d10_servo_sweep/xiao_esp32c3_d10_servo_sweep.ino` for a
simple sweep.
The one-servo version of the previous smooth pan/tilt motion is
`firmware/xiao_esp32c3_d10_single_smooth_servo/xiao_esp32c3_d10_single_smooth_servo.ino`.

For five serial-controlled servos named motor A-E, use
`firmware/xiao_esp32c3_five_motor_angle_control/xiao_esp32c3_five_motor_angle_control.ino`.
That five-motor sketch uses GPIO numbers directly, not `D` aliases.
For the three-finger robot hand driven from the PC camera bridge, use
`firmware/xiao_esp32c3_three_finger_camera_control/xiao_esp32c3_three_finger_camera_control.ino`.

If the servo has holding torque but still does not move, flash
`firmware/esp32c3_a0090_servo_pin_sweep/esp32c3_a0090_servo_pin_sweep.ino`.
Then move the servo signal wire across candidate GPIO pins one at a time.

Example logger usage:

```bash
python3 scripts/serial_motor_logger.py --port /dev/tty.usbmodemXXXX --command "PING" --duration 5
python3 scripts/serial_motor_logger.py --port /dev/tty.usbmodemXXXX --command "LIMIT 80 100" --command "ENABLE" --command "ANGLE 90" --duration 3
```

Install `pyserial` if the logger reports that the `serial` package is missing:

```bash
python3 -m pip install -r requirements.txt
```

## Camera hand skeleton detection

The camera detector uses MediaPipe Hands to detect 21 landmarks for each visible
hand. Keep this stage separate from servo commands until the landmark signal,
hand orientation, and joint-angle mapping have been validated.

1. Install the Python dependencies:

   ```bash
   python3 -m pip install -r requirements.txt
   ```

2. Start the camera preview:

   ```bash
   python3 scripts/hand_landmark_camera.py
   ```

The preview labels landmark IDs, draws the detected skeleton, and shows
per-finger curl/grasp-candidate status (`th`, `in`, `mi`, `ri`, `pi`). Press `q`
or Escape to exit. On macOS, allow camera access for the terminal or Python
process when prompted.

To record normalized image landmarks, world landmarks in meters, finger curl,
and grasp-candidate status as JSONL:

```bash
python3 scripts/hand_landmark_camera.py --jsonl logs/hand_landmarks.jsonl
```

For a short headless smoke test:

```bash
python3 scripts/hand_landmark_camera.py --no-display --jsonl logs/hand_test.jsonl --max-frames 30
```

Use `--camera 1` if the target camera is not index 0, `--num-hands 2` to detect
both hands, and `--no-mirror` when the image must preserve the camera's original
orientation. The default mirrored preview is intuitive for teleoperation, but
it also changes the image coordinate direction and must be accounted for during
robot-hand calibration.

## Convert hand landmarks to servo targets

After recording hand landmarks, convert the 21-point skeleton into robot-hand
axis values, per-finger grasp candidates, and conservative servo target angles.
The default layout matches the current three-finger robot hand:

- motor A: index finger open/close
- motor B: middle finger open/close
- motor C: thumb base flexion
- motor D: thumb distal two-joint flexion
- motor E: thumb orientation/opposition

```bash
python scripts/hand_pose_to_servo.py \
  --input logs/hand_landmarks.jsonl \
  --output logs/servo_targets.csv \
  --command-output logs/servo_commands.txt \
  --layout three-finger
```

For `--layout three-finger`, the calibrated default range is:

- A index: open 180, closed 0
- B middle: open 180, closed 0
- C thumb base: open 180, closed 50
- D thumb tip: open 0, closed 180
- E thumb orientation: open 120, closed 30

Tune these only after checking the robot hand's real mechanical limits:

```bash
python scripts/hand_pose_to_servo.py --layout three-finger
python scripts/hand_pose_to_servo.py --layout three-finger --open-angles 180,180,180,0,120 --closed-angles 0,0,50,180,30
```

The grasp columns are heuristic camera-only estimates. They indicate that a
finger is curled and its fingertip is near the palm, not guaranteed physical
contact with an object.

## Drive the robot hand from the camera

Use the live bridge after flashing
`firmware/xiao_esp32c3_three_finger_camera_control/xiao_esp32c3_three_finger_camera_control.ino`
to the ESP32-C3. First run without `--send` to confirm the camera mapping and
printed commands:

```bash
python scripts/hand_camera_servo_bridge.py
```

When the preview and commands look correct, connect the ESP32-C3 serial port and
explicitly enable sending:

```bash
python scripts/hand_camera_servo_bridge.py \
  --port /dev/cu.usbmodemXXXX \
  --send \
  --layout three-finger
```

The default command range for `--layout three-finger` is the calibrated range
above. The bridge rate-limits commands, smooths angles, limits per-command angle
steps, and sends open angles if hand tracking is lost:

```bash
python scripts/hand_camera_servo_bridge.py \
  --port /dev/cu.usbmodemXXXX \
  --send \
  --layout three-finger \
  --rate-hz 10 \
  --max-step-deg 3
```

If using PlatformIO, the default board is `esp32-c3-devkitm-1` in
`platformio.ini`. Change it if your ESP32-C3 board package uses another board
ID.
