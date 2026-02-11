# OBS Stabilizer Architecture Decision Document

## 1. Functional Requirements

### Core Features
- Real-time video stabilization for OBS sources using point feature matching
- Adaptive stabilization based on motion type (camera shake vs. intentional motion)
- Adjustable smoothing radius and correction strength
- Edge handling modes (crop vs. padding)
- Multi-source support with independent filter instances

### Motion Classification
- Automatic detection of motion type: pan, zoom, rotation, shake, stable
- Adaptive algorithm selection based on motion characteristics
- Threshold-based motion classification with configurable parameters

### User Interface
- OBS properties panel integration for real-time parameter adjustment
- Preset save/load functionality for common scenarios
- Performance metrics display for optimization feedback

### Performance Optimization
- SIMD acceleration for NEON (ARM) and SSE/AVX (x86)
- Feature detection optimization with quality/speed trade-off
- Memory-efficient buffer management

## 2. Non-Functional Requirements

### 2.1. Performance

#### Latency
- Processing latency < 1 frame at 60fps (16.67ms)
- Target processing time: 5-10ms per 1080p frame

#### CPU Usage
- Single-core usage < 15% for 1080p@60fps
- Minimal impact on OBS overall performance

#### Memory
- Memory footprint < 100MB per filter instance
- No memory leaks during extended operation (verified with leak detection tests)

### 2.2. Reliability

#### Stability
- No crashes during 24+ hour continuous operation
- Graceful degradation on resource constraints

#### Error Handling
- Comprehensive exception handling with detailed logging
- Fallback to original frame on processing failure
- Input validation for all parameters

### 2.3. Compatibility

#### Platform Support
- Windows 10/11 (x64)
- macOS 11+ (Intel and Apple Silicon)
- Linux (Ubuntu 20.04+, CentOS 7+)

#### OBS Version
- OBS Studio 28.0+
- Future compatibility through OBS API abstraction

### 2.4. Security

- No external network dependencies
- Safe handling of untrusted input frames
- Buffer overflow prevention
- Deprecated function elimination

### 2.5. Maintainability

#### Code Quality
- Consistent coding style (clang-format)
- Comprehensive inline documentation
- Clear separation of concerns

#### Testing
- Unit test coverage > 80%
- Integration tests for OBS interaction
- Performance regression detection

## 3. Acceptance Criteria

### Core Functionality
- Visible reduction in video shake in typical use cases
- Smooth camera pan/zoom without over-correction
- Stable scenes remain stable without jitter

### Performance
- Processing time < 10ms per 1080p frame on target hardware
- CPU usage increase < 10% when filter is active
- Memory usage stable over 24-hour operation

### User Experience
- Parameters adjustable in real-time with immediate feedback
- No OBS crashes when filter is applied to multiple sources
- Intuitive default settings for common scenarios

### Cross-Platform
- Basic operations confirmed on Windows, macOS, and Linux
- Consistent behavior across all supported platforms

### Testing
- All unit tests pass on all platforms
- No memory leaks detected (valgrind/AddressSanitizer)
- Performance regressions < 5% from baseline

## 4. Design Policy

### Core Principles
1. **YAGNI**: Implement only necessary features
2. **DRY**: Eliminate code duplication
3. **KISS**: Keep implementations simple
4. **Test-Driven**: Write tests before implementation
5. **Documentation-First**: Document decisions clearly

### Technology Choices

#### Image Processing
- OpenCV 4.5+ for computer vision operations
- Point feature matching (goodFeaturesToTrack + calcOpticalFlowPyrLK)
- Rigid transform estimation with RANSAC

#### Performance
- SIMD intrinsics for critical paths
- Memory pooling for frequent allocations
- Lazy evaluation for non-critical operations

#### Architecture
- Modular design with clear interfaces
- OBS API abstraction for testability
- Separation of core logic from platform-specific code

## 5. Architecture

### 5.1. Component Overview

