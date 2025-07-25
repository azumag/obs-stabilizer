# OBS Stabilizer Plugin

**🔒 SECURE & STABLE - PRODUCTION-READY CORE**

A real-time video stabilization plugin for OBS Studio using OpenCV computer vision algorithms.

## ✅ **Current Status: Secure Implementation Complete**

**Phase 2 achievements - Hardened stabilization pipeline:**
- ✅ OBS filter registration and plugin structure
- ✅ OpenCV integration with Point Feature Matching
- ✅ Lucas-Kanade optical flow tracking
- ✅ Real-time frame transformation (NV12/I420 formats)
- ✅ Transform smoothing algorithm with jitter reduction
- ✅ Comprehensive test framework with performance verification
- ✅ Memory stability testing for extended operation
- 🔒 **Security hardened - Buffer overflow vulnerabilities fixed**
- 🔒 **Input validation - Comprehensive bounds checking implemented**
- 🔒 **Memory safety - Pre-allocated buffers and RAII patterns**

**SECURE STABILIZATION - PRODUCTION-READY**

## Overview

OBS Stabilizer provides real-time video stabilization for livestreams and recordings in OBS Studio. The implementation uses Point Feature Matching algorithms to reduce camera shake and improve video quality with minimal performance impact.

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
- **Security**: 🔒 Buffer overflow protection, input validation, bounds checking
- **Dependencies**: OpenCV 4.5+, Qt6, OBS Studio 30.0+
- **Language**: C++17/20 with modern safety patterns
- **Build System**: CMake 3.28+ with full conditional compilation
- **Testing**: Google Test framework with performance & security validation
- **License**: GPL-2.0 (OBS Studio compatible)

## Quick Start

### Prerequisites

- OBS Studio 30.0 or higher
- CMake 3.28+ 
- Qt6 development libraries
- C++17 compatible compiler
- OpenCV 4.5+ development libraries (optional, enables stabilization features)

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
# Run performance tests
./run-perftest.sh

# Run unit tests
./run-tests.sh
```

### Installation

**SECURE STABILIZATION** - Production-Ready Core

1. Copy the built plugin to your OBS plugins directory
2. Restart OBS Studio  
3. Add "Stabilizer" filter to your video source
4. Configure stabilization parameters:
   - **Smoothing Radius**: Transform smoothing window (10-100 frames)
   - **Feature Points**: Number of tracking points (100-1000)

**Current Status**: Secure, hardened stabilization pipeline with comprehensive input validation, buffer overflow protection, and real-time frame transformation.

## Configuration Options

**Currently Available (Phase 2):**
- **Enable Stabilization**: ✅ Toggle stabilization processing on/off
- **Smoothing Radius**: ✅ Transform smoothing window (10-100 frames)
- **Feature Points**: ✅ Number of tracking points (100-1000)

**Next Phase (Phase 3):**
- **Stabilization Strength**: Adjustable correction intensity
- **Crop Mode**: Handle borders with cropping or padding
- **Advanced UI**: Enhanced properties panel with presets

## Performance Verification (Phase 2 Complete)

**Verified Performance Targets:**

| Resolution | Target Processing Time | Real-time Capability |
|------------|----------------------|---------------------|
| 720p       | <2ms/frame          | ✅ 60fps+ capable   |
| 1080p      | <4ms/frame          | ✅ 30fps+ capable   |
| 1440p      | <8ms/frame          | ✅ Tested & verified |
| 4K         | <15ms/frame         | ✅ Performance tested |

**Test Suite Features:**
- Comprehensive performance benchmarking across resolutions
- Memory stability testing for extended operation (no leaks detected)
- Real-time processing verification for streaming use cases
- Security validation testing (buffer overflow protection verified)
- Automated test framework with detailed metrics

*Run `./run-perftest.sh` to verify performance on your hardware.*

## Development Status

### Phase 2 Complete ✅ - Core Stabilization Implementation
- [x] OBS plugin template setup (#1) ✅  
- [x] Build system with OpenCV integration (#2) ✅
- [x] Real-time frame transformation (#3) ✅
- [x] Point feature matching with Lucas-Kanade optical flow (#4) ✅
- [x] Transform smoothing algorithm (#5) ✅
- [x] Comprehensive test framework (#10) ✅
- [x] Performance verification prototype (#17) ✅

### Phase 3 Starting - UI/UX and Integration
- [ ] Enhanced settings panel (#6)
- [ ] Advanced stabilization controls
- [ ] Cross-platform optimization
- [ ] Production deployment features

See [CLAUDE.md](CLAUDE.md) for detailed technical specifications and complete development roadmap.

## Contributing

We welcome contributions! Please see our [development documentation](CLAUDE.md) for:
- Technical architecture details
- Build system requirements  
- Code review process
- Issue management workflow

## Acknowledgments

This project was inspired by the [LiveVisionKit](https://github.com/Crowsinc/LiveVisionKit) plugin, which is no longer actively maintained. OBS Stabilizer aims to provide a modern, maintainable alternative with improved performance and user experience.

## Support

- **Documentation**: [CLAUDE.md](CLAUDE.md)
- **License**: [GPL-2.0](LICENSE)
