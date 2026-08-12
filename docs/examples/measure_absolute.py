#!/usr/bin/env python3
"""Measure the absolute camera trajectory against the pattern image.

Registers every frame of a baseline and a filtered (stabilized) recording
against the same static pattern image to recover each frame's *absolute*
translation (not just frame-to-frame motion), then separates that
trajectory into a low-frequency component (intentional camera motion, e.g.
a pan) and a high-frequency component (shake) with a zero-phase Gaussian
filter. Quality is reported as high-frequency reduction (shake removed),
low-frequency fidelity (intentional motion preserved), plus secondary
indicators: frame-to-frame jerk, blur, and (for pan samples) how cleanly
the low-frequency trajectory settles onto the baseline during pan holds.
"""

import argparse
import glob
import json
import math
from pathlib import Path

import cv2
import numpy as np


SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_PATTERN = SCRIPT_DIR / "pattern.png"
WIDTH, HEIGHT = 640, 360
MIN_INLIERS = 12

# Pan-and-hold sample timeline (see generate_shake_samples.py's
# make_pan_offsets): absolute, 0-based frame indices for the two hold
# segments. These are intentionally independent of --skip-frames.
PAN_HOLD_SEGMENTS = {
    "hold_10_17s": (300, 510),
    "hold_21_24s": (630, 720),
}
PAN_SETTLE_THRESHOLD_PX = 2.0


def read_frames(path):
    """Yield BGR frames in order from a video file or a directory of PNGs."""
    p = Path(path)
    if p.is_dir():
        files = sorted(glob.glob(str(p / "*.png")))
        if not files:
            raise RuntimeError(f"No PNG frames found in directory: {p}")
        for f in files:
            frame = cv2.imread(f)
            if frame is None:
                raise RuntimeError(f"Cannot read frame: {f}")
            yield frame
    else:
        capture = cv2.VideoCapture(str(p))
        if not capture.isOpened():
            raise RuntimeError(f"Cannot open video: {p}")
        try:
            while True:
                ok, frame = capture.read()
                if not ok:
                    break
                yield frame
        finally:
            capture.release()


def analyze_series(path, base_gray, base_pts):
    """Register every frame of `path` against the pattern image.

    Returns per-frame arrays (dx, dy, angle_rad, sharpness) plus the total
    decoded frame count and the number of frames that failed registration
    (fewer than MIN_INLIERS RANSAC inliers). Failed frames are NaN.
    """
    dx_list, dy_list, angle_list, sharp_list = [], [], [], []
    failures = 0
    frame_count = 0
    for frame in read_frames(path):
        frame_count += 1
        small = cv2.resize(frame, (WIDTH, HEIGHT))
        gray = cv2.cvtColor(small, cv2.COLOR_BGR2GRAY)
        sharp_list.append(float(cv2.Laplacian(gray, cv2.CV_64F).var()))

        dx = dy = angle = math.nan
        inlier_count = 0
        tracked, status, _ = cv2.calcOpticalFlowPyrLK(base_gray, gray, base_pts, None)
        if tracked is not None and status is not None:
            valid = status.reshape(-1) == 1
            src = base_pts.reshape(-1, 2)[valid]
            dst = tracked.reshape(-1, 2)[valid]
            if len(src) >= MIN_INLIERS:
                transform, inliers = cv2.estimateAffinePartial2D(
                    src, dst, method=cv2.RANSAC, ransacReprojThreshold=3.0,
                )
                if transform is not None and inliers is not None:
                    inlier_count = int(inliers.sum())
                    if inlier_count >= MIN_INLIERS:
                        dx = float(transform[0, 2])
                        dy = float(transform[1, 2])
                        angle = math.atan2(float(transform[1, 0]), float(transform[0, 0]))
        if inlier_count < MIN_INLIERS:
            failures += 1
        dx_list.append(dx)
        dy_list.append(dy)
        angle_list.append(angle)

    if frame_count == 0:
        raise RuntimeError(f"No frames decoded from: {path}")

    return {
        "dx": np.asarray(dx_list, dtype=float),
        "dy": np.asarray(dy_list, dtype=float),
        "angle": np.asarray(angle_list, dtype=float),
        "sharpness": np.asarray(sharp_list, dtype=float),
        "frame_count": frame_count,
        "failures": failures,
    }


