# OBS Stabilizer Architecture Design Document

## Overview
This document describes the architecture and design decisions for the OBS Stabilizer plugin, a real-time video stabilization solution using OpenCV.

## Functional Requirements

### Core Features
1. **Real-time Video Stabilization**
   - Process video frames in real-time (30fps+ target)
   - Use Lucas-Kanade optical flow for motion tracking
   - Apply smooth motion correction to reduce camera shake

2. **Frame Processing Pipeline**
   - Accept BGRA/BGR format frames from OBS
   - Convert to grayscale for feature detection
   - Detect and track feature points between frames
   - Estimate motion transform (translation, rotation)
   - Apply smoothing to motion trajectory
   - Warp frames to stabilize output

3. **Configurable Parameters**
   - Smoothing radius (number of frames to average)
   - Feature detection settings (count, quality, min distance)
   - Maximum correction intensity
   - Edge handling mode (Padding, Crop, Scale)

4. **OBS Integration**
   - Video Filter plugin type
   - Property panel for parameter adjustment
   - Preset system for common use cases

### Edge Cases Handling
- Content boundary detection (auto-crop black borders)
- Frame validation (empty frames, invalid dimensions, unsupported formats)
- Tracking failure recovery (graceful degradation)
- First frame initialization

## Non-Functional Requirements

### Performance
- **Processing Time**: <10ms per frame (1080p target)
- **Memory Usage**: Minimal allocation during processing loop
- **CPU Utilization**: Efficient single-threaded design with SIMD where possible
- **Real-time Capability**: Maintain 30fps+ for HD resolution

### Reliability
- **Thread Safety**: Single-threaded core with thread-safe wrapper layer
- **Error Handling**: Graceful degradation on failures
- **Memory Safety**: No memory leaks, proper RAII patterns
- **Robustness**: Handle corrupted frames, tracking failures, edge cases

### Maintainability
- **Code Organization**: Clear separation of concerns (Core, Wrapper, UI)
- **Testing**: Comprehensive unit and integration tests
- **Documentation**: Inline comments explaining design rationale
- **DRY Principle**: Eliminate code duplication

### Platform Support
- **Primary**: macOS (Apple Silicon), Linux
- **Secondary**: Windows (future)
- **Build System**: CMake 3.16+
- **Dependencies**: OpenCV 4.5+, OBS Studio API

## Acceptance Criteria

### Core Functionality
- [x] Real-time stabilization at 30fps+ for 1080p video
- [x] Lucas-Kanade optical flow implementation
- [x] Configurable smoothing radius (1-100 frames)
- [x] Configurable feature detection parameters
- [x] Edge handling modes (Padding, Crop, Scale)

### Quality Metrics
- [x] >80% of tests passing (currently 174/174)
- [x] Memory leak detection (valgrind/asan clean)
- [x] Performance benchmarks (<10ms/frame on HD)
- [x] Visual quality assessment (reduced shake, minimal artifacts)

### Integration
- [x] OBS plugin loads without errors
- [x] Property panel displays and functions correctly
- [x] Preset system works (Gaming, Streaming, Recording)
- [x] Plugin can be enabled/disabled without crashes

### Testing
- [x] Unit tests cover core algorithms (28 StabilizerCore tests)
- [x] Integration tests cover frame processing pipeline
- [x] Performance tests verify real-time capability
- [x] Edge case tests validate error handling

## Design Principles

### KISS (Keep It Simple, Stupid)
- Single-threaded core design for simplicity
- No mutex locking in processing path
- Straightforward data structures (deque for transforms)

### DRY (Don't Repeat Yourself)
- Common validation in FRAME_UTILS namespace
- Shared color conversion utilities
- Preset creation uses helper function

### YAGNI (You Aren't Gonna Need It)
- Only implement Point Feature Matching (avoid SURF/ORB complexity)
- No GPU acceleration until needed
- No advanced algorithms until baseline is solid

### SOLID Principles
- **Single Responsibility**: Each class has one clear purpose
  - StabilizerCore: Algorithm implementation
  - StabilizerWrapper: Thread safety and OBS integration
  - PresetManager: Configuration management
- **Open/Closed**: Extensible via parameters, closed for modification
- **Dependency Inversion**: Core depends on abstractions (OpenCV), not OBS specifics

## Architecture Decisions

