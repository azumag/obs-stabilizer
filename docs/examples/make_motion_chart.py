#!/usr/bin/env python3
"""Plot frame-to-frame motion from three OBS before/after recording pairs."""

import sys
from pathlib import Path

import cv2
import numpy as np

from measure_motion_v2 import collect_motion


def motion_series(video_path):
    """Estimate one robust camera-translation magnitude per decoded frame."""
    _, values, _ = collect_motion(video_path)
    return np.asarray(values, dtype=float)


def draw_series(canvas, values, bounds, color):
    """Draw finite runs without connecting gaps from rejected flow tracks."""
    x0, y0, x1, y1, maximum = bounds
    if len(values) < 2:
        return
    points = []
    for index, value in enumerate(values):
        if not np.isfinite(value):
            if len(points) > 1:
                cv2.polylines(canvas, [np.asarray(points, np.int32)],
                              False, color, 2, cv2.LINE_AA)
            points = []
            continue
        x = x0 + index * (x1 - x0) / max(1, len(values) - 1)
        y = y1 - min(float(value), maximum) * (y1 - y0) / maximum
        points.append((round(x), round(y)))
    if len(points) > 1:
        cv2.polylines(canvas, [np.asarray(points, np.int32)],
                      False, color, 2, cv2.LINE_AA)


def draw_chart(samples, output):
    width, height = 880, 990
    canvas = np.full((height, width, 3), 255, dtype=np.uint8)
    text_color = (30, 30, 30)
    grid_color = (225, 225, 225)
    baseline_color = (180, 119, 31)   # Matplotlib blue in BGR.
    filtered_color = (14, 127, 255)   # Matplotlib orange in BGR.

    cv2.putText(canvas,
                "Observed frame-to-frame camera motion: OBS Stabilizer on/off",
                (80, 34), cv2.FONT_HERSHEY_SIMPLEX, 0.68, text_color, 1,
                cv2.LINE_AA)

    for panel, (title, baseline, filtered) in enumerate(samples):
        top = 65 + panel * 295
        x0, x1 = 72, 860
        y0, y1 = top + 42, top + 250
        finite_values = np.concatenate((baseline[np.isfinite(baseline)],
                                        filtered[np.isfinite(filtered)]))
        if finite_values.size == 0:
            raise RuntimeError(f"No usable motion samples for {title}")
        maximum = max(1.0, float(np.percentile(finite_values, 99)) * 1.08)

        cv2.putText(canvas, title, (x0, top + 21),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.55, text_color, 1,
                    cv2.LINE_AA)
        for tick in range(5):
            y = round(y1 - tick * (y1 - y0) / 4)
            value = tick * maximum / 4
            cv2.line(canvas, (x0, y), (x1, y), grid_color, 1,
                     cv2.LINE_AA)
            cv2.putText(canvas, f"{value:.1f}", (22, y + 5),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.38, text_color, 1,
                        cv2.LINE_AA)
        cv2.rectangle(canvas, (x0, y0), (x1, y1), text_color, 1,
                      cv2.LINE_AA)
        cv2.putText(canvas, "motion (px)", (x0, y0 - 8),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.4, text_color, 1,
                    cv2.LINE_AA)

        draw_series(canvas, baseline, (x0, y0, x1, y1, maximum),
                    baseline_color)
        draw_series(canvas, filtered, (x0, y0, x1, y1, maximum),
                    filtered_color)

        legend_x = x1 - 155
        cv2.line(canvas, (legend_x, y0 + 15), (legend_x + 24, y0 + 15),
                 baseline_color, 2, cv2.LINE_AA)
        cv2.putText(canvas, "no filter", (legend_x + 31, y0 + 20),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.4, text_color, 1,
                    cv2.LINE_AA)
        cv2.line(canvas, (legend_x, y0 + 37), (legend_x + 24, y0 + 37),
                 filtered_color, 2, cv2.LINE_AA)
        cv2.putText(canvas, "stabilizer", (legend_x + 31, y0 + 42),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.4, text_color, 1,
                    cv2.LINE_AA)

    cv2.putText(canvas, "frame", (415, 976), cv2.FONT_HERSHEY_SIMPLEX,
                0.45, text_color, 1, cv2.LINE_AA)
    if not cv2.imwrite(str(output), canvas):
        raise RuntimeError(f"Cannot write chart: {output}")


def main():
    if len(sys.argv) not in (7, 8):
        raise SystemExit(
            "usage: make_motion_chart.py BASE_FINE FILT_FINE BASE_LARGE "
            "FILT_LARGE BASE_MIXED FILT_MIXED [OUTPUT]")

    output = Path(sys.argv[7]) if len(sys.argv) == 8 else (
        Path(__file__).resolve().parent / "motion-comparison.png")
    paths = [
        ("Fine shake (2 px, 9 Hz)", sys.argv[1], sys.argv[2]),
        ("Large shake (22 px, 3 Hz)", sys.argv[3], sys.argv[4]),
        ("Mixed shake (10 px, 7 Hz + 0.5 Hz sway)", sys.argv[5], sys.argv[6]),
    ]
    samples = []
    for title, baseline_path, filtered_path in paths:
        baseline = motion_series(baseline_path)
        filtered = motion_series(filtered_path)
        count = min(len(baseline), len(filtered))
        samples.append((title, baseline[:count], filtered[:count]))
    draw_chart(samples, output)
    print(f"wrote {output}")


if __name__ == "__main__":
    main()
