# Architecture Decisions

This document records the decisions that currently shape OBS Stabilizer. It complements `ARCHITECTURE.md` by explaining why the current boundaries exist, which guarantees callers may rely on, and which changes require a dedicated design review.

## ADR-001: Keep the OBS boundary thin

**Status:** Accepted

`src/stabilizer_opencv.cpp` owns OBS registration, property callbacks, settings conversion, and frame callback integration. Stabilization algorithms must remain outside this file.

**Rationale**

- OBS callbacks are C ABI boundaries and must not allow C++ exceptions to escape.
- Keeping OBS-specific code thin allows the core to be tested without loading OBS.
- UI and lifecycle changes should not require changes to motion estimation.

**Consequences**

- `StabilizerWrapper` is the exception-safe bridge between callbacks and `StabilizerCore`.
- New algorithm behavior belongs in `src/core/`.
- New OBS settings must be converted into validated core parameters before processing.

## ADR-002: Use OpenCV types inside the current core boundary

**Status:** Accepted, with future reconsideration

`StabilizerCore` currently accepts and returns `cv::Mat`.

**Rationale**

- Feature detection, optical flow, affine estimation, and warping are all implemented with OpenCV.
- Introducing an image abstraction before a second backend exists would add ownership and conversion complexity without reducing current runtime cost.

**Consequences**

- OpenCV remains a required core dependency.
- A future abstraction layer must define pixel format, stride, ownership, mutability, and lifetime explicitly before replacing `cv::Mat` in public interfaces.
- Conversion at the OBS boundary should remain observable and covered by tests.

## ADR-003: Process frames synchronously

**Status:** Accepted

A filter instance processes each frame synchronously in the OBS callback path. The core does not create worker threads.

**Rationale**

- Synchronous processing preserves frame order and keeps state transitions deterministic.
- Background queues would require explicit backpressure, cancellation, frame lifetime, and shutdown rules.
- Predictable latency is more important than maximizing throughput for a real-time filter.

**Consequences**

- Frame processing must remain bounded and avoid blocking I/O.
- Mutable state is scoped to a filter instance.
- Cross-thread settings updates must pass through the wrapper's synchronization policy.
- Any asynchronous design requires a separate ADR covering queue depth, dropped-frame behavior, ownership, and shutdown.

## ADR-004: Validate parameters before they reach the algorithm

**Status:** Accepted

User-provided settings are normalized through the parameter validation layer before use by `StabilizerCore`.

**Rationale**

- OBS settings can be missing, stale, or outside supported ranges.
- Central validation prevents each algorithm stage from implementing slightly different fallback behavior.

**Consequences**

- Core code may assume validated ranges after initialization or update.
- Validation changes require boundary-value tests.
- Clamping behavior is part of the user-visible compatibility contract and should not change silently.

## ADR-005: Prefer graceful frame fallback over terminating OBS

**Status:** Accepted

Invalid frames, tracking failures, and recoverable OpenCV errors must not terminate the host process.

**Rationale**

- A video filter runs inside OBS; an unhandled exception would affect the entire broadcast or recording session.
- A single unusable frame is less severe than a process crash.

**Consequences**

- Exceptions are caught at C++ and OBS boundaries.
- Failures must be logged with enough context to diagnose the category without logging every pixel or frame.
- The filter returns an empty or pass-through result according to the documented API contract.
- Repeated failures should be rate-limited when telemetry is implemented.

## ADR-006: Keep edge handling explicit

**Status:** Accepted

Padding, cropping, and scaling are explicit modes rather than hidden post-processing choices.

**Rationale**

- Each mode trades field of view, black borders, and geometric distortion differently.
- No single choice is correct for gaming, webcam, and recording workloads.

**Consequences**

- Presets may select defaults, but users retain control.
- New edge modes require visual-quality tests and documented performance impact.
- Output dimensions must remain consistent with the selected mode's public contract.

## ADR-007: Treat frame ownership as a first-class contract

**Status:** Accepted

OBS frame buffers and OpenCV matrices may reference external memory. Ownership and lifetime must be explicit at every conversion boundary.

**Rationale**

- Avoiding copies can improve performance but can also expose buffers after OBS or a temporary object has released them.
- Static or shared frame buffers can be overwritten before consumers finish reading them.

**Consequences**

- A conversion that returns a non-owning view must document the source lifetime.
- A returned frame must not depend on mutable scratch storage that may be overwritten by the next call unless that limitation is explicit and enforced.
- Copy-elimination work must be paired with lifecycle tests and cannot be treated as a local micro-optimization.

## ADR-008: Separate deterministic correctness tests from environment-sensitive performance tests

**Status:** Accepted

Functional tests must be deterministic across supported CI runners. Absolute CPU usage and timing thresholds belong in dedicated performance jobs.

**Rationale**

- Shared CI runners have variable load and hardware.
- Flaky performance assertions reduce trust in the entire test suite.

**Consequences**

- Unit and integration jobs verify correctness, ownership, error handling, and invariants.
- Performance jobs may use wider tolerances, dedicated runners, or trend-based baselines.
- A performance regression must include the environment and baseline used to reproduce it.

## Change process

Create a new ADR, or update this document, when a change affects one or more of the following:

- public API ownership or lifetime;
- threading, queues, or frame ordering;
- OBS callback responsibilities;
- image representation or OpenCV coupling;
- error propagation and fallback behavior;
- parameter validation compatibility;
- supported output dimensions or edge semantics.

Implementation PRs should link the relevant decision and state whether they preserve or supersede it.
