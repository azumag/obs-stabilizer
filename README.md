# OBS Stabilizer Plugin

**⚠️ EARLY DEVELOPMENT - NOT FUNCTIONAL YET**

A planned real-time video stabilization plugin for OBS Studio using OpenCV computer vision algorithms.

## ⚠️ **IMPORTANT: Current Limitations**

**This plugin is in early development and currently NON-FUNCTIONAL:**
- ❌ No video stabilization implemented
- ❌ No OBS filter registration 
- ❌ Build system has template processing issues
- ❌ Plugin loads but provides no user functionality
- ⚠️ Buffer overflow vulnerability in support files

**DO NOT USE IN PRODUCTION**

## Overview

OBS Stabilizer will provide real-time video stabilization for livestreams and recordings in OBS Studio. The planned implementation uses Point Feature Matching algorithms to reduce camera shake and improve video quality without significant performance impact.

### Planned Features

- **Real-time Processing**: Target 30+ fps performance on HD video
- **Low Latency**: Target <4ms processing time per frame on modern hardware  
- **Adaptive Algorithms**: Intelligent feature detection with fallback strategies
- **User-Friendly UI**: Integrated OBS properties panel with presets
- **Cross-Platform**: Windows, macOS, and Linux support

## Technical Specifications (Planned)

- **Core Algorithm**: Point Feature Matching with Lucas-Kanade Optical Flow
- **Dependencies**: OpenCV 4.5+ (optional), Qt6, OBS Studio 30.0+
- **Language**: C++17/20
- **Build System**: CMake 3.28+ with conditional compilation (incomplete)
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

**⚠️ Warning**: The plugin currently has build system issues and will not function even when built successfully. Template file processing is incomplete.

### Installation

**NOT RECOMMENDED** - Plugin is non-functional

~~1. Copy the built plugin to your OBS plugins directory~~
~~2. Restart OBS Studio~~
~~3. Add "Video Stabilizer" filter to your video source~~
~~4. Configure stabilization parameters as needed~~

**Current Status**: Plugin loads but provides no filter or functionality to OBS Studio.

## Planned Configuration Options

**NOTE: These options do not currently exist**

- **Enable Stabilization**: Toggle stabilization on/off
- **Smoothing Radius**: Frame averaging window (10-100 frames)
- **Feature Points**: Number of tracking points (100-1000)
- **Crop Mode**: Handle borders with cropping or padding

## Performance Targets (Planned)

| Resolution | Target Processing Time | Estimated CPU Usage |
|------------|----------------------|-------------------|
| 720p       | <2ms/frame          | <15%             |
| 1080p      | <4ms/frame          | <20%             |
| 4K         | <10ms/frame         | <35%             |

*Performance targets are preliminary and subject to validation through development.*

## Development Status

This project is in active development. See [CLAUDE.md](CLAUDE.md) for detailed technical specifications and development roadmap.

### Current Phase: Phase 1 - Foundation Setup ⚠️
- [x] OBS plugin template setup (#1) ✅  
- [x] Build system improvements (Partial) 🔄
- [ ] Plugin support files completion (#23) - **CRITICAL BLOCKER**
- [ ] Basic OBS filter registration - **MISSING**
- [ ] OpenCV integration (#2) - Pending blocker resolution
- [ ] Basic video filter implementation (#3) - Pending

See project issues for complete development tracking.

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
