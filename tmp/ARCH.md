# OBS Stabilizer Plugin Architecture

## Document Purpose

This document defines the architecture decisions for the OBS Stabilizer Plugin project.
It serves as a reference for developers and stakeholders to understand the system design,
component interactions, and implementation rationale.

---

## Functional Requirements (What to Implement)

### Core Features

1. **Real-time Video Stabilization**
   - Support 30fps+ processing for HD resolution (1280x720)
   - Target <4ms/frame processing time for HD resolution
   - Support up to 4K resolution (3840x2160) with performance trade-offs

2. **Stabilization Algorithm**
   - Point Feature Matching using Lucas-Kanade optical flow
   - `goodFeaturesToTrack()` for feature detection
   - RANSAC-based outlier rejection
   - Smooth transform averaging over configurable window

3. **OBS Studio Integration**
   - Video Filter plugin type
   - Seamless integration with OBS pipeline
   - Real-time frame processing
   - Property panel UI for configuration

4. **Configuration Management**
   - Preset system for common use cases (Gaming, Streaming, Recording)
   - Save/load custom presets
   - Real-time parameter adjustment

5. **Edge Handling Modes**
   - Padding: Keep black borders (default)
   - Crop: Remove black borders by cropping
   - Scale: Scale frame to fit original dimensions

### Supported Formats

- Input: BGRA, BGRX, BGR3, NV12, I420
- Output: BGRA (OBS standard)
- Resolution: 32x32 to 7680x4320 (MIN to MAX supported)

---

## Non-Functional Requirements (Performance, Security, etc.)

### Performance

1. **Processing Time**
   - HD (1280x720): <4ms/frame (target: 1-2ms)
   - 4K (3840x2160): <16ms/frame
   - VGA (640x480): <1ms/frame

2. **Memory Usage**
   - Minimal memory footprint
   - No memory leaks
   - Efficient buffer management
   - RAII for automatic cleanup

3. **CPU Usage**
   - <30% CPU increase over baseline in CI environments
   - <5% CPU increase over baseline in local development
   - Optimized for single-threaded OBS filter pipeline

### Reliability

1. **Error Handling**
   - Graceful degradation on errors
   - No crashes on invalid input
   - Comprehensive error logging
   - Recovery from tracking failures

2. **Thread Safety**
   - OBS filters are single-threaded by design
   - No mutex overhead (YAGNI principle)
   - Safe for rapid create/destroy cycles

3. **Platform Compatibility**
   - Windows (x64)
   - macOS (arm64, x64)
   - Linux (x64)

### Maintainability

1. **Code Quality**
   - DRY: Don't Repeat Yourself
   - KISS: Keep It Simple, Stupid
   - YAGNI: You Aren't Gonna Need It
   - TDD: Test-Driven Development (t-wada style)

2. **Documentation**
   - Inline comments explaining implementation rationale
   - API documentation
   - Architecture documentation (this file)
   - User and developer guides

3. **Testing**
   - Comprehensive unit tests (170+ tests)
   - Integration tests
   - Performance benchmarks
   - Visual quality tests

---

## Acceptance Criteria (Completion Conditions)

### Phase Completion Criteria

#### Phase 1: Foundation (COMPLETE ✅)
- [x] OBS plugin template configured
- [x] OpenCV integration working
- [x] Basic Video Filter implemented
- [x] Performance prototype created
- [x] Test framework set up

#### Phase 2: Core Features (COMPLETE ✅)
- [x] Point Feature Matching implemented
- [x] Smoothing algorithm implemented
- [x] Error handling standardized
- [x] Unit tests implemented

#### Phase 3: UI/UX & QA (COMPLETE ✅)
- [x] Settings panel created
- [x] Performance tests automated
- [x] Memory management optimized
- [x] Integration tests built

#### Phase 4: Optimization & Release (IN PROGRESS)
- [ ] All CI tests passing
- [ ] Performance thresholds met
- [ ] Cross-platform builds working
- [ ] Documentation complete

#### Phase 5: Production Ready (TODO)
- [ ] CI/CD pipeline fully automated
- [ ] Plugin distribution system
- [ ] Security audit completed
- [ ] Community contribution guidelines

### Quality Gates

1. **All unit tests pass** (170 tests)
2. **CI pipeline succeeds** (GitHub Actions)
3. **Plugin loads in OBS Studio** (all platforms)
4. **30fps+ achieved** for HD resolution
5. **Preset system functional**
6. **Zero memory leaks** (valgrind/clean)
7. **Documentation complete**

---

## Design Principles

### 1. YAGNI (You Aren't Gonna Need It)

