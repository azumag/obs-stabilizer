# OBS Stabilizer API Reference

This document describes the main internal C++ interfaces. It is intended for contributors; the plugin does not currently provide a stable external binary API.

## `StabilizerCore`

Declared in `src/core/stabilizer_core.hpp`.

### Purpose

Owns the stabilization algorithm state for one stream: previous grayscale frame, tracked points, transform history, validated parameters, metrics, and last-error text.

### `bool initialize(uint32_t width, uint32_t height, const StabilizerParams& params)`

Initializes or resets the core for a frame size and parameter set.

- `width`, `height`: input frame dimensions; must be non-zero and large enough for feature detection.
- `params`: requested settings; values are validated and clamped.
- Returns `true` on success and `false` when dimensions or configuration cannot be accepted.
- On failure, `get_last_error()` provides details.

### `cv::Mat process_frame(const cv::Mat& frame)`

Processes one frame.

- `frame`: source image supported by the frame conversion helpers.
- Returns a stabilized image when processing succeeds.
- May return the original frame when stabilization is disabled or recovery from tracking failure is safe.
- May return an empty matrix for invalid input that cannot be processed.
- Updates processing metrics and error state.

The core is not a general thread-safe object. Access mutable state through the wrapper used by the integration layer.

### Parameter updates

Parameter-setting functions accept a `StabilizerParams` value and apply shared validation. Changes that invalidate tracking assumptions may reset internal state.

### Metrics

The metrics structure tracks values such as processed and successful frame counts, tracking failures, and average processing time. Treat metrics as observational data rather than synchronization primitives.

### Error access

`get_last_error()` returns the most recent actionable error string. A successful later operation may replace or clear prior context depending on the call path.

### Presets

Static preset helpers return parameter sets for:

- Gaming: responsiveness-oriented;
- Streaming: balanced default;
- Recording: stronger smoothing.

Preset values should be passed through normal validation before use.

## `StabilizerWrapper`

Declared in `src/core/stabilizer_wrapper.hpp`.

### Purpose

Provides the lifecycle and synchronization boundary used by OBS integration. It owns or coordinates a `StabilizerCore` instance and should be preferred by callers that may update settings while frames are processed.

### Initialization

The wrapper initialization method accepts frame dimensions and `StabilizerParams`, returning success or failure while preserving the underlying error description.

### Frame processing

The wrapper processing method delegates to `StabilizerCore` while enforcing its synchronization policy.

### Status and errors

- `is_initialized()` reports wrapper/core readiness.
- `get_last_error()` exposes the latest initialization or processing error.

Do not use readiness checks as a substitute for calling initialization on the first valid frame.

## Frame utilities

Declared under `src/core/frame_utils.*`.

### Color conversion

`FRAME_UTILS::ColorConversion::convert_to_grayscale` converts supported OpenCV input formats to a single-channel grayscale matrix.

- Returns an empty matrix for unsupported input.
- Callers should log channel and dimension context on failure.

### OBS/OpenCV conversion

Conversion helpers map OBS source-frame buffers to OpenCV matrices and back. Their ownership rules are critical:

- Never retain a view after its underlying OBS buffer lifetime ends.
- Avoid unnecessary deep copies, but do not transfer ownership implicitly.
- Treat writable output buffers as scoped to the current filter callback unless explicitly documented otherwise.

## Parameter validation

Declared in `src/core/parameter_validation.hpp`.

### `VALIDATION::validate_parameters`

Returns a safe, normalized `StabilizerParams` value. Use this function at settings boundaries instead of duplicating clamps in UI, wrapper, or algorithm code.

Validation covers smoothing limits, feature detection values, correction limits, and related algorithm constraints.

## Preset manager

Declared in `src/core/preset_manager.*`.

### Purpose

Serializes and loads named parameter sets. JSON support depends on the nlohmann/json headers found during configuration.

Callers should:

- validate loaded values before applying them;
- handle missing or malformed files without terminating frame processing;
- avoid overwriting built-in preset semantics with user files;
- report filesystem errors with the affected path.

## OBS integration callbacks

Implemented in `src/stabilizer_opencv.cpp`.

### Create

Allocates filter context, records the OBS source, loads initial settings, and initializes metrics. The algorithm may wait for the first valid frame before dimension-dependent initialization.

### Destroy

Releases context through RAII. Destruction must not leak frame buffers, OpenCV matrices, or wrapper state.

### Update

Converts OBS settings to validated parameters and applies them to an initialized wrapper when dimensions are available.

### Filter video

Receives one OBS frame, initializes using its dimensions when necessary, converts it to OpenCV, processes it, updates metrics, and converts the result back.

Failure paths should return the original frame when doing so is safe for OBS.

### Properties and defaults

The properties callback builds the OBS UI. Setting keys form a compatibility surface; renaming them can break existing scene collections. Defaults should be generated from the same preset and validation definitions used by runtime code.

## Compatibility policy

Internal class and function names may change between releases. OBS setting keys, preset identifiers, documented behavior, and serialized preset formats require migration planning when changed.

## Adding API documentation

For every new public or cross-module interface, document:

- ownership and lifetime;
- thread-safety expectations;
- valid input ranges;
- return and failure behavior;
- side effects on tracking state or metrics;
- compatibility implications.
