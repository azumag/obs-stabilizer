# Error Handling Policy

This document defines the error-handling contract for the OBS Stabilizer processing pipeline. It complements the existing exception logging in `StabilizerCore` and provides a common policy for future error propagation and telemetry work.

## Goals

- Never allow an exception to cross an OBS C callback boundary.
- Preserve the input frame when a recoverable processing failure occurs.
- Return an empty frame only when the input itself is invalid or no safe output exists.
- Record a stable, machine-readable error category in addition to a human-readable message.
- Avoid logging the same failure repeatedly at frame rate.

## Error categories

Implementations should map failures to one of these stable categories:

| Category | Meaning | Default fallback |
| --- | --- | --- |
| `none` | No current error | Continue normally |
| `invalid_input` | Empty frame, unsupported format, or invalid dimensions | Reject the frame or return an empty result |
| `not_initialized` | Processing requested before successful initialization | Return the original frame |
| `tracking_failure` | Feature tracking or transform estimation failed | Return the original frame and allow recovery |
| `opencv_exception` | `cv::Exception` from OpenCV | Return the original frame |
| `standard_exception` | Other `std::exception` | Return the original frame |
| `unknown_exception` | Non-standard exception | Return the original frame |
| `internal_error` | Invariant violation or unexpected internal state | Return the safest available frame |

The category names are part of the telemetry contract and should not contain exception messages, file paths, or other high-cardinality data.

## Boundary rules

### `StabilizerCore`

`StabilizerCore` owns detailed algorithm errors. Public processing methods must catch `cv::Exception`, then `std::exception`, then unknown exceptions. They must update the last error message before returning a fallback.

### `StabilizerWrapper`

`StabilizerWrapper` is the synchronization and RAII boundary. It must not erase a useful core error. Wrapper-specific failures should add context while preserving the underlying message where possible.

### OBS callbacks

OBS callbacks are C ABI boundaries. No exception may escape them. Callbacks should log one concise message and return the original OBS frame or a null object according to the callback contract.

## Logging format

Use a consistent structure:

```text
component=<component> operation=<operation> category=<category> message=<message>
```

Example:

```text
component=StabilizerCore operation=process_frame category=opencv_exception message=<OpenCV message>
```

Messages may contain exception details, but telemetry labels must use only the stable category. Do not include full frame data, image content, or user paths in logs.

## Error propagation

The current API exposes `get_last_error()`. New code should pair the message with a stable error category. Until a typed result API is introduced, callers should use this contract:

1. An empty result means no safe processed frame is available.
2. A non-empty result equal to the input may be a deliberate recoverable fallback.
3. `get_last_error()` identifies the most recent failure and should be cleared after successful initialization or reset.
4. Callers must not infer an error category by parsing arbitrary exception text; use the stable category once implemented.

A future typed result can use an `ErrorCode` enum and a lightweight result object without changing the OBS callback behavior.

## Telemetry

Telemetry must remain local and low-cardinality. At minimum, track:

- total processing attempts;
- successful frames;
- tracking failures;
- exception count by stable category;
- the timestamp or frame number of the last error.

Do not use exception type names returned by `typeid(...).name()` as metric labels because names are implementation-dependent and can create unbounded label sets.

### Rate limiting

Repeated frame-level failures can otherwise flood OBS logs. Log the first occurrence immediately, then aggregate repeated occurrences. A recommended policy is to log again when the count reaches a power of two or after a fixed time window.

## Testing requirements

Changes to error handling should include tests for:

- empty and invalid frames;
- unsupported channel formats;
- OpenCV exception fallback where a deterministic trigger is available;
- preservation of the original frame for recoverable failures;
- stable error category and message updates;
- metric increment and reset behavior;
- no exception escaping the wrapper or OBS callback boundary.

Tests must not depend on exact vendor-specific OpenCV exception text.

## Operational guidance

- A single recoverable tracking failure is expected and should not be treated as fatal.
- Repeated exceptions indicate a configuration, format, or implementation problem and should be surfaced through aggregated metrics.
- Unknown exceptions are always logged as errors and should be investigated before release.
- Error handling must not allocate large buffers or perform expensive formatting on the normal frame path.
