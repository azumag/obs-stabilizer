# OBS Stabilizer Architecture

## Overview

OBS Stabilizer is a real-time video stabilization plugin for OBS Studio, built with C++17/20, OpenCV 4.5+, and CMake. The plugin implements feature-based optical flow stabilization with adaptive algorithms optimized for live streaming scenarios.

## System Architecture

### Layer Structure

```
┌─────────────────────────────────────────────────────────────┐
│                    OBS Plugin Interface                      │
│  (stabilizer_opencv.cpp, plugin-support.c)                  │
└─────────────────────────┬───────────────────────────────────┘
                           │
┌─────────────────────────▼───────────────────────────────────┐
│                   Stabilization Core                         │
│  (stabilizer_core.cpp, stabilizer_wrapper.cpp)              │
└─────────────────────────┬───────────────────────────────────┘
                           │
         ┌─────────────────┼─────────────────┐
         │                 │                 │
┌───────▼────────┐ ┌──────▼────────┐ ┌─────▼──────────┐
│  Feature       │ │   Motion      │ │  Adaptive      │
│  Detection     │ │   Analysis    │ │  Stabilizer    │
│  (OpenCV)      │ │   (Motion     │ │  (adaptive_    │
│   Standard)    │ │   Classifier) │ │   stabilizer)  │
└────────────────┘ └───────────────┘ └────────────────┘
```

### Core Components

#### 1. Plugin Interface Layer
- **stabilizer_opencv.cpp**: Main OBS plugin entry point, handles filter registration and video frame callbacks
- **plugin-support.c**: Platform-specific plugin loading support

#### 2. Stabilization Core
- **stabilizer_core.cpp**: Core stabilization algorithm using optical flow and motion smoothing
- **stabilizer_wrapper.cpp**: Wrapper interface between OBS plugin and core stabilization logic