def interpolate_nans(values):
    """Fill NaN gaps by linear interpolation (edges clamp to nearest value)."""
    values = values.copy()
    n = len(values)
    idx = np.arange(n)
    mask = np.isnan(values)
    if mask.all():
        raise RuntimeError("All frames failed registration; cannot interpolate")
    if mask.any():
        values[mask] = np.interp(idx[mask], idx[~mask], values[~mask])
    return values


def gaussian_kernel(sigma, radius):
    xs = np.arange(-radius, radius + 1, dtype=float)
    kernel = np.exp(-0.5 * (xs / sigma) ** 2)
    return kernel / kernel.sum()


def lowpass_reflect(values, sigma=10.0, radius=None):
    """Zero-phase Gaussian smoothing with reflect padding at the edges."""
    if radius is None:
        radius = int(round(3 * sigma))
    kernel = gaussian_kernel(sigma, radius)
    padded = np.pad(values, radius, mode="reflect")
    return np.convolve(padded, kernel, mode="valid")


def split_lf_hf(values):
    """Interpolate NaNs, then split into low-frequency and high-frequency parts."""
    interp = interpolate_nans(values)
    lf = lowpass_reflect(interp)
    hf = interp - lf
    return interp, lf, hf


def pearson(a, b):
    if len(a) < 2 or np.std(a) == 0.0 or np.std(b) == 0.0:
        return float("nan")
    return float(np.corrcoef(a, b)[0, 1])


def percentile(values, amount):
    return float(np.percentile(values, amount)) if len(values) else float("nan")


def analyze_pan_segment(baseline_lf_dx, filtered_lf_dx, start, end):
    """Overshoot/drift/settling of a pan hold, using absolute frame indices."""
    n = min(len(baseline_lf_dx), len(filtered_lf_dx))
    start = max(0, min(start, n))
    end = max(start, min(end, n))
    if end - start < 1:
        return {
            "frame_range": [start, end],
            "diff_mean_px": None,
            "diff_max_abs_px": None,
            "settling_frames": None,
        }
    diff = filtered_lf_dx[start:end] - baseline_lf_dx[start:end]
    abs_diff = np.abs(diff)
    below = abs_diff < PAN_SETTLE_THRESHOLD_PX
    settling_frames = None
    for i in range(len(below)):
        if below[i:].all():
            settling_frames = i
            break
    return {
        "frame_range": [start, end],
        "diff_mean_px": float(diff.mean()),
        "diff_max_abs_px": float(abs_diff.max()),
        "settling_frames": settling_frames,
    }


def summarize(series, skip_frames, common_len):
    """Per-video headline stats, plus the LF/HF dx/dy arrays for cross-video use."""
    dx_interp, dx_lf, dx_hf = split_lf_hf(series["dx"][:common_len])
    dy_interp, dy_lf, dy_hf = split_lf_hf(series["dy"][:common_len])

    window = slice(skip_frames, common_len)
    hf_mag = np.hypot(dx_hf, dy_hf)[window]
    jump = np.hypot(np.diff(dx_interp), np.diff(dy_interp))
    jump_window = jump[max(0, skip_frames - 1):]

    stats = {
        "frames_decoded": series["frame_count"],
        "registration_failures": series["failures"],
        "hf_median_px": percentile(hf_mag, 50),
        "hf_rms_px": float(np.sqrt(np.mean(np.square(hf_mag)))) if len(hf_mag) else float("nan"),
        "hf_p95_px": percentile(hf_mag, 95),
        "max_frame_jump_px": float(jump_window.max()) if len(jump_window) else float("nan"),
        "sharpness_median": percentile(series["sharpness"][:common_len][window], 50),
    }
    return stats, dx_lf, dy_lf


def reduction_percent(base_value, filt_value):
    if base_value is None or not math.isfinite(base_value) or base_value == 0:
        return None
    return round(100.0 * (base_value - filt_value) / base_value, 3)


