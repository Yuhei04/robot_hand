#!/usr/bin/env python3
"""Detect and visualize hand landmarks from a camera using MediaPipe."""

from __future__ import annotations

import argparse
import json
import os
import pathlib
import sys
import tempfile
import time
from typing import Any, TextIO

try:
    from hand_pose_metrics import FINGER_ORDER, compute_hand_metrics
except ImportError:
    from scripts.hand_pose_metrics import FINGER_ORDER, compute_hand_metrics


HAND_CONNECTIONS = (
    (0, 1),
    (1, 2),
    (2, 3),
    (3, 4),
    (0, 5),
    (5, 6),
    (6, 7),
    (7, 8),
    (5, 9),
    (9, 10),
    (10, 11),
    (11, 12),
    (9, 13),
    (13, 14),
    (14, 15),
    (15, 16),
    (13, 17),
    (0, 17),
    (17, 18),
    (18, 19),
    (19, 20),
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Detect 21-point hand skeletons from a camera."
    )
    parser.add_argument(
        "--model-complexity",
        type=int,
        default=1,
        choices=(0, 1),
        help="MediaPipe Hands landmark model complexity.",
    )
    parser.add_argument("--camera", type=int, default=0, help="OpenCV camera index.")
    parser.add_argument("--width", type=int, default=1280, help="Requested frame width.")
    parser.add_argument("--height", type=int, default=720, help="Requested frame height.")
    parser.add_argument(
        "--num-hands", type=int, default=1, help="Maximum number of hands to detect."
    )
    parser.add_argument(
        "--confidence",
        type=float,
        default=0.5,
        help="Detection, presence, and tracking confidence threshold.",
    )
    parser.add_argument(
        "--no-mirror",
        action="store_true",
        help="Do not mirror the preview image.",
    )
    parser.add_argument(
        "--jsonl",
        default=None,
        help="Write one JSON object per frame to this path, or '-' for stdout.",
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


def import_dependencies():
    mpl_cache = pathlib.Path(tempfile.gettempdir()) / "robot_hand_matplotlib"
    general_cache = pathlib.Path(tempfile.gettempdir()) / "robot_hand_cache"
    mpl_cache.mkdir(parents=True, exist_ok=True)
    general_cache.mkdir(parents=True, exist_ok=True)
    os.environ.setdefault("MPLCONFIGDIR", str(mpl_cache))
    os.environ.setdefault("XDG_CACHE_HOME", str(general_cache))

    try:
        import cv2  # type: ignore
        import mediapipe as mp  # type: ignore
        import numpy as np  # type: ignore
    except ImportError as exc:
        raise SystemExit(
            "Missing camera dependencies. Install them with "
            "`python3 -m pip install -r requirements.txt`."
        ) from exc
    return cv2, mp, np


def open_jsonl(path: str | None) -> tuple[TextIO | None, bool]:
    if path is None:
        return None, False
    if path == "-":
        return sys.stdout, False

    output_path = pathlib.Path(path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    return output_path.open("w", encoding="utf-8"), True


def category_value(category: Any, name: str, fallback: Any = None) -> Any:
    value = getattr(category, name, fallback)
    return fallback if value is None else value


def result_to_dict(result: Any, timestamp_ms: int) -> dict[str, Any]:
    hands = []
    image_landmarks = result.multi_hand_landmarks or []
    world_landmarks = result.multi_hand_world_landmarks or []
    handedness = result.multi_handedness or []

    for index, landmarks in enumerate(image_landmarks):
        classification = None
        if index < len(handedness) and handedness[index].classification:
            classification = handedness[index].classification[0]
        hand_world_landmarks = world_landmarks[index] if index < len(world_landmarks) else None
        hand = {
            "handedness": (
                category_value(classification, "label", "Unknown")
                if classification
                else "Unknown"
            ),
            "score": (
                float(category_value(classification, "score", 0.0))
                if classification
                else 0.0
            ),
            "landmarks": [
                {"id": landmark_id, "x": point.x, "y": point.y, "z": point.z}
                for landmark_id, point in enumerate(landmarks.landmark)
            ],
            "world_landmarks_m": [
                {"id": landmark_id, "x": point.x, "y": point.y, "z": point.z}
                for landmark_id, point in enumerate(
                    hand_world_landmarks.landmark if hand_world_landmarks else []
                )
            ],
        }
        hand.update(compute_hand_metrics(hand))
        hands.append(hand)
    return {"timestamp_ms": timestamp_ms, "hands": hands}


def draw_result(cv2: Any, frame: Any, result: Any, frame_data: dict[str, Any]) -> None:
    height, width = frame.shape[:2]
    image_landmarks = result.multi_hand_landmarks or []
    handedness = result.multi_handedness or []

    for hand_index, landmarks in enumerate(image_landmarks):
        points = [
            (
                max(0, min(width - 1, int(point.x * width))),
                max(0, min(height - 1, int(point.y * height))),
            )
            for point in landmarks.landmark
        ]

        for start, end in HAND_CONNECTIONS:
            cv2.line(frame, points[start], points[end], (0, 220, 0), 2, cv2.LINE_AA)
        for landmark_id, point in enumerate(points):
            cv2.circle(frame, point, 4, (0, 80, 255), -1, cv2.LINE_AA)
            cv2.putText(
                frame,
                str(landmark_id),
                (point[0] + 4, point[1] - 4),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.35,
                (255, 255, 255),
                1,
                cv2.LINE_AA,
            )

        classification = None
        if hand_index < len(handedness) and handedness[hand_index].classification:
            classification = handedness[hand_index].classification[0]
        label = "Unknown"
        if classification:
            name = category_value(classification, "label", "Unknown")
            score = float(category_value(classification, "score", 0.0))
            label = f"{name} {score:.2f}"
        label_y = max(24, min(point[1] for point in points) - 10)
        cv2.putText(
            frame,
            label,
            (min(point[0] for point in points), label_y),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.7,
            (255, 255, 0),
            2,
            cv2.LINE_AA,
        )

        if hand_index < len(frame_data.get("hands", [])):
            hand_data = frame_data["hands"][hand_index]
            curls = hand_data.get("finger_curl", {})
            grasp = hand_data.get("finger_grasp_candidate", {})
            for row, finger in enumerate(FINGER_ORDER):
                text = f"{finger[:2]} {float(curls.get(finger, 0.0)):.2f}"
                if grasp.get(finger, False):
                    text += " grasp"
                cv2.putText(
                    frame,
                    text,
                    (12, 28 + 22 * row),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.55,
                    (0, 255, 255) if grasp.get(finger, False) else (220, 220, 220),
                    2,
                    cv2.LINE_AA,
                )


def validate_args(args: argparse.Namespace) -> None:
    if args.num_hands < 1:
        raise SystemExit("--num-hands must be at least 1.")
    if not 0.0 <= args.confidence <= 1.0:
        raise SystemExit("--confidence must be between 0.0 and 1.0.")
    if args.no_display and args.jsonl is None:
        raise SystemExit("--no-display requires --jsonl so results are observable.")
    if args.max_frames is not None and args.max_frames < 1:
        raise SystemExit("--max-frames must be at least 1.")


def main() -> int:
    args = parse_args()
    validate_args(args)
    cv2, mp, np = import_dependencies()

    output, close_output = open_jsonl(args.jsonl)
    capture = cv2.VideoCapture(args.camera)
    started_at = time.monotonic()
    last_timestamp_ms = -1
    frame_count = 0
    try:
        capture.set(cv2.CAP_PROP_FRAME_WIDTH, args.width)
        capture.set(cv2.CAP_PROP_FRAME_HEIGHT, args.height)
        if not capture.isOpened():
            raise SystemExit(
                f"Could not open camera index {args.camera}. "
                "Check the camera index and OS camera permission."
            )

        with mp.solutions.hands.Hands(
            static_image_mode=False,
            max_num_hands=args.num_hands,
            model_complexity=args.model_complexity,
            min_detection_confidence=args.confidence,
            min_tracking_confidence=args.confidence,
        ) as hands:
            while True:
                ok, frame = capture.read()
                if not ok:
                    print("Camera frame read failed.", file=sys.stderr)
                    return 1
                if not args.no_mirror:
                    frame = cv2.flip(frame, 1)

                rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                rgb_frame = np.ascontiguousarray(rgb_frame)
                timestamp_ms = max(
                    last_timestamp_ms + 1, int((time.monotonic() - started_at) * 1000)
                )
                last_timestamp_ms = timestamp_ms
                result = hands.process(rgb_frame)
                frame_data = result_to_dict(result, timestamp_ms)

                if output is not None:
                    output.write(
                        json.dumps(
                            frame_data,
                            ensure_ascii=True,
                            separators=(",", ":"),
                        )
                        + "\n"
                    )
                    output.flush()
                frame_count += 1

                if not args.no_display:
                    draw_result(cv2, frame, result, frame_data)
                    cv2.imshow("Robot Hand - Hand Landmarks", frame)
                    key = cv2.waitKey(1) & 0xFF
                    if key in (ord("q"), 27):
                        break
                if args.max_frames is not None and frame_count >= args.max_frames:
                    break
    except KeyboardInterrupt:
        pass
    finally:
        capture.release()
        if not args.no_display:
            cv2.destroyAllWindows()
        if close_output and output is not None:
            output.close()

    return 0


if __name__ == "__main__":
    sys.exit(main())
