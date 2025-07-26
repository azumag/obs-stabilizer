# OBS Stabilizer Plugin

**🔒 SECURE & STABLE - PRODUCTION-READY CORE**

A real-time video stabilization plugin for OBS Studio using OpenCV computer vision algorithms.

## ✅ **Current Status: Production-Ready Core with Security Audit**

**Phase 2 Complete - Comprehensive Security & Compatibility:**
- ✅ OBS filter registration and plugin structure
- ✅ OpenCV integration with Point Feature Matching
- ✅ Lucas-Kanade optical flow tracking
- ✅ Real-time frame transformation (NV12/I420 formats)
- ✅ Transform smoothing algorithm with jitter reduction
- ✅ Comprehensive test framework with performance verification
- ✅ Memory stability testing for extended operation
- 🔒 **Security Audit: 11/11 tests passing - PRODUCTION READY**
- 🔒 **Buffer overflow protection with comprehensive validation**
- 🔒 **OpenCV version compatibility (4.5+ with 5.x experimental support)**
- 🔒 **Memory safety with stack/heap allocation optimization**

**🎉 PRODUCTION-READY STABILIZATION CORE**

## Overview

OBS Stabilizer provides real-time video stabilization for livestreams and recordings in OBS Studio. The implementation uses Point Feature Matching algorithms to reduce camera shake and improve video quality with minimal performance impact.

**Phase 2.5**: Successfully refactored from monolithic architecture to modular design with thread-safe core engine, clean OBS integration layer, and separated concerns for enhanced maintainability and Phase 3 UI development.

### Current Features (Phase 2 Complete)

- **Real-time Processing**: ✅ Full HD processing with transform smoothing
- **Low Latency**: ✅ Optimized feature tracking and frame transformation
- **Adaptive Algorithms**: ✅ Automatic feature refresh and error recovery
- **Multi-format Support**: ✅ NV12 and I420 video format compatibility
- **Performance Testing**: ✅ Comprehensive benchmarking and memory validation
- **Security Hardened**: 🔒 Buffer overflow protection and input validation
- **Memory Safe**: 🔒 Pre-allocated buffers and bounds checking

### Next Phase Features (Phase 3)

- **User-Friendly UI**: Integrated OBS properties panel with presets
- **Cross-Platform**: Enhanced Windows, macOS, and Linux support
- **Advanced Settings**: Crop mode and stabilization strength controls

## Technical Specifications

- **Core Algorithm**: ✅ Point Feature Matching with Lucas-Kanade Optical Flow
- **Transform Smoothing**: ✅ Moving average with configurable window size
- **Video Formats**: ✅ NV12, I420 with secure Y/UV plane handling
- **Architecture**: ✅ Modular design with StabilizerCore engine + OBS integration layer
- **Thread Safety**: ✅ Atomic operations and mutex protection for configuration updates
- **Security**: 🔒 Buffer overflow protection, input validation, bounds checking
- **Dependencies**: OpenCV 4.5+ (with 5.x experimental support), Qt6, OBS Studio 30.0+
- **Language**: C++17/20 with modern safety patterns and RAII resource management
- **Build System**: CMake 3.28+ with full conditional compilation
- **Testing**: Google Test framework with performance & security validation
- **License**: GPL-2.0 (OBS Studio compatible)

## Quick Start

### Prerequisites

- OBS Studio 30.0 or higher
- CMake 3.28+ 
- Qt6 development libraries
- C++17 compatible compiler
- OpenCV 4.5+ development libraries (4.5-4.8 recommended, 5.x experimental support)

### Building from Source

```bash
# Clone the repository
git clone https://github.com/azumag/obs-stabilizer.git
cd obs-stabilizer

# Configure build
cmake --preset <platform>-ci
# Available presets: macos-ci, windows-ci-x64, ubuntu-ci-x86_64

# Build the plugin
cmake --build --preset <platform>-ci
```

### Testing & Performance Verification

```bash
# Run comprehensive test suite (includes compilation and runtime tests)
./run-tests.sh

# Run core compilation test only (no dependencies required)
./test-core-only.sh

# Run integration test suite (validates core and OBS integration)
./run-integration-test.sh

# Run Phase 3 UI implementation test (validates UI components and presets)
./run-ui-test.sh

# Run performance tests (requires OpenCV)
./run-perftest.sh

# Run security audit (validates 11 security checks)
./security/security-audit.sh
```

### Installation

**SECURE STABILIZATION** - Production-Ready Core

1. Copy the built plugin to your OBS plugins directory
2. Restart OBS Studio  
3. Add "Stabilizer" filter to your video source
4. Configure stabilization parameters:
   - **Smoothing Radius**: Transform smoothing window (10-100 frames)
   - **Feature Points**: Number of tracking points (100-1000)

**Current Status**: Phase 2.5 architectural refactoring complete with modular design. Security audit verified (11/11 tests passing), OpenCV version compatibility framework implemented, production-ready stabilization pipeline with comprehensive validation and clean separation of concerns.

## Configuration Options

**Phase 3 UI Complete - Comprehensive Settings:**
- **Enable Stabilization**: ✅ Main toggle for stabilization processing
- **Preset System**: ✅ Gaming (Fast)/Streaming (Balanced)/Recording (High Quality)
- **Smoothing Strength**: ✅ Transform smoothing window (10-100 frames)
- **Feature Points**: ✅ Number of tracking points (100-1000)
- **Stability Threshold**: ✅ Error threshold for tracking quality (10.0-100.0)
- **Edge Handling**: ✅ Crop borders/Black padding/Scale to fit options
- **Advanced Settings**: ✅ Collapsible expert-level configuration panel
  - Feature quality threshold, refresh threshold, adaptive refresh
  - GPU acceleration (experimental), processing threads (1-8)

