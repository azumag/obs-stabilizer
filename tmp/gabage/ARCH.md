# OBS Stabilizer Architecture Document

## Document Version
- **Version**: 1.0
- **Date**: 2026-02-16
- **Status**: Current Design
- **Author**: Architecture Team

---

## 1. Executive Summary

OBS Stabilizer is a real-time video stabilization plugin for OBS Studio that uses Point Feature Matching with Lucas-Kanade optical flow to eliminate camera shake. The codebase consists of approximately 4,023 lines of C++17 code organized around a core stabilization engine with supporting utilities for frame conversion, parameter validation, preset management, and performance monitoring.

**Key Characteristics:**
- Single-threaded design (intentional, per OBS filter requirements)
- Tight coupling to OpenCV (critical issue requiring abstraction layer)
- Comprehensive parameter validation and error handling
- Production-ready with 173+ passing tests
- Multi-platform support (Windows, macOS, Linux)

---

## 2. Functional Requirements

### 2.1 Core Stabilization

**FR-1: Real-time Processing**
- Process video frames at 30fps+ for HD (1920x1080)
- Support resolutions from VGA (640x480) to 4K (3840x2160)
- Maximum processing time per frame: 33.33ms (30fps budget)

**FR-2: Shake Detection**
- Detect camera shake using optical flow
- Support horizontal, vertical, and rotational motion
- Detect zoom operations
- Handle complex backgrounds and varying lighting conditions

**FR-3: Stabilization Application**
- Apply affine transforms to compensate for detected motion
- Support configurable correction intensity (0-100%)
- Maintain temporal consistency through smoothing

**FR-4: Edge Handling**
- **Padding Mode**: Keep black borders (default)
- **Crop Mode**: Crop borders to remove black areas
- **Scale Mode**: Scale to fit original frame dimensions

### 2.2 Parameter Configuration

**FR-5: Tunable Parameters**
- Smoothing radius: 1-200 frames (default: 30)
- Feature count: 50-2000 features (default: 500)
- Quality level: 0.001-0.1 (default: 0.01)
- Min distance: 1-200 pixels (default: 30)
- Block size: 3-31 pixels (default: 21, must be odd)
- Max correction: 0-100% (default: 50%)
- Edge mode: Padding, Crop, or Scale

**FR-6: Preset Management**
- Built-in presets: Gaming, Streaming, Recording
- Custom preset creation, deletion, and loading
- Persistent storage in OBS config directory
- JSON-based format for easy editing

### 2.3 Validation and Error Handling

**FR-7: Input Validation**
- Validate frame dimensions (32px - 7680x4320)
- Validate pixel depth (CV_8U only)
- Validate channel count (1, 3, or 4 channels)
- Clamp all parameters to safe ranges

**FR-8: Error Recovery**
- Graceful degradation on errors (return original frame)
- Comprehensive exception handling for OpenCV operations
- Error logging and reporting
- Performance monitoring with slow frame detection

---

## 3. Non-Functional Requirements

### 3.1 Performance

**NFR-1: Real-time Performance**
- HD (1920x1080): Process in < 10ms (target)
- Full HD (1920x1080): Process in < 33.33ms (requirement)
- 4K (3840x2160): Process in < 66.67ms (acceptable)

**NFR-2: Memory Efficiency**
- Memory usage: < 100MB for HD processing
- No memory leaks (verified by tests)
- Efficient buffer management with RAII

**NFR-3: CPU Optimization**
- Use SIMD optimizations (SSE/AVX/NEON)
- Single-threaded OpenCV operations (prevents internal threading)
- Branch prediction hints for hot paths

### 3.2 Reliability

**NFR-4: Error Handling**
- Zero crashes from invalid inputs
- All exceptions caught and logged
- Return original frame on processing failure
- Comprehensive test coverage (173+ tests)

**NFR-5: Thread Safety**
- Designed for single-threaded use (OBS filter requirement)
- Documented as non-thread-safe
- Consider using atomics for performance counters

### 3.3 Maintainability

**NFR-6: Code Quality**
- Clean code with clear naming
- Comprehensive inline documentation
- Consistent error handling patterns
- No code duplication (DRY principle)

**NFR-7: Testing**
- Unit tests for core functionality
- Integration tests for end-to-end scenarios
- Performance benchmarks with regression detection
- Memory leak detection

### 3.4 Portability

**NFR-8: Multi-Platform Support**
- Windows, macOS, Linux
- Apple Silicon (arm64) and Intel (x86_64) support
- Standard C++17 with minimal platform-specific code

**NFR-9: Dependency Management**
- Minimal external dependencies (OpenCV, nlohmann/json)
- Optional OBS headers for plugin integration
- Standalone mode for testing

---

## 4. Design Principles

### 4.1 YAGNI (You Aren't Gonna Need It)
- Single-threaded design (OBS filters are single-threaded)
- No mutexes or thread synchronization (not needed)
- Minimal feature set focused on stabilization only

### 4.2 DRY (Don't Repeat Yourself)
- Centralized parameter validation in `VALIDATION` namespace
- Shared constants in `StabilizerConstants` namespace
- Unified logging infrastructure
- Common frame utilities in `FRAME_UTILS` namespace

