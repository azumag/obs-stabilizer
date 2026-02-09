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
│  (NEON/        │ │   (Motion     │ │  (adaptive_    │
│   OpenCV)      │ │   Classifier) │ │   stabilizer)  │
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
- **neon_feature_detection.hpp/cpp**: Apple Silicon-optimized feature detection using ARM NEON instructions
- Fallback to OpenCV's goodFeaturesToTrack() for non-ARM platforms

#### 4. Motion Analysis
- **motion_classifier.cpp**: Classifies motion types (pan, tilt, shake, etc.) for adaptive stabilization

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
    │   ├─> NEON (Apple Silicon)
    │   └─> OpenCV (Generic)
    │
    ├─> Optical Flow Tracking
    │   └─> Lucas-Kanade sparse optical flow
    │
    ├─> Motion Estimation
    │   └─> Compute translation/rotation from feature motion
    │
    ├─> Motion Classification
    │   └─> Classify motion type (pan/tilt/shake)
    │
    ├─> Adaptive Smoothing
    │   └─> Apply adaptive smoothing parameters
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

**Optimizations**:
- ARM NEON SIMD for Apple Silicon
- Adaptive feature density based on motion complexity
- Spatial distribution optimization

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
- Dynamic motion: alpha = 0.15 (moderate smoothing)
- Fast motion: alpha = 0.25 (weak smoothing)

### 4. Motion Classification

**Classes**:
1. **Static**: Minimal motion (velocity < 0.5 pixels/frame)
2. **Slow**: Gradual motion (0.5 - 2.0 pixels/frame)
3. **Moderate**: Normal motion (2.0 - 5.0 pixels/frame)
4. **Fast**: Rapid motion (> 5.0 pixels/frame)

**Features Used**:
- Motion magnitude (velocity)
- Motion direction consistency
- Motion acceleration

## Performance Optimization

### Platform-Specific Optimizations

#### macOS (Apple Silicon)
- ARM NEON intrinsics for feature detection
- Accelerate framework for vector operations
- Metal-based GPU acceleration (future)

#### Windows
- AVX2/SSE4.2 SIMD (future)
- DirectCompute GPU acceleration (future)

#### Linux
- Generic OpenCV optimizations
- CUDA GPU acceleration (future)

### Memory Management

**Memory Pool Strategy**:
- Reusable frame buffers
- Pre-allocated feature point vectors
- Fixed-size transformation matrices

**Cache Locality**:
- Frame-aligned memory allocation
- Compact data structures
- Sequential memory access patterns

### Parallel Processing

**Multi-threading Strategy**:
- OpenCV TBB backend
- Parallel optical flow computation
- Concurrent feature detection in pyramids

**Thread Safety**:
- Lock-free queue for frame processing
- Thread-local buffers
- Atomic operations for state updates

## Configuration Management

### Runtime Parameters

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| smoothing_radius | float | 0.0 - 1.0 | 0.5 | Smoothing strength (EMA alpha) |
| quality_level | float | 0.001 - 0.1 | 0.01 | Feature detection quality threshold |
| min_distance | float | 1.0 - 100.0 | 10.0 | Minimum distance between features |
| block_size | int | 3 - 31 | 3 | Block size for corner detection |
| ksize | int | 1 - 31 | 3 | Sobel aperture size |
| crop_mode | bool | - | true | Enable cropping vs. padding |
| adaptive_enabled | bool | - | true | Enable adaptive stabilization |

### Adaptive Parameters

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| static_smoothing | float | 0.0 - 0.1 | 0.05 | Alpha for static motion |
| slow_smoothing | float | 0.1 - 0.2 | 0.15 | Alpha for slow motion |
| moderate_smoothing | float | 0.15 - 0.3 | 0.25 | Alpha for moderate motion |
| fast_smoothing | float | 0.2 - 0.4 | 0.35 | Alpha for fast motion |

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

- **test_basic.cpp**: Basic functionality tests
- **test_stabilizer_core.cpp**: Core algorithm tests
- **test_adaptive_stabilizer.cpp**: Adaptive algorithm tests
- **test_motion_classifier.cpp**: Motion classification tests
- **test_neon_feature_detection.cpp**: NEON optimization tests
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
2. CUDA GPU acceleration for NVIDIA GPUs
3. Enhanced motion classification ML model
4. Real-time performance dashboard

### Medium-term (6-12 months)
1. Metal GPU acceleration for Apple Silicon
2. Advanced stabilization algorithms (EIS, OIS fusion)
3. User-defined presets and profiles
4. Integration with OBS filters system

### Long-term (12+ months)
1. Deep learning-based stabilization
2. Multi-camera synchronization
3. 3D motion estimation
4. VR/360 video stabilization

## Design Principles

### YAGNI (You Aren't Gonna Need It)
- Implement only what's needed for current requirements
- Avoid premature optimization of features not yet requested
- Focus on core stabilization functionality

### DRY (Don't Repeat Yourself)
- Reusable feature detection interface
- Common motion analysis utilities
- Shared configuration management

### KISS (Keep It Simple, Stupid)
- Straightforward linear processing pipeline
- Minimal external dependencies
- Clear separation of concerns

### Performance-First Design
- Target > 30 fps at 1080p on mainstream hardware
- Memory usage < 500 MB
- CPU usage < 50% single core

## Security Considerations

### Input Validation
- Frame size and format validation
- Parameter range checking
- Overflow prevention in calculations

### Resource Management
- Memory leak prevention (RAII, smart pointers)
- Resource cleanup on plugin unload
- Protection against malicious frame data

### Plugin Isolation
- No file system access
- No network operations
- Limited system interaction

## Maintenance Strategy

### Code Quality
- Comprehensive inline documentation
- Regular code reviews
- Static analysis integration
- Performance regression testing

### Versioning
- Semantic versioning (MAJOR.MINOR.PATCH)
- API stability guarantees
- Backward compatibility where possible

### Documentation
- Inline code comments explaining algorithm choices
- Architecture decision records
- User-facing documentation for OBS Studio integration
