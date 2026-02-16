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

- **`plugin-main.cpp`**: Serves as the plugin entry point. Responsible for registering the stabilizer filter with OBS Studio.
- **`stabilizer_opencv.cpp`**: The main filter implementation. It interfaces with OBS, receiving raw video frames and passing them to the `StabilizerCore`. It also manages the UI interaction and applies the final transformed frame back to the OBS video pipeline.
- **`stabilizer-core.cpp`**: The heart of the plugin. This component contains the entire stabilization logic, including frame analysis, motion vector calculation, smoothing, and image transformation using OpenCV. It is designed to be completely independent of OBS APIs.
- **`stabilizer-ui.cpp`**: Implements the user settings panel using Qt. It communicates parameter changes to the `stabilizer_opencv`.

### 4.3. Data Flow

1.  **Frame Capture**: OBS captures a video frame from a source.
2.  **Filter Input**: The `stabilizer_opencv` receives the frame.
3.  **Core Processing**: The frame is converted to `cv::Mat` and passed to the `StabilizerCore`.
4.  **Stabilization Steps (`StabilizerCore`)**:
    a. Track feature points between the current and previous frames (Optical Flow).
    b. Calculate motion vectors from the tracked points.
    c. Apply a smoothing algorithm to the motion vectors over a temporal window.
    d. Compute the affine transformation matrix required to counteract the smoothed motion.
    e. Apply the transformation (`warpAffine`) to the current frame.
5.  **Filter Output**: The `stabilizer_opencv` receives the processed frame and returns it to the OBS rendering pipeline.

## 5. Trade-offs

- **Performance vs. Accuracy**: We are prioritizing real-time performance by choosing Point Feature Matching over more computationally expensive methods like SURF or ORB. Users can adjust parameters to find a suitable balance between stability and processing overhead.
- **Cropping vs. Padding**: To handle motion, the transformed video will have empty edges. The initial implementation will use **cropping** to present a clean, zoomed-in view, which is the most common approach. A user-configurable option to switch between cropping and padding (black borders) will be considered for future releases.
- **CPU vs. GPU Processing**: The initial version will be CPU-based to ensure broad compatibility and minimize external dependencies (e.g., CUDA, OpenCL). GPU acceleration is a potential future optimization but is not part of the core architecture to maintain simplicity.
