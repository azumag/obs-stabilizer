# OBS Stabilizer Plugin Project Overview

## Purpose
Real-time video stabilization plugin for OBS Studio using OpenCV computer vision algorithms. Provides stabilization for livestreams and recordings with minimal performance impact.

## Tech Stack
- **Language**: C++17/20 (Modern C++)
- **Video Processing**: OpenCV 4.5+ (core, imgproc, video, features2d components)
- **Build System**: CMake 3.16+
- **GUI**: Qt6 (currently disabled due to version conflicts, using OBS internal Qt)
- **Platform**: Windows, macOS, Linux
- **Testing**: Google Test framework
- **License**: GPL-2.0

## Architecture
- Modular design with separated concerns:
  - `plugin_main.cpp`: OBS plugin entry point
  - `obs_plugin.cpp`: OBS filter implementation
  - `plugin-support.c`: Symbol bridge for OBS API compatibility
  - Point Feature Matching algorithm with Lucas-Kanade optical flow
  - Transform smoothing with configurable window size

## Current Status
- Plugin loading issues resolved (July 30, 2025)
- OpenCV 4.12.0 integration working
- Qt-independent minimal plugin to avoid version conflicts
- Symbol bridge implementation for OBS API differences
- Production-ready core functionality

## Build Configuration
- Simplified standalone build system (not building entire OBS)
- Automatic OBS framework detection on macOS
- Direct library linking: `/Applications/OBS.app/Contents/Frameworks/libobs.framework/Versions/A/libobs`
- Module library type for proper macOS plugin bundle format