### Layered Architecture

```
┌─────────────────────────────────────┐
│   OBS Studio Plugin Interface       │  (stabilizer_opencv.cpp)
├─────────────────────────────────────┤
│   Thread Safety & Integration      │  (stabilizer_wrapper.cpp)
├─────────────────────────────────────┤
│   Core Stabilization Engine       │  (stabilizer_core.cpp)
│   - Feature Detection            │
│   - Optical Flow Tracking        │
│   - Transform Estimation        │
│   - Motion Smoothing            │
├─────────────────────────────────────┤
│   Utilities                    │  (frame_utils.cpp)
│   - Frame Conversion           │
│   - Validation                │
│   - Color Conversion          │
└─────────────────────────────────────┘
```

### Module Descriptions

#### 1. StabilizerCore (src/core/stabilizer_core.cpp)
**Responsibility**: Core stabilization algorithm implementation
- Single-threaded design for performance
- No mutex locking (thread safety provided by wrapper)
- Direct OpenCV API usage

**Key Classes**:
- `StabilizerCore`: Main processing class
- `StabilizerParams`: Configuration parameters
- `PerformanceMetrics`: Timing and success tracking

**Key Methods**:
- `process_frame()`: Main processing pipeline
- `detect_features()`: Feature point detection
- `track_features()`: Lucas-Kanade tracking
- `estimate_transform()`: Motion estimation
- `smooth_transforms()`: Trajectory smoothing

#### 2. StabilizerWrapper (src/core/stabilizer_wrapper.cpp)
**Responsibility**: Thread safety and OBS integration
- Provides mutex-protected access to StabilizerCore
- Handles OBS frame conversion
- Manages plugin lifecycle

**Key Design Decisions**:
- Wrapper handles thread safety (not core)
- Allows core to remain single-threaded (fast)
- Clear separation of concerns

#### 3. FRAME_UTILS (src/core/frame_utils.hpp)
**Responsibility**: Common utilities for frame processing
- Frame conversion (OBS <-> OpenCV)
- Validation (frame data, dimensions, format)
- Color conversion utilities

**Key Design Decisions**:
- Inline implementations for performance
- Works in both OBS and standalone modes
- Eliminates code duplication

#### 4. PresetManager (src/core/preset_manager.cpp)
**Responsibility**: Configuration preset management
- Pre-defined presets (Gaming, Streaming, Recording)
- Save/load custom presets
- Parameter validation

**Design Note**:
- Uses nlohmann/json in standalone mode
- Uses OBS APIs in OBS mode

### Data Flow

```
OBS Frame (BGRA)
    ↓
[StabilizerWrapper]
    ↓ mutex lock
[StabilizerCore::process_frame]
    ↓ validate_frame
    ↓ convert_to_grayscale
    ↓ detect_features (first frame only)
    ↓ track_features (Lucas-Kanade)
    ↓ estimate_transform (RANSAC)
    ↓ smooth_transforms (exponential moving average)
    ↓ apply_transform (warpAffine)
    ↓ apply_edge_handling
    ↓ mutex unlock
Stabilized Frame (BGRA)
```

### State Management

**Per-Core Instance**:
- Previous grayscale frame (for tracking)
- Previous feature points
- Transform history (deque)
- Performance metrics
- Current parameters

**Reset Triggers**:
- Scene change (large motion detected)
- Parameter update
- Manual reset call

## Trade-offs

### Algorithm Choice: Point Feature Matching vs Feature-Based

**Chosen**: Point Feature Matching (goodFeaturesToTrack + Lucas-Kanade)

**Rationale**:
- **Performance**: 1-4ms/frame on HD (vs 10-20ms for ORB/SURF)
- **Memory**: Low memory footprint (no descriptor storage)
- **Simplicity**: Single-pass processing, no descriptor matching

**Trade-off**:
- Less robust to large scene changes (tracking can fail)
- Fewer features tracked vs feature-based methods

**Mitigation**:
- RANSAC for outlier rejection
- Consecutive failure detection and reset
- Configurable feature density

### Single-Threaded vs Multi-Threaded Core

**Chosen**: Single-threaded core with thread-safe wrapper

**Rationale**:
- **Simplicity**: No race conditions, easier to reason about
- **Performance**: Mutex-free processing path (fast)
- **Debugging**: Easier to profile and optimize

