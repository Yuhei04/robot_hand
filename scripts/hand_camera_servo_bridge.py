#!/usr/bin/env python3
"""Drive the robot hand from camera-detected hand motion."""

from __future__ import annotations

import argparse
import sys
import time
from typing import Any

try:
    import hand_landmark_camera as camera
    from hand_pose_metrics import FINGER_ORDER, ROBOT_AXES
    from hand_pose_to_servo import (
        LAYOUT_LABELS,
        MOTOR_NAMES,
        choose_hand,
        default_angles,
        firmware_command,
        parse_angle_list,
        servo_angles,
    )
except ImportError:
    from scripts import hand_landmark_camera as camera
    from scripts.hand_pose_metrics import FINGER_ORDER, ROBOT_AXES
    from scripts.hand_pose_to_servo import (
        LAYOUT_LABELS,
        MOTOR_NAMES,
        choose_hand,
        default_angles,
        firmware_command,
        parse_angle_list,
        servo_angles,
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Map camera hand landmarks to live five-motor serial commands."
    )
    parser.add_argument("--camera", type=int, default=0, help="OpenCV camera index.")
    parser.add_argument("--width", type=int, default=1280, help="Requested frame width.")
    parser.add_argument("--height", type=int, default=720, help="Requested frame height.")
    parser.add_argument(
        "--model-complexity",
        type=int,
        default=1,
        choices=(0, 1),
        help="MediaPipe Hands landmark model complexity.",
    )
    parser.add_argument(
        "--confidence",
        type=float,
        default=0.5,
        help="Detection and tracking confidence threshold.",
    )
    parser.add_argument(
        "--no-mirror",
        action="store_true",
        help="Do not mirror the camera image before detection.",
    )
    parser.add_argument(
        "--layout",
        choices=("three-finger", "five-finger"),
        default="three-finger",
        help="Robot hand layout. three-finger maps A-E to index, middle, thumb base, thumb tip, thumb orientation.",
    )
    parser.add_argument(
        "--hand",
        choices=("first", "left", "right"),
        default="first",
        help="Which detected hand controls the robot hand.",
    )
    parser.add_argument(
        "--min-score",
        type=float,
        default=0.5,
        help="Skip hands below this handedness confidence score.",
    )
    parser.add_argument(
        "--open-angles",
        default=None,
        help="Robot servo angle for fully open fingers. One value or five comma-separated values.",
    )
    parser.add_argument(
        "--closed-angles",
        default=None,
        help="Robot servo angle for fully curled fingers. One value or five comma-separated values.",
    )
    parser.add_argument(
        "--rate-hz",
        type=float,
        default=10.0,
        help="Maximum serial command rate.",
    )
    parser.add_argument(
        "--ema-alpha",
        type=float,
        default=0.35,
        help="Smoothing factor for target angles. 1.0 disables smoothing.",
    )
    parser.add_argument(
        "--max-step-deg",
        type=float,
        default=3.0,
        help="Maximum angle change per motor per command.",
    )
    parser.add_argument(
        "--loss-timeout",
        type=float,
        default=0.5,
        help="Seconds without a tracked hand before sending open angles.",
    )
    parser.add_argument(
        "--port",
        default=None,
        help="Serial port for ESP32, for example /dev/cu.usbmodemXXXX.",
    )
    parser.add_argument("--baud", type=int, default=115200, help="Serial baud rate.")
    parser.add_argument(
        "--send",
        action="store_true",
        help="Actually write commands to the serial port. Without this, dry-run only.",
    )
    parser.add_argument(
        "--serial-wait",
        type=float,
        default=1.5,
        help="Seconds to wait after opening serial, because many ESP32 boards reset.",
    )
    parser.add_argument(
        "--no-open-on-exit",
        action="store_true",
        help="Do not send open angles before exiting.",
    )
    parser.add_argument(
        "--no-display",
        action="store_true",
        help="Run without an OpenCV preview window.",
    )
    parser.add_argument(
        "--max-frames",
        type=int,
        default=None,
        help="Stop after this many frames. Useful for smoke tests.",
    )
    return parser.parse_args()


def validate_args(args: argparse.Namespace) -> None:
    if not 0.0 <= args.confidence <= 1.0:
        raise SystemExit("--confidence must be between 0.0 and 1.0.")
    if not 0.0 <= args.min_score <= 1.0:
        raise SystemExit("--min-score must be between 0.0 and 1.0.")
    if args.rate_hz <= 0.0:
        raise SystemExit("--rate-hz must be greater than 0.")
    if not 0.0 < args.ema_alpha <= 1.0:
        raise SystemExit("--ema-alpha must be greater than 0 and at most 1.")
    if args.max_step_deg <= 0.0:
        raise SystemExit("--max-step-deg must be greater than 0.")
    if args.loss_timeout < 0.0:
        raise SystemExit("--loss-timeout must be non-negative.")
    if args.max_frames is not None and args.max_frames < 1:
        raise SystemExit("--max-frames must be at least 1.")
    if args.send and not args.port:
        raise SystemExit("--send requires --port.")


def import_serial():
    try:
        import serial  # type: ignore
    except ImportError as exc:
        raise SystemExit(
            "Missing dependency: pyserial. Install with `python -m pip install -r requirements.txt`."
        ) from exc
    return serial


def open_serial(args: argparse.Namespace):
    if not args.send:
        return None
    serial = import_serial()
    ser = serial.Serial(args.port, args.baud, timeout=0.1)
    if args.serial_wait > 0:
        time.sleep(args.serial_wait)
        ser.reset_input_buffer()
    return ser


