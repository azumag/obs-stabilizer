# Current Architecture

This document describes the repository's current module boundaries and runtime data flow. It complements `ARCHITECTURE.md`, which contains broader requirements and design history, and `ARCHITECTURE_DECISIONS.md`, which records the rationale behind important decisions.

## Runtime boundary

OBS owns source frames and invokes the plugin filter callbacks. The plugin layer must therefore remain a narrow boundary that translates OBS settings and frame data into the core API, catches failures before they cross the C ABI boundary, and returns either a processed frame or a safe fallback.

```text
OBS source frame
      |
      v
stabilizer_opencv.cpp
  - filter lifecycle
  - properties and callbacks
  - OBS/OpenCV boundary
      |
      v
StabilizerWrapper
  - RAII ownership
  - exception-safe facade
  - parameter and metrics access
      |
      v
StabilizerCore
  - feature detection
  - optical flow
  - motion estimation
  - trajectory smoothing
  - edge handling
      |
      v
processed cv::Mat / fallback frame
```

## Module responsibilities

### `src/stabilizer_opencv.cpp`

The plugin integration layer registers `obs_source_info`, creates and destroys one filter instance per OBS source, converts OBS settings into `StabilizerParams`, and delegates frame processing. It should not contain stabilization algorithms.

### `src/core/stabilizer_wrapper.*`

`StabilizerWrapper` is the ownership and error boundary between the plugin layer and the core engine. It exposes a stable facade for initialization, parameter updates, frame processing, reset, status, and metrics while preventing C++ exceptions from escaping into OBS callbacks.

### `src/core/stabilizer_core.*`

`StabilizerCore` owns the temporal state required to compare adjacent frames. Its processing pipeline is:

1. Validate readiness and input frame properties.
2. Convert the current frame to the working grayscale representation.
3. Detect or reuse feature points.
4. Track features between the previous and current frame with pyramidal Lucas-Kanade optical flow.
5. Reject invalid correspondences and estimate frame-to-frame motion.
6. Append the motion to the trajectory and smooth it according to the active parameters.
7. Derive the corrective transform.
8. Warp the frame and apply the configured edge mode.
9. Update temporal state and performance metrics.

The core currently processes frames synchronously. Any future asynchronous pipeline must preserve frame ordering and must make frame ownership explicit.

### `src/core/frame_utils.*`

Frame utilities contain OBS-frame validation, pixel-format conversion, stride-aware data access, and conversion back to an OBS-compatible frame. This module is the only place where raw frame layout assumptions should be concentrated.

### `src/core/parameter_validation.hpp`

Parameter validation clamps externally supplied values before they reach OpenCV algorithms. Callers may request out-of-range values, but algorithm code should only receive normalized values.

### `src/core/preset_manager.*`

Preset management serializes and restores named parameter sets. It does not perform frame processing and should remain independent of the temporal state in `StabilizerCore`.

### `src/core/stabilizer_constants.hpp`

Constants define supported ranges and defaults shared by validation, presets, UI construction, and tests. A range change should be made here first and accompanied by validation and preset tests.

## Adaptive stabilization flow

The implementation adapts to frame content through the number and quality of successfully tracked points rather than switching to a separate algorithm. When a frame does not provide enough reliable correspondences, the core avoids applying an untrustworthy transform and preserves a safe output. On later frames it can reacquire features and resume normal estimation.

Parameters influencing this behavior include:

- feature count and feature quality threshold;
- minimum distance between detected points;
- optical-flow window and pyramid settings;
- smoothing radius and correction limit;
- edge mode used after applying the correction.

This means parameter updates affect subsequent frames immediately, while temporal reset is reserved for changes that invalidate accumulated history or when processing must recover from an unusable state.

## State and ownership

Each OBS filter instance owns its own wrapper and core state. The previous grayscale frame, tracked points, motion history, smoothing history, and metrics belong to that instance and must not be shared globally.

Input frames remain owned by OBS. OpenCV views may reference OBS memory only while the callback is active. Any data retained after the callback must own its storage. Returned frame buffers must remain valid for the lifetime required by OBS; changes to frame conversion or buffer reuse must be reviewed together with the frame-lifecycle contract.

## Failure behavior

Failures are contained at module boundaries:

- invalid settings are normalized before algorithm use;
- invalid or unsupported frames produce a safe fallback instead of terminating OBS;
- insufficient tracking data skips unreliable correction;
- OpenCV and standard exceptions are logged and converted into a non-throwing result at the wrapper/plugin boundary;
- reset clears temporal state so processing can restart from the next valid frame.

## Test mapping

The current test suite is organized around these boundaries:

- `test_stabilizer_core.cpp`: initialization, processing modes, transforms, metrics, and content bounds;
- `test_edge_cases.cpp`: invalid and boundary inputs;
- `test_integration.cpp` and `test_obs_integration.cpp`: interactions across modules and OBS-facing behavior;
- `test_frame_utils.cpp`: frame validation and conversion;
- `test_preset_manager.cpp`: preset persistence;
- `test_thread_safety.cpp`: concurrent access contracts;
- `test_memory_leaks.cpp`: long-running resource ownership;
- performance and visual-quality tests: non-functional behavior.

A change to a module boundary should update this document and add tests at the closest corresponding layer.