#### 3. Feature Detection
- **feature_detection.hpp/cpp**: Platform-independent feature detection using OpenCV's goodFeaturesToTrack
- No platform-specific SIMD optimizations (YAGNI principle - OpenCV's optimized implementations provide sufficient performance)

#### 4. Motion Analysis
- **motion_classifier.cpp**: Classifies motion types (Static, SlowMotion, FastMotion, CameraShake, PanZoom) for adaptive stabilization

#### 5. Adaptive Stabilization
- **adaptive_stabilizer.cpp**: Implements adaptive stabilization parameters based on motion type

## Data Flow

### Frame Processing Pipeline

```
Input Frame (OBS)
    │
    ├─> Convert to Grayscale
    │
    ├─> Feature Detection
    │   └─> OpenCV goodFeaturesToTrack (Shi-Tomasi corner detection)
    │
    ├─> Optical Flow Tracking
    │   └─> Lucas-Kanade sparse optical flow (3-level pyramid)
    │
    ├─> Motion Estimation
    │   └─> Compute translation/rotation from feature motion
    │
    ├─> Motion Classification
    │   └─> Classify motion type (Static/SlowMotion/FastMotion/CameraShake/PanZoom)
    │
    ├─> Adaptive Smoothing
    │   └─> Apply adaptive smoothing parameters based on motion type
    │
    ├─> Transform Computation
    │   └─> Compute affine transformation matrix
    │
    └─> Apply Transformation
        └─> Warp frame using cv::warpAffine()
```

### Configuration Flow

```
User Settings (OBS UI)
    │
    ├─> Stabilizer Parameters
    │   ├─> Smoothing radius
    │   ├─> Feature detection quality level
    │   ├─> Minimum distance between features
    │   └─> Crop vs. Padding mode
    │
    └─> Adaptive Parameters
        ├─> Motion type detection threshold
        ├─> Adaptive smoothing strength
        └─> Emergency stabilization level
```

## Key Algorithms

### 1. Feature Detection

**Algorithm**: Shi-Tomasi corner detection (goodFeaturesToTrack)

**Implementation**: OpenCV standard functions

**Rationale**: No platform-specific SIMD optimizations are implemented following YAGNI principle. OpenCV's optimized implementations already provide sufficient performance (>30fps @ 1080p).

**Parameters**:
- `quality_level`: 0.01 - 0.1 (default: 0.01)
- `min_distance`: 5.0 - 50.0 (default: 10.0)
- `block_size`: 3 - 31 (default: 3)
- `ksize`: 1 - 31 (default: 3)

### 2. Optical Flow Tracking

**Algorithm**: Lucas-Kanade sparse optical flow

**Implementation**:
- cv::calcOpticalFlowPyrLK() with 3-level pyramid
- Backward tracking for error estimation
- RANSAC outlier rejection

**Error Handling**:
- Tracking error threshold: 3.0 pixels
- Minimum valid features: 8
- Maximum tracking iterations: 30

### 3. Motion Smoothing

**Algorithm**: Exponential moving average (EMA) with adaptive parameters

**Formula**:
```
smoothed_value = alpha * current_value + (1 - alpha) * previous_smoothed_value
```

**Adaptive Alpha Calculation**:
- Static motion: alpha = 0.05 (strong smoothing)
- SlowMotion: alpha = 0.15 (moderate smoothing)
- FastMotion: alpha = 0.35 (weak smoothing)

### 4. Motion Classification

**Classes**:
1. **Static**: Minimal motion (magnitude < 6.0 in calculation including scale/rotation)
2. **SlowMotion**: Gentle motion (6.0 - 15.0 in calculation including scale/rotation)
3. **FastMotion**: Rapid motion (15.0 - 40.0 in calculation including scale/rotation)
4. **CameraShake**: High-frequency jitter (detected via frequency analysis)
5. **PanZoom**: Systematic directional motion (high consistency, low directional variance)

**Features Used**:
- Motion magnitude (velocity including scale and rotation deviations)
- Motion direction consistency
- Motion acceleration
- High-frequency ratio

**Magnitude Calculation**:
The motion magnitude is calculated as:
```
magnitude = translation_magnitude + scale_deviation * 100.0 + rotation_deviation * 200.0
```

This explains the higher threshold values compared to pure translation-only motion.

## Performance Optimization

### Current Implementation

#### All Platforms
- ✅ OpenCV standard functions (already optimized by platform vendors)
- ✅ Reference counting for cv::Mat objects
- ✅ EMA smoothing algorithm (minimal CPU overhead)
- ✅ Pre-resized vectors where possible (std::vector::resize before use)

### Memory Management

- ✅ RAII pattern for automatic resource cleanup
- ✅ OpenCV reference counting for efficient memory use
- ✅ Pre-allocated vectors where possible (std::vector::reserve)

### Thread Safety

- ✅ No mutex used (OBS filters are single-threaded)
- ✅ Simplified design following YAGNI principle
- ✅ Performance optimized for single-threaded context

### Future Optimizations (Not Yet Implemented)

The following optimizations may be considered if performance profiling shows bottlenecks:

1. **Memory Pool Strategy**:
   - Reusable frame buffers
   - Pre-allocated feature point vectors
   - Fixed-size transformation matrices

2. **GPU Acceleration**:
   - Metal (Apple Silicon)
   - CUDA (Linux)
   - DirectCompute (Windows)

3. **Advanced SIMD**:
   - Custom NEON implementation for Apple Silicon (if OpenCV not sufficient)
   - AVX2/SSE4.2 for Windows

**Note**: Current implementation meets >30fps @ 1080p requirement, so these optimizations are not needed at this time (YAGNI principle).

## Configuration Management

### Runtime Parameters

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| smoothing_radius | int | 0 - 100 | 30 | Number of frames to average for smoothing |
| max_correction | float | 0.0 - 100.0 | 30.0 | Maximum correction percentage |
| feature_count | int | 50 - 2000 | 500 | Number of feature points to track |
| quality_level | float | 0.001 - 0.1 | 0.01 | Feature detection quality threshold |
| min_distance | float | 1.0 - 100.0 | 30.0 | Minimum distance between features |
| block_size | int | 3 - 31 | 3 | Block size for corner detection |
| ksize | int | 1 - 31 | 3 | Sobel aperture size |
| crop_mode | enum | Padding/Crop/Scale | Padding | Edge handling mode |
| adaptive_enabled | bool | - | true | Enable adaptive stabilization |

### Adaptive Parameters

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| static_smoothing | float | 0.0 - 0.1 | 0.05 | Alpha for static motion |
| slow_smoothing | float | 0.1 - 0.2 | 0.15 | Alpha for slow motion |
| moderate_smoothing | float | 0.15 - 0.3 | 0.25 | Alpha for moderate motion |
| fast_smoothing | float | 0.2 - 0.4 | 0.35 | Alpha for fast motion |

### Motion Classification Thresholds

| Threshold | Type | Base Value | Description |
|-----------|------|------------|-------------|
| STATIC_THRESHOLD | double | 6.0 | Threshold for static motion detection |
| SLOW_THRESHOLD | double | 15.0 | Threshold for slow motion detection |
| FAST_THRESHOLD | double | 40.0 | Threshold for fast motion detection |
| VARIANCE_THRESHOLD | double | 3.0 | Motion variance threshold |
| HIGH_FREQ_THRESHOLD | double | 0.70 | Ratio threshold for high-frequency shake detection |
| CONSISTENCY_THRESHOLD | double | 0.96 | Direction consistency threshold for pan/zoom detection |

**Note**: These base thresholds are multiplied by the sensitivity factor during classification.

## Error Handling

### Recovery Strategies

1. **Tracking Failure Detection**
   - Insufficient valid features (< 8)
   - Excessive tracking error (> 3.0 pixels)
   - Sudden motion discontinuity

2. **Recovery Actions**
   - Re-initialize feature detection
   - Increase feature density temporarily
   - Apply emergency stabilization (strong smoothing)
   - Skip frame if unrecoverable

3. **Quality Degradation Handling**
   - Gradual parameter adjustment
   - User notification via OBS log
   - Automatic fallback to conservative settings

## Testing Strategy

### Unit Tests

- **test_basic.cpp**: Basic functionality tests (16 tests)
- **test_stabilizer_core.cpp**: Core algorithm tests (29 tests)
- **test_adaptive_stabilizer.cpp**: Adaptive algorithm tests (18 tests)
- **test_motion_classifier.cpp**: Motion classification tests (20 tests)
- **test_feature_detection.cpp**: Feature detection tests (11 tests)
- **test_data_generator.cpp**: Test data generation utilities

### Performance Benchmarks

- **performance_benchmark.cpp**: Comprehensive performance testing
- **benchmark.cpp**: Low-level benchmark utilities
- **performance_regression.cpp**: Performance regression detection
- **singlerun.cpp**: Quick validation tool

### Test Coverage

- Feature detection accuracy
- Optical flow tracking accuracy
- Motion smoothing quality
- Real-time performance (> 30 fps @ 1080p)
- Memory usage (< 500 MB)
- CPU usage (< 50% single core)

**Current Test Status**: ✅ 94/94 tests passing (100% pass rate)

## Build System

### CMake Configuration

**Minimum Requirements**: CMake 3.16

**Dependencies**:
- OpenCV 4.5+ (core, imgproc, video, calib3d)
- GTest 1.10+ (for testing)
- OBS Studio 27+ (for plugin build, optional)

**Build Types**:
- Debug: Full debugging symbols
- RelWithDebInfo: Optimized with debug symbols (default)
- Release: Maximum optimization

### Platform Support

| Platform | Status | Compiler | OpenCV |
|----------|--------|----------|---------|
| macOS (Apple Silicon) | ✅ Primary | AppleClang | Homebrew |
| macOS (Intel) | ✅ Supported | AppleClang | Homebrew |
| Windows (x64) | 🚧 In Progress | MSVC 2022 | vcpkg |
| Linux (x64) | ✅ Supported | GCC 13+ | apt |

**Legend**:
- ✅ Fully supported
- 🚧 Work in progress
- ❓ Not tested

## Future Enhancements

### Short-term (3-6 months)
1. Windows vcpkg integration completion
2. Enhanced motion classification ML model
3. Real-time performance dashboard

### Medium-term (6-12 months)
1. Advanced stabilization algorithms (EIS, OIS fusion)
2. User-defined presets and profiles
3. Integration with OBS filters system

### Long-term (12+ months)
1. Deep learning-based stabilization
2. Multi-camera synchronization
3. 3D motion estimation
4. VR/360 video stabilization

## Design Principles

### YAGNI (You Aren't Gonna Need It)
- ✅ Implement only what's needed for current requirements
- ✅ Avoid premature optimization of features not yet requested
- ✅ Focus on core stabilization functionality
- ✅ No platform-specific SIMD (OpenCV optimizations are sufficient)

### DRY (Don't Repeat Yourself)
- ✅ Reusable feature detection interface
- ✅ Common motion analysis utilities
- ✅ Shared configuration management

### KISS (Keep It Simple, Stupid)
- ✅ Straightforward linear processing pipeline
- ✅ Minimal external dependencies (OpenCV only)
- ✅ Clear separation of concerns
- ✅ No mutex complexity (single-threaded context)

### Performance-First Design
- ✅ Target > 30 fps at 1080p on mainstream hardware
- ✅ Memory usage < 500 MB
- ✅ CPU usage < 50% single core
- ✅ Achieved with current implementation

## Security Considerations

### Input Validation
- ✅ Frame size and format validation
- ✅ Parameter range checking
- ✅ Overflow prevention in calculations

### Resource Management
- ✅ Memory leak prevention (RAII, smart pointers)
- ✅ Resource cleanup on plugin unload
- ✅ Protection against malicious frame data

### Plugin Isolation
- ✅ No file system access
- ✅ No network operations
- ✅ Limited system interaction

## Maintenance Strategy

### Code Quality
- ✅ Comprehensive inline documentation
- ✅ Regular code reviews
- ✅ Static analysis integration
- ✅ Performance regression testing

### Versioning
- Semantic versioning (MAJOR.MINOR.PATCH)
- API stability guarantees
- Backward compatibility where possible

### Documentation
- ✅ Inline code comments explaining algorithm choices
- ✅ Architecture decision records
- ✅ User-facing documentation for OBS Studio integration
