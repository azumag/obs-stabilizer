# OBS Stabilizer Plugin - System Architecture

## Overview

The OBS Stabilizer Plugin provides real-time video stabilization for OBS Studio using OpenCV computer vision algorithms. This document defines the system architecture, component relationships, and technical interfaces.

## System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                           OBS Studio                            │
├─────────────────────────────────────────────────────────────────┤
│                     OBS Plugin Interface                        │
├─────────────────────────────────────────────────────────────────┤
│  OBS Stabilizer Plugin                                          │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │   UI Module     │  │   Core Engine   │  │ OBS Integration │ │
│  │                 │  │                 │  │                 │ │
│  │ - Properties    │◄─┤ - Feature       │◄─┤ - Filter        │ │
│  │ - Settings      │  │   Detection     │  │   Registration  │ │
│  │ - Presets       │  │ - Tracking      │  │ - Frame         │ │
│  │                 │  │ - Transform     │  │   Processing    │ │
│  └─────────────────┘  │ - Smoothing     │  │ - Memory        │ │
│                       │                 │  │   Management    │ │
│                       └─────────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│                         OpenCV Library                          │
└─────────────────────────────────────────────────────────────────┘
```

## Core Components

### 1. OBS Integration Module (`obs_integration.hpp`)

**Responsibilities:**
- OBS Studio plugin registration and lifecycle management
- Video frame input/output handling
- Memory management for video buffers
- Format conversion (NV12, I420, RGB)

**Key Interfaces:**
```cpp
// Plugin lifecycle
obs_module_load()
obs_module_unload()

// Filter interface
stabilizer_create()
stabilizer_destroy()
stabilizer_video_tick()
stabilizer_video_render()
```

### 2. Core Stabilization Engine (`stabilizer_core.hpp`)

**Responsibilities:**
- OpenCV feature detection and tracking
- Transform matrix calculation and smoothing
- Frame stabilization application
- Error handling and recovery

**Key Interfaces:**
```cpp
class StabilizerCore {
public:
    bool initialize(const StabilizerConfig& config);
    TransformResult process_frame(const VideoFrame& frame);
    void update_configuration(const StabilizerConfig& config);
    StabilizerStatus get_status() const;
};
```

### 3. UI Module (`stabilizer_ui.hpp`)

**Responsibilities:**
- OBS properties panel integration
- Real-time parameter updates
- Preset management
- User feedback and status display

**Key Interfaces:**
```cpp
// Properties interface
obs_properties_t* stabilizer_properties(void* data);
void stabilizer_defaults(obs_data_t* settings);
void stabilizer_update(void* data, obs_data_t* settings);
```

## Data Flow Architecture

### Video Processing Pipeline

```
Video Frame Input
       │
       ▼
┌─────────────────┐
│ Format Detection│
│ (NV12/I420/RGB) │
└─────────────────┘
       │
       ▼
┌─────────────────┐
│  Frame Buffer   │
│   Validation    │
└─────────────────┘
       │
       ▼
┌─────────────────┐
│ Feature Tracking│
│   (OpenCV)      │
└─────────────────┘
       │
       ▼
┌─────────────────┐
│ Transform Calc  │
│ & Smoothing     │
└─────────────────┘
       │
       ▼
┌─────────────────┐
│ Frame Transform │
│   Application   │
└─────────────────┘
       │
       ▼
Video Frame Output
```

### Configuration Update Flow

```
UI Parameter Change
       │
       ▼
┌─────────────────┐
│   Validation    │
│  & Bounds Check │
└─────────────────┘
       │
       ▼
┌─────────────────┐
│ Thread-Safe     │
│  Config Update  │
└─────────────────┘
       │
       ▼