### 4.3 KISS (Keep It Simple, Stupid)
- Direct OpenCV usage (simple but creates coupling)
- Moving average smoothing (simple and effective)
- Clear, linear code flow
- Minimal abstraction layers (current state)

### 4.4 RAII (Resource Acquisition Is Initialization)
- `StabilizerWrapper` for automatic cleanup
- `obs_source_frame` management in `FrameBuffer`
- Exception-safe boundaries for OBS callbacks

---

## 5. Architecture Overview

### 5.1 System Context

```
┌─────────────────────────────────────────────────────────────┐
│                     OBS Studio                          │
│  ┌──────────────────────────────────────────────────────┐ │
│  │  Video Source → OBS Stabilizer → Filtered Output   │ │
│  └──────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
         ↓                    ↓                    ↓
    obs_source_frame   StabilizerCore    obs_source_frame
```

**Components:**
1. **OBS Studio**: Host application that loads the plugin
2. **OBS Stabilizer Plugin**: Video filter that stabilizes frames
3. **StabilizerCore**: Core stabilization engine
4. **Frame Utils**: Frame conversion utilities (OBS ↔ OpenCV)
5. **Preset Manager**: Preset persistence and management

### 5.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    OBS Stabilizer Plugin                      │
├─────────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │     OBS Integration Layer (stabilizer_opencv.cpp)      │   │
│  │  - Plugin registration                                 │   │
│  │  - OBS API calls                                      │   │
│  │  - Parameter conversion (obs_data_t ↔ StabilizerParams)│   │
│  └────────────────────┬────────────────────────────────────┘   │
│                       │                                        │
│  ┌────────────────────▼────────────────────────────────────┐   │
│  │         RAII Wrapper (stabilizer_wrapper.cpp)          │   │
│  │  - Automatic resource management                        │   │
│  │  - Exception-safe boundaries                           │   │
│  └────────────────────┬────────────────────────────────────┘   │
│                       │                                        │
│  ┌────────────────────▼────────────────────────────────────┐   │
│  │         Core Engine (stabilizer_core.cpp)               │   │
│  │  - Feature detection (goodFeaturesToTrack)             │   │
│  │  - Optical flow tracking (calcOpticalFlowPyrLK)        │   │
│  │  - Transform estimation (estimateAffinePartial2D)       │   │
│  │  - Smoothing (moving average)                          │   │
│  │  - Edge handling (padding/crop/scale)                  │   │
│  └────────────────────┬────────────────────────────────────┘   │
│                       │                                        │
│  ┌────────────────────▼────────────────────────────────────┐   │
│  │     Supporting Modules                                  │   │
│  │  ┌───────────────────────────────────────────────────┐ │   │
│  │  │ Frame Utils (frame_utils.cpp)                    │ │   │
│  │  │ - OBS ↔ OpenCV conversion                        │ │   │
│  │  │ - Frame validation                               │ │   │
│  │  │ - Color space conversion                         │ │   │
│  │  └───────────────────────────────────────────────────┘ │   │
│  │  ┌───────────────────────────────────────────────────┐ │   │
│  │  │ Preset Manager (preset_manager.cpp)              │ │   │
│  │  │ - Preset persistence (JSON)                      │ │   │
│  │  │ - Built-in presets (Gaming/Streaming/Recording)   │ │   │
│  │  └───────────────────────────────────────────────────┘ │   │
│  │  ┌───────────────────────────────────────────────────┐ │   │
│  │  │ Parameter Validation (parameter_validation.hpp)     │ │   │
│  │  │ - Parameter clamping                             │ │   │
│  │  │ - Range validation                               │ │   │
│  │  └───────────────────────────────────────────────────┘ │   │
│  └───────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

---

## 6. Data Flow

### 6.1 Frame Processing Pipeline

```
Input: obs_source_frame (from OBS)
         ↓
┌─────────────────────────────────────────────────────────────┐
│ Step 1: Frame Validation                                   │
│ - Check dimensions (32px - 7680x4320)                   │
│ - Validate depth (CV_8U only)                            │
│ - Check channels (1, 3, or 4)                           │
└────────────────────┬────────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────────────────┐
│ Step 2: Convert to OpenCV (frame_utils.cpp)             │
│ - obs_source_frame → cv::Mat                              │
│ - Color space conversion (BGRA → BGR/Gray)               │
└────────────────────┬────────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────────────────┐
│ Step 3: First Frame?                                     │
│     YES → Detect features, initialize tracking             │
│     NO  → Track features to current frame                 │
│ - goodFeaturesToTrack() for detection                     │
│ - calcOpticalFlowPyrLK() for tracking                   │
└────────────────────┬────────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────────────────┐
│ Step 4: Estimate Transform                                │
│ - estimateAffinePartial2D() with RANSAC                  │
│ - Apply max correction limits (1-100%)                   │
└────────────────────┬────────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────────────────┐
│ Step 5: Smooth Transform                                 │
│ - Moving average over smoothing radius (1-200)           │
│ - Maintains rolling window of recent transforms          │
└────────────────────┬────────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────────────────┐
│ Step 6: Apply Transform to Frame                         │
│ - cv::warpAffine() with smoothed transform              │
└────────────────────┬────────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────────────────┐
│ Step 7: Edge Handling                                    │
│ - Padding (default): Keep black borders                    │
│ - Crop: Crop borders to remove black areas                │
│ - Scale: Scale to fit original dimensions                │
└────────────────────┬────────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────────────────┐
│ Step 8: Convert to OBS (frame_utils.cpp)               │
│ - cv::Mat → obs_source_frame                            │
│ - Color space conversion (BGR → BGRA)                    │
└────────────────────┬────────────────────────────────────────┘
                     ↓
Output: obs_source_frame (to OBS)
```

