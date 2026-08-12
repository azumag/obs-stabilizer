#!/usr/bin/env python3
"""Generate deterministic shake sample videos from the pattern image.

Each sample is a 640x360, 30 fps, 24 second H.264 video. The motion is
applied as a per-frame affine warp (translation + slight rotation) around
the frame center, which is exactly the motion model the stabilizer tracks.
"""

import argparse
import math
import random
from pathlib import Path

import cv2
import numpy as np


OUTPUT_DIR = Path(__file__).resolve().parent
PATTERN = OUTPUT_DIR / "pattern.png"
WIDTH, HEIGHT = 640, 360
FPS = 30
DURATION_SECONDS = 24
TOTAL_FRAMES = FPS * DURATION_SECONDS


def make_offsets(seed, amp_px, low_hz, high_hz, rot_deg=0.0):
    """Return per-frame (dx, dy, angle) offset lists.

    A slow sinusoid plus a faster sinusoid approximates hand-held camera
    motion (low-frequency sway plus high-frequency micro-jitter).
    """
    rng = random.Random(seed)
    offsets = []
    for i in range(TOTAL_FRAMES):
        t = i / FPS
        phase_low = 2.0 * math.pi * low_hz * t
        phase_high = 2.0 * math.pi * high_hz * t
        dx = amp_px * (0.7 * math.sin(phase_low) + 0.3 * math.sin(phase_high))
        dy = amp_px * (0.6 * math.sin(phase_low + 1.1) + 0.4 * math.sin(phase_high + 0.6))
        dx += rng.uniform(-0.3, 0.3) * amp_px
        dy += rng.uniform(-0.3, 0.3) * amp_px
        angle = rot_deg * math.sin(phase_low + 0.3)
        offsets.append((dx, dy, angle))
    return offsets


def render(base, offsets, out_path):
    writer = cv2.VideoWriter(
        str(out_path),
        cv2.VideoWriter_fourcc(*"avc1"),
        FPS,
        (WIDTH, HEIGHT),
    )
    cx, cy = WIDTH / 2.0, HEIGHT / 2.0
    for dx, dy, angle_deg in offsets:
        a = math.radians(angle_deg)
        cos_a, sin_a = math.cos(a), math.sin(a)
        # Build a 2x3 affine around the frame center: rotation + translation.
        m = np.array(
            [
                [cos_a, -sin_a, (1 - cos_a) * cx + sin_a * cy + dx],
                [sin_a, cos_a, -sin_a * cx + (1 - cos_a) * cy + dy],
            ],
            dtype=np.float32,
        )
        warped = cv2.warpAffine(base, m, (WIDTH, HEIGHT), borderMode=cv2.BORDER_REFLECT)
        writer.write(warped)
    writer.release()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pattern", default=str(PATTERN))
    args = parser.parse_args()

    pattern = cv2.imread(args.pattern)
    if pattern is None:
        raise SystemExit(f"Cannot read pattern image: {args.pattern}")
    base = cv2.resize(pattern, (WIDTH, HEIGHT), interpolation=cv2.INTER_AREA)

    samples = {
        # Fine high-frequency shake: small amplitude, fast micro-jitter.
        "fine-shake": dict(seed=11, amp_px=2.0, low_hz=1.5, high_hz=9.0, rot_deg=0.15),
        # Large low-frequency shake: strong sway, camera-like drift.
        "large-shake": dict(seed=22, amp_px=22.0, low_hz=0.7, high_hz=3.0, rot_deg=0.8),
        # Mixed: both fine jitter and large low-frequency sway.
        "mixed-shake": dict(seed=33, amp_px=10.0, low_hz=0.5, high_hz=7.0, rot_deg=0.4),
    }

    for name, params in samples.items():
        offsets = make_offsets(**params)
        out_path = OUTPUT_DIR / f"{name}.mp4"
        render(base, offsets, out_path)
        print(f"wrote {out_path} ({out_path.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
