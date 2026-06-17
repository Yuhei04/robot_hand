#!/usr/bin/env python3
"""Convert MediaPipe hand landmarks JSONL to five servo target angles."""

from __future__ import annotations

import argparse
import csv
import json
import pathlib
import sys
from typing import Any, Iterable, TextIO

try:
    from hand_pose_metrics import FINGER_ORDER, ROBOT_AXES, compute_hand_metrics
except ImportError:
    from scripts.hand_pose_metrics import FINGER_ORDER, ROBOT_AXES, compute_hand_metrics

MOTOR_NAMES = ("a", "b", "c", "d", "e")
DEFAULT_THREE_FINGER_OPEN = "180,180,180,0,120"
DEFAULT_THREE_FINGER_CLOSED = "0,0,50,180,30"
LAYOUT_LABELS = {
    "three-finger": ROBOT_AXES,
    "five-finger": FINGER_ORDER,
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert hand landmark JSONL to normalized finger curl and servo commands."
    )
    parser.add_argument(
        "--input",
        "-i",
        default="logs/hand_landmarks.jsonl",
        help="Input JSONL path from hand_landmark_camera.py, or '-' for stdin.",
    )
    parser.add_argument(
        "--output",
        "-o",
        default="logs/servo_targets.csv",
        help="Output CSV path, or '-' for stdout.",
    )
    parser.add_argument(
        "--command-output",
        default=None,
        help="Optional text file for firmware commands such as 'a90 b91 ...'.",
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
        help="Which detected hand to convert.",
    )
    parser.add_argument(
        "--open-angles",
        default=None,
        help="Servo angle for fully open fingers. Use one value or five comma-separated values.",
    )
    parser.add_argument(
        "--closed-angles",
        default=None,
        help="Servo angle for fully curled fingers. Use one value or five comma-separated values.",
    )
    parser.add_argument(
        "--sample-every",
        type=int,
        default=1,
        help="Use every Nth input frame.",
    )
    parser.add_argument(
        "--min-score",
        type=float,
        default=0.5,
        help="Skip hands below this handedness confidence score.",
    )
    return parser.parse_args()


def parse_angle_list(raw: str, name: str, count: int = 5) -> list[float]:
    values = [float(value.strip()) for value in raw.split(",") if value.strip()]
    if len(values) == 1:
        return values * count
    if len(values) == count:
        return values
    raise SystemExit(f"{name} must contain one value or {count} comma-separated values.")


def default_angles(layout: str) -> tuple[str, str]:
    if layout == "three-finger":
        return DEFAULT_THREE_FINGER_OPEN, DEFAULT_THREE_FINGER_CLOSED
    return "80", "100"


def open_text(path: str, mode: str) -> tuple[TextIO, bool]:
    if path == "-":
        return (sys.stdin if "r" in mode else sys.stdout), False

    file_path = pathlib.Path(path)
    if "w" in mode:
        file_path.parent.mkdir(parents=True, exist_ok=True)
    return file_path.open(mode, encoding="utf-8", newline=""), True


def clamp(value: float, low: float, high: float) -> float:
    return max(low, min(high, value))


def choose_hand(frame: dict[str, Any], hand_choice: str, min_score: float) -> dict[str, Any] | None:
    hands = [
        hand
        for hand in frame.get("hands", [])
        if float(hand.get("score", 0.0)) >= min_score
    ]
    if not hands:
        return None
    if hand_choice == "first":
        return hands[0]

    expected = hand_choice.lower()
    for hand in hands:
        if str(hand.get("handedness", "")).lower() == expected:
            return hand
    return None


def servo_angles(
    controls: dict[str, float],
    open_angles: list[float],
    closed_angles: list[float],
    labels: tuple[str, ...] = FINGER_ORDER,
) -> dict[str, int]:
    angles: dict[str, int] = {}
    for index, label in enumerate(labels):
        angle = open_angles[index] + controls[label] * (closed_angles[index] - open_angles[index])
        angles[MOTOR_NAMES[index]] = int(round(clamp(angle, 0.0, 180.0)))
    return angles