### 6.2 Transform Smoothing

```
Transform History (rolling window):
┌──────────────────────────────────────────────────────────────┐
│ T₁  T₂  T₃  T₄  T₅  T₆  T₇  ...  Tₙ₋₂  Tₙ₋₁  Tₙ  │
│ ↓                                                    ↑  │
│ └──────────────────────────────────────────────────────┘    │
│                   (smoothing radius = n)                    │
└──────────────────────────────────────────────────────────────┘
                          ↓
        Average(T₁, T₂, T₃, ..., Tₙ₋₁, Tₙ)
                          ↓
              Smoothed Transform Sₙ
```

**Formula:**
```
Sₙ = (T₁ + T₂ + T₃ + ... + Tₙ) / n

Where:
- Sₙ = Smoothed transform at frame n
- Tᵢ = Transform at frame i
- n = Smoothing radius
```

---

## 7. Key Components

### 7.1 StabilizerCore

**Responsibility:** Core stabilization engine using Lucas-Kanade optical flow

**Key Methods:**
```cpp
class StabilizerCore {
public:
    // Initialization
    bool initialize(uint32_t width, uint32_t height,
                  const StabilizerParams& params);
    void reset();

    // Frame processing
    cv::Mat process_frame(const cv::Mat& frame);

    // Parameters
    void update_parameters(const StabilizerParams& params);
    StabilizerParams get_current_params() const;

    // Performance
    PerformanceMetrics get_performance_metrics() const;

    // Presets
    static StabilizerParams get_preset_gaming();
    static StabilizerParams get_preset_streaming();
    static StabilizerParams get_preset_recording();

private:
    // Feature detection
    bool detect_features(const cv::Mat& gray,
                       std::vector<cv::Point2f>& points);

    // Optical flow tracking
    bool track_features(const cv::Mat& prev_gray,
                      const cv::Mat& curr_gray,
                      const std::vector<cv::Point2f>& prev_pts,
                      std::vector<cv::Point2f>& curr_pts,
                      std::vector<uchar>& status,
                      std::vector<float>& err);

    // Transform estimation
    cv::Mat estimate_transform(const std::vector<cv::Point2f>& prev_pts,
                           const std::vector<cv::Point2f>& curr_pts);

    // Smoothing
    cv::Mat smooth_transforms_optimized();

    // Edge handling
    cv::Mat apply_edge_handling(const cv::Mat& frame, EdgeMode mode);
    cv::Rect detect_content_bounds(const cv::Mat& frame);

    // Content-aware scaling
    cv::Mat apply_content_aware_scale(const cv::Mat& frame);
};
```

**Internal State:**
```cpp
struct StabilizerCore::InternalState {
    cv::Mat prev_gray_;                    // Previous grayscale frame
    std::vector<cv::Point2f> prev_pts_;   // Previous feature points
    std::vector<cv::Point2f> curr_pts_;   // Current feature points
    std::deque<cv::Mat> transforms_;       // Transform history
    StabilizerParams params_;              // Current parameters
    uint32_t width_;                      // Frame width
    uint32_t height_;                     // Frame height
    std::string last_error_;               // Last error message
    int consecutive_failures_;             // Tracking failure counter
    PerformanceMetrics metrics_;           // Performance metrics
};
```

### 7.2 FrameUtils

**Responsibility:** Unified frame conversion utilities (OBS ↔ OpenCV)

**Key Methods:**
```cpp
namespace FRAME_UTILS {
    // Color conversion
    namespace ColorConversion {
        cv::Mat convert_to_grayscale(const cv::Mat& frame);
    }

    // OBS ↔ OpenCV conversion (when OBS headers available)
#ifdef HAVE_OBS_HEADERS
    namespace Conversion {
        cv::Mat obs_to_cv(const obs_source_frame* frame);
        obs_source_frame* cv_to_obs(const cv::Mat& mat,
                                   const obs_source_frame* reference_frame);
        std::string get_format_name(uint32_t obs_format);
        bool is_supported_format(uint32_t obs_format);
    }

    // Frame buffer management
    class FrameBuffer {
    public:
        static obs_source_frame* create(const cv::Mat& mat,
                                      const obs_source_frame* reference_frame);
        static void release(obs_source_frame* frame);
    };

    // Validation
    namespace Validation {
        bool validate_obs_frame(const obs_source_frame* frame);
        bool validate_cv_mat(const cv::Mat& mat);
        std::string get_frame_error_message(const obs_source_frame* frame);
    }
#else
    // Minimal validation for standalone mode
    namespace Validation {
        bool validate_cv_mat(const cv::Mat& mat);
    }
#endif

    // Performance monitoring
    namespace Performance {
        void track_conversion_failure();
        struct ConversionStats {
            size_t total_conversions;
            double avg_conversion_time;
            size_t failed_conversions;
        };
        static ConversionStats get_stats();
    }
}
```

