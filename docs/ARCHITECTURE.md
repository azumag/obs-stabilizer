# Architecture Document

## Project Overview

**OBS Stabilizer Plugin** - A real-time video stabilization plugin for OBS Studio using OpenCV computer vision algorithms.

**Status**: Functional with minor technical debt items to address

## Functional Requirements

### Core Functionality
- **Real-time Video Stabilization**: Apply video stabilization to live streams and recordings in OBS Studio
- **Point Feature Matching**: Use OpenCV's feature detection and tracking algorithms (Point Feature Matching + Lucas-Kanade Optical Flow)
- **Transform Smoothing**: Apply configurable smoothing to stabilization transforms
- **User Interface**: Provide OBS properties panel for configuration
- **Multi-format Support**: Support NV12 and I420 video formats

### User-Configurable Parameters
- **Enable Stabilization**: Main toggle for stabilization processing
- **Smoothing Radius**: Transform smoothing window (10-100 frames)
- **Feature Points**: Number of tracking points (100-1000)
- **Stability Threshold**: Error threshold for tracking quality (10.0-100.0)
- **Edge Handling**: Crop borders/Black padding/Scale to fit options

## Non-Functional Requirements

### Performance Requirements
- **720p**: <2ms/frame (60fps+ capable)
- **1080p**: <4ms/frame (30fps+ capable)
- **1440p**: <8ms/frame
- **4K**: <15ms/frame

### Quality Attributes
- **Reliability**: No crashes or major memory issues during normal operation
- **Maintainability**: Follow YAGNI, KISS, DRY principles
- **Code Quality**: Consistent coding standards, proper error handling
- **Thread Safety**: Thread-safe configuration updates and frame processing
- **Memory Safety**: RAII resource management, bounds checking, buffer overflow protection

### Standards Compliance
- **OBS Plugin Standards**: Use obs_log() for logging (not printf) in production code
- **Cross-Platform**: Support macOS, Windows, Linux
- **Build System**: Use CMake with proper dependency management

## Acceptance Criteria

### Functional Acceptance
- [x] Plugin loads successfully in OBS Studio
- [x] "Stabilizer" filter appears in OBS filters list
- [x] Stabilization processing works correctly
- [x] Configuration UI is accessible and functional
- [x] All performance targets met

### Code Quality Acceptance
- [x] No compiler warnings
- [x] All tests passing
- [x] Code follows project coding standards
- [x] Logging standardized to obs_log() in production code
- [x] Build system consolidated to essential files

### Integration Acceptance
- [x] CI/CD pipeline operational
- [x] Cross-platform builds successful
- [x] Plugin installation working on target platforms

## Design Principles