**Rationale**: Only implement features that are currently needed.
**Application**:
- No premature optimization
- No speculative features
- Focus on core stabilization functionality
- Add features only when justified by use case

### 2. DRY (Don't Repeat Yourself)

**Rationale**: Avoid code duplication for maintainability.
**Application**:
- Unified frame conversion (FRAME_UTILS)
- Shared parameter validation
- Common logging infrastructure
- Reusable test data generators

### 3. KISS (Keep It Simple, Stupid)

**Rationale**: Simple solutions are more maintainable and less error-prone.
**Application**:
- Single-threaded design (matches OBS architecture)
- Direct algorithm implementation (no over-abstraction)
- Minimal dependencies
- Clear, straightforward code paths

### 4. t-wada TDD (Test-Driven Development)

**Rationale**: Tests drive design and ensure correctness.
**Application**:
- Write tests before implementation
- 170+ unit tests
- Test coverage for all public APIs
- Continuous integration testing

---

## Architecture Design

### Overall Structure

```
┌─────────────────────────────────────────────────────────────────┐
│                         OBS Studio                            │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │           OBS Stabilizer Plugin                            │ │
│  │                                                           │ │
│  │  ┌──────────────────────────────────────────────────────┐  │ │
│  │  │        StabilizerWrapper (RAII)                      │  │ │
│  │  │  - Exception safety                                  │  │ │
│  │  │  - Resource management                              │  │ │
│  │  └──────────────────────────────────────────────────────┘  │ │
│  │                           │                              │ │
│  │                           ▼                              │ │
│  │  ┌──────────────────────────────────────────────────────┐  │ │
│  │  │        StabilizerCore (Algorithm Engine)            │  │ │
│  │  │  - Feature detection                                │  │ │
│  │  │  - Optical flow tracking                            │  │ │
│  │  │  - Transform estimation                             │  │ │
│  │  │  - Transform smoothing                              │  │ │
│  │  │  - Frame transformation                            │  │ │
│  │  └──────────────────────────────────────────────────────┘  │ │
│  │                           │                              │ │
│  │          ┌────────────────┼────────────────┐             │ │
│  │          │                │                │             │ │
│  │          ▼                ▼                ▼             │ │
│  │  ┌─────────────┐  ┌──────────┐  ┌─────────────┐   │ │
│  │  │FrameUtils  │  │Parameter │  │PresetManager│   │ │
│  │  │            │  │Validation│  │             │   │ │
│  │  │- Conversion│  │          │  │- Save/Load  │   │ │
│  │  │- Validation│  │- Ranges  │  │- Presets    │   │ │
│  │  │- Buffer    │  │- Clamping│  │- Built-ins  │   │ │
│  │  └─────────────┘  └──────────┘  └─────────────┘   │ │
│  │                                                     │ │
│  └───────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

### Component Descriptions

#### StabilizerWrapper

**Purpose**: RAII wrapper for StabilizerCore
**Responsibilities**:
- Exception-safe resource management
- Initialization and cleanup
- Error handling
- Public API for OBS integration

**Design Rationale**:
- RAII ensures automatic cleanup
- Prevents memory leaks
- Provides exception-safe boundaries
- No mutex needed (OBS single-threaded)

#### StabilizerCore

**Purpose**: Core stabilization algorithm engine
**Responsibilities**:
- Feature detection (goodFeaturesToTrack)
- Optical flow tracking (Lucas-Kanade)
- Transform estimation (RANSAC)
- Transform smoothing (moving average)
- Frame transformation (warpAffine)
- Performance metrics

**Design Rationale**:
- Separated from OBS integration for testability
- Single-threaded design (matches OBS architecture)
- Optimized for real-time processing
- Clear algorithmic phases

**Algorithm Flow**:
1. **Feature Detection**:
   - Convert frame to grayscale
   - Detect corners using goodFeaturesToTrack
   - Validate feature quality and distribution

2. **Feature Tracking**:
   - Track features using Lucas-Kanade optical flow
   - Validate tracking success
   - Handle tracking failures

3. **Transform Estimation**:
   - Estimate affine transform from feature matches
   - Use RANSAC for outlier rejection
   - Validate transform constraints

4. **Transform Smoothing**:
   - Average transforms over smoothing window
   - Apply exponential smoothing
   - Limit maximum correction

5. **Frame Transformation**:
   - Apply smoothed transform to frame
   - Handle edges (padding/crop/scale)
   - Return stabilized frame

#### FRAME_UTILS

**Purpose**: Unified frame conversion utilities
**Responsibilities**:
- OBS frame to OpenCV Mat conversion
- OpenCV Mat to OBS frame conversion
- Frame validation
- Color space conversion
- Performance tracking

**Design Rationale**:
- Eliminates code duplication (DRY)
- Centralized conversion logic
- Consistent error handling
- Performance monitoring

#### VALIDATION

**Purpose**: Parameter and input validation
**Responsibilities**:
- Parameter range checking
- Parameter clamping
- Frame dimension validation
- Format validation

**Design Rationale**:
- Centralized validation logic
- Consistent error messages
- Prevents invalid states
- Easy to extend

#### PresetManager

**Purpose**: Preset configuration management
**Responsibilities**:
- Save custom presets
- Load presets
- Built-in presets (Gaming, Streaming, Recording)
- Preset validation

**Design Rationale**:
- User-friendly configuration
- Quick setup for common use cases
- Extensible system
- JSON-based storage

### Module Interaction

```
OBS Frame → StabilizerWrapper
                ↓
            StabilizerCore
                ↓
        ┌───────┴────────┐
        ↓                ↓
   FRAME_UTILS    VALIDATION
        ↓                ↓
   OpenCV Mat    Parameter Check
        ↓                ↓
   └────────┬───────────┘
            ↓
    Stabilization Algorithm
            ↓
        Transformed Frame
            ↓
    FRAME_UTILS (convert back)
            ↓
       OBS Frame Output
