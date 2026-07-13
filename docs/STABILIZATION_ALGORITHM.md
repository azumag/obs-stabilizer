# Stabilization Algorithm Notes

This document records the design rationale behind the transform smoothing path implemented in `StabilizerCore`. It complements the public API comments by explaining why the current algorithm uses a bounded history and a direct arithmetic mean of affine transform coefficients.

## Processing pipeline

For each frame after initialization, the core performs the following steps:

1. Convert the frame to grayscale.
2. Track previously detected feature points with pyramidal Lucas–Kanade optical flow.
3. Estimate a 2×3 partial affine transform with RANSAC.
4. Clamp transform components to the configured maximum correction.
5. Append the transform to a bounded history controlled by `smoothing_radius`.
6. Average the transform history coefficient by coefficient.
7. Warp the current frame and apply the selected edge-handling mode.

When tracking or transform estimation fails, the core preserves stream continuity by returning the original frame instead of propagating an invalid transform.

## Why a bounded moving average is used

The smoothing history is a `std::deque<cv::Mat>` whose length never exceeds `smoothing_radius`. The current implementation computes an arithmetic mean over the six coefficients of each 2×3 transform matrix.

This design was chosen because it provides:

- **Deterministic cost:** each frame requires one pass over at most `smoothing_radius` transforms.
- **Bounded memory:** history size is limited directly by configuration.
- **Stable latency:** the algorithm does not require iterative optimization or a second pass.
- **Simple failure behavior:** an empty history maps to an identity transform.
- **Cross-platform behavior:** it does not depend on architecture-specific SIMD code.

The direct coefficient average is intentionally simple. The estimated transform is already constrained to partial affine motion and clamped before entering the history, which limits extreme values that would otherwise distort the mean.

## Matrix layout

OpenCV stores the partial affine transform as a 2×3 matrix:

```text
[a00 a01 tx]
[a10 a11 ty]
```

The implementation accesses the six contiguous `double` values directly. Named indices correspond to:

- `a00`: horizontal scale/rotation component
- `a01`: horizontal shear/rotation component
- `tx`: horizontal translation
- `a10`: vertical shear/rotation component
- `a11`: vertical scale/rotation component
- `ty`: vertical translation

Direct pointer access avoids temporary matrix expressions and keeps accumulation to a single cache-friendly pass. The loop is not manually unrolled beyond the six fixed coefficients because modern compilers can optimize this small, predictable operation without platform-specific branches.

## Complexity and performance characteristics

Let `n` be the number of transforms currently stored, where `n <= smoothing_radius`.

- Time complexity: **O(n)** per frame
- Additional working memory: **O(1)** beyond the returned 2×3 matrix
- Persistent history memory: **O(smoothing_radius)**

The smoothing step is small relative to feature detection, optical flow, and image warping. Optimizing it further should therefore be based on profiling rather than assumption.

## Preconditions and invariants

The smoothing implementation relies on these invariants:

- Every history entry is a non-empty 2×3 `CV_64F` matrix.
- History entries have already passed correction clamping.
- The history is modified by the same serialized processing path that reads it.
- `StabilizerCore` itself is not internally synchronized; callers requiring concurrent access must use `StabilizerWrapper` or provide equivalent serialization.

If the history is empty, the function returns a 2×3 identity transform. This preserves frame dimensions and avoids applying undefined motion.

## Trade-offs

An arithmetic mean can lag behind abrupt intentional camera motion and is not robust to unconstrained outliers. The current pipeline mitigates those limitations through RANSAC estimation, correction clamping, bounded history, and tracking-failure fallback.

Possible future alternatives include weighted moving averages, exponential smoothing, trajectory-domain smoothing, or motion-model-aware filtering. Such changes should include visual-quality benchmarks, latency measurements, and regression tests because they alter stabilization behavior rather than merely implementation details.

## Validation guidance

Changes to transform smoothing should verify at least:

- empty history returns identity;
- one transform is returned unchanged;
- multiple translations are averaged correctly;
- scale and rotation-related coefficients are averaged independently;
- history length remains bounded by `smoothing_radius`;
- invalid tracking does not add undefined transforms;
- concurrent callers use the wrapper-level synchronization contract.