```
┌─────────────────────────────────────────────────────────────┐
│                     OBS Studio                             │
└────────────────────────┬────────────────────────────────────┘
                         │ obs_source_frame
                         ▼
┌─────────────────────────────────────────────────────────────┐
│              Stabilizer Plugin (obs-stabilizer-opencv)     │
├─────────────────────────────────────────────────────────────┤
│  stabilizer_opencv.cpp  │ OBS Filter Interface              │
│                        │ - obs_source_info                 │
│                        │ - obs_filter_info                 │
└────────────┬────────────┴────────────┬───────────────────┘
             │                         │
             ▼                         ▼
┌────────────────────────┐  ┌────────────────────────────┐
│  UI Management       │  │  Stabilizer Core           │
│  - Properties panel  │  │  - Feature detection       │
│  - Preset manager    │  │  - Motion estimation       │
│  - Callbacks         │  │  - Transform smoothing     │
└────────────────────────┘  └────────┬───────────────────┘
                                     │
                                     ▼
                      ┌──────────────────────────────────┐
                      │  Adaptive Stabilizer          │
                      │  - Motion classifier          │
                      │  - Algorithm selection        │
                      │  - Parameter adaptation       │
                      └──────────────────────────────────┘
                                     │
                                     ▼
                      ┌──────────────────────────────────┐
                      │  Frame Utils                  │
                      │  - OBS frame conversion       │
                      │  - Buffer management          │
                      │  - Performance tracking       │
                      └──────────────────────────────────┘
```

### 5.2. Module Responsibilities

#### StabilizerCore (src/core/stabilizer_core.cpp)
- Feature detection and tracking
- Motion vector calculation
- Transform estimation and smoothing
- Core stabilization algorithms

#### AdaptiveStabilizer (src/core/adaptive_stabilizer.cpp)
- Motion type classification (pan/zoom/rotation/shake)
- Adaptive algorithm selection
- Parameter adjustment based on motion

#### MotionClassifier (src/core/motion_classifier.cpp)
- Motion statistics calculation
- Classification threshold management
- State machine for motion types

#### FeatureDetection (src/core/feature_detection.cpp)
- Good features to track algorithm
- Quality assessment for feature points
- Feature filtering and validation

#### FrameUtils (src/core/frame_utils.cpp)
- OBS frame to OpenCV Mat conversion
- Buffer allocation and management
- Performance metrics tracking

### 5.3. Data Flow

```
Input Frame (OBS)
    │
    ▼
┌─────────────────────────────────────────┐
│ 1. Convert to OpenCV format           │
│ 2. Detect features (if needed)        │
│ 3. Track features to previous frame    │
│ 4. Calculate motion vectors           │
└─────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────┐
│ 5. Estimate transform (RANSAC)        │
│ 6. Classify motion type              │
│ 7. Select appropriate algorithm       │
└─────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────┐
│ 8. Smooth transform history           │
│ 9. Apply adaptive correction          │
│ 10. Warp frame                       │
└─────────────────────────────────────────┘
    │
    ▼
Output Frame (OBS)
```

## 6. Trade-offs

### 6.1. Performance vs. Accuracy

**Decision**: Prioritize real-time performance with tunable quality

**Rationale**:
- OBS operates under strict time constraints (16.67ms per frame at 60fps)
- Users can adjust quality parameters for their hardware
- Good visual results achievable with optimized algorithms

**Trade-off Details**:
- Feature count: 50-200 points (quality setting)
- Processing time: 5-10ms vs. 20-50ms for high quality
- Visual difference: Minimal for typical use cases

### 6.2. Library Usage vs. Custom Implementation

**Decision**: Use OpenCV for core algorithms

**Rationale**:
- OpenCV is mature, well-tested, and optimized
- Reduces development time significantly
- Active community support and updates

**Limitations**:
- Some algorithms may not be optimal for specific use cases
- Dependency management complexity
- Potential licensing considerations (MIT license)

### 6.3. Global vs. Local Stabilization

**Decision**: Implement global stabilization with optional local cropping

**Rationale**:
- Global stabilization preserves scene context
- Simpler implementation and debugging
- Most users prefer natural camera movement

