# OBS Stabilizer Architecture Decision

## Overview
This document outlines the architecture decisions for the OBS Stabilizer plugin, which provides real-time video stabilization using OpenCV.

## Functional Requirements

### Core Functionality
- **Real-time Video Stabilization**: Process video frames at 30+ FPS for 1080p resolution
- **Feature-Based Motion Detection**: Use Lucas-Kanade optical flow for feature tracking
- **Transform Estimation**: Estimate affine transformations between consecutive frames
- **Motion Smoothing**: Apply temporal smoothing to reduce jitter
- **Edge Handling**: Support multiple edge handling modes (Padding, Crop, Scale)

### Adaptive Stabilization
- **Motion Classification**: Automatically detect motion type (Static, Slow, Fast, Shake, Pan/Zoom)
- **Parameter Adaptation**: Dynamically adjust stabilization parameters based on motion type
- **Smooth Transitions**: Gradual parameter changes to avoid visible artifacts

### OBS Integration
- **Plugin Architecture**: Implement as OBS video filter
- **Settings UI**: Provide user-configurable parameters via OBS properties panel
- **Presets**: Offer pre-configured settings for Gaming, Streaming, and Recording

## Non-Functional Requirements

### Performance
- **Target Processing Time**: < 10ms per frame (1080p) for real-time performance
- **Memory Usage**: < 200MB for 1080p @ 30fps
- **CPU Utilization**: < 50% on modern hardware for 1080p @ 30fps

### Reliability
- **Error Handling**: Graceful degradation on frame processing failures
- **Validation**: Robust parameter validation to prevent crashes
- **Thread Safety**: Single-threaded design (OBS filters are single-threaded)

### Maintainability
- **Modular Design**: Clear separation of concerns
- **DRY Principle**: Avoid code duplication
- **KISS Principle**: Keep implementation simple and straightforward
- **Documentation**: Inline comments for implementation rationale

### Portability
- **Cross-Platform**: Support Windows, macOS, Linux
- **Standalone Mode**: Support testing without OBS headers

## Acceptance Criteria

### Phase 1: Core Functionality (COMPLETE)
- [x] Basic video stabilization using feature tracking
- [x] Transform estimation and smoothing
- [x] OBS plugin integration
- [x] Basic settings UI
- [x] Performance testing infrastructure

### Phase 2: Advanced Features (COMPLETE)
- [x] Adaptive stabilization with motion classification
- [x] High-pass filter for camera shake
- [x] Directional smoothing for pan/zoom
- [x] Edge handling modes
- [x] Performance optimization

### Phase 3: Quality Assurance (COMPLETE)
- [x] Unit tests for core modules
- [x] Integration tests
- [x] Performance benchmarks
- [x] Static analysis
- [x] Code coverage

### Phase 4: Production Readiness
- [ ] Comprehensive documentation
- [ ] User guide
- [ ] Developer guide
- [ ] CI/CD pipeline
- [ ] Release packages

## Design Principles

### YAGNI (You Aren't Gonna Need It)
- Only implement features that are needed now
- Avoid premature optimization
- Focus on minimal viable functionality

### DRY (Don't Repeat Yourself)
- Centralize parameter validation
- Reuse frame conversion utilities
- Consolidate color conversion logic

### KISS (Keep It Simple Stupid)
- Use standard OpenCV algorithms
- Avoid unnecessary abstractions
- Keep error handling straightforward

## Architecture Decisions

### Modular Architecture
The plugin is organized into clear modules with well-defined responsibilities:

```
obs-stabilizer/
├── src/
│   ├── stabilizer_opencv.cpp      # OBS integration & UI
│   ├── core/
│   │   ├── stabilizer_core.hpp    # Core stabilization engine
│   │   ├── stabilizer_core.cpp
│   │   ├── adaptive_stabilizer.hpp # Adaptive stabilization
│   │   ├── adaptive_stabilizer.cpp
│   │   ├── motion_classifier.hpp   # Motion type detection
│   │   ├── motion_classifier.cpp
│   │   ├── feature_detection.hpp   # Feature point detection
│   │   ├── feature_detection.cpp
│   │   ├── stabilizer_wrapper.hpp  # RAII wrapper
│   │   ├── stabilizer_wrapper.cpp
│   │   ├── frame_utils.hpp        # Frame conversion utilities
│   │   ├── frame_utils.cpp
│   │   ├── parameter_validation.hpp # Parameter validation
│   │   └── stabilizer_constants.hpp # Named constants
├── tests/                         # Unit tests
├── tools/                         # Performance benchmarking
└── CMakeLists.txt
```

### Core Algorithm Selection

**Chosen Algorithm**: Point Feature Matching (Shi-Tomasi + Lucas-Kanade)

**Rationale**:
- Real-time performance (>30fps @ 1080p)
- Low memory usage (< 100MB)
- Well-suited for video stabilization
- Efficient OpenCV implementation
- No need for GPU acceleration

### Trade-offs

**Algorithm Selection**:
- *Alternative*: Feature-Based (SURF/ORB) with GPU acceleration
- *Trade-off*: Higher precision but increased complexity and hardware dependencies
- *Decision*: Use simpler algorithm for better portability and simplicity

**Thread Safety**:
- *Design*: Single-threaded (no mutex)
- *Rationale*: OBS filters are single-threaded
- *Trade-off*: Not thread-safe for multi-threaded use
- *Decision*: Accept this limitation for simplicity and performance

**Edge Handling**:
- *Design*: Three modes (Padding, Crop, Scale)
- *Trade-off*: Each has different quality vs performance characteristics
- *Decision*: Support all modes for flexibility

### Parameter Management

**Preset System**:
- Gaming: Performance-focused (fewer features, faster smoothing)
- Streaming: Balanced (moderate features, medium smoothing)
- Recording: Quality-focused (more features, longer smoothing)

**Adaptive Mode**:
- Dynamically adjusts parameters based on motion type
- Uses smooth transitions to avoid artifacts
- Can be disabled for manual control

## Implementation Notes

### Performance Optimizations
- Pre-allocated memory for feature points
- Branch prediction hints for common cases
- Lookup tables for adaptive feature count
- Division by multiplication (inverse calculation)
- Efficient content detection using findNonZero()

### Error Handling
- Try-catch blocks around OpenCV operations
- Graceful degradation on failures
- Comprehensive error messages
- Return empty frames on errors (OBS handles missing frames)

### Logging
- Debug mode for detailed logging
- Error logging for failures
- Performance metrics tracking
- Motion type logging in adaptive mode

## Future Enhancements

### Potential Features
- Kalman filter for improved smoothing
- Custom preset management (save/load)
- Real-time parameter adjustment UI
- Performance metrics visualization
- Region of interest (ROI) support

### Technical Debt
- Consider thread safety for future multi-threaded use
- Evaluate GPU acceleration for 4K support
- Improve test coverage for edge cases
- Enhance documentation with examples

## Conclusion

The OBS Stabilizer plugin is designed with a focus on:
- **Simplicity**: Clean modular architecture
- **Performance**: Real-time processing at 30fps
- **Reliability**: Robust error handling
- **Maintainability**: Well-documented, DRY code

The architecture follows KISS, YAGNI, and DRY principles, providing a solid foundation for future enhancements while keeping the current implementation straightforward and efficient.