┌─────────────────┐
│ Core Engine     │
│ Reconfiguration │
└─────────────────┘
```

## Threading Model

### Thread Safety Design

**Primary Video Thread**: OBS Studio video processing thread
- Handles all video frame processing
- Executes stabilization algorithms
- Must maintain real-time performance (<16ms for 60fps)

**UI Thread**: Main application thread
- Handles user interface updates
- Configuration changes
- Status reporting

**Thread Synchronization**:
```cpp
class ThreadSafeConfig {
private:
    std::atomic<bool> config_dirty_;
    std::mutex config_mutex_;
    StabilizerConfig active_config_;
    StabilizerConfig pending_config_;
    
public:
    void update_config(const StabilizerConfig& new_config);
    bool apply_pending_config();
};
```

## API Specifications

### Configuration API

```cpp
struct StabilizerConfig {
    // Core parameters
    int smoothing_radius = 30;        // Range: 10-100
    int max_features = 200;           // Range: 100-1000  
    float error_threshold = 30.0f;    // Range: 10.0-100.0
    
    // Processing options
    bool enable_stabilization = true;
    ProcessingMode mode = QUALITY;    // QUALITY, PERFORMANCE, BALANCED
    
    // Advanced parameters
    float min_feature_quality = 0.01f;
    int refresh_threshold = 25;
    bool adaptive_refresh = true;
};
```

### Status API

```cpp
enum class StabilizerStatus {
    INACTIVE,           // Stabilization disabled
    INITIALIZING,       // First frame processing
    ACTIVE,            // Normal operation
    DEGRADED,          // Reduced quality mode
    ERROR_RECOVERY,    // Recovering from error
    FAILED            // Stabilization failed
};

struct StabilizerMetrics {
    uint32_t tracked_features;
    float processing_time_ms;
    float transform_stability;
    uint32_t error_count;
    StabilizerStatus status;
};
```

## Error Handling Architecture

### Error Classification

**Level 1 - Recoverable Errors**:
- Insufficient feature points: Retry with relaxed parameters
- Tracking failure: Reset tracking state
- Transform validation failure: Skip frame transform

**Level 2 - Degraded Operation**:
- Consistent tracking issues: Reduce feature count
- Performance problems: Switch to performance mode
- Memory pressure: Reduce buffer sizes

**Level 3 - Critical Errors**:
- OpenCV exceptions: Reset entire stabilization state
- Memory allocation failures: Disable stabilization
- Invalid frame data: Pass-through mode

### Error Recovery Strategy

```cpp
class ErrorRecoveryManager {
public:
    enum RecoveryAction {
        RETRY_OPERATION,
        DEGRADE_QUALITY,
        RESET_STATE,
        DISABLE_FEATURE
    };
    
