# OBS Stabilizer Plugin Architecture

## 1. Functional Requirements

- **Real-time Video Stabilization**: The plugin must function as a video filter within OBS Studio to correct shaky video footage in real-time.
- **Configurable Parameters**: Users must be able to adjust stabilization parameters through a dedicated UI panel integrated into the OBS filter properties. Key parameters include:
  - Smoothing Radius
  - Correction Strength
  - Feature Detection Threshold
- **Preset Management**: The plugin should allow users to save and load their preferred settings as presets.

## 2. Non-Functional Requirements

- **Performance**: The plugin must process video streams with minimal latency to be usable for live streaming.
  - **HD (1280x720)**: 60 FPS
  - **FHD (1920x1080)**: 30 FPS
- **Resource Efficiency**: CPU and memory usage should be optimized to prevent interference with other OBS functions or plugins.
- **Cross-Platform Compatibility**: The plugin must build and run on Windows, macOS, and Linux.
- **Stability**: The implementation must be robust, avoiding crashes, memory leaks, or other instabilities during long-duration streaming sessions.
- **Maintainability**: The codebase will be written in Modern C++ (C++17/20), following a modular design with clear separation of concerns to facilitate testing and future development.

## 3. Acceptance Criteria

- The CI pipeline successfully builds the plugin for all target platforms (Windows, macOS, Linux).
- Unit and integration tests achieve a minimum of 80% code coverage.
- Performance benchmarks for 720p and 1080p resolutions meet the specified FPS targets.
- The filter can be added to a video source in OBS, and parameter adjustments are reflected in the video output in real-time.
- User-created presets persist after restarting OBS Studio.

## 4. Design and Architecture

The architecture follows the proposed structure in `CLAUDE.md`, separating concerns into distinct components.

### 4.1. Core Algorithm

The primary stabilization algorithm will be **Point Feature Matching** using OpenCV's `goodFeaturesToTrack` for feature detection and Lucas-Kanade optical flow for tracking. This approach is chosen for its excellent real-time performance and low resource consumption.

### 4.2. Component Breakdown

- **`stabilizer_opencv.cpp`**: Serves as the plugin entry point, main filter implementation, and UI interface using OBS properties system. Integrates with OBS API, receiving raw video frames and passing them to the `StabilizerWrapper` for thread-safe processing. Manages the UI property panel and applies the final transformed frame back to the OBS video pipeline.

- **`stabilizer_core.cpp`**: The heart of the plugin. Contains the entire stabilization logic, including frame analysis, motion vector calculation, smoothing, and image transformation using OpenCV. Designed to be completely independent of OBS APIs.

- **`stabilizer_wrapper.cpp`**: Thread-safe RAII wrapper for StabilizerCore. Provides thread safety for concurrent UI thread and video thread access.

- **`preset_manager.cpp`**: Handles persistence of custom presets to JSON files. Supports both OBS mode (using obs_data API) and standalone mode (using nlohmann/json).

- **`frame_utils.cpp`**: Centralized frame conversion utilities for OBS ↔ OpenCV conversions. Eliminates code duplication and provides consistent frame handling.

- **`parameter_validation.hpp`**: Centralized parameter validation and clamping logic. Ensures all parameters stay within safe ranges.

- **`stabilizer_constants.hpp`**: Named constants for all magic numbers. Improves code readability and maintainability.

### 4.3. Data Flow

1.  **Frame Capture**: OBS captures a video frame from a source.
2.  **Filter Input**: The `stabilizer_opencv` receives the frame via `stabilizer_filter_video()`.
3.  **Frame Conversion**: The OBS frame is converted to `cv::Mat` using `FRAME_UTILS::obs_frame_to_cv_mat()`.
4.  **Thread-Safe Processing**: The frame is passed to `StabilizerWrapper` which ensures thread-safe access to the core.
5.  **Core Processing (`StabilizerCore`)**:
    a. Convert frame to grayscale for feature detection.
    b. Detect feature points using `goodFeaturesToTrack()`.
    c. Track feature points between current and previous frames using Lucas-Kanade optical flow.
    d. Calculate motion vectors from the tracked points.
    e. Apply RANSAC-based robust estimation to compute the affine transformation matrix.
    f. Apply temporal smoothing algorithm to the motion vectors over a window.
    g. Apply the transformation (`warpAffine`) to the current frame.
    h. Apply edge handling (Padding/Crop/Scale) to handle transformed frame boundaries.
6.  **Filter Output**: The processed `cv::Mat` is converted back to OBS frame format using `FRAME_UTILS::cv_mat_to_obs_frame()` and returned to the OBS rendering pipeline.

## 5. Trade-offs

- **Performance vs. Accuracy**: We are prioritizing real-time performance by choosing Point Feature Matching over more computationally expensive methods like SURF or ORB. Users can adjust parameters to find a suitable balance between stability and processing overhead. Performance benchmarks show 227fps at 1080p (7.5x faster than the 30fps requirement).

- **Edge Handling Strategy**: To handle motion, transformed video frames have empty edges at the boundaries. The implementation provides **three user-configurable edge handling modes**:
  - **Padding** (default): Fills empty edges with black borders, preserving the full frame content
  - **Crop**: Removes empty edges by zooming in, providing a clean view at the cost of losing some content
  - **Scale**: Scales the transformed frame to fit the original dimensions, filling empty edges with replicated edge pixels

  This flexibility allows users to choose the best approach for their use case.

- **CPU vs. GPU Processing**: The current implementation is CPU-based with SIMD optimizations (cv::setUseOptimized(true)) to ensure broad compatibility and minimize external dependencies. GPU acceleration via CUDA or OpenCL is a potential future optimization but is not part of the core architecture to maintain simplicity and cross-platform compatibility.

- **Threading Strategy**: The core stabilization algorithm (`StabilizerCore`) is intentionally single-threaded for simplicity and to avoid complex synchronization issues. Thread safety is provided at the wrapper layer (`StabilizerWrapper`) which manages concurrent access from the OBS UI thread and video thread. OpenCV threading is set to 1 to prevent threading conflicts, relying on SIMD optimizations for performance.
- **CPU vs. GPU Processing**: The initial version will be CPU-based to ensure broad compatibility and minimize external dependencies (e.g., CUDA, OpenCL). GPU acceleration is a potential future optimization but is not part of the core architecture to maintain simplicity.
