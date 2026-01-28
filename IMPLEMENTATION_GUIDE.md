# OBS Stabilizer Implementation Guide

## Quick Start

### Building from Source

```bash
mkdir build && cd build
cmake -B build
cmake --build build
```

### Alternative Build Commands

```bash
# Or use make directly
cmake -B build
make -C build

# Or configure and build in current directory
cmake .
make
```

## Installation Instructions

### macOS

```bash
# Copy plugin to OBS plugins directory
cp build/obs-stabilizer-opencv.so ~/Library/Application\ Support/obs-studio/plugins/

# Fix plugin loading (required for macOS)
./scripts/fix-plugin-loading.sh

# Optional: Bundle OpenCV libraries
./scripts/bundle_opencv.sh
```

### Linux

```bash
# Copy plugin to OBS plugins directory
cp build/obs-stabilizer-opencv.so ~/.config/obs-studio/plugins/
```

### Windows

```bash
# Copy plugin to OBS plugins directory
copy build\\obs-stabilizer-opencv.dll "%APPDATA%\\obs-studio\\plugins\\"
```

## Build System Details

### CMake Configuration

The project uses a simple, straightforward CMake build system:
- Direct OpenCV integration via `find_package(OpenCV)`
- No complex deployment strategies or conditional compilation
- Cross-platform support (Windows, macOS, Linux)
- Standalone executable mode when OBS headers are not available

### Build Options

```bash
# Standard build (detects OBS automatically)
cmake -B build

# Custom OBS path (if needed)
cmake -DOBS_INCLUDE_PATH=/path/to/obs/include -B build

# Custom OpenCV path (if needed)
cmake -DOpenCV_DIR=/path/to/opencv/cmake -B build
```

## Testing

### Run Unit Tests

```bash
cd build
./stabilizer_tests
```

### Run Performance Benchmarks

```bash
# Quick performance validation
./scripts/quick-perf.sh

# Full benchmark suite
./scripts/run-perf-benchmark.sh

# Performance regression detection
./scripts/run-perf-regression.sh
```

### Security Audit

```bash
# Run security audit script
./security/security-audit.sh
```

## Architecture

The plugin follows a modular architecture:
- **StabilizerCore**: Core stabilization engine
- **OBSIntegration**: OBS Studio API integration layer
- **AdaptiveStabilizer**: Motion-aware parameter adjustment
- **MotionClassifier**: Motion type detection
- **FrameUtils**: Video format conversion utilities

For complete architecture details, see `docs/ARCHITECTURE.md`.