```

---

## Trade-offs

### 1. Algorithm Choice

**Options**:
- **Point Feature Matching** (CHOSEN)
- Feature-Based (SURF/ORB)
- Transform-Based

**Decision**: Point Feature Matching

**Rationale**:
- Best real-time performance (1-4ms/frame HD)
- Lowest memory usage
- Sufficient accuracy for video stabilization
- Well-supported in OpenCV

**Trade-offs**:
- Lower accuracy than SURF/ORB (acceptable for stabilization)
- Requires good texture in video (common in most content)
- May fail on featureless scenes (handled gracefully)

### 2. Threading Model

**Options**:
- Multi-threaded with mutexes
- Single-threaded (CHOSEN)

**Decision**: Single-threaded

**Rationale**:
- OBS filters are single-threaded by design
- No mutex overhead (simpler, faster)
- Avoids race conditions
- Matches OBS architecture

**Trade-offs**:
- Cannot utilize multiple CPU cores (not a bottleneck)
- Slightly less parallel processing overhead (acceptable)

### 3. Edge Handling

**Options**:
- Padding (CHOSEN as default)
- Crop
- Scale

**Decision**: Padding as default, with Crop and Scale options

**Rationale**:
- Padding preserves full frame content
- No information loss
- Simplest implementation
- Users can choose alternative modes

**Trade-offs**:
- Black borders visible (expected behavior)
- Crop loses content (user choice)
- Scale adds distortion (user choice)

### 4. Smoothing Algorithm

**Options**:
- Moving Average (CHOSEN)
- Exponential Smoothing
- Kalman Filter

**Decision**: Moving Average

**Rationale**:
- Simple to implement (KISS)
- Predictable behavior
- Minimal computational overhead
- Sufficient for most use cases

**Trade-offs**:
- Less sophisticated than Kalman (not needed)
- Fixed window size (configurable)
- May have latency (acceptable for stabilization)

### 5. Memory Management

**Options**:
- Manual memory management
- RAII with smart pointers (CHOSEN)
- Garbage collection

**Decision**: RAII with smart pointers

**Rationale**:
- Automatic cleanup
- Exception-safe
- No memory leaks
- Modern C++ best practice

**Trade-offs**:
- Slight overhead from smart pointers (negligible)
- Requires C++11+ (already required)

---

## Implementation Details

### Key Technologies

- **Language**: C++17/20
- **Image Processing**: OpenCV 4.5+
- **Build System**: CMake
- **Testing**: GoogleTest
- **Platform API**: OBS Studio API

### File Structure

```
obs-stabilizer/
├── src/
│   ├── core/
│   │   ├── stabilizer_core.hpp       # Core algorithm engine
│   │   ├── stabilizer_core.cpp
│   │   ├── stabilizer_wrapper.hpp    # RAII wrapper
│   │   ├── stabilizer_wrapper.cpp
│   │   ├── frame_utils.hpp          # Frame conversion
│   │   ├── frame_utils.cpp
│   │   ├── parameter_validation.hpp # Parameter validation
│   │   ├── stabilizer_constants.hpp # Constants
│   │   ├── preset_manager.hpp       # Preset management
│   │   ├── preset_manager.cpp
│   │   ├── benchmark.hpp           # Performance tracking
│   │   └── logging.hpp             # Logging utilities
│   └── stabilizer_opencv.cpp       # OBS plugin entry
├── tests/
│   ├── test_basic.cpp               # Basic functionality tests
│   ├── test_stabilizer_core.cpp    # Core algorithm tests
│   ├── test_edge_cases.cpp         # Edge case tests
│   ├── test_visual_quality.cpp     # Visual quality tests
│   ├── test_performance_thresholds.cpp # Performance tests
│   ├── test_integration.cpp        # Integration tests
│   ├── test_multi_source.cpp       # Multi-source tests
│   ├── test_memory_leaks.cpp      # Memory leak tests
│   ├── test_preset_manager.cpp    # Preset system tests
│   ├── test_data_generator.cpp    # Test data utilities
│   └── test_constants.hpp         # Test constants
├── CMakeLists.txt                 # Build configuration
└── .github/workflows/             # CI/CD pipelines
```

### Performance Optimization

1. **OpenCV Optimizations**
    - SIMD auto-optimization (cv::setUseOptimized)
    - Single-threaded mode (cv::setNumThreads(1))
    - Efficient data structures
    - Platform-specific optimizations are handled automatically by OpenCV

2. **Algorithm Optimizations**
   - RANSAC for outlier rejection
   - Feature count limiting
   - Early termination on errors
   - Minimal memory allocations

3. **Code Optimizations**
   - Inline critical paths
   - Cache-friendly data structures
   - Efficient frame buffers
   - Minimal copies

---

## Future Extensions (Not Implemented Yet)

### Phase 4: Optimization & Release

1. **Performance Tuning**
   - GPU acceleration (OpenCV CUDA)
   - SIMD optimizations
   - Advanced smoothing algorithms

2. **Cross-Platform**
   - Windows-specific optimizations
   - macOS Silicon optimizations
   - Linux performance tuning

3. **Debug & Diagnostics**
   - Visual debugging overlays
   - Performance profiling
   - Logging improvements

4. **Documentation**
   - User guide
   - Developer guide
   - API documentation

### Phase 5: Production Ready

1. **CI/CD Pipeline**
   - Automated testing
   - Automated releases
   - Performance regression tests

2. **Distribution**
   - Plugin installer
   - Update mechanism
   - Version management

3. **Security**
   - Security audit
   - Vulnerability scanning
   - Secure updates

4. **Community**
   - Contribution guidelines
   - Issue templates
   - Code review process

---

## Decision Record

### DECISION-001: Single-Threaded Design

**Date**: 2025-02-16
**Status**: Accepted

**Context**: OBS filters run in a single thread. Multi-threading adds complexity and mutex overhead.

**Decision**: Use single-threaded design throughout the plugin.

**Consequences**:
- **Positive**: Simpler code, no mutex overhead, matches OBS architecture
- **Negative**: Cannot utilize multiple cores (not a bottleneck for this use case)

### DECISION-002: Point Feature Matching Algorithm

**Date**: 2025-02-16
**Status**: Accepted

**Context**: Need real-time stabilization with <4ms/frame for HD resolution.

**Decision**: Use Lucas-Kanade optical flow with goodFeaturesToTrack.

**Consequences**:
- **Positive**: Best performance, low memory usage, sufficient accuracy
- **Negative**: Lower accuracy than SURF/ORB (acceptable), requires texture

### DECISION-003: RAII Resource Management

**Date**: 2025-02-16
**Status**: Accepted

**Context**: Need automatic cleanup and exception safety.

**Decision**: Use RAII with smart pointers throughout.

**Consequences**:
- **Positive**: No memory leaks, exception-safe, modern C++ best practice
- **Negative**: Slight overhead (negligible)

### DECISION-004: Test-Driven Development

**Date**: 2025-02-16
**Status**: Accepted

**Context**: Need high code quality and reliability.

**Decision**: Use TDD (t-wada style) with comprehensive test suite.

**Consequences**:
- **Positive**: High code coverage, regression prevention, better design
- **Negative**: More initial development time (offset by faster debugging)

---

## Conclusion

This architecture provides a solid foundation for the OBS Stabilizer Plugin, with:

- **Clear separation of concerns** (modular design)
- **High performance** (optimized algorithms, minimal overhead)
- **High reliability** (comprehensive testing, error handling)
- **Easy maintainability** (DRY, KISS, YAGNI principles)
- **Cross-platform support** (Windows, macOS, Linux)

The design balances competing concerns (performance vs. accuracy, simplicity vs. features) while maintaining focus on core stabilization functionality.

---

**Document Version**: 1.0
**Last Updated**: 2026-02-16
**Status**: APPROVED