**Trade-off**:
- Cannot parallelize frame processing (must be serial)
- Limited to single CPU core for stabilization

**Mitigation**:
- SIMD optimizations for vector operations
- OpenCV's optimized implementations
- Future: GPU acceleration via OpenCV CUDA

### Smoothing: EMA vs Kalman Filter

**Chosen**: Exponential Moving Average (EMA)

**Rationale**:
- **Simplicity**: One parameter to tune (alpha)
- **Performance**: O(1) per frame (vs O(n²) for Kalman)
- **Predictable**: Stable behavior, less parameter sensitivity

**Trade-off**:
- Less optimal for highly erratic motion
- Fixed lag (adaptive with Kalman)

**Mitigation**:
- Configurable smoothing radius (controls alpha)
- Future: Adaptive EMA based on motion magnitude

### Edge Handling: Padding vs Crop

**Chosen**: All three modes (Padding, Crop, Scale) as options

**Rationale**:
- **Flexibility**: Different use cases need different handling
- **User Choice**: Expose as configuration option
- **Quality**: Crop provides best quality, Scale provides best coverage

**Trade-off**:
- Padding: Black borders visible
- Crop: Reduced frame size
- Scale: Potential quality loss

**Mitigation**:
- Content boundary detection (auto-crop)
- User can choose based on content

## Performance Considerations

### Critical Path Optimization

1. **Feature Detection** (goodFeaturesToTrack)
   - Optimized by limiting feature count
   - Harris detector optional (corners only)

2. **Optical Flow** (calcOpticalFlowPyrLK)
   - Uses pyramidal approach (faster, more robust)
   - Early termination on tracking failures

3. **Transform Estimation** (estimateAffinePartial2D)
   - RANSAC for outlier rejection
   - Iterative refinement

4. **Frame Warping** (warpAffine)
   - Single-pass transformation
   - Interpolation flags configurable

### Memory Management

- **Pre-allocation**: Mats allocated once during initialization
- **Reuse**: Temporary buffers reused across frames
- **No allocations**: Processing loop is allocation-free
- **Smart pointers**: Use std::unique_ptr for OBS frames

### Profiling

Performance metrics tracked:
- Average processing time
- Total frames processed
- Successful frames
- Tracking failures
- Transform history size

## Future Enhancements

### Phase 4 (Week 9-10)
1. **Performance Tuning**
   - SIMD optimizations for custom functions
   - Profile hotspots with perf/VTune
   - Cache optimization

2. **Cross-Platform**
   - Windows support testing
   - Linux package dependencies
   - Platform-specific optimizations

3. **Debug Features**
   - Visualize feature points
   - Debug overlay with motion vectors
   - Performance metrics display

### Phase 5 (Week 11-12)
1. **CI/CD**
   - Automated release builds
   - Plugin packaging for distribution
   - Version management

2. **Advanced Features** (if needed)
   - GPU acceleration (OpenCV CUDA)
   - Adaptive smoothing
   - Scene change detection

## Testing Strategy

### Unit Tests (tests/)
- **test_basic.cpp**: Basic functionality
- **test_stabilizer_core.cpp**: Core algorithm (28 tests)
- **test_data_generator.cpp**: Test data utilities
- **test_edge_cases.cpp**: Edge cases and errors
- **test_integration.cpp**: Full pipeline tests
- **test_memory_leaks.cpp**: Memory safety
- **test_visual_quality.cpp**: Quality assessment
- **test_performance_thresholds.cpp**: Performance validation
- **test_multi_source.cpp**: Multiple video sources
- **test_preset_manager.cpp**: Configuration management

### Integration Tests
- Full pipeline processing (frame in → frame out)
- OBS plugin loading and operation
- Property panel interaction
- Preset save/load

### Performance Tests
- Benchmarking tools (tools/performance_benchmark.cpp)
- Real-time capability verification
- Memory usage profiling
- CPU utilization measurement

## References

- OpenCV Documentation: https://docs.opencv.org/
- OBS Plugin API: https://obsproject.com/docs/reference-plugins.html
- Lucas-Kanade Paper: "An iterative image registration technique with an application to stereo vision" (B.D. Lucas & T. Kanade, 1981)
- Good Features to Track: "Good Features to Track" (J. Shi & C. Tomasi, 1994)