### 7.3 PresetManager

**Responsibility:** Preset persistence and management (JSON-based)

**Key Methods:**
```cpp
namespace STABILIZER_PRESETS {
    // Preset management
    bool save_preset(const std::string& preset_name,
                   const StabilizerCore::StabilizerParams& params,
                   const std::string& description);
    bool load_preset(const std::string& preset_name,
                   StabilizerCore::StabilizerParams& params);
    bool delete_preset(const std::string& preset_name);
    std::vector<std::string> list_presets();
    bool preset_exists(const std::string& preset_name);
}
```

**Preset Format (JSON):**
```json
{
  "name": "Gaming",
  "description": "Optimized for gaming streams",
  "params": {
    "enabled": true,
    "smoothing_radius": 25,
    "max_correction": 40.0,
    "feature_count": 150,
    "quality_level": 0.01,
    "min_distance": 30.0,
    "block_size": 21,
    "use_harris": false,
    "tracking_error_threshold": 50.0,
    "edge_mode": 0
  }
}
```

### 7.4 Parameter Validation

**Responsibility:** Centralized parameter validation and clamping

**Key Methods:**
```cpp
namespace VALIDATION {
    // Parameter validation
    inline StabilizerCore::StabilizerParams validate_parameters(
        const StabilizerCore::StabilizerParams& params);

    // Individual validators
    inline bool validate_dimensions(uint32_t width, uint32_t height);
    inline bool validate_smoothing_radius(int radius);
    inline bool validate_max_correction(float correction);
    inline bool validate_feature_count(int count);
    inline bool validate_quality_level(float level);
    inline bool validate_min_distance(float distance);
    inline bool validate_block_size(int size);
    inline bool validate_harris_k(float k);

    // Feature validation
    inline bool is_valid_feature_point(const cv::Point2f& point,
                                     int width, int height);

    // Transform validation
    inline bool is_valid_transform(const cv::Mat& transform);
}
```

**Validation Ranges:**
```cpp
namespace StabilizerConstants {
    namespace Smoothing {
        constexpr int MIN_RADIUS = 1;
        constexpr int MAX_RADIUS = 200;
    }
    namespace Correction {
        constexpr float MIN_MAX = 0.0f;
        constexpr float MAX_MAX = 100.0f;
    }
    namespace Features {
        constexpr int MIN_COUNT = 50;
        constexpr int MAX_COUNT = 2000;
    }
    namespace Quality {
        constexpr float MIN_LEVEL = 0.001f;
        constexpr float MAX_LEVEL = 0.1f;
    }
    namespace Distance {
        constexpr float MIN = 1.0f;
        constexpr float MAX = 200.0f;
    }
    namespace Block {
        constexpr int MIN_SIZE = 3;
        constexpr int MAX_SIZE = 31;
    }
    namespace Harris {
        constexpr float MIN_K = 0.01f;
        constexpr float MAX_K = 0.1f;
    }
}
```

---

## 8. Dependencies

### 8.1 OpenCV (Required)

**Components Used:**
- `core`: Matrix operations, `cv::Mat`
- `imgproc`: Image processing, color conversion
- `video`: Optical flow, `calcOpticalFlowPyrLK`
- `calib3d`: Feature detection, `goodFeaturesToTrack`
- `features2d`: Feature matching
- `flann`: Fast Approximate Nearest Neighbor

**Usage Pattern:**
```
┌─────────────────────────────────────────────────────────────┐
│                   OpenCV Dependency                      │
├─────────────────────────────────────────────────────────────┤
│ 1. Core Types: cv::Mat, cv::Point2f, cv::Rect          │
│ 2. Feature Detection: goodFeaturesToTrack()               │
│ 3. Optical Flow: calcOpticalFlowPyrLK()                  │
│ 4. Transform: estimateAffinePartial2D()                   │
│ 5. Image Ops: warpAffine(), cvtColor()                  │
└─────────────────────────────────────────────────────────────┘
```