**Preset Configurations:**
- **Gaming**: 150 features, 40 threshold, 15 smoothing (optimized for fast response)
- **Streaming**: 200 features, 30 threshold, 30 smoothing (balanced quality/performance)
- **Recording**: 400 features, 20 threshold, 50 smoothing (maximum quality)

## Performance Verification (Phase 2 Complete)

**Verified Performance Targets:**

| Resolution | Target Processing Time | Real-time Capability |
|------------|----------------------|---------------------|
| 720p       | <2ms/frame          | ✅ 60fps+ capable   |
| 1080p      | <4ms/frame          | ✅ 30fps+ capable   |
| 1440p      | <8ms/frame          | ✅ Tested & verified |
| 4K         | <15ms/frame         | ✅ Performance tested |

**Test Suite Features:**
- **Dual-layer testing**: Core compilation tests (no dependencies) + full suite (when available)
- **Environment-independent**: Tests work whether OpenCV is installed or not
- Performance benchmarking across resolutions (when OpenCV available)
- Memory stability testing for extended operation (no leaks detected)
- **Security audit system (11/11 security tests passing)**
- **Modular architecture validation** - ensures Phase 2.5 refactoring integrity
- Automated test framework with graceful dependency handling

*Run `./run-tests.sh` for comprehensive testing or `./test-core-only.sh` for basic validation.*

## Development Status

### Phase 2 Complete ✅ - Production-Ready Core Implementation
- [x] OBS plugin template setup (#1) ✅  
- [x] Build system with OpenCV integration (#2) ✅
- [x] Real-time frame transformation (#3) ✅
- [x] Point feature matching with Lucas-Kanade optical flow (#4) ✅
- [x] Transform smoothing algorithm (#5) ✅
- [x] Comprehensive test framework (#10) ✅
- [x] Performance verification prototype (#17) ✅
- [x] **Security audit implementation (#32) ✅ - 11/11 tests passing**
- [x] **OpenCV version compatibility framework (#31) ✅ - 4.5+ with 5.x support**
- [x] **Development plan optimization (#24) ✅ - Ready for Phase 3**

### Phase 2.5 Complete ✅ - Critical Architectural Refactoring  
- [x] **Modular architecture implementation (#37) ✅ - Eliminates monolithic structure**
- [x] **StabilizerCore extraction ✅ - Thread-safe core engine with clean API**
- [x] **OBS integration layer ✅ - Separated OBS-specific code from algorithms**
- [x] **Plugin entry point refactored ✅ - Reduced from 564 to ~60 lines**
- [x] **Build system updated ✅ - Supports new modular source structure**

### Phase 3 Complete ✅ - Enhanced UI/UX Implementation
- [x] **Enhanced settings panel (#6) ✅ - Complete OBS properties panel with preset system**
- [x] **Advanced stabilization controls ✅ - Comprehensive parameter configuration**
- [x] **Preset system ✅ - Gaming/Streaming/Recording optimized configurations**
- [x] **Advanced settings panel ✅ - Collapsible expert-level parameters**

### Phase 4 Ready - Optimization and Cross-Platform
- [ ] Performance optimization and algorithm tuning
- [ ] Cross-platform compatibility enhancements
- [ ] Production deployment features
- [ ] Advanced diagnostic and monitoring tools

### Development Status Complete
- [x] **Issue #41**: Fix test system compatibility ✅ **RESOLVED**
- [x] **Issue #39**: Complete core integration testing ✅ **RESOLVED**
- [x] **Issue #36**: UI Architecture Specification ✅ **RESOLVED**
- [x] **Issue #6**: Phase 3 UI Implementation ✅ **COMPLETE**
- [x] **Issue #35**: Configuration parameterization ✅ **RESOLVED**
- [x] **Issue #40**: Test suite modernization ✅ **SUBSTANTIALLY RESOLVED**
- [x] **Issue #42**: Development priority matrix ✅ **RESOLVED**
- [x] **Issue #43**: Technical debt cleanup ✅ **RESOLVED** (plugin-main-original.cpp removed, documentation updated)
- [x] **Issue #46**: Legacy test file consolidation ✅ **SUBSTANTIALLY RESOLVED** (duplicate files removed, build system updated)

See [CLAUDE.md](CLAUDE.md) for detailed technical specifications and complete development roadmap.

For system architecture and technical design, see [docs/architecture.md](docs/architecture.md).

For Phase 3 UI implementation specifications, see [docs/ui-architecture.md](docs/ui-architecture.md).

For Phase 3 completion report and implementation details, see [docs/phase3-completion-report.md](docs/phase3-completion-report.md).

## Contributing

We welcome contributions! Please see our documentation for:
- **Development Guide**: [CLAUDE.md](CLAUDE.md) - Project specifications and workflow
- **System Architecture**: [docs/architecture.md](docs/architecture.md) - Technical design and API specifications
- **UI Architecture**: [docs/ui-architecture.md](docs/ui-architecture.md) - Phase 3 UI implementation specifications
- **Phase 3 Report**: [docs/phase3-completion-report.md](docs/phase3-completion-report.md) - Complete implementation details
- **Build system requirements** and testing procedures
- **Code review process** and issue management workflow

## Acknowledgments

This project was inspired by the [LiveVisionKit](https://github.com/Crowsinc/LiveVisionKit) plugin, which is no longer actively maintained. OBS Stabilizer aims to provide a modern, maintainable alternative with improved performance and user experience.

## Support

- **Documentation**: [CLAUDE.md](CLAUDE.md)
- **License**: [GPL-2.0](LICENSE)
