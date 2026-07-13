# GPU Acceleration Evaluation

**Status:** Phase 1 recommendation  
**Related issue:** #329  
**Scope:** Architecture and implementation planning; no GPU runtime is enabled by this document.

## 1. Decision summary

The recommended path is an incremental hybrid design:

1. Keep the existing CPU path as the reference implementation and mandatory fallback.
2. Add an OpenCL-backed `cv::UMat` prototype first because it is the lowest-risk cross-platform option available through standard OpenCV builds.
3. Add an optional CUDA backend only after measurements show that optical flow and warping dominate end-to-end frame time and that transfer overhead is acceptable.
4. Treat Metal as a separate macOS-specific investigation. OpenCV does not provide a direct Metal implementation for this pipeline, so a production Metal backend would require a substantially larger native implementation and maintenance commitment.
5. Do not introduce OpenVINO, TensorRT, or custom CUDA kernels for the current feature-based algorithm. Their dependency and maintenance costs are not justified until the simpler OpenCV backends have been measured.

GPU acceleration must remain opt-in until it demonstrates a repeatable end-to-end improvement, including upload and download costs, on supported hardware.

## 2. Current CPU pipeline

The current stabilization pipeline performs the following expensive operations for each frame:

1. OBS frame to OpenCV image conversion.
2. Grayscale conversion.
3. Feature detection with `goodFeaturesToTrack()`.
4. Pyramidal Lucas-Kanade optical flow with `calcOpticalFlowPyrLK()`.
5. Affine transform estimation and smoothing.
6. Output transformation with `warpAffine()`.
7. OpenCV image to OBS frame conversion.

Feature detection, optical flow, and affine warping are the primary GPU candidates. Transform estimation and smoothing operate on small data sets and should remain on the CPU.

## 3. Options evaluated

### 3.1 OpenCL through `cv::UMat`

**Fit:** Best first prototype.

**Advantages**

- Cross-vendor support on Windows and Linux where a functioning OpenCL runtime is present.
- Smallest expected source-code divergence from the current OpenCV implementation.
- Runtime fallback remains possible without distributing a CUDA-specific binary.
- Useful for measuring whether persistent device-side frame storage can offset transfer costs.

**Limitations**

- Actual acceleration depends on the OpenCV build, driver, and operation coverage.
- Transparent API execution can silently fall back to CPU, so backend telemetry is required.
- Apple has deprecated OpenCL; it must not be considered the long-term macOS backend.
- Repeated `Mat`/`UMat` transitions can eliminate performance gains.

**Estimated effort:** 3-5 engineering days for a guarded prototype and benchmark harness; additional platform validation afterward.

### 3.2 OpenCV CUDA modules

**Fit:** Strong NVIDIA-specific option after the OpenCL prototype.

**Advantages**

- GPU implementations exist for the pipeline's expensive operations.
- Explicit device buffers make data movement and execution behavior observable.
- Likely to provide the best result for 1440p and 4K on supported NVIDIA GPUs.

**Limitations**

- Requires an OpenCV build with CUDA modules and compatible CUDA runtime.
- Packaging and CI become significantly more complex.
- NVIDIA-only; a CPU or OpenCL path is still required.
- Upload/download overhead may make low-resolution frames slower than CPU processing.

**Estimated effort:** 1-2 weeks for a prototype, build integration, fallback handling, and representative NVIDIA validation.

### 3.3 Apple Metal

**Fit:** Future macOS-specific project, not the first implementation.

**Advantages**

- Native acceleration and efficient unified memory access on Apple Silicon.
- Potentially avoids some discrete-GPU transfer costs.

**Limitations**

- No direct OpenCV Metal equivalent for the complete pipeline.
- Requires Objective-C++/Metal shaders or another native image-processing framework.
- Creates a separate implementation that must be kept behaviorally aligned with CPU/OpenCV paths.
- Requires dedicated Apple Silicon performance and compatibility testing.

**Estimated effort:** At least 3-6 weeks for a meaningful prototype, excluding production hardening.

### 3.4 Custom CUDA, OpenVINO, and TensorRT

These are not recommended for the current algorithm.

- Custom CUDA duplicates mature OpenCV functionality and raises maintenance cost.
- OpenVINO and TensorRT are optimized primarily for inference workloads, while the current pipeline is classical computer vision.
- Each option adds deployment complexity without first proving that standard OpenCV acceleration is insufficient.

## 4. Benchmark plan

All backend decisions must use end-to-end measurements from the OBS-facing frame boundary, not isolated kernel timings.

