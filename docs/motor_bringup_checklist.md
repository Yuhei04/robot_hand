# Servo Bring-Up Checklist

Use this checklist before any A0090 servo test. The goal is to prove wiring,
signal behavior, direction, and mechanical clearance before applying meaningful
force to the hand mechanism.

## Before applying servo power

- Confirm servo supply polarity and rated voltage.
- Power the servo from a separate 5 V supply that can handle startup and stall
  current spikes.
- Do not power the servo from the ESP32-C3 3.3 V pin.
- Confirm ESP32-C3 GND and servo supply GND are connected together.
- Confirm signal pin wiring against the config file.
- Set the bench supply current limit low enough that a wiring error will not
  damage the servo or mechanism.
- Make sure an emergency stop or immediate power disconnect is reachable.
- If possible, disconnect tendons, gears, or linkages for the first motion test.
- Make sure the firmware boots with servo output disabled.

## First servo motion test

1. Flash the servo test firmware with the correct signal pin.
2. Open the serial logger.
3. Send `PING` and confirm a response.
4. Send `LIMIT 80 100`.
5. Send `ENABLE`.
6. Send `ANGLE 90`.
7. Send `ANGLE 85`.
8. Send `ANGLE 95`.
9. Send `STOP`.
10. Record direction, current, noise, heat, and any unexpected motion.

Do not widen the angle range until direction, stop behavior, mechanical
clearance, and current draw are understood.

## After the first motion test

- Label the positive direction in the config file.
- Measure no-load current during small angle moves.
- Check servo temperature after repeated short commands.
- If an encoder or position sensor is installed, confirm sign and scale before
  moving to closed-loop control.
- Add one mechanical constraint at a time: pulley, tendon, linkage, then load.

## Stop criteria

Stop testing immediately if any of these occur:

- The servo moves in an unexpected direction near a hard stop.
- Current rises sharply or reaches the supply limit.
- The servo, cable, or connector heats quickly.
- Serial control becomes unresponsive.
- The mechanism binds, slips, or makes abnormal noise.
