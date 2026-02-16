# OBS Stabilizer - Architecture Documentation

**Version**: 1.0 (Current Implementation)
**Last Updated**: 2026-02-16
**Status**: Reflects actual implementation as of commit 497c2d5

---

## 1. System Overview

### 1.1 Purpose

OBS Stabilizer is a real-time video stabilization plugin for OBS Studio. It reduces camera shake and unwanted motion in video streams using computer vision techniques.

### 1.1.1 Key Features

- **Real-time Stabilization**: Processes video frames at 30+ FPS
- **Feature-based Tracking**: Uses Lucas-Kanade optical flow for motion estimation
- **Adaptive Smoothing**: Configurable smoothing radius for different use cases
- **Edge Handling**: Multiple modes (Padding, Crop, Scale) to handle stabilized output
- **Preset System**: Built-in presets (Gaming, Streaming, Recording) and custom presets
- **Parameter Validation**: Automatic clamping and validation of all parameters
- **Performance Monitoring**: Built-in metrics and slow frame detection

### 1.2 Design Principles

The architecture follows these core principles:

1. **YAGNI (You Aren't Gonna Need It)**: Only implement features that are actually needed
2. **DRY (Don't Repeat Yourself)**: Centralize common functionality (e.g., FRAME_UTILS, VALIDATION)
3. **KISS (Keep It Simple, Stupid)**: Maintain simple, straightforward implementations
4. **TDD (Test-Driven Development)**: Comprehensive test coverage for reliability

### 1.3 High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         OBS Studio                             │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │        OBS Stabilizer Plugin (.so/.dll)               │   │
│  │                                                          │   │
│  │  ┌────────────────────────────────────────────────┐    │   │
│  │  │  Plugin Interface Layer                         │    │   │
│  │  │  (stabilizer_opencv.cpp)                       │    │   │
│  │  │  - obs_source_info registration                │    │   │
│  │  │  - Property callbacks (UI)                    │    │   │
│  │  │  - Frame callbacks (video processing)          │    │   │
│  │  │  - Preset management                         │    │   │
│  │  └────────────────────────────────────────────────┘    │   │
│  │                      │                                   │   │
│  │                      ▼                                   │   │
│  │  ┌────────────────────────────────────────────────┐    │   │
│  │  │  Core Layer                                    │    │   │
│  │  │                                                │    │   │
│  │  │  ┌──────────────────────────────────────────┐  │    │   │
│  │  │  │  StabilizerWrapper (RAII)              │  │    │   │
│  │  │  │  - Memory safety                        │  │    │   │
│  │  │  │  - Exception boundaries                 │  │    │   │
│  │  │  └──────────────────────────────────────────┘  │    │   │
│  │  │                      │                          │    │   │
│  │  │                      ▼                          │    │   │
│  │  │  ┌──────────────────────────────────────────┐  │    │   │
│  │  │  │  StabilizerCore (Core Algorithm)        │  │    │   │
│  │  │  │  - Feature detection (Shi-Tomasi)      │  │    │   │
│  │  │  │  - Optical flow (Lucas-Kanade)        │  │    │   │
│  │  │  │  - Motion estimation                     │  │    │   │
│  │  │  │  - Smoothing (Gaussian)                 │  │    │   │
│  │  │  │  - Edge handling                        │  │    │   │
│  │  │  └──────────────────────────────────────────┘  │    │   │
│  │  │                                                │    │   │
│  │  │  Supporting Modules:                           │    │   │
│  │  │  - PresetManager (JSON persistence)            │    │   │
│  │  │  - FRAME_UTILS (Frame conversion)              │    │   │
│  │  │  - VALIDATION (Parameter validation)           │    │   │
│  │  │  - StabilizerConstants (Named constants)       │    │   │
│  │  │                                                │    │   │
│  │  └────────────────────────────────────────────────┘    │   │
│  │                                                          │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. Component Architecture

### 2.1 Plugin Interface Layer

**File**: `src/stabilizer_opencv.cpp` (465 lines)

**Purpose**: Provides OBS plugin interface and integrates with OBS API

**Key Components**:

```cpp
struct stabilizer_filter {
    obs_source_t *source;
    StabilizerWrapper stabilizer;  // RAII wrapper
    bool initialized;
    StabilizerCore::StabilizerParams params;
    uint64_t frame_count;
    double avg_processing_time;
};
```

**Responsibilities**:
- Register plugin with OBS (`obs_register_source`)
- Handle OBS callbacks:
  - `create()`: Initialize filter instance
  - `destroy()`: Cleanup filter instance
  - `update()`: Update parameters from UI
  - `video_filter()`: Process video frames
  - `get_properties()`: Build UI properties
  - `get_defaults()`: Set default parameter values
- Convert between OBS and OpenCV data formats
- Handle preset selection and loading

**Design Decisions**:
- Single-threaded execution (OBS filters are single-threaded by design)
- RAII pattern via StabilizerWrapper for automatic cleanup
- Exception boundaries in all OBS callbacks

---

### 2.2 Core Layer

#### 2.2.1 StabilizerWrapper

**Files**:
- `src/core/stabilizer_wrapper.hpp` (94 lines)
- `src/core/stabilizer_wrapper.cpp` (81 lines)

**Purpose**: RAII wrapper for StabilizerCore providing memory safety and exception boundaries

**Key Methods**:
```cpp
class StabilizerWrapper {
    bool initialize(uint32_t width, uint32_t height,
                   const StabilizerCore::StabilizerParams& params);
    cv::Mat process_frame(cv::Mat frame);
    bool is_initialized();
    std::string get_last_error();
    StabilizerCore::PerformanceMetrics get_performance_metrics();
    void update_parameters(const StabilizerCore::StabilizerParams& params);
    void reset();
};
```

**Design Decisions**:
- Non-copyable, non-movable (singleton per filter instance)
- Manages `std::unique_ptr<StabilizerCore>`
- No mutex needed (OBS filters are single-threaded)
- Exception-safe boundaries for OBS callbacks

---

#### 2.2.2 StabilizerCore

**Files**:
- `src/core/stabilizer_core.hpp` (183 lines)
- `src/core/stabilizer_core.cpp` (649 lines)

**Purpose**: Core stabilization algorithm implementation

**Key Algorithms**:

1. **Feature Detection**: Shi-Tomasi corner detection (`goodFeaturesToTrack`)
   - Parameters: quality level, min distance, block size
   - Harris corner detector option

2. **Optical Flow**: Lucas-Kanade pyramidal optical flow (`calcOpticalFlowPyrLK`)
   - Tracks feature points between consecutive frames
   - Provides success rate tracking
   - Automatic redetection on consecutive failures

3. **Motion Estimation**: RANSAC-based robust estimation
   - Estimates affine transformation matrix
   - Filters outliers
   - Adaptive RANSAC thresholds

4. **Smoothing**: Gaussian smoothing over temporal window
   - Configurable smoothing radius (1-200 frames)
   - Weighted average of transformations
   - Separate presets for different use cases

5. **Edge Handling**: Multiple modes for handling output borders
   - **Padding**: Keep original frame size, add black borders
   - **Crop**: Remove black borders, reduce frame size
   - **Scale**: Scale to fit original frame size

**Key Methods**:
```cpp
class StabilizerCore {
    bool initialize(uint32_t width, uint32_t height,
                   const StabilizerParams& params);
    cv::Mat process_frame(const cv::Mat& frame);
    void update_parameters(const StabilizerParams& params);
    void reset();
    PerformanceMetrics get_performance_metrics() const;
    const std::deque<cv::Mat>& get_current_transforms() const;
    bool is_ready() const;
    std::string get_last_error() const;

    // Presets
    static StabilizerParams get_preset_gaming();
    static StabilizerParams get_preset_streaming();
    static StabilizerParams get_preset_recording();

    // Content detection
    cv::Rect detect_content_bounds(const cv::Mat& frame);

private:
    bool detect_features(const cv::Mat& gray, std::vector<cv::Point2f>& points);
    bool track_features(const cv::Mat& prev_gray, const cv::Mat& curr_gray,
                       std::vector<cv::Point2f>& prev_pts,
                       std::vector<cv::Point2f>& curr_pts,
                       float& success_rate);
    cv::Mat estimate_transform(const std::vector<cv::Point2f>& prev_pts,
                              const std::vector<cv::Point2f>& curr_pts);
    cv::Mat smooth_transforms();
    cv::Mat apply_transform(const cv::Mat& frame, const cv::Mat& transform);
    cv::Mat apply_edge_handling(const cv::Mat& frame, EdgeMode mode);
};
```

**Parameters**:
```cpp
struct StabilizerParams {
    bool enabled = true;
    int smoothing_radius = 30;         // Frames to average
    float max_correction = 30.0f;      // Max correction percentage
    int feature_count = 500;            // Feature points to track
    float quality_level = 0.01f;       // Corner quality threshold
    float min_distance = 30.0f;        // Min distance between corners
    int block_size = 3;                // Block size
    bool use_harris = false;           // Use Harris detector
    float k = 0.04f;                   // Harris parameter
    bool debug_mode = false;

    // Motion thresholds
    float frame_motion_threshold = 0.25f;
    float max_displacement = 1000.0f;
    double tracking_error_threshold = 50.0;

    // RANSAC parameters
    float ransac_threshold_min = 1.0f;
    float ransac_threshold_max = 10.0f;

    // Point validation
    float min_point_spread = 10.0f;
    float max_coordinate = 100000.0f;

    // Edge handling
    EdgeMode edge_mode = EdgeMode::Padding;
};
```

**Design Decisions**:
- Single-threaded execution (no mutex needed)
- OpenCV single-threaded mode (`cv::setNumThreads(1)`) for OBS compatibility
- SIMD optimizations enabled (`cv::setUseOptimized(true)`)
- Early returns for common cases (empty frames, disabled stabilizer)
- Pre-allocated buffers to avoid reallocations
- Comprehensive error handling with try-catch blocks
- Performance monitoring with slow frame detection

**Performance Characteristics**:
- First frame: ~2x normal processing time (initialization overhead)
- Subsequent frames: 1-10ms depending on resolution and feature count
- HD (1920x1080) @ 30fps: ~3-5ms per frame
- Memory: <50MB per instance

---

#### 2.2.3 PresetManager

**Files**:
- `src/core/preset_manager.hpp` (126 lines)
- `src/core/preset_manager.cpp` (557 lines)

**Purpose**: Manages custom preset persistence using JSON

**Key Methods**:
```cpp
class PresetManager {
    static std::string get_preset_directory();
    static bool save_preset(const std::string& preset_name,
                           const StabilizerParams& params,
                           const std::string& description = "");
    static bool load_preset(const std::string& preset_name,
                           StabilizerParams& params);
    static bool delete_preset(const std::string& preset_name);
    static std::vector<std::string> list_presets();
    static bool preset_exists(const std::string& preset_name);
    static std::string get_preset_file_path(const std::string& preset_name);

#ifdef HAVE_OBS_HEADERS
    static obs_data_t* preset_info_to_obs_data(const PresetInfo& info);
    static PresetInfo obs_data_to_preset_info(obs_data_t* data);
#endif
};
```

**Design Decisions**:
- Static class (no instantiation needed)
- Uses OBS's obs_data API for JSON serialization when available
- Falls back to nlohmann/json for standalone testing
- Presets stored in OBS config directory
- Thread-safe for read operations (single-writer, multiple-reader pattern)

---

#### 2.2.4 FRAME_UTILS

**Files**:
- `src/core/frame_utils.hpp` (163 lines)
- `src/core/frame_utils.cpp` (448 lines)

**Purpose**: Centralized frame conversion and validation utilities

**Namespaces**:

```cpp
namespace FRAME_UTILS {
    // Frame format enumeration
    enum class FrameFormat {
        BGRA, BGRX, BGR3, NV12, I420, UNKNOWN
    };

#ifdef HAVE_OBS_HEADERS
    // Frame conversion utilities
    namespace Conversion {
        cv::Mat obs_to_cv(const obs_source_frame* frame);
        obs_source_frame* cv_to_obs(const cv::Mat& mat,
                                   const obs_source_frame* reference_frame);
        std::string get_format_name(uint32_t obs_format);
        bool is_supported_format(uint32_t obs_format);
    }

    // Per-call frame buffer management
    class FrameBuffer {
        static obs_source_frame* create(const cv::Mat& mat,
                                        const obs_source_frame* reference_frame);
        static void release(obs_source_frame* frame);
    };

    // Validation utilities
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

    // Color conversion utilities (both modes)
    namespace ColorConversion {
        cv::Mat convert_to_grayscale(const cv::Mat& frame);
    }
}
```

**Design Decisions**:
- Centralized conversion logic (DRY principle)
- Conditional compilation for OBS vs standalone mode
- Performance tracking for conversion failures
- Support for multiple frame formats (BGRA, BGRX, BGR3, NV12, I420)

---

#### 2.2.5 VALIDATION

**File**: `src/core/parameter_validation.hpp` (176 lines)

**Purpose**: Centralized parameter validation and clamping

**Key Functions**:
```cpp
namespace VALIDATION {
    StabilizerCore::StabilizerParams validate_parameters(
        const StabilizerCore::StabilizerParams& params);

    bool validate_dimensions(uint32_t width, uint32_t height);

    bool is_valid_feature_point(const cv::Point2f& point,
                               int width, int height);

    bool is_valid_transform(const cv::Mat& transform);
}
```

**Design Decisions**:
- Inline functions for performance
- Uses `std::clamp` for parameter clamping
- Validates all parameters against constants in `StabilizerConstants`
- Ensures block_size is odd (required by Shi-Tomasi)
- Checks for NaN/infinite values

---

#### 2.2.6 StabilizerConstants

**File**: `src/core/stabilizer_constants.hpp` (100 lines)

**Purpose**: Named constants for magic numbers and parameter limits

**Namespaces**:
```cpp
namespace StabilizerConstants {
    // Image size constraints
    constexpr int MIN_IMAGE_SIZE = 32;
    constexpr int MAX_IMAGE_WIDTH = 7680;
    constexpr int MAX_IMAGE_HEIGHT = 4320;

    // Smoothing Parameters
    namespace Smoothing {
        constexpr int MIN_RADIUS = 1;
        constexpr int MAX_RADIUS = 200;
        constexpr int DEFAULT_RADIUS = 10;
        constexpr int GAMING_RADIUS = 25;
        constexpr int STREAMING_RADIUS = 30;
        constexpr int RECORDING_RADIUS = 50;
    }

    // Correction Parameters
    namespace Correction {
        constexpr float MIN_MAX = 0.0f;
        constexpr float MAX_MAX = 100.0f;
        constexpr float DEFAULT_MAX = 20.0f;
        constexpr float GAMING_MAX = 40.0f;
        constexpr float STREAMING_MAX = 30.0f;
        constexpr float RECORDING_MAX = 20.0f;
    }

    // Feature Detection Parameters
    namespace Features {
        constexpr int MIN_COUNT = 50;
        constexpr int MAX_COUNT = 2000;
        constexpr int DEFAULT_COUNT = 200;
        constexpr int GAMING_COUNT = 150;
        constexpr int RECORDING_COUNT = 400;
    }

    // Quality Parameters
    namespace Quality {
        constexpr float MIN_LEVEL = 0.001f;
        constexpr float MAX_LEVEL = 0.1f;
        constexpr float DEFAULT_LEVEL = 0.01f;
        constexpr float GAMING_LEVEL = 0.015f;
        constexpr float RECORDING_LEVEL = 0.005f;
    }

    // Distance Parameters
    namespace Distance {
        constexpr float MIN = 1.0f;
        constexpr float MAX = 200.0f;
        constexpr float DEFAULT = 10.0f;
        constexpr float GAMING = 25.0f;
        constexpr float RECORDING = 20.0f;
    }

    // Block Size Parameters
    namespace Block {
        constexpr int MIN_SIZE = 3;
        constexpr int MAX_SIZE = 31;
        constexpr int DEFAULT_SIZE = 3;
    }

    // Harris Corner Detection Parameters
    namespace Harris {
        constexpr float MIN_K = 0.01f;
        constexpr float MAX_K = 0.1f;
        constexpr float DEFAULT_K = 0.04f;
    }

    // Optical Flow Parameters
    namespace OpticalFlow {
        constexpr int MAX_ITERATIONS = 30;
        constexpr float EPSILON = 0.01f;
    }

    // Content Detection Parameters
    namespace ContentDetection {
        constexpr int CONTENT_THRESHOLD = 10;
        constexpr int BORDER_SEARCH_MAX = 100;
    }

    // Performance Monitoring Parameters
    namespace Performance {
        constexpr double FRAME_BUDGET_30FPS_MS = 33.33;
        constexpr double FRAME_BUDGET_60FPS_MS = 16.67;
        constexpr double SLOW_FRAME_THRESHOLD_MS = 10.0;
        constexpr double MAX_STD_DEV_MS = 5.0;
    }
}
```

**Design Decisions**:
- Namespace-based organization for clarity
- Separate presets for Gaming, Streaming, and Recording
- Named constants eliminate magic numbers
- Performance thresholds based on FPS requirements

---

### 2.3 Test Layer

**Directory**: `tests/`

**Purpose**: Comprehensive test coverage for reliability

**Test Files**:
- `test_basic.cpp`: Basic functionality tests
- `test_stabilizer_core.cpp`: Core algorithm tests
- `test_data_generator.cpp`: Test data generation utilities
- `test_edge_cases.cpp`: Edge case testing
- `test_integration.cpp`: Integration tests
- `test_memory_leaks.cpp`: Memory leak detection
- `test_visual_quality.cpp`: Visual quality metrics
- `test_performance_thresholds.cpp`: Performance validation
- `test_multi_source.cpp`: Multi-source testing
- `test_preset_manager.cpp`: Preset management tests

**Test Framework**: Google Test (GTest)

**Coverage**: Target 80%+ coverage

**CI Integration**: All tests run on every push via GitHub Actions

---

## 3. Data Flow

### 3.1 Frame Processing Pipeline

```
1. OBS Frame (obs_source_frame)
   │
   ├─► Validate (FRAME_UTILS::Validation::validate_obs_frame)
   │
   ├─► Convert to cv::Mat (FRAME_UTILS::Conversion::obs_to_cv)
   │
   ├─► StabilizerWrapper::process_frame
   │   │
   │   ├─► StabilizerCore::process_frame
   │   │   │
   │   │   ├─► Validate frame (validate_frame)
   │   │   │
   │   │   ├─► Convert to grayscale (FRAME_UTILS::ColorConversion::convert_to_grayscale)
   │   │   │
   │   │   ├─► Detect features (detect_features)
   │   │   │   └─► goodFeaturesToTrack()
   │   │   │
   │   │   ├─► Track features (track_features)
   │   │   │   └─► calcOpticalFlowPyrLK()
   │   │   │
   │   │   ├─► Estimate transform (estimate_transform)
   │   │   │   └─► RANSAC-based affine estimation
   │   │   │
   │   │   ├─► Smooth transforms (smooth_transforms)
   │   │   │   └─► Gaussian smoothing
   │   │   │
   │   │   ├─► Apply transform (apply_transform)
   │   │   │   └─► warpAffine()
   │   │   │
   │   │   ├─► Edge handling (apply_edge_handling)
   │   │   │   ├─► Padding / Crop / Scale
   │   │   │
   │   │   └─► Update metrics (update_metrics)
   │   │
   │   └─► Exception handling (return original frame on error)
   │
   ├─► Convert to OBS frame (FRAME_UTILS::Conversion::cv_to_obs)
   │
   └─► Return to OBS (obs_source_frame)
```

### 3.2 Parameter Update Flow

```
UI Update (obs_properties_t)
   │
   ├─► stabilizer_filter_update()
   │   │
   │   ├─► settings_to_params()
   │   │   └─► VALIDATION::validate_parameters()
   │   │
   │   ├─► StabilizerWrapper::update_parameters()
   │   │   └─► StabilizerCore::update_parameters()
   │   │
   │   └─► Reinitialize if needed
   │       └─► StabilizerCore::initialize()
```

### 3.3 Preset Loading Flow

```
Preset Selection (UI dropdown)
   │
   ├─► preset_changed_callback()
   │   │
   │   ├─► Check preset type
   │   │   ├─► Gaming (built-in)
   │   │   ├─► Streaming (built-in)
   │   │   ├─► Recording (built-in)
   │   │   └─► Custom (load from file)
   │   │       └─► PresetManager::load_preset()
   │   │           └─► Parse JSON file
   │   │
   │   ├─► params_to_settings()
   │   │
   │   └─► Trigger update
   │       └─► stabilizer_filter_update()
```

---

## 4. Design Decisions

### 4.1 Algorithm Selection

| Algorithm | Chosen | Alternative | Rationale |
|-----------|--------|-------------|-----------|
| Feature Detection | Shi-Tomasi | SURF/ORB/SIFT | Faster, simpler, good for optical flow |
| Optical Flow | Lucas-Kanade | Deep learning | Real-time performance, lightweight |
| Smoothing | Gaussian | Moving average, Kalman | Balance of quality and performance |
| Transform | Affine | Homography | Sufficient for camera shake, faster |
| Edge Handling | Padding/Crop/Scale | - | User preference flexibility |

### 4.2 Threading Model

**Decision**: Single-threaded execution

**Rationale**:
- OBS filters are single-threaded by design
- Prevents threading issues and race conditions
- Reduces complexity (YAGNI principle)
- Better performance for video pipeline integration

**Implementation**:
- No mutexes or locks
- `cv::setNumThreads(1)` to prevent OpenCV internal threading
- `cv::setUseOptimized(true)` for SIMD optimizations

### 4.3 Memory Management

**Decision**: RAII pattern with smart pointers

**Rationale**:
- Automatic cleanup prevents memory leaks
- Exception-safe
- Clear ownership semantics
- Prevents double-free and use-after-free bugs

**Implementation**:
- `StabilizerWrapper` manages `std::unique_ptr<StabilizerCore>`
- `FRAME_UTILS::FrameBuffer` manages OBS frame lifecycle
- OpenCV Mat handles its own memory automatically

### 4.4 Error Handling

**Decision**: Exception boundaries in OBS callbacks

**Rationale**:
- OBS API is not exception-safe
- Prevents crashes from unhandled exceptions
- Graceful degradation on errors

**Implementation**:
- All OBS callbacks wrapped in try-catch
- Return original frame on errors
- Log errors to OBS log system
- No exceptions cross plugin boundary

### 4.5 Parameter Validation

**Decision**: Centralized validation with clamping

**Rationale**:
- Prevents invalid values from causing issues
- Consistent behavior across all parameters
- Single source of truth for limits
- Easy to test

**Implementation**:
- `VALIDATION::validate_parameters()` clamps all parameters
- `VALIDATION::is_valid_feature_point()` validates points
- `VALIDATION::is_valid_transform()` validates matrices
- Constants defined in `StabilizerConstants`

### 4.6 Frame Format Support

**Decision**: Multiple format support with conversion

**Rationale**:
- OBS uses various formats (BGRA, NV12, I420)
- OpenCV prefers BGRA/BGR
- User flexibility for different sources

**Supported Formats**:
- BGRA (primary)
- BGRX (treated as BGRA)
- BGR3 (BGR)
- NV12 (with conversion)
- I420 (with conversion)

### 4.7 Preset System

**Decision**: Built-in presets + custom presets

**Rationale**:
- Built-in presets for common use cases (Gaming, Streaming, Recording)
- Custom presets for advanced users
- JSON-based for human readability and editing
- Stored in OBS config directory for easy backup

---

## 5. Trade-offs

### 5.1 Performance vs. Quality

| Aspect | Trade-off | Decision |
|--------|-----------|----------|
| Algorithm complexity | Higher quality = slower | Choose simpler algorithms (Shi-Tomasi vs SURF) |
| Smoothing radius | More frames = smoother but slower | Configurable (1-200 frames) |
| Feature count | More features = better tracking but slower | Configurable (50-2000 features) |
| Edge handling | Crop = better quality but smaller frame | User choice (Padding/Crop/Scale) |

### 5.2 Flexibility vs. Simplicity

| Aspect | Trade-off | Decision |
|--------|-----------|----------|
| Algorithm options | More algorithms = more flexibility | Choose one proven algorithm (Lucas-Kanade) |
| Configurable parameters | More parameters = more control | Expose key parameters with presets |
| Modularity | More modules = more flexibility | Keep simple structure (Core + UI) |

### 5.3 Library Dependencies

| Aspect | Trade-off | Decision |
|--------|-----------|----------|
| OpenCV | Heavy dependency but powerful | Use OpenCV (mature, optimized) |
| OBS API | Platform-specific but necessary | Use OBS API (required for plugin) |
| nlohmann/json | Extra dependency for presets | Use nlohmann/json (modern, header-only) |

---

## 6. Performance Characteristics

### 6.1 Processing Time Breakdown

| Operation | Typical Time (HD @ 30fps) |
|-----------|-------------------------|
| Frame conversion (OBS → CV) | 0.5-1ms |
| Grayscale conversion | 0.1-0.5ms |
| Feature detection | 1-3ms |
| Optical flow | 1-3ms |
| Transform estimation | 0.5-1ms |
| Smoothing | 0.1-0.5ms |
| Transform application | 0.5-1ms |
| Edge handling | 0.1-0.5ms |
| Frame conversion (CV → OBS) | 0.5-1ms |
| **Total** | **4-12ms** |

### 6.2 Memory Usage

| Component | Memory Usage |
|-----------|---------------|
| StabilizerCore instance | ~10-30MB |
| Frame buffers (2-3 frames) | ~20-30MB |
| Transform history (200 frames) | ~2-3MB |
| Feature points (500-2000) | ~0.1-0.5MB |
| **Total per instance** | **~35-65MB** |

### 6.3 Resolution Scaling

| Resolution | Processing Time | Memory Usage |
|------------|----------------|--------------|
| 640x480 (SD) | 1-3ms | ~10-15MB |
| 1280x720 (HD) | 2-5ms | ~20-30MB |
| 1920x1080 (Full HD) | 4-12ms | ~35-65MB |
| 3840x2160 (4K) | 10-25ms | ~80-150MB |

---

## 7. Future Enhancements

### 7.1 Potential Improvements

1. **GPU Acceleration**
   - Use OpenCV CUDA modules
   - Offload optical flow to GPU
   - Expected speedup: 2-5x

2. **Adaptive Stabilization**
   - Motion type classification (pan, zoom, shake)
   - Adaptive parameter adjustment
   - Better handling of intentional camera movement

3. **Motion Classification**
   - Detect and preserve intentional pans
   - Reduce jitter in slow pans
   - Better zoom handling

4. **Advanced Smoothing**
   - Kalman filtering for better prediction
   - Adaptive smoothing radius based on motion magnitude
   - Motion-aware smoothing

5. **Image Abstraction Layer**
   - Decouple from OpenCV
   - Support for other image processing libraries
   - Better testability with mocks

6. **Thread Safety**
   - Support for multi-threaded OBS scenarios
   - Lock-free data structures
   - Better performance on multi-core systems

### 7.2 Known Limitations

1. **Single-threaded**: May not scale to multi-core systems
2. **OpenCV dependency**: Large library size (~100MB)
3. **No motion classification**: Cannot distinguish between shake and intentional movement
4. **Fixed smoothing radius**: Does not adapt to motion type
5. **Limited format support**: Some formats require conversion overhead

---

## 8. Build System

### 8.1 CMake Configuration

**File**: `CMakeLists.txt` (401 lines)

**Key Components**:
- C++17 standard
- OpenCV 4.5+ (core, imgproc, video, calib3d, features2d, flann)
- Google Test for testing
- nlohmann/json for presets
- OBS Studio SDK (runtime dependency)
- Conditional compilation for OBS vs standalone mode

### 8.2 Platform Support

| Platform | Status | Notes |
|----------|--------|-------|
| Windows | Supported | .dll output |
| macOS | Supported | .so output, arm64 default |
| Linux | Supported | .so output |

### 8.3 Build Targets

1. **obs-stabilizer-opencv**: Main plugin library
2. **stabilizer_tests**: Test executable
3. **performance_benchmark**: Performance testing tool
4. **singlerun**: Single-run validation tool

---

## 9. CI/CD

### 9.1 GitHub Actions Workflows

1. **Build OBS Stabilizer**
   - Build on Ubuntu, Windows, macOS
   - Verify compilation

2. **Quality Assurance**
   - Run all tests (173 tests)
   - Run benchmarks
   - Static analysis (cppcheck)
   - Coverage reporting

3. **Performance Tests**
   - Benchmark various resolutions
   - Validate performance thresholds

4. **Feature Implementation Flow**
   - Pre-commit checks
   - Post-commit validation

### 9.2 Current CI Status

- ✅ All tests passing (173/173)
- ✅ All benchmarks passing
- ✅ No critical issues
- ✅ Coverage: 80%+ target

---

## 10. Documentation

### 10.1 Code Documentation

- Inline comments for complex logic
- Doxygen-style comments for public APIs
- Rationale comments for design decisions
- Performance annotations for critical paths

### 10.2 External Documentation

- CLAUDE.md: Project overview and guidelines
- docs/ (planned): User guide, developer guide
- GitHub Issues: Task tracking and discussions

---

## 11. Troubleshooting

### 11.1 Common Issues

1. **Plugin not loading**
   - Check OBS version compatibility
   - Verify OpenCV libraries are in RPATH
   - Check for missing dependencies

2. **Poor stabilization quality**
   - Increase feature_count
   - Increase smoothing_radius
   - Decrease max_correction for subtle stabilization

3. **Slow performance**
   - Decrease feature_count
   - Decrease smoothing_radius
   - Lower resolution
   - Check for slow frame warnings in log

4. **Black borders**
   - Try different edge_mode (Crop vs Scale)
   - Increase max_correction to see more of frame

### 11.2 Debug Mode

Enable `debug_mode` parameter to:
- Log feature detection results
- Log optical flow success rates
- Log transform details
- Log processing times
- Visualize tracked points (future feature)

---

## 12. References

### 12.1 Algorithms

- Shi-Tomasi Corner Detection: Shi & C. Tomasi, "Good Features to Track"
- Lucas-Kanade Optical Flow: B.D. Lucas & T. Kanade, "An Iterative Image Registration Technique"
- RANSAC: Fischler & Bolles, "Random Sample Consensus"

### 12.2 OpenCV Documentation

- [OpenCV Tutorials](https://docs.opencv.org/master/d9/df8/tutorial_root.html)
- [OpenCV API Reference](https://docs.opencv.org/master/)

### 12.3 OBS Plugin Development

- [OBS Plugin Development](https://obsproject.com/docs/reference-plugins.html)
- [OBS Source Code](https://github.com/obsproject/obs-studio)

---

## Appendix A: File Structure

```
obs-stabilizer/
├── src/
│   ├── stabilizer_opencv.cpp          # Plugin interface (465 lines)
│   └── core/
│       ├── stabilizer_core.hpp        # Core algorithm header (183 lines)
│       ├── stabilizer_core.cpp        # Core algorithm implementation (649 lines)
│       ├── stabilizer_wrapper.hpp      # RAII wrapper header (94 lines)
│       ├── stabilizer_wrapper.cpp      # RAII wrapper implementation (81 lines)
│       ├── preset_manager.hpp          # Preset manager header (126 lines)
│       ├── preset_manager.cpp          # Preset manager implementation (557 lines)
│       ├── frame_utils.hpp            # Frame utilities header (163 lines)
│       ├── frame_utils.cpp            # Frame utilities implementation (448 lines)
│       ├── parameter_validation.hpp   # Parameter validation (176 lines)
│       ├── stabilizer_constants.hpp    # Named constants (100 lines)
│       ├── logging.hpp                # Logging utilities
│       └── benchmark.hpp              # Benchmark utilities
├── tests/
│   ├── test_basic.cpp
│   ├── test_stabilizer_core.cpp
│   ├── test_data_generator.cpp
│   ├── test_edge_cases.cpp
│   ├── test_integration.cpp
│   ├── test_memory_leaks.cpp
│   ├── test_visual_quality.cpp
│   ├── test_performance_thresholds.cpp
│   ├── test_multi_source.cpp
│   └── test_preset_manager.cpp
├── include/
│   ├── obs_minimal.h                 # Minimal OBS headers
│   ├── obs-frontend-api.h            # OBS frontend API
│   └── obs-data.h                   # OBS data API
├── tools/
│   ├── performance_benchmark.cpp
│   └── singlerun.cpp
├── CMakeLists.txt                    # Build configuration (401 lines)
├── CLAUDE.md                        # Project guidelines
└── STATE.md                         # Project state

Total Source Code: ~2,200 lines
```

---

## Appendix B: Terminology

- **Feature Point**: Distinct point in an image that can be tracked across frames
- **Optical Flow**: Pattern of apparent motion of objects between frames
- **Affine Transform**: Linear transformation that preserves parallelism (rotation, translation, scale, shear)
- **RANSAC**: Robust estimation algorithm that filters outliers
- **Smoothing**: Temporal averaging of transformations to reduce jitter
- **Edge Handling**: Method for dealing with black borders after stabilization

---

**End of Architecture Documentation**