### 4.1 Test matrix

Measure at least:

- 1280x720 at 60 fps
- 1920x1080 at 30 and 60 fps
- 2560x1440 at 30 and 60 fps
- 3840x2160 at 30 fps

Use three representative scenes:

- Low texture / few features
- Normal indoor camera scene
- High texture / rapid motion

### 4.2 Metrics

Capture:

- Mean, median, p95, and p99 frame-processing time
- Sustained processed fps
- CPU utilization
- GPU utilization where available
- Host-to-device and device-to-host time
- Number of tracked features
- Frame drops and fallback count
- Output equivalence or bounded transform deviation from the CPU reference
- Peak resident memory and GPU memory

Run a warm-up period before measurement and collect at least 1,000 frames per case. Record hardware, OS, driver, OpenCV build flags, OBS version, and compiler configuration with every result.

### 4.3 Success criteria

A backend is eligible for production integration only when it:

- Improves p95 end-to-end processing time by at least 25% at 1440p or 4K.
- Does not regress 1080p p95 time by more than 10% when enabled.
- Produces stabilization output within agreed numerical tolerances of the CPU reference.
- Falls back to CPU without dropping or corrupting the current frame.
- Adds no mandatory GPU dependency to the default CPU build.

The 25% threshold intentionally leaves margin for driver variance and OBS workload contention.

## 5. Proposed architecture

Introduce a narrow processing-backend boundary rather than adding GPU conditionals throughout `StabilizerCore`.

```text
StabilizerCore
  -> FrameProcessingBackend
       -> CpuOpenCvBackend       (required reference/fallback)
       -> OpenClOpenCvBackend    (optional prototype)
       -> CudaOpenCvBackend      (optional build)
       -> MetalBackend           (future, separate project)
```

The backend should own persistent working buffers so frames are not uploaded and downloaded between every operation. Small transform data can cross back to the CPU for estimation and smoothing.

The interface should expose:

- Capability probing
- Backend initialization and failure reason
- Feature detection
- Optical flow
- Affine warp
- Runtime statistics and fallback count

Backend selection should use the deterministic policy introduced for #319, while concrete capability detection remains platform-specific.

## 6. Implementation sequence

### Phase A: CPU baseline

- Add an end-to-end benchmark around the real processing path.
- Record reproducible baseline results and build metadata.
- Add transform-output comparison utilities.

### Phase B: OpenCL prototype

- Add compile-time and runtime capability detection.
- Keep grayscale, feature images, pyramids, and output buffers as `cv::UMat` across operations.
- Emit telemetry when an operation falls back to CPU.
- Compare end-to-end results against the CPU baseline.

### Phase C: CUDA decision

Proceed only if OpenCL is unavailable on important target systems or cannot meet the success criteria.

- Add an optional CUDA-enabled build profile.
- Implement persistent `GpuMat` buffers.
- Benchmark transfer-inclusive performance.
- Package CUDA support separately from the default binary if runtime requirements cannot be isolated cleanly.

### Phase D: Product integration

- Add `Auto`, `CPU`, and available GPU choices to settings.
- Default to `Auto`, but prefer CPU until a backend has passed a short capability/self-test.
- Display the selected backend and fallback reason in diagnostics.
- Preserve settings compatibility when a requested backend is unavailable.

## 7. Risks and mitigations

| Risk | Mitigation |
| --- | --- |
| Transfer overhead exceeds kernel savings | Persist device buffers and measure end-to-end time before integration. |
| Driver or OpenCV build lacks requested support | Runtime capability checks and mandatory CPU fallback. |
| GPU output diverges from CPU behavior | Golden transform comparisons with explicit tolerances. |
| CI cannot provide every GPU vendor | Keep CPU CI mandatory; run vendor-specific benchmarks on labelled/self-hosted hardware before release. |
| Packaging size and dependencies increase | Compile GPU backends optionally and keep the default CPU artifact independent. |
| GPU contention harms OBS rendering | Track p95/p99 latency and frame drops under realistic OBS workloads, not standalone benchmarks only. |

## 8. Recommendation

Start with the CPU baseline and OpenCL `UMat` prototype. This produces the highest-value evidence with the least platform lock-in. Evaluate CUDA second for NVIDIA systems if measurements justify the extra build and packaging complexity. Defer Metal until there is verified macOS demand and capacity to maintain an independent native backend.

No estimated performance gain should be treated as a product claim until the benchmark matrix is executed on identified hardware. The immediate deliverable after this evaluation should be a benchmark issue, followed by a narrowly scoped OpenCL prototype issue.