def amplitude_ratio(base_series, filt_series):
    base_std = float(np.std(base_series))
    if base_std == 0.0:
        return float("nan")
    return float(np.std(filt_series) / base_std)


def build_report(args, baseline_series, filtered_series):
    common_len = min(baseline_series["frame_count"], filtered_series["frame_count"])
    skip_frames = min(max(0, args.skip_frames), max(0, common_len - 1))

    baseline_stats, base_dx_lf, base_dy_lf = summarize(baseline_series, skip_frames, common_len)
    filtered_stats, filt_dx_lf, filt_dy_lf = summarize(filtered_series, skip_frames, common_len)

    window = slice(skip_frames, common_len)

    report = {
        "sample_name": args.sample_name,
        "skip_frames": skip_frames,
        "frames": common_len - skip_frames,
        "baseline": baseline_stats,
        "filtered": filtered_stats,
        "hf_reduction_percent": {
            "median": reduction_percent(baseline_stats["hf_median_px"], filtered_stats["hf_median_px"]),
            "rms": reduction_percent(baseline_stats["hf_rms_px"], filtered_stats["hf_rms_px"]),
            "p95": reduction_percent(baseline_stats["hf_p95_px"], filtered_stats["hf_p95_px"]),
        },
        "lf_correlation": {
            "dx": pearson(base_dx_lf[window], filt_dx_lf[window]),
            "dy": pearson(base_dy_lf[window], filt_dy_lf[window]),
        },
        "lf_amplitude_ratio": {
            "dx": amplitude_ratio(base_dx_lf[window], filt_dx_lf[window]),
            "dy": amplitude_ratio(base_dy_lf[window], filt_dy_lf[window]),
        },
        "max_frame_jump_reduction_percent": reduction_percent(
            baseline_stats["max_frame_jump_px"], filtered_stats["max_frame_jump_px"]),
        "sharpness_ratio": (
            filtered_stats["sharpness_median"] / baseline_stats["sharpness_median"]
            if baseline_stats["sharpness_median"] else float("nan")
        ),
    }

    if args.sample_name and "pan" in args.sample_name.lower():
        report["pan"] = {
            name: analyze_pan_segment(base_dx_lf, filt_dx_lf, start, end)
            for name, (start, end) in PAN_HOLD_SEGMENTS.items()
        }

    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", required=True,
                         help="baseline video file or PNG-sequence directory")
    parser.add_argument("--filtered", required=True,
                         help="filtered/stabilized video file or PNG-sequence directory")
    parser.add_argument("--pattern", default=str(DEFAULT_PATTERN),
                         help="registration target image (default: docs/examples/pattern.png)")
    parser.add_argument("--sample-name", default="",
                         help="sample identifier, e.g. fine-shake or pan-shake; enables "
                              "pan-hold analysis when it contains 'pan'")
    parser.add_argument("--skip-frames", type=int, default=45,
                         help="leading frames excluded from headline stats (default: 45)")
    parser.add_argument("--json-out", default=None,
                         help="also write the JSON report to this file")
    args = parser.parse_args()

    pattern = cv2.imread(args.pattern)
    if pattern is None:
        raise SystemExit(f"Cannot read pattern image: {args.pattern}")
    base = cv2.resize(pattern, (WIDTH, HEIGHT), interpolation=cv2.INTER_AREA)
    base_gray = cv2.cvtColor(base, cv2.COLOR_BGR2GRAY)
    base_pts = cv2.goodFeaturesToTrack(base_gray, maxCorners=800, qualityLevel=0.01, minDistance=8)
    if base_pts is None or len(base_pts) < MIN_INLIERS:
        raise SystemExit("Not enough trackable features in pattern image")

    baseline_series = analyze_series(args.baseline, base_gray, base_pts)
    filtered_series = analyze_series(args.filtered, base_gray, base_pts)

    report = build_report(args, baseline_series, filtered_series)
    text = json.dumps(report, indent=2)
    print(text)
    if args.json_out:
        Path(args.json_out).write_text(text + "\n")


if __name__ == "__main__":
    main()