def write_command(ser: Any, command: str, send: bool) -> None:
    if send:
        ser.write((command + "\n").encode("utf-8"))
        ser.flush()
    print(command)


def clamp_step(current: float, target: float, max_step: float) -> float:
    delta = target - current
    if delta > max_step:
        return current + max_step
    if delta < -max_step:
        return current - max_step
    return target


def smooth_angles(
    previous: dict[str, float] | None,
    target: dict[str, int],
    alpha: float,
    max_step: float,
) -> dict[str, float]:
    if previous is None:
        return {motor: float(target[motor]) for motor in MOTOR_NAMES}

    smoothed: dict[str, float] = {}
    for motor in MOTOR_NAMES:
        blended = previous[motor] + alpha * (float(target[motor]) - previous[motor])
        smoothed[motor] = clamp_step(previous[motor], blended, max_step)
    return smoothed


def rounded_angles(angles: dict[str, float]) -> dict[str, int]:
    return {motor: int(round(angles[motor])) for motor in MOTOR_NAMES}


def draw_bridge_status(
    cv2: Any,
    frame: Any,
    command: str,
    sending: bool,
    tracked: bool,
) -> None:
    status = "SEND" if sending else "DRY-RUN"
    tracked_text = "tracked" if tracked else "lost/open"
    cv2.putText(
        frame,
        f"{status} {tracked_text} {command}",
        (12, frame.shape[0] - 18),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.6,
        (0, 255, 0) if tracked else (0, 165, 255),
        2,
        cv2.LINE_AA,
    )


def main() -> int:
    args = parse_args()
    validate_args(args)

    layout_labels = LAYOUT_LABELS[args.layout]
    default_open, default_closed = default_angles(args.layout)
    open_angles = parse_angle_list(
        args.open_angles or default_open,
        "--open-angles",
        len(layout_labels),
    )
    closed_angles = parse_angle_list(
        args.closed_angles or default_closed,
        "--closed-angles",
        len(layout_labels),
    )
    cv2, mp, np = camera.import_dependencies()
    ser = open_serial(args)

    capture = cv2.VideoCapture(args.camera)
    capture.set(cv2.CAP_PROP_FRAME_WIDTH, args.width)
    capture.set(cv2.CAP_PROP_FRAME_HEIGHT, args.height)
    if not capture.isOpened():
        raise SystemExit(
            f"Could not open camera index {args.camera}. "
            "Check the camera index and OS camera permission."
        )

    open_targets = {
        motor: int(round(open_angles[index])) for index, motor in enumerate(MOTOR_NAMES)
    }
    current_angles: dict[str, float] | None = {
        motor: float(open_targets[motor]) for motor in MOTOR_NAMES
    }
    last_command = ""
    last_send_at = 0.0
    last_seen_at = time.monotonic()
    frame_count = 0
    send_interval = 1.0 / args.rate_hz

    try:
        with mp.solutions.hands.Hands(
            static_image_mode=False,
            max_num_hands=1,
            model_complexity=args.model_complexity,
            min_detection_confidence=args.confidence,
            min_tracking_confidence=args.confidence,
        ) as hands:
            while True:
                ok, frame = capture.read()
                if not ok:
                    print("Camera frame read failed.", file=sys.stderr)
                    return 1
                frame_count += 1
                if not args.no_mirror:
                    frame = cv2.flip(frame, 1)

                rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                rgb_frame = np.ascontiguousarray(rgb_frame)
                result = hands.process(rgb_frame)
                frame_data = camera.result_to_dict(result, int(time.monotonic() * 1000))
                hand = choose_hand(frame_data, args.hand, args.min_score)

                now = time.monotonic()
                tracked = hand is not None
                if tracked:
                    last_seen_at = now
                    curls = hand.get("finger_curl", {})
                    robot_axes = hand.get("robot_axis", {})
                    controls = robot_axes if args.layout == "three-finger" else curls
                    if len(controls) == len(layout_labels):
                        target = servo_angles(
                            controls,
                            open_angles,
                            closed_angles,
                            layout_labels,
                        )
                        current_angles = smooth_angles(
                            current_angles,
                            target,
                            args.ema_alpha,
                            args.max_step_deg,
                        )
                elif now - last_seen_at >= args.loss_timeout:
                    current_angles = smooth_angles(
                        current_angles,
                        open_targets,
                        args.ema_alpha,
                        args.max_step_deg,
                    )

                if current_angles is not None and now - last_send_at >= send_interval:
                    command = firmware_command(rounded_angles(current_angles))
                    if command != last_command:
                        write_command(ser, command, args.send)
                        last_command = command
                    last_send_at = now

                if not args.no_display:
                    camera.draw_result(cv2, frame, result, frame_data)
                    draw_bridge_status(cv2, frame, last_command, args.send, tracked)
                    cv2.imshow("Robot Hand - Camera Servo Bridge", frame)
                    key = cv2.waitKey(1) & 0xFF
                    if key in (ord("q"), 27):
                        break

                if args.max_frames is not None and frame_count >= args.max_frames:
                    break
    except KeyboardInterrupt:
        pass
    finally:
        if ser is not None and not args.no_open_on_exit:
            write_command(ser, firmware_command(open_targets), args.send)
        if ser is not None:
            ser.close()
        capture.release()
        if not args.no_display:
            cv2.destroyAllWindows()

    return 0


if __name__ == "__main__":
    sys.exit(main())