**Coupling Level:** EXTREME (Critical Issue #321)

**Impact:**
- Public interfaces depend on OpenCV types
- Cannot replace OpenCV with alternative implementations
- Testing requires full OpenCV dependency
- Performance optimization limited to OpenCV capabilities

### 8.2 nlohmann/json (Optional)

**Usage:** Preset serialization in `preset_manager.cpp`

**Coupling Level:** LOW (Isolated to one component)

### 8.3 OBS Studio Headers (Conditional)

**Usage:** OBS plugin API integration in `stabilizer_opencv.cpp`

**Coupling Level:** MODERATE (Isolated to OBS integration layer)

**Standalone Mode:** Can build and test without OBS headers using `obs_minimal.h` stubs

### 8.4 Google Test (Testing Only)

**Usage:** All test files

**Coupling Level:** NONE (Not in production code)

---

## 9. Design Patterns

### 9.1 RAII (Resource Acquisition Is Initialization)

**Component:** `StabilizerWrapper`

**Purpose:** Automatic resource management for StabilizerCore

**Benefits:**
- Exception-safe boundaries for OBS callbacks
- No manual memory management required
- Guaranteed cleanup when wrapper goes out of scope

### 9.2 Facade Pattern

**Component:** `settings_to_params()` and `params_to_settings()`

**Purpose:** Simplified interface for complex parameter conversion

**Benefits:**
- Hides complexity of OBS ↔ StabilizerParams conversion
- Single point of change for conversion logic

### 9.3 Strategy Pattern

**Component:** Edge handling modes (`EdgeMode` enum)

**Purpose:** Runtime selection of edge handling algorithm

**Benefits:**
- Easy to add new edge handling strategies
- Encapsulates algorithm-specific logic

### 9.4 Singleton Pattern

**Component:** Global log level (`get_global_log_level()`)

**Purpose:** Single point of configuration for logging

**Benefits:**
- Consistent log level across all components
- Runtime configuration support

### 9.5 Template Method Pattern

**Component:** `safe_call()` in logging infrastructure

**Purpose:** Exception-safe wrapper with default return value

**Benefits:**
- Consistent exception handling
- Logging of exceptions
- Safe default values

### 9.6 Builder Pattern (Implied)

**Component:** `preset_info_to_obs_data()` in preset_manager

**Purpose:** Build complex data structures step-by-step

**Benefits:**
- Clear construction sequence
- Readable code

---

## 10. Threading Model

### 10.1 Current Design: Single-Threaded

**Rationale:**
- OBS filters are called on a single thread by OBS framework
- Adding mutexes would add unnecessary overhead (YAGNI principle)
- No need for thread synchronization

**Documentation:**
```cpp
// stabilizer_core.cpp - Line 36
// Note: Mutex is not used because OBS filters are single-threaded
// This is intentional for performance (YAGNI principle)
```

**Thread Safety Assessment:**

| Component | Thread Safe? | Notes |
|-----------|--------------|-------|
| `StabilizerCore` | **NO** | Designed for single-threaded use only |
| `StabilizerWrapper` | **NO** | RAII wrapper, no synchronization |
| `PresetManager` | **NO** | File I/O only, not concurrent |
| `FrameUtils` | **NO** | Stateless, conversion only |
| `Performance::get_stats()` | **NO** | Static local variable, no mutex |

**Potential Issues:**

1. **Performance tracking race conditions:**
```cpp
// frame_utils.cpp - Lines 412-424
PerformanceData* get_perf_data() {
    static PerformanceData instance;  // NOT thread-safe!
    return &instance;
}

void Performance::track_conversion_failure() {
    PerformanceData* data = get_perf_data();
    data->failed_conversions++;  // Race condition if used multi-threaded
}
```

**Recommendations:**
- Add atomic operations to performance counters if ever used multi-threaded
- Document thread safety guarantees clearly
- Consider using `std::atomic` for simple counters

---

## 11. Error Handling

### 11.1 Multi-Layer Error Handling Strategy

**Layer 1: Validation (Proactive)**
- Frame validation in `validate_cv_mat()`
- Parameter validation in `validate_parameters()`
- Transform validation in `is_valid_transform()`

**Layer 2: Exception Handling (Reactive)**
```cpp
try {
    // Processing logic
} catch (const cv::Exception& e) {
    last_error_ = std::string("OpenCV exception in process_frame: ") + e.what();
    log_opencv_exception("process_frame", e);
    return frame;  // Return original frame on error
} catch (const std::exception& e) {
    last_error_ = std::string("Standard exception in process_frame: ") + e.what();
    log_exception("process_frame", e);
    return frame;
} catch (...) {
    last_error_ = "Unknown exception in process_frame";
    log_unknown_exception("process_frame");
    return frame;
}
```

**Layer 3: Graceful Degradation**
- Return original frame on error
- Pass through unprocessed frames
- Continue operation after failures

### 11.2 Error Handling Patterns

| Error Type | Handling Strategy |
|------------|------------------|
| Invalid parameters | Clamped to valid ranges |
| OpenCV exceptions | Logged, return original frame |
| Empty frames | Early return, skip processing |
| Initialization failure | Set error flag, return unprocessed |
| Feature detection failure | Return original frame, log warning |
| Tracking failure | Re-detect features after 5 consecutive failures |

---

## 12. Performance Considerations

### 12.1 Current Performance Bottlenecks

**1. Feature Detection (`detect_features`)**
- Complexity: O(n) where n = number of pixels
- Impact: High for high-resolution frames (4K+)
- Optimization: OpenCV SIMD optimizations (`cv::setUseOptimized(true)`)

**2. Optical Flow Tracking (`track_features`)**
- Complexity: O(m) where m = number of features
- Impact: Medium, depends on feature count
- Fixed window size: 21x21 pixels (3 pyramid levels)
- Optimization: Uses `cv::OPTFLOW_USE_INITIAL_FLOW` flag

**3. Transform Smoothing (`smooth_transforms_optimized`)**
- Complexity: O(k) where k = smoothing radius (1-200)
- Impact: Low (simple averaging)
- Optimization: Pre-multiplied by `inv_size`, loop unrolled

**4. Frame Conversion (`obs_to_cv`)**
- Impact: Medium-high (happens every frame)
- Formats supported: BGRA, BGRX, BGR3, NV12, I420
- Optimization: OpenCV's efficient `cv::Mat` constructors

### 12.2 Performance Optimizations Implemented

**1. SIMD Optimizations:**
```cpp
cv::setUseOptimized(true);  // Enable SSE/AVX/NEON
```

**2. Single-Threaded OpenCV:**
```cpp
cv::setNumThreads(1);  // Prevent internal threading
```

**3. Memory Pre-allocation:**
```cpp
points.reserve(params_.feature_count);  // Prevent reallocations
uint8_t* data_buffer = new (std::nothrow) uint8_t[required_size];
```

**4. Branch Prediction Hints:**
```cpp
if (status[j]) {  // Likely to be true (branch prediction)
    prev_pts[i] = prev_pts[j];
    curr_pts[i] = curr_pts[j];
    i++;
    tracked++;
}
```

**5. Early Returns:**
```cpp
if (frame.empty()) return frame;
if (!params_.enabled) return frame;
```

### 12.3 Performance Thresholds

```cpp
namespace Performance {
    constexpr double FRAME_BUDGET_30FPS_MS = 33.33;
    constexpr double FRAME_BUDGET_60FPS_MS = 16.67;
    constexpr double SLOW_FRAME_THRESHOLD_MS = 10.0;
    constexpr double MAX_STD_DEV_MS = 5.0;
}
```

---

## 13. Critical Issues and Recommendations

### 13.1 Critical: Extreme OpenCV Coupling (Issue #321)

**Problem:**
- Public interfaces depend directly on OpenCV types (`cv::Mat`, `cv::Point2f`)
- Cannot replace OpenCV with alternative implementations
- Testing requires full OpenCV dependency
- Performance optimization limited to OpenCV capabilities

**Impact:**
- Cannot use platform-specific optimizations (Metal, CUDA, Vulkan)
- Cannot use lightweight implementations for specific use cases
- Difficult to add GPU acceleration
- Testing complexity increased

**Recommendation:** Implement image abstraction layer

**Proposed Design:**
```cpp
namespace IMAGE {
    enum class PixelFormat {
        BGRA, BGR, GRAY, RGB, RGBA
    };

    class IImageBuffer {
    public:
        virtual ~IImageBuffer() = default;
        virtual int width() const = 0;
        virtual int height() const = 0;
        virtual int channels() const = 0;
        virtual PixelFormat format() const = 0;
        virtual uint8_t* data() = 0;
        virtual const uint8_t* data() const = 0;
        virtual std::unique_ptr<IImageBuffer> clone() const = 0;
        virtual std::unique_ptr<IImageBuffer> convert_to(PixelFormat target) const = 0;
        virtual std::unique_ptr<IImageBuffer> resize(int w, int h) const = 0;
    };

    class OpenCVImageBuffer : public IImageBuffer {
        cv::Mat mat_;
    public:
        explicit OpenCVImageBuffer(const cv::Mat& mat);
        // Implement all virtual methods
    };

    std::unique_ptr<IImageBuffer> create_from_obs_frame(const obs_source_frame* frame);
}
```

**Benefits:**
- Decouple from OpenCV: Public interfaces use `IImageBuffer`
- Swappable implementations: Can create `MetalImageBuffer`, `CUDAImageBuffer`
- Testable: Can create `MockImageBuffer` for unit tests
- Performance: Can use optimized implementations per-platform
- Migration path: Gradual refactoring, existing code remains

### 13.2 High: Incomplete Architecture Documentation (Issue #323)

**Problem:**
- Current ARCHITECTURE.md is too high-level
- No detailed component interaction diagrams
- No data flow documentation
- No coupling analysis
- No design rationale explanations

**Recommendation:** Update ARCHITECTURE.md with:
1. Detailed component descriptions
2. Data flow diagrams
3. Coupling analysis
4. Performance characteristics
5. Design rationale
6. Refactoring guidelines

### 13.3 Medium: Performance Tracking Race Conditions

**Problem:**
- Static variables without mutexes in performance tracking
- Potential race conditions if ever used multi-threaded

**Recommendation:**
- Use `std::atomic` for simple counters
- Document thread safety guarantees clearly

### 13.4 Medium: Code Duplication

**Problem:**
- Duplicated validation code in `frame_utils.hpp` and `frame_utils.cpp`

**Recommendation:**
- Unified validation in one location
- Follow DRY principle consistently

### 13.5 Low: No GPU Acceleration (Issue #319)

**Problem:**
- All processing on CPU
- Performance limited for high-resolution frames

**Recommendation:**
- Investigate GPU acceleration options (CUDA, Metal, OpenCL)
- Implement after image abstraction layer (#321)

---

## 14. Refactoring Roadmap

### Phase 1: Image Abstraction Layer (Priority: CRITICAL)

**Tasks:**
1. Define `IImageBuffer` interface
2. Implement `OpenCVImageBuffer`
3. Refactor `StabilizerCore` to use `IImageBuffer`
4. Update all public interfaces
5. Add unit tests with `MockImageBuffer`

**Estimated Effort:** 2-3 weeks

**Addresses:** Issue #321

### Phase 2: Architecture Documentation Update (Priority: HIGH)

**Tasks:**
1. Document all components in detail
2. Add data flow diagrams
3. Document coupling points
4. Add performance characteristics
5. Document design rationale

**Estimated Effort:** 1-2 weeks

**Addresses:** Issue #323

### Phase 3: Performance Tracking Safety (Priority: MEDIUM)

**Tasks:**
1. Use `std::atomic` for performance counters
2. Document thread safety guarantees
3. Add tests for thread safety (if applicable)

**Estimated Effort:** 1 week

### Phase 4: GPU Acceleration Investigation (Priority: LOW)

**Tasks:**
1. Research GPU acceleration options
2. Evaluate performance gains
3. Implement prototype (if beneficial)

**Estimated Effort:** 2-3 weeks

**Addresses:** Issue #319

---

## 15. Testing Strategy

### 15.1 Test Coverage

**Unit Tests:**
- `test_basic.cpp`: Basic functionality tests
- `test_stabilizer_core.cpp`: Core stabilization tests
- `test_data_generator.cpp`: Test data generation
- `test_edge_cases.cpp`: Edge case handling
- `test_integration.cpp`: Integration tests
- `test_memory_leaks.cpp`: Memory leak detection
- `test_visual_quality.cpp`: Visual quality metrics
- `test_performance_thresholds.cpp`: Performance threshold tests
- `test_multi_source.cpp`: Multi-source scenarios
- `test_preset_manager.cpp`: Preset management tests

**Total Tests:** 173+ passing

### 15.2 Performance Benchmarks

**Benchmark Scenarios:**
- Static scene, slow pan, fast shake, zoom operation
- Complex background, extended run
- Resolutions: 480p, 720p, 1080p, 1440p, 4K

**Benchmark Framework:**
- Multi-resolution testing
- Real-time requirement validation (30fps/60fps)
- Baseline comparison for regression detection
- Memory tracking
- CSV/JSON output

### 15.3 Regression Detection

**Performance Thresholds:**
- HD (1920x1080): Process in < 33.33ms
- Slow frame threshold: 10.0ms
- Max standard deviation: 5.0ms

---

## 16. Deployment and Operations

### 16.1 Build Configuration

**Platforms:**
- Windows (x86_64)
- macOS (arm64, x86_64)
- Linux (x86_64)

**Build System:**
- CMake 3.16+
- C++17 standard
- OpenCV 4.x (required)
- nlohmann/json (optional)
- OBS Studio headers (conditional)

### 16.2 Installation

**OBS Plugin Directory:**
- Windows: `%APPDATA%\obs-studio\obs-plugins`
- macOS: `/Library/Application Support/obs-studio/obs-plugins`
- Linux: `~/.config/obs-studio/obs-plugins`

**Preset Storage:**
- OBS config directory: `obs_get_config_path("obs-stabilizer/presets")`
- Fallback: `/tmp/obs-stabilizer-presets` (testing)

### 16.3 Monitoring and Logging

**Log Levels:**
- `LOG_ERROR`: Errors that prevent operation
- `LOG_WARNING`: Warnings for non-fatal issues
- `LOG_INFO`: Informational messages
- `LOG_DEBUG`: Debug information (configurable)

**Performance Monitoring:**
- Slow frame detection (threshold: 10.0ms)
- Conversion failure tracking
- Performance metrics reporting

---

## 17. Security Considerations

### 17.1 Input Validation

**Frame Validation:**
- Validate dimensions (32px - 7680x4320)
- Validate pixel depth (CV_8U only)
- Validate channel count (1, 3, or 4)
- Prevent integer overflow with max dimensions

**Parameter Validation:**
- Clamp all parameters to safe ranges
- Ensure block_size is odd
- Validate feature counts (50-2000)

### 17.2 Memory Safety

**RAII Wrappers:**
- `StabilizerWrapper` for automatic cleanup
- `FrameBuffer` for OBS frame management
- No manual memory management required

**Exception Safety:**
- All exceptions caught and logged
- Return original frame on error
- No resource leaks

### 17.3 Data Sanitization

**Preset Loading:**
- JSON validation before parsing
- Parameter validation after loading
- Graceful failure on invalid presets

---

## 18. Future Enhancements

### 18.1 GPU Acceleration

**Options:**
- CUDA (NVIDIA)
- Metal (Apple Silicon)
- OpenCL (Cross-platform)
- Vulkan (Cross-platform)

**Implementation:**
- After image abstraction layer (#321)
- Create `CUDAImageBuffer`, `MetalImageBuffer`
- Runtime selection based on platform

### 18.2 Advanced Smoothing

**Options:**
- Kalman filtering
- Gaussian smoothing
- Adaptive smoothing (scene-aware)
- Motion-aware smoothing

### 18.3 Additional Features

- Motion prediction
- Scene change detection
- Adaptive feature count
- Multi-resolution processing

---

## 19. Acceptance Criteria

### 19.1 Functional Requirements

**FR-1 (Real-time Processing):**
- [ ] HD (1920x1080) processed in < 33.33ms
- [ ] Support for VGA (640x480) to 4K (3840x2160)
- [ ] 30fps+ sustained performance

**FR-2 (Shake Detection):**
- [ ] Detect horizontal motion
- [ ] Detect vertical motion
- [ ] Detect rotational motion
- [ ] Detect zoom operations

**FR-3 (Stabilization Application):**
- [ ] Apply affine transforms correctly
- [ ] Support configurable correction intensity (0-100%)
- [ ] Maintain temporal consistency

**FR-4 (Edge Handling):**
- [ ] Padding mode works correctly
- [ ] Crop mode works correctly
- [ ] Scale mode works correctly

**FR-5 (Tunable Parameters):**
- [ ] All parameters configurable
- [ ] Parameters validated and clamped
- [ ] Parameters persist in presets

**FR-6 (Preset Management):**
- [ ] Built-in presets work correctly
- [ ] Custom presets can be created
- [ ] Custom presets can be loaded
- [ ] Custom presets can be deleted

**FR-7 (Input Validation):**
- [ ] Invalid frames rejected
- [ ] Invalid parameters clamped
- [ ] No crashes from invalid inputs

**FR-8 (Error Recovery):**
- [ ] All exceptions caught
- [ ] Original frame returned on error
- [ ] Error logging works correctly

### 19.2 Non-Functional Requirements

**NFR-1 (Real-time Performance):**
- [ ] HD processed in < 10ms (target)
- [ ] HD processed in < 33.33ms (requirement)
- [ ] 4K processed in < 66.67ms (acceptable)

**NFR-2 (Memory Efficiency):**
- [ ] Memory usage < 100MB for HD
- [ ] No memory leaks (verified by tests)

**NFR-3 (CPU Optimization):**
- [ ] SIMD optimizations enabled
- [ ] Single-threaded OpenCV operations
- [ ] Branch prediction hints used

**NFR-4 (Reliability):**
- [ ] Zero crashes from invalid inputs
- [ ] All exceptions caught
- [ ] 173+ tests passing

**NFR-5 (Thread Safety):**
- [ ] Documented as single-threaded
- [ ] No race conditions in current use

**NFR-6 (Code Quality):**
- [ ] Clean code with clear naming
- [ ] Comprehensive inline documentation
- [ ] No code duplication

**NFR-7 (Testing):**
- [ ] Unit tests pass
- [ ] Integration tests pass
- [ ] Performance benchmarks pass
- [ ] Memory leak tests pass

**NFR-8 (Portability):**
- [ ] Windows build successful
- [ ] macOS build successful (arm64, x86_64)
- [ ] Linux build successful

**NFR-9 (Dependency Management):**
- [ ] Minimal external dependencies
- [ ] Optional OBS headers
- [ ] Standalone mode works

---

## 20. Glossary

| Term | Definition |
|------|------------|
| **Affine Transform** | Linear transformation that preserves lines and parallelism |
| **Lucas-Kanade Optical Flow** | Algorithm for tracking motion between frames |
| **Point Feature Matching** | Tracking of distinctive points across frames |
| **RANSAC** | Random Sample Consensus: Algorithm for robust model fitting |
| **Shi-Tomasi** | Corner detection algorithm |
| **Single-Threaded** | Design that doesn't use multiple threads |
| **Smoothing Radius** | Number of frames used for averaging transforms |
| **YAGNI** | You Aren't Gonna Need It: Simplicity principle |
| **DRY** | Don't Repeat Yourself: Code reuse principle |
| **KISS** | Keep It Simple, Stupid: Simplicity principle |
| **RAII** | Resource Acquisition Is Initialization: Memory management pattern |

---

## 21. References

- **OBS Studio Documentation**: https://obsproject.com/wiki
- **OpenCV Documentation**: https://docs.opencv.org/
- **Lucas-Kanade Paper**: Lucas, B.D., Kanade, T. (1981). "An iterative image registration technique with an application to stereo vision"
- **Shi-Tomasi Paper**: Shi, J., Tomasi, C. (1994). "Good features to track"
- **RANSAC Paper**: Fischler, M.A., Bolles, R.C. (1981). "Random Sample Consensus: A paradigm for model fitting with applications to image analysis and automated cartography"

---

## 22. Appendix: Code Statistics

| Metric | Value |
|--------|--------|
| Total Lines of Code | 4,023 |
| Source Files | 9 |
| Header Files | 9 |
| Test Files | 11 |
| Tests | 173+ |
| Maximum Cyclomatic Complexity | 15 (stabilizer_core.cpp) |
| Average Function Length | 12.5 lines |
| Comment Coverage | ~25% |

---

**Document End**
