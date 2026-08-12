#!/usr/bin/env python3
"""Measure inter-frame motion with outlier-robust statistics.

Compares a baseline and a stabilized recording frame-by-frame using both
optical-flow translation (median, which tolerates occasional bad tracks) and
the mean absolute pixel difference between consecutive frames. The pixel
difference is a direct perceptual proxy: a stabilized sequence keeps the
image content nearly still frame to frame.
"""

import json
import math
import sys

import cv2
import numpy as np


def percentile(values, amount):
    return float(np.percentile(np.asarray(values), amount)) if values else 0.0


def analyze(video_path):
    capture = cv2.VideoCapture(video_path)
    if not capture.isOpened():
        raise RuntimeError(f"Cannot open video: {video_path}")

    translations = []
    pixel_diffs = []
    frame_index = 0
    previous = None

    while True:
        ok, frame = capture.read()
        if not ok:
            break
        frame_index += 1
        gray = cv2.cvtColor(cv2.resize(frame, (640, 360)), cv2.COLOR_BGR2GRAY)
        if previous is None:
            previous = gray
            continue

        if frame_index > 45:
            # Direct pixel-level difference between consecutive frames.
            diff = cv2.absdiff(previous, gray)
            pixel_diffs.append(float(diff.mean()))

            points = cv2.goodFeaturesToTrack(
                previous,
                maxCorners=500,
                qualityLevel=0.01,
                minDistance=8,
            )
            if points is not None:
                tracked, status, _ = cv2.calcOpticalFlowPyrLK(
                    previous,
                    gray,
                    points,
                    None,
                )
                valid = status.reshape(-1) == 1
                source_points = points.reshape(-1, 2)[valid]
                target_points = tracked.reshape(-1, 2)[valid]
                if len(source_points) >= 12:
                    transform, inliers = cv2.estimateAffinePartial2D(
                        source_points,
                        target_points,
                        method=cv2.RANSAC,
                        ransacReprojThreshold=2.0,
                    )
                    if transform is not None and inliers is not None and int(inliers.sum()) >= 8:
                        dx = float(transform[0, 2])
                        dy = float(transform[1, 2])
                        translations.append(math.hypot(dx, dy))
        previous = gray

    capture.release()
    if not translations:
        raise RuntimeError(f"No usable motion samples: {video_path}")

    translation_array = np.asarray(translations)
    diff_array = np.asarray(pixel_diffs)
    return {
        "video": video_path,
        "decoded_frames": frame_index,
        "motion_samples": len(translations),
        "translation_pixels": {
            "mean": float(translation_array.mean()),
            "rms": float(np.sqrt(np.mean(np.square(translation_array)))),
            "median": percentile(translations, 50),
            "p95": percentile(translations, 95),
        },
        "pixel_diff_0_255": {
            "mean": float(diff_array.mean()),
            "median": percentile(pixel_diffs, 50),
            "p95": percentile(pixel_diffs, 95),
        },
    }


if len(sys.argv) != 3:
    raise SystemExit("usage: measure_motion_v2.py BASELINE FILTERED")

baseline = analyze(sys.argv[1])
filtered = analyze(sys.argv[2])
baseline_rms = baseline["translation_pixels"]["rms"]
filtered_rms = filtered["translation_pixels"]["rms"]
baseline_median = baseline["translation_pixels"]["median"]
filtered_median = filtered["translation_pixels"]["median"]
baseline_diff = baseline["pixel_diff_0_255"]["mean"]
filtered_diff = filtered["pixel_diff_0_255"]["mean"]

print(json.dumps({
    "baseline": baseline,
    "filtered": filtered,
    "translation_rms_reduction_percent":
        round(100.0 * (baseline_rms - filtered_rms) / baseline_rms, 2),
    "translation_median_reduction_percent":
        round(100.0 * (baseline_median - filtered_median) / baseline_median, 2)
        if baseline_median > 0 else None,
    "pixel_diff_reduction_percent":
        round(100.0 * (baseline_diff - filtered_diff) / baseline_diff, 2),
}, indent=2))
