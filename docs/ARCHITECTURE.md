# OBS Stabilizer Architecture

## 1. Functional Requirements

- Real-time stabilization of OBS video sources.
- User interface to adjust the stabilization level.
- Applicability to multiple video sources.
- Low CPU usage to avoid impacting streaming performance.

## 2. Non-Functional Requirements

### 2.1. Performance

- Video processing latency should be less than one frame.
- Minimize memory usage.

### 2.2. Security

- Address vulnerabilities in external libraries.
- Ensure robustness against invalid inputs.

### 2.3. Compatibility

- Support OBS on Windows, macOS, and Linux.
- Support the latest version of OBS Studio.

### 2.4. Maintainability

- Modularize code for easy feature additions and bug fixes.
- Enhance debug logs for easy troubleshooting.

## 3. Acceptance Criteria

- Visible reduction in video shake.
- Stabilization level can be adjusted from the settings screen and reflected in real-time.
- OBS does not crash when the filter is applied to multiple video sources.
- The increase in CPU usage when the filter is applied is below a specified threshold (e.g., 5%).
- Basic operations can be confirmed with the latest version of OBS on Windows, macOS, and Linux.

## 4. Design and Architecture

### 4.1. Design Policy

- Implement as an OBS filter plugin.
- Utilize existing libraries such as OpenCV for video processing.
- Implement the UI using OBS's standard UI framework.

### 4.2. Architecture

- **Video Acquisition:** Acquire video frames from OBS's `obs_source_frame` structure.
- **Shake Detection:** Detect motion vectors between frames using OpenCV functions such as `cv::goodFeaturesToTrack` and `cv::calcOpticalFlowPyrLK`.
- **Shake Correction:** Calculate a transformation matrix to stabilize the video from the detected motion vectors, and transform the frame using `cv::warpAffine` or `cv::warpPerspective`.
- **Video Output:** Output the processed frame to OBS.
- **UI:** Create settings items such as the correction level using OBS's property view. Use functions such as `obs_properties_create` and `obs_properties_add_int`.

### 4.3. Trade-offs

- **Performance vs. Accuracy:** Increasing the accuracy of shake detection increases the amount of calculation and CPU load. It is necessary to strike a balance between accuracy and performance. Provide parameters that the user can adjust so that they can be selected according to the situation.
- **Using Libraries vs. Self-implementation:** Using libraries such as OpenCV can reduce development time, but it depends on the specifications of the library and may reduce the degree of freedom in performance tuning. This time, we will prioritize development speed and use OpenCV.



