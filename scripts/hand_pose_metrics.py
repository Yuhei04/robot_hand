"""Hand pose metrics derived from MediaPipe's 21 hand landmarks."""

from __future__ import annotations

import math
from typing import Any


FINGERS = {
    "thumb": (0, 1, 2, 3, 4),
    "index": (0, 5, 6, 7, 8),
    "middle": (0, 9, 10, 11, 12),
    "ring": (0, 13, 14, 15, 16),
    "pinky": (0, 17, 18, 19, 20),
}

FINGER_ORDER = ("thumb", "index", "middle", "ring", "pinky")
TIP_IDS = {
    "thumb": 4,
    "index": 8,
    "middle": 12,
    "ring": 16,
    "pinky": 20,
}
PALM_IDS = (0, 5, 9, 13, 17)
ROBOT_AXES = (
    "index_flex",
    "middle_flex",
    "thumb_base_flex",
    "thumb_tip_flex",
    "thumb_opposition",
)


Point = tuple[float, float, float]


def point_tuple(point: dict[str, Any]) -> Point:
    return (float(point["x"]), float(point["y"]), float(point.get("z", 0.0)))


def subtract(a: Point, b: Point) -> Point:
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def norm(v: Point) -> float:
    return math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])


def distance(a: Point, b: Point) -> float:
    return norm(subtract(a, b))


def angle_degrees(a: Point, b: Point, c: Point) -> float:
    ba = subtract(a, b)
    bc = subtract(c, b)
    denom = norm(ba) * norm(bc)
    if denom <= 1e-12:
        return 180.0
    cos_value = (ba[0] * bc[0] + ba[1] * bc[1] + ba[2] * bc[2]) / denom
    cos_value = max(-1.0, min(1.0, cos_value))
    return math.degrees(math.acos(cos_value))


def clamp(value: float, low: float = 0.0, high: float = 1.0) -> float:
    return max(low, min(high, value))


def landmarks_by_id(hand: dict[str, Any]) -> dict[int, Point]:
    source = hand.get("world_landmarks_m") or hand.get("landmarks") or []
    return {int(point["id"]): point_tuple(point) for point in source}


def finger_curl(points: dict[int, Point], ids: tuple[int, ...]) -> float:
    base, mcp, pip, dip, tip = ids
    triples = ((base, mcp, pip), (mcp, pip, dip), (pip, dip, tip))
    flexion_sum = 0.0
    for a, b, c in triples:
        flexion_sum += max(0.0, 180.0 - angle_degrees(points[a], points[b], points[c]))
    return clamp(flexion_sum / 240.0)


def joint_flexion(points: dict[int, Point], a: int, b: int, c: int) -> float:
    return clamp(max(0.0, 180.0 - angle_degrees(points[a], points[b], points[c])) / 90.0)


def palm_center(points: dict[int, Point]) -> Point:
    count = float(len(PALM_IDS))
    return (
        sum(points[index][0] for index in PALM_IDS) / count,
        sum(points[index][1] for index in PALM_IDS) / count,
        sum(points[index][2] for index in PALM_IDS) / count,
    )


def palm_size(points: dict[int, Point]) -> float:
    wrist_to_middle = distance(points[0], points[9])
    index_to_pinky = distance(points[5], points[17])
    size = max(wrist_to_middle, index_to_pinky)
    return max(size, 1e-6)


def tip_proximity_to_palm(points: dict[int, Point], finger: str) -> float:
    center = palm_center(points)
    size = palm_size(points)
    tip_distance = distance(points[TIP_IDS[finger]], center)
    # 1.0 means the fingertip is near the palm center; 0.0 means clearly away.
    return clamp((1.35 * size - tip_distance) / (0.85 * size))


def thumb_opposition(points: dict[int, Point]) -> float:
    size = palm_size(points)
    thumb_tip_to_index_base = distance(points[4], points[5])
    thumb_tip_to_palm = distance(points[4], palm_center(points))
    index_base_score = clamp((1.35 * size - thumb_tip_to_index_base) / (0.9 * size))
    palm_score = clamp((1.25 * size - thumb_tip_to_palm) / (0.8 * size))
    return clamp(0.65 * index_base_score + 0.35 * palm_score)


def robot_axis_values(points: dict[int, Point], curls: dict[str, float]) -> dict[str, float]:
    thumb_base = joint_flexion(points, 0, 1, 2)
    thumb_tip = clamp(
        0.5 * joint_flexion(points, 1, 2, 3)
        + 0.5 * joint_flexion(points, 2, 3, 4)
    )
    return {
        "index_flex": curls["index"],
        "middle_flex": curls["middle"],
        "thumb_base_flex": thumb_base,
        "thumb_tip_flex": thumb_tip,
        "thumb_opposition": thumb_opposition(points),
    }


def compute_hand_metrics(hand: dict[str, Any]) -> dict[str, dict[str, float] | dict[str, bool]]:
    points = landmarks_by_id(hand)
    if len(points) < 21:
        return {
            "finger_curl": {},
            "finger_tip_palm_proximity": {},
            "finger_grasp_score": {},
            "finger_grasp_candidate": {},
            "robot_axis": {},
        }

    curls = {finger: finger_curl(points, FINGERS[finger]) for finger in FINGER_ORDER}
    proximities = {
        finger: tip_proximity_to_palm(points, finger) for finger in FINGER_ORDER
    }
    grasp_scores = {
        finger: clamp(0.65 * curls[finger] + 0.35 * proximities[finger])
        for finger in FINGER_ORDER
    }
    grasp_candidates = {
        finger: grasp_scores[finger] >= 0.55 and curls[finger] >= 0.25
        for finger in FINGER_ORDER
    }
    return {
        "finger_curl": curls,
        "finger_tip_palm_proximity": proximities,
        "finger_grasp_score": grasp_scores,
        "finger_grasp_candidate": grasp_candidates,
        "robot_axis": robot_axis_values(points, curls),
    }