**Trade-off**:
- Local stabilization could handle moving objects better
- But increases computational complexity
- May introduce unnatural motion artifacts

### 6.4. Motion Buffer Size vs. Latency

**Decision**: Default buffer size of 30 frames with configurable range

**Rationale**:
- 30 frames = 0.5 seconds at 60fps (reasonable smoothing)
- Larger buffers increase memory and latency
- Smaller buffers reduce smoothing effectiveness

**Trade-off**:
- Buffer size: 10-100 frames (configurable)
- Memory impact: ~10-100MB per instance
- Latency impact: 0.17-1.67 seconds

### 6.5. Adaptive vs. Static Algorithms

**Decision**: Implement adaptive stabilization with fallback to static

**Rationale**:
- Different content types require different approaches
- Adaptive improves overall user experience
- Static fallback ensures reliability

**Trade-off**:
- Adaptive: Better results but more complex
- Static: Simpler but less effective for varied content
- Classification accuracy impacts adaptive performance

## 7. Implementation Priorities

### Phase 4: Optimization & Release Preparation (Week 9-10)

**High Priority**:
1. Fix frame buffer lifecycle management (Issue #XXX)
2. Add edge case and integration tests
3. Add memory leak detection tests
4. Improve exception handling and logging

**Medium Priority**:
5. Separate concerns in StabilizerCore
6. Extract UI logic to separate module
7. Reduce unnecessary frame copies
8. Update documentation

**Low Priority**:
9. Consider GPU acceleration for high-end systems
10. Add advanced preset management
11. Implement custom motion profiles

## 8. Technical Debt

### Known Issues

1. **Frame Buffer Lifecycle** (HIGH)
   - Static buffer may be overwritten before OBS consumes
   - Impact: Potential crashes or visual artifacts
   - Solution: Implement frame pooling or per-call buffers

2. **Thread Safety** (MEDIUM)
   - No explicit thread safety guarantees
   - Impact: Issues in multi-source or multi-threaded OBS scenarios
   - Solution: Add mutex protection or document single-threaded guarantee

3. **Error Handling** (MEDIUM)
   - Inconsistent error handling across modules
   - Impact: Difficult debugging, unclear failure modes
   - Solution: Implement consistent error policy (result/expected pattern)

### Code Quality

1. **Const Cast Usage** (LOW)
   - const_cast removes const from OBS data pointers
   - Impact: Potential API violations
   - Solution: Create proper wrapper functions

2. **Magic Numbers** (LOW)
   - Unexplained constants in motion classifier
   - Impact: Difficult parameter tuning
   - Solution: Add named constants with derivation explanations

## 9. Future Enhancements

### Potential Features

1. **GPU Acceleration**
   - OpenCV CUDA support for high-end systems
   - Transparent OpenCL fallback for mid-range hardware
   - Estimated performance gain: 2-5x

2. **Advanced Motion Analysis**
   - Optical flow field visualization
   - Motion graph for debugging
   - User-selectable motion profiles

3. **Machine Learning**
   - Content-aware stabilization
   - Object tracking for selective stabilization
   - Learned motion classification

### Platform-Specific

1. **Windows**
   - DirectShow integration for low-latency capture
   - GPU scheduling optimization

2. **macOS**
   - Metal Performance Shaders integration
   - Apple Silicon-specific optimizations

3. **Linux**
   - VDPAU/VAAPI acceleration
   - PipeWire/Wayland support

## 10. Success Metrics

### Performance Targets
- Processing time: < 10ms (1080p@60fps)
- CPU usage: < 15% (single core)
- Memory: < 100MB per instance
- Latency: < 2 frames

### Quality Metrics
- Shake reduction: > 70% (perceptual)
- Natural motion preservation: > 80%
- User satisfaction: > 4/5 stars

### Stability Metrics
- Crash rate: < 0.1% per hour
- Memory leaks: 0 detected
- Test coverage: > 80%

---

*Document Version: 1.0*
*Last Updated: 2026-02-10*
*Status: Phase 4 Planning*