def firmware_command(angles: dict[str, int]) -> str:
    return " ".join(f"{motor}{angles[motor]}" for motor in MOTOR_NAMES)


def iter_frames(input_file: Iterable[str]) -> Iterable[dict[str, Any]]:
    for line_number, line in enumerate(input_file, start=1):
        line = line.strip()
        if not line:
            continue
        try:
            yield json.loads(line)
        except json.JSONDecodeError as exc:
            raise SystemExit(f"Invalid JSON on line {line_number}: {exc}") from exc


def main() -> int:
    args = parse_args()
    if args.sample_every < 1:
        raise SystemExit("--sample-every must be at least 1.")
    if not 0.0 <= args.min_score <= 1.0:
        raise SystemExit("--min-score must be between 0.0 and 1.0.")

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

    input_file, close_input = open_text(args.input, "r")
    output_file, close_output = open_text(args.output, "w")
    command_file = None
    close_command = False
    if args.command_output is not None:
        command_file, close_command = open_text(args.command_output, "w")

    rows_written = 0
    frames_seen = 0
    try:
        writer = csv.writer(output_file)
        writer.writerow(
            [
                "timestamp_ms",
                "handedness",
                "score",
                "layout",
                "thumb_curl",
                "thumb_grasp_candidate",
                "thumb_grasp_score",
                "index_curl",
                "index_grasp_candidate",
                "index_grasp_score",
                "middle_curl",
                "middle_grasp_candidate",
                "middle_grasp_score",
                "ring_curl",
                "ring_grasp_candidate",
                "ring_grasp_score",
                "pinky_curl",
                "pinky_grasp_candidate",
                "pinky_grasp_score",
                "index_flex",
                "middle_flex",
                "thumb_base_flex",
                "thumb_tip_flex",
                "thumb_opposition",
                "motor_a_deg",
                "motor_b_deg",
                "motor_c_deg",
                "motor_d_deg",
                "motor_e_deg",
                "command",
            ]
        )

        for frame in iter_frames(input_file):
            frames_seen += 1
            if (frames_seen - 1) % args.sample_every != 0:
                continue

            hand = choose_hand(frame, args.hand, args.min_score)
            if hand is None:
                continue
            metrics = compute_hand_metrics(hand)
            curls = metrics["finger_curl"]
            grasp_candidates = metrics["finger_grasp_candidate"]
            grasp_scores = metrics["finger_grasp_score"]
            robot_axes = metrics["robot_axis"]
            controls = robot_axes if args.layout == "three-finger" else curls
            if len(curls) < len(FINGER_ORDER) or len(controls) < len(layout_labels):
                continue

            angles = servo_angles(controls, open_angles, closed_angles, layout_labels)
            command = firmware_command(angles)
            finger_columns = []
            for finger in FINGER_ORDER:
                finger_columns.extend(
                    [
                        f"{curls[finger]:.6f}",
                        int(bool(grasp_candidates[finger])),
                        f"{grasp_scores[finger]:.6f}",
                    ]
                )
            writer.writerow(
                [
                    int(frame.get("timestamp_ms", 0)),
                    hand.get("handedness", "Unknown"),
                    f"{float(hand.get('score', 0.0)):.6f}",
                    args.layout,
                    *finger_columns,
                    *(f"{robot_axes[axis]:.6f}" for axis in ROBOT_AXES),
                    *(angles[motor] for motor in MOTOR_NAMES),
                    command,
                ]
            )
            if command_file is not None:
                command_file.write(command + "\n")
            rows_written += 1
    finally:
        if close_input:
            input_file.close()
        if close_output:
            output_file.close()
        if close_command and command_file is not None:
            command_file.close()

    print(f"frames_seen={frames_seen} rows_written={rows_written}", file=sys.stderr)
    return 0 if rows_written > 0 else 1


if __name__ == "__main__":
    sys.exit(main())