### Core Principles
1. **YAGNI (You Aren't Gonna Need It)**: Implement only what's needed
2. **KISS (Keep It Simple, Stupid)**: Prioritize simplicity and clarity
3. **DRY (Don't Repeat Yourself)**: Eliminate code duplication
4. **TDD (Test-Driven Development)**: Write tests before implementation (when applicable)

### Architectural Guidelines
- Keep components focused and single-purpose
- Use minimal abstractions
- Prefer direct implementation over complex patterns
- Maintain clear separation of concerns

## Current Architecture

### Source File Structure
```
src/
├── plugin_main.cpp              # OBS module entry point
├── obs_plugin.cpp               # Minimal test implementation (no OpenCV)
├── obs_module_exports.c         # C linkage layer for OBS module functions
├── obs_stubs.c                  # OBS stubs for standalone builds
├── plugin-support.c             # OBS API compatibility bridge
├── stabilizer_opencv.cpp        # OpenCV-based stabilizer (main implementation)
├── stabilizer_filter.cpp        # OBS filter integration
├── stabilizer.cpp               # Legacy stabilizer (may be removed)
├── minimal_*.cpp                # Test/standalone implementations
└── tests/
    └── ...                     # Test files
```

### Component Overview

#### 1. OBS Module Layer (plugin_main.cpp, obs_module_exports.c)
- Purpose: OBS module entry point and C linkage
- Exported functions: obs_module_load(), obs_module_unload(), obs_module_name()
- Status: Functional

#### 2. OBS API Bridge (plugin-support.c, obs_stubs.c)
- Purpose: Compatibility layer for OBS API differences
- Functions: obs_register_source(), obs_log()
- Status: Functional

#### 3. Stabilizer Implementation (stabilizer_opencv.cpp)
- Purpose: Core stabilization logic using OpenCV
- Algorithm: Point Feature Matching + Lucas-Kanade Optical Flow
- Status: Functional

#### 4. OBS Filter Integration (stabilizer_filter.cpp, obs_plugin.cpp)
- Purpose: OBS filter registration and callbacks
- Status: obs_plugin.cpp is minimal test, stabilizer_filter.cpp needs verification

## Technical Debt Items

### High Priority

#### 1. Memory Management Audit (Issue #167) ⚠️ **ACTIVE**
**Problem**: C++ memory management mixing with OBS C callbacks

**Current State**:
- stabilizer_opencv.cpp: Uses calloc/free for struct containing C++ objects
- stabilizer_filter.cpp: Uses new/delete for struct containing C++ objects
- C++ objects (cv::Mat, std::vector, std::deque, std::mutex) not properly initialized/destroyed

**Solution**: Implement proper C++ object lifecycle management in OBS callbacks
- Use placement new for C++ objects in C-allocated memory
- Ensure proper destructor calls before freeing memory
- Consider converting to pure C implementation for OBS compatibility

**Impact**: High effort, High impact

#### 2. Logging Standardization (Issue #168) ✅ **RESOLVED**
**Problem**: Mixed usage of printf() and obs_log() across codebase

**Current State**:
- obs_log: stabilizer_opencv.cpp (correct)
- obs_log: obs_plugin.cpp (now correct - was printf)
- obs_log: plugin_main.cpp (now correct - was printf)
- printf: obs_stubs.c, minimal_*.cpp (acceptable for tests)

**Solution Implemented**:
- ✅ Replaced printf() with obs_log() in obs_plugin.cpp
- ✅ Replaced printf() with obs_log() in plugin_main.cpp
- ✅ Kept printf() in test files (obs_stubs.c, minimal_*.cpp)

**Impact**: Medium effort, Medium impact

#### 3. Build System Consolidation (Issue #169) ✅ **RESOLVED**
**Problem**: Multiple CMakeLists.txt files create maintenance burden

**Current State**:
- CMakeLists.txt (root) - Consolidated main build configuration
- No src/CMakeLists.txt (merged into root)
- No src/tests/CMakeLists.txt (removed)
- No tmp/tests/CMakeLists.txt (removed)

**Solution Implemented**:
- ✅ Merged src/CMakeLists.txt into root CMakeLists.txt
- ✅ Removed src/tests/CMakeLists.txt
- ✅ Removed tmp/tests/CMakeLists.txt
- ✅ Target achieved: 1 essential CMakeLists.txt file

**Impact**: Medium effort, Medium impact

### Medium Priority

#### 4. Deployment Strategy (Issue #171)
**Problem**: OpenCV dependency creates end-user installation complexity

**Current State**: Plugin requires OpenCV runtime libraries

**Solution**: Implement static linking or bundled distribution strategy

**Impact**: High effort, Medium impact

#### 5. Magic Numbers (Issue #170)
**Problem**: Magic numbers throughout stabilizer.cpp impact maintainability

**Current State**: Hard-coded values in algorithm parameters

**Solution**: Replace with named constants or configuration parameters

**Impact**: Medium effort, Medium impact

### Low Priority

#### 6. Test Coverage Expansion (Issue #172)
**Problem**: Limited automated testing of stabilization algorithms

**Current State**: Manual testing only for OBS integration

**Solution**: Expand automated test coverage

**Impact**: High effort, Medium impact

## Design Decisions and Trade-offs

### 1. OpenCV Integration
**Decision**: Use OpenCV for computer vision algorithms
**Trade-off**:
- Pro: Powerful, well-tested algorithms
- Con: Creates deployment complexity, large dependency

### 2. C/C++ Hybrid
**Decision**: Use C++ for core logic, C for OBS module interface
**Trade-off**:
- Pro: Proper C linkage for OBS module functions
- Con: Added complexity in memory management

### 3. Minimal Test Implementation
**Decision**: Keep obs_plugin.cpp as minimal test without OpenCV
**Trade-off**:
- Pro: Allows development without OpenCV dependency
- Con: Maintains two implementations

## Implementation Roadmap

### Phase 1: Memory Management Audit (Current Sprint)
- [ ] Audit C++ object lifecycle in OBS callbacks
- [ ] Implement proper placement new/destructor calls
- [ ] Add memory leak detection tests
- [ ] Document memory management best practices

### Phase 2: Medium Priority Items
- [ ] Design deployment strategy
- [ ] Replace magic numbers with constants
- [ ] Expand automated test coverage

### Phase 3: Low Priority Items
- [ ] Performance optimization
- [ ] GPU acceleration support
- [ ] Enhanced algorithms

## Metrics and Success Criteria

### Code Quality Metrics
- **Total Lines of Code**: Currently ~2,277 lines (reasonable)
- **Complexity**: Maintain cyclomatic complexity < 10
- **Test Coverage**: Increase from current level (target to be defined)

### Performance Metrics
- **720p Processing Time**: <2ms/frame ✅
- **1080p Processing Time**: <4ms/frame ✅
- **Memory Usage**: Stable over long periods ✅

### Build Metrics
- **CMakeLists.txt Files**: Target 1-2 files (current: 3-4)
- **Build Time**: <5 minutes on CI

## Dependencies

### External Dependencies
- **OBS Studio**: 30.0+
- **OpenCV**: 4.5+ (with 5.x experimental support)
- **CMake**: 3.16+
- **C++17**: Compatible compiler

### Internal Dependencies
- Core logic depends on OpenCV
- OBS integration depends on OBS headers
- Tests depend on Google Test framework

## Future Considerations

### Potential Improvements
- GPU acceleration for OpenCV operations
- Enhanced algorithms (rolling shutter correction, etc.)
- Preset system for different use cases
- Advanced diagnostic and monitoring tools

### Scalability
- Multi-threaded processing for higher resolutions
- Adaptive algorithms based on camera movement
- Machine learning-based stabilization

## Related Documentation

- **README.md**: Project overview and build instructions
- **CLAUDE.md**: Development workflow and principles
- **docs/architecture.md**: Detailed technical architecture
- **docs/ARCHITECTURE_SIMPLIFICATION.md**: Simplification plan
- **docs/TDD_METHODOLOGY.md**: Test-driven development guidelines

## Change History

| Date | Version | Author | Description |
|------|---------|--------|-------------|
| 2025-08-04 | 1.0 | azumag | Initial architecture documentation |
| 2025-08-04 | 1.1 | azumag | Updated with Issue #173 technical debt items |
| 2026-01-18 | 1.2 | azumag | Added tmp directory cleanup design (Issue #166) |
| 2026-01-19 | 2.0 | azumag | Renamed previous version, focused on Issue #167 memory management audit |

---

# Issue #167: Memory Management Audit Design

## Overview

This section outlines architecture and implementation strategy for resolving Issue #167: "[MEMORY SAFETY] Critical C++/C memory management audit required in OBS integration".

## Problem Analysis

### Current Memory Management Patterns

#### 1. stabilizer_opencv.cpp (Main Implementation)
**Create Function**:
```cpp
static void *stabilizer_filter_create(obs_data_t *settings, obs_source_t *source)
{
    struct stabilizer_filter *filter = (struct stabilizer_filter *)calloc(1, sizeof(struct stabilizer_filter));
    // ... initialization ...
    return filter;
}
```

**Destroy Function**:
```cpp
static void stabilizer_filter_destroy(void *data)
{
    struct stabilizer_filter *filter = (struct stabilizer_filter *)data;
    if (filter) {
        free(filter);
    }
}
```

**Problem**: C++ objects in struct not properly initialized/destroyed:
- `cv::Mat prev_gray`
- `std::vector<cv::Point2f> prev_pts`
- `std::deque<cv::Mat> transforms`
- `cv::Mat cumulative_transform`
- `std::mutex mutex`

#### 2. stabilizer_filter.cpp (Alternative Implementation)
**Create Function**:
```cpp
static void *minimal_stabilizer_create(obs_data_t *settings, obs_source_t *source)
{
    minimal_stabilizer_data *data = new minimal_stabilizer_data;
    // ... initialization ...
    return data;
}
```

**Destroy Function**:
```cpp
static void minimal_stabilizer_destroy(void *data)
{
    minimal_stabilizer_data *filter = (minimal_stabilizer_data *)data;
    if (filter) {
        pthread_mutex_destroy(&filter->mutex);
        if (filter->prev_frame) {
            bfree(filter->prev_frame);
        }
        delete filter;
    }
}
```

**Problem**: C++ objects mixed with C-style memory management:
- `std::deque<Transform> transform_history`
- `std::deque<Transform> smoothed_transforms`
- `pthread_mutex_t mutex`

## Functional Requirements

### FR-1: Proper C++ Object Lifecycle
- Ensure all C++ objects are properly constructed
- Ensure all C++ objects are properly destructed
- No memory leaks during plugin load/unload cycles

### FR-2: OBS Callback Compatibility
- Memory allocation must be compatible with OBS C callbacks
- Memory deallocation must not crash OBS shutdown
- Exception handling must not propagate to OBS runtime

### FR-3: Cross-Platform Memory Safety
- Consistent behavior across macOS, Windows, Linux
- Platform-specific memory management handled correctly
- No undefined behavior from mixing C/C++ allocation

## Non-Functional Requirements

### NFR-1: Performance
- Memory allocation overhead < 1% of frame processing time
- No performance regression compared to current implementation

### NFR-2: Maintainability
- Clear memory management patterns
- Well-documented lifecycle
- Easy to audit and verify

### NFR-3: Safety
- No null pointer dereferences
- No double-free or use-after-free
- Exception safety guarantees

## Acceptance Criteria

### AC-1: Memory Leak Testing
- [ ] No memory leaks detected in plugin load/unload cycles
- [ ] Valgrind/AddressSanitizer reports no issues
- [ ] Memory usage stable over extended operation

### AC-2: Exception Safety
- [ ] No exceptions propagate to OBS callbacks
- [ ] Graceful handling of allocation failures
- [ ] Safe cleanup on error conditions

### AC-3: Cross-Platform Validation
- [ ] Tests pass on macOS
- [ ] Tests pass on Windows
- [ ] Tests pass on Linux

### AC-4: Code Quality
- [ ] Compiler warnings resolved
- [ ] Static analysis passes
- [ ] Code review approved

## Design Architecture

### Solution Options Analysis

#### Option 1: Placement New (Recommended for stabilizer_opencv.cpp)
**Approach**: Use placement new to construct C++ objects in C-allocated memory

**Implementation**:
```cpp
// In create function
static void *stabilizer_filter_create(obs_data_t *settings, obs_source_t *source)
{
    // Allocate memory with calloc
    struct stabilizer_filter *filter = (struct stabilizer_filter *)calloc(1, sizeof(struct stabilizer_filter));
    if (!filter) return nullptr;
    
    // Construct C++ objects using placement new
    new (&filter->prev_gray) cv::Mat();
    new (&filter->prev_pts) std::vector<cv::Point2f>();
    new (&filter->transforms) std::deque<cv::Mat>();
    new (&filter->cumulative_transform) cv::Mat();
    new (&filter->mutex) std::mutex();
    
    // ... other initialization ...
    return filter;
}

// In destroy function
static void stabilizer_filter_destroy(void *data)
{
    struct stabilizer_filter *filter = (struct stabilizer_filter *)data;
    if (filter) {
        // Destroy C++ objects explicitly
        filter->prev_gray.~cv::Mat();
        filter->prev_pts.~vector<cv::Point2f>();
        filter->transforms.~deque<cv::Mat>();
        filter->cumulative_transform.~cv::Mat();
        filter->mutex.~mutex();
        
        free(filter);
    }
}
```

**Pros**:
- Minimal code changes
- Maintains C allocation pattern
- Compatible with OBS callbacks

**Cons**:
- Manual destructor calls required
- Error-prone if order is wrong
- Complex to understand

#### Option 2: Smart Pointers (Recommended for stabilizer_filter.cpp)
**Approach**: Replace new/delete with std::unique_ptr

**Implementation**:
```cpp
// In create function
static void *minimal_stabilizer_create(obs_data_t *settings, obs_source_t *source)
{
    auto data = std::make_unique<minimal_stabilizer_data>();
    // ... initialization ...
    return data.release();
}

// In destroy function
static void minimal_stabilizer_destroy(void *data)
{
    auto filter = std::unique_ptr<minimal_stabilizer_data>(
        static_cast<minimal_stabilizer_data*>(data)
    );
    // unique_ptr destructor will automatically clean up
}
```

**Pros**:
- Automatic memory management
- Exception-safe
- Modern C++ idiom

**Cons**:
- Requires C++11 or later (already satisfied)
- Still mixing C/C++ patterns

#### Option 3: Pure C Implementation (Not Recommended)
**Approach**: Convert all C++ code to C

**Implementation**:
- Replace std::vector with dynamic arrays
- Replace std::deque with linked lists
- Replace cv::Mat with custom struct
- Replace std::mutex with pthread_mutex_t

**Pros**:
- Pure C compatibility
- No C++ runtime overhead

**Cons**:
- Massive code rewrite
- Loss of C++ benefits
- High maintenance burden

### Recommended Solution

**Hybrid Approach**: Use Option 1 for stabilizer_opencv.cpp, Option 2 for stabilizer_filter.cpp

**Rationale**:
1. **stabilizer_opencv.cpp**: Heavily dependent on OpenCV (C++), placement new is most appropriate
2. **stabilizer_filter.cpp**: Uses standard C++ containers, smart pointers are cleaner

## Implementation Plan

### Phase 1: stabilizer_opencv.cpp Refactoring

#### Step 1: Update Create Function
```cpp
static void *stabilizer_filter_create(obs_data_t *settings, obs_source_t *source)
{
    obs_log(LOG_INFO, "Creating OpenCV stabilizer filter");
    
    // Allocate memory
    struct stabilizer_filter *filter = (struct stabilizer_filter *)calloc(1, sizeof(struct stabilizer_filter));
    if (!filter) {
        obs_log(LOG_ERROR, "Failed to allocate memory for stabilizer filter");
        return nullptr;
    }
    
    // Construct C++ objects using placement new
    try {
        new (&filter->prev_gray) cv::Mat();
        new (&filter->prev_pts) std::vector<cv::Point2f>();
        new (&filter->transforms) std::deque<cv::Mat>();
        new (&filter->cumulative_transform) cv::Mat();
        new (&filter->mutex) std::mutex();
    } catch (const std::exception& e) {
        obs_log(LOG_ERROR, "Failed to construct C++ objects: %s", e.what());
        free(filter);
        return nullptr;
    }
    
    // ... rest of initialization ...
    return filter;
}
```

#### Step 2: Update Destroy Function
```cpp
static void stabilizer_filter_destroy(void *data)
{
    struct stabilizer_filter *filter = (struct stabilizer_filter *)data;
    
    if (filter) {
        obs_log(LOG_INFO, "Destroying OpenCV stabilizer filter");
        
        // Destroy C++ objects explicitly in reverse order of construction
        filter->mutex.~mutex();
        filter->cumulative_transform.~cv::Mat();
        filter->transforms.~deque<cv::Mat>();
        filter->prev_pts.~vector<cv::Point2f>();
        filter->prev_gray.~cv::Mat();
        
        free(filter);
    }
}
```

### Phase 2: stabilizer_filter.cpp Refactoring

#### Step 1: Update Create Function
```cpp
static void *minimal_stabilizer_create(obs_data_t *settings, obs_source_t *source)
{
    try {
        auto data = std::make_unique<minimal_stabilizer_data>();
        
        data->context = source;
        data->enabled = true;
        data->smoothing_window = 5;
        data->stabilization_strength = 0.8f;
        data->prev_frame = nullptr;
        data->frame_width = 0;
        data->frame_height = 0;
        
        pthread_mutex_init(&data->mutex, nullptr);
        
        if (settings) {
            data->enabled = obs_data_get_bool(settings, "enabled");
            data->smoothing_window = (int)obs_data_get_int(settings, "smoothing_window");
            data->stabilization_strength = (float)obs_data_get_double(settings, "strength");
        }
        
        return data.release();
    } catch (const std::exception& e) {
        obs_log(LOG_ERROR, "Failed to create stabilizer: %s", e.what());
        return nullptr;
    }
}
```

#### Step 2: Update Destroy Function
```cpp
static void minimal_stabilizer_destroy(void *data)
{
    if (data) {
        auto filter = std::unique_ptr<minimal_stabilizer_data>(
            static_cast<minimal_stabilizer_data*>(data)
        );
        
        pthread_mutex_destroy(&filter->mutex);
        
        if (filter->prev_frame) {
            bfree(filter->prev_frame);
        }
        
        // unique_ptr destructor automatically deletes filter
    }
}
```

### Phase 3: Testing

#### Memory Leak Testing
```bash
# Build with AddressSanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address -g" -DCMAKE_C_FLAGS="-fsanitize=address -g" ..
cmake --build .

# Run tests
./stabilizer_tests

# Run plugin with leak detection
valgrind --leak-check=full --track-origins=yes ./obs-stabilizer-opencv
```

#### Exception Safety Testing
- Test with invalid video frames
- Test with out-of-memory conditions
- Test with concurrent access patterns

## Testing Strategy

### Pre-Implementation Testing
1. Benchmark current memory usage
2. Identify memory leak patterns
3. Document current behavior

### Post-Implementation Testing
1. Verify no memory leaks with valgrind/ASan
2. Test exception safety with invalid inputs
3. Verify cross-platform behavior
4. Performance regression testing

## Related Issues

- Issue #167: [MEMORY SAFETY] Critical C++/C memory management audit
- Issue #168: [CODE QUALITY] Replace printf() logging with OBS logging system ✅ RESOLVED
- Issue #169: [BUILD SYSTEM] Consolidate duplicate CMakeLists.txt files ✅ RESOLVED

## Success Metrics

- No memory leaks detected
- Exception safety verified
- Cross-platform compatibility confirmed
- Performance regression < 5%

## Timeline Estimate

- **Phase 1 (stabilizer_opencv.cpp)**: 2 hours
- **Phase 2 (stabilizer_filter.cpp)**: 1 hour
- **Phase 3 (Testing)**: 2 hours

**Total Estimated Time**: 5 hours