    RecoveryAction handle_error(ErrorLevel level, ErrorContext context);
    void reset_error_counters();
    bool should_escalate_error(const Error& error);
};
```

## Performance Requirements

### Real-time Processing Targets

| Resolution | Target Time | Max Features | Performance Mode |
|------------|-------------|--------------|------------------|
| 720p       | <2ms        | 300          | QUALITY          |
| 1080p      | <4ms        | 250          | BALANCED         |
| 1440p      | <8ms        | 200          | BALANCED         |
| 4K          | <15ms       | 150          | PERFORMANCE      |

### Memory Usage Guidelines

- **Feature buffers**: Pre-allocated, reused across frames
- **Transform history**: Circular buffer, configurable size
- **OpenCV Mat objects**: Stack-allocated where possible
- **Maximum memory footprint**: <50MB per instance

## Platform Considerations

### Windows
- DirectShow filter integration
- Windows-specific error handling
- Visual Studio compiler optimizations

### macOS  
- AVFoundation integration considerations
- Metal performance shader potential
- Xcode build system integration

### Linux
- V4L2 compatibility
- GStreamer integration potential
- Distribution-specific packaging

## Security Considerations

### Input Validation
- Frame dimension bounds checking
- Parameter range validation
- Buffer overflow protection
- Integer overflow prevention

### Memory Safety
- RAII resource management
- Smart pointer usage
- Bounds checking for all array access
- Exception safety guarantees

## Extension Points

### Future Enhancements
- GPU acceleration (CUDA/OpenCL)
- Machine learning-based tracking
- Multi-threaded processing
- Advanced stabilization modes

### Plugin Architecture
- Configurable algorithms
- Custom preset systems
- Third-party integration APIs
- Analytics and telemetry framework

## Development Guidelines

### Code Organization
```
src/
├── core/               # Core stabilization engine
├── obs/                # OBS Studio integration
├── ui/                 # User interface components  
├── utils/              # Utility functions
└── platform/           # Platform-specific code
```

### Testing Strategy
- Unit tests for core algorithms
- Integration tests with OBS Studio
- Performance regression tests
- Memory leak validation
- Cross-platform compatibility tests

## Deployment Strategy (Issue #171)

### Overview

The OBS Stabilizer plugin addresses OpenCV dependency complexity through multiple deployment strategies, eliminating end-user installation complexity while maintaining full functionality.

### Deployment Options

#### 1. Static Linking (Recommended)
- **Description**: Statically links OpenCV libraries into the plugin binary
- **Size Impact**: +15-20MB (from 72KB to ~20MB)
- **Dependencies**: Zero external dependencies
- **Installation**: Single file deployment

#### 2. Bundled Distribution
- **Description**: Packages OpenCV dynamic libraries with the plugin
- **Size Impact**: +8MB total package size
- **Dependencies**: Self-contained relative loading
- **Installation**: Multi-file but self-contained

#### 3. Hybrid Approach
- **Description**: Core components static, advanced features optional
- **Size Impact**: +12MB base, optional downloads for advanced features
- **Dependencies**: Core static, optional external components
- **Installation**: Progressive enhancement model

#### 4. System Dependencies (Current)
- **Description**: Uses system-installed OpenCV libraries
- **Size Impact**: Minimal (72KB plugin)
- **Dependencies**: External OpenCV installation required
- **Installation**: Complex user setup process

### Build Configuration

```cmake
# Deployment strategy selection
option(OPENCV_DEPLOYMENT_STRATEGY "OpenCV deployment strategy" 
       "Static" CACHE STRING "Static|Bundled|Hybrid|System")

set_property(CACHE OPENCV_DEPLOYMENT_STRATEGY 
             PROPERTY STRINGS "Static;Bundled;Hybrid;System")
```

### Platform-Specific Implementation

#### macOS
- **Static**: Universal binary support (Intel + Apple Silicon)
- **Bundled**: Framework-based distribution with relative RPATH
- **Installation**: Drag-and-drop to OBS plugins directory

#### Windows  
- **Static**: Visual C++ runtime bundling
- **Bundled**: DLL bundling with relative PATH
- **Installation**: NSIS installer with OBS detection

#### Linux
- **Static**: glibc compatibility across distributions
- **Bundled**: AppImage/Snap/Flatpak distribution
- **Installation**: Package manager integration

### CI/CD Integration

```yaml
# Multi-strategy builds
jobs:
  build-static:
    strategy:
      matrix:
        platform: [ubuntu, windows, macos]
    steps:
      - name: Build Static Version
        run: |
          cmake -DOPENCV_DEPLOYMENT_STRATEGY=Static ..
          make -j4
```

### Migration Path

1. **Phase 1**: Implement static linking as primary deployment
2. **Phase 2**: Maintain bundled distribution for advanced features
3. **Phase 3**: Phase out system dependencies over 2 release cycles
4. **Phase 4**: Optimize binary size through selective module inclusion

### Success Metrics

- **Installation Success Rate**: Target >95% (from ~60% currently)
- **Support Ticket Reduction**: 50% decrease in dependency issues
- **User Satisfaction**: >4.5/5 rating for installation experience
- **Adoption Rate**: 20% increase in new users

This architecture provides a solid foundation for Phase 3 UI implementation while maintaining the production-ready core established in Phase 2, now enhanced with simplified deployment resolving Issue #171.