# Control Roadmap

This roadmap keeps the prototype bring-up incremental. Each stage should leave
behind logs and configuration updates so later control issues can be traced
back to hardware facts.

## Stage 0: Hardware inventory

- Servo model, rated voltage, stall current, horn geometry.
- ESP32-C3 board variant, signal pin, and available power rails.
- Sensor type: encoder, potentiometer, Hall sensor, current sensor, force
  sensor, or no sensor.
- Mechanical transmission: tendon, linkage, pulley radius, gear train, joint
  limit, return spring.

Exit condition: every servo has an entry in `config/robot_hand.example.yaml` or
the copied working config.

## Stage 1: Servo signal test

- Test one servo at a time.
- Confirm enable, stop, home angle, positive angle direction, and angle limits.
- Log current draw and direction for small angle commands.
- Keep commands inside the configured angle window.

Exit condition: every servo can be stopped reliably and its positive direction
is known.

## Stage 2: Sensor validation

- Read position or velocity sensor values while moving by hand.
- Confirm units, zero point, sign, resolution, and wraparound behavior.
- Check repeatability after returning to the same physical pose.

Exit condition: the controller can estimate position or velocity with the
correct sign.

## Stage 3: Single-servo calibrated control

- Calibrate open, neutral, and closed angles.
- Add software angle limits before moving with tendon load.
- Tune one servo path without the full hand load first.
- Log target angle, measured joint state if available, command, and current.

Exit condition: one servo follows small angle commands without driving into a
mechanical limit.

## Stage 4: Finger-level control

- Add tendon/linkage load.
- Calibrate open and closed positions.
- Add software joint limits.
- Add low-speed trajectories instead of step commands.

Exit condition: one finger can open and close repeatedly within safe limits.

## Stage 5: Camera hand landmark validation

- Detect the operator's hand without sending servo commands.
- Record the 21 hand landmarks, handedness, confidence, and detection dropouts.
- Confirm the intended camera mirroring and coordinate directions.
- Derive finger joint angles from landmark vectors and validate them against
  known open, neutral, and closed poses.
- Add filtering, rate limits, and a loss-of-tracking stop condition before
  connecting the signal to the robot hand.

Exit condition: the camera signal produces stable, bounded finger targets and
tracking loss can be detected reliably.

## Stage 6: Hand-level coordination

- Run multiple fingers with conservative current and angle limits.
- Add grasp primitives such as open, pinch, wrap, and release.
- Add contact or current-based stop conditions.

Exit condition: the hand can run repeatable low-force grasp motions with logs.
