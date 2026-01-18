# OBS Stabilizer Plugin - Deployment Strategy (Issue #171)

## Executive Summary

This document provides a comprehensive deployment strategy to resolve Issue #171: "OpenCV dependency creates end-user installation complexity." The strategy analyzes multiple approaches with detailed pros/cons, implementation complexity, and user experience impact.

## Current State Analysis

### Current Dependencies
- **Plugin Size**: 72KB (minimal core)
- **OpenCV Dependencies**: 7 libraries totaling ~15MB dynamic libraries
  - libopencv_core.412.dylib (3.0MB)
  - libopencv_imgproc.412.dylib (3.5MB) 
  - libopencv_video.412.dylib (540KB)
  - libopencv_features2d.412.dylib (706KB)
  - libopencv_calib3d.412.dylib (2.1MB)
  - libopencv_flann.412.dylib (481KB)
  - libopencv_dnn.412.dylib (6.0MB)

### Current Installation Complexity
- Users must install OpenCV separately via Homebrew (macOS), package managers (Linux), or manual download (Windows)
- Version compatibility issues between OpenCV builds
- Path configuration challenges for dynamic library loading
- Platform-specific installation procedures

## Deployment Strategy Options

### Option 1: Static Linking (Primary Recommendation)

#### Technical Analysis
- **Feasibility**: High - OpenCV supports static builds with `-DBUILD_SHARED_LIBS=OFF`
- **Plugin Size Impact**: +15-20MB (includes all OpenCV components)
- **License Compatibility**: Apache 2.0 is permissive for static linking in commercial products
- **Implementation Complexity**: Medium

#### Pros
- **Zero Dependencies**: Single binary deployment
- **Version Consistency**: No OpenCV version conflicts
- **Simplified Installation**: Drag-and-drop installation
- **Cross-Platform**: Same approach works on all platforms
- **Reliability**: No external dependency failures

#### Cons
- **Larger Distribution**: ~20MB plugin instead of 72KB
- **Update Complexity**: OpenCV updates require full plugin rebuild
- **Memory Usage**: Higher memory footprint (no sharing between processes)

#### Implementation Plan
```cmake
# Static linking configuration
option(BUILD_STATIC_OPENCV "Build with static OpenCV linking" ON)

if(BUILD_STATIC_OPENCV)
    find_package(OpenCV REQUIRED COMPONENTS 
        core imgproc video features2d calib3d flann
        NO_MODULE NO_DEFAULT_PATH
        PATHS ${CMAKE_SOURCE_DIR}/opencv-static/lib/cmake/opencv4
    )
    
    # Force static linking
    set(OpenCV_LIBS 
        ${CMAKE_SOURCE_DIR}/opencv-static/lib/libopencv_core.a
        ${CMAKE_SOURCE_DIR}/opencv-static/lib/libopencv_imgproc.a
        # ... other static libraries
    )
endif()
```

#### Platform-Specific Considerations
- **macOS**: Code signing for larger binaries, notarization requirements
- **Windows**: Static library compatibility, MSVC vs MinGW considerations
- **Linux**: glibc version compatibility across distributions

### Option 2: Bundled Distribution (Secondary Recommendation)

#### Technical Analysis
- **Directory Structure**: Package OpenCV dynamic libraries with plugin
- **Runtime Loading**: Dynamic loading from relative paths
- **Cross-Platform**: Compatible with existing OBS plugin architecture

#### Pros
- **Moderate Size**: Smaller than static linking (~8MB)
- **Memory Efficiency**: Shared libraries between processes
- **Modular Updates**: Can update OpenCV independently

#### Cons
- **Path Management**: Complex runtime library resolution
- **Security**: Multiple binary files to sign and verify
- **Installation**: Still multi-file deployment

#### Implementation Architecture
```
obs-stabilizer-opencv.so
└── opencv-libraries/
    ├── libopencv_core.412.dylib
    ├── libopencv_imgproc.412.dylib
    └── ...
```

#### CMake Configuration
```cmake
# Bundled distribution setup
if(BUILD_BUNDLED_OPENCV)
    install(FILES ${OPENCV_LIBS}
        DESTINATION obs-plugins/opencv-libraries
        COMPONENT Runtime
    )
    
    # Set RPATH for relative loading
    set_target_properties(obs-stabilizer-opencv PROPERTIES
        INSTALL_RPATH "$ORIGIN/opencv-libraries"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()
```

### Option 3: Hybrid Approach (Adaptive Strategy)

#### Strategy Overview
- **Static Core**: Core OpenCV components statically linked
- **Optional Extensions**: Heavy components (DNN, G-API) as optional downloads
- **Fallback Mode**: Graceful degradation when optional components missing

#### Implementation Phases
1. **Phase 1**: Static link core components (core, imgproc, features2d)
2. **Phase 2**: Optional DNN module for advanced features
3. **Phase 3**: Advanced G-API for GPU acceleration

#### Pros
- **Balanced Approach**: Manageable size with full functionality
- **Progressive Enhancement**: Basic features always available
- **User Choice**: Advanced users can opt-in to larger downloads

#### Cons
- **Complex Build System**: Multiple build configurations
- **Feature Fragmentation**: Different feature sets across users

## Build System Modifications

### New CMake Configuration
```cmake
# Deployment strategy selection
option(OPENCV_DEPLOYMENT_STRATEGY "OpenCV deployment strategy" 
       "static" CACHE STRING "Static|Bundled|Hybrid|System")

set_property(CACHE OPENCV_DEPLOYMENT_STRATEGY 
             PROPERTY STRINGS "Static;Bundled;Hybrid;System")

# Strategy-specific configurations
if(OPENCV_DEPLOYMENT_STRATEGY STREQUAL "Static")
    include(cmake/StaticOpenCV.cmake)
elseif(OPENCV_DEPLOYMENT_STRATEGY STREQUAL "Bundled")
    include(cmake/BundledOpenCV.cmake)
elseif(OPENCV_DEPLOYMENT_STRATEGY STREQUAL "Hybrid")
    include(cmake/HybridOpenCV.cmake)
else()
    # System (current approach)
    include(cmake/SystemOpenCV.cmake)
endif()
```

### OpenCV Build Integration
```cmake
# Custom OpenCV build for static linking
if(OPENCV_DEPLOYMENT_STRATEGY STREQUAL "Static" OR 
   OPENCV_DEPLOYMENT_STRATEGY STREQUAL "Hybrid")
    
    # Build OpenCV as static libraries
    include(ExternalProject)
    ExternalProject_Add(opencv-static
        URL ${OPENCV_SOURCE_URL}
        CMAKE_ARGS
            -DBUILD_SHARED_LIBS=OFF
            -DBUILD_opencv_dnn=OFF  # Disable heavy modules
            -DBUILD_EXAMPLES=OFF
            -DBUILD_TESTS=OFF
            -DBUILD_PERF_TESTS=OFF
            -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/opencv-static
    )
endif()
```

## CI/CD Pipeline Changes

### Multi-Strategy Builds
```yaml
# Updated GitHub Actions workflow
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
          
  build-bundled:
    strategy:
      matrix:
        platform: [ubuntu, windows, macos]
    steps:
      - name: Build Bundled Version
        run: |
          cmake -DOPENCV_DEPLOYMENT_STRATEGY=Bundled ..
          make -j4
```

### Artifact Management
```yaml
# Enhanced artifact uploads
- name: Upload Static Build
  uses: actions/upload-artifact@v3
  with:
    name: obs-stabilizer-${{ matrix.platform }}-static-${{ github.run_number }}
    path: |
      build/obs-stabilizer-opencv.*
      build/opencv-static/lib/
      
- name: Upload Bundled Build  
  uses: actions/upload-artifact@v3
  with:
    name: obs-stabilizer-${{ matrix.platform }}-bundled-${{ github.run_number }}
    path: |
      build/obs-stabilizer-opencv.*
      build/opencv-libraries/
```

## Installation Process Simplification

### Static Distribution Installation
```bash
# macOS
cp obs-stabilizer-opencv.plugin ~/Library/Application\ Support/obs-studio/plugins/

# Windows  
xcopy obs-stabilizer-opencv.dll "%PROGRAMFILES%\obs-studio\obs-plugins\64bit\"

# Linux
cp obs-stabilizer-opencv.so ~/.config/obs-studio/plugins/
```

### Installation Script (Cross-Platform)
```python
#!/usr/bin/env python3
"""
OBS Stabilizer Plugin Installer
Automatically detects OBS installation and deploys plugin
"""

import os
import platform
import shutil

def install_plugin():
    system = platform.system()
    plugin_paths = {
        'Darwin': '/Library/Application Support/obs-studio/plugins/',
        'Windows': os.path.expandvars('%PROGRAMFILES%\\obs-studio\\obs-plugins\\64bit\\'),
        'Linux': '~/.config/obs-studio/plugins/'
    }
    
    # Detect installation and copy files
    # Handle permissions and existing installations
    # Validate installation success
```

### Fallback Mechanisms
```cpp
// Graceful degradation when components missing
class StabilizerCore {
public:
    bool initialize() {
        if (!load_opencv()) {
            status_ = DEGRADED_MODE;
            return enable_basic_stabilization();
        }
        return enable_full_stabilization();
    }
    
private:
    bool load_opencv() {
        // Try static first, then bundled, then system
        return load_static_opencv() || 
               load_bundled_opencv() || 
               load_system_opencv();
    }
};
```

## Platform-Specific Strategies

### macOS Deployment
- **Framework Integration**: Bundle OpenCV as embedded framework
- **Code Signing**: Sign all components for Gatekeeper compatibility
- **Notarization**: Submit to Apple for distribution
- **Universal Binary**: Support Intel and Apple Silicon

#### macOS Implementation
```bash
# Create universal static library
lipo -create opencv-arm64.a opencv-x86_64.a -output opencv-universal.a

# Bundle in plugin framework
mkdir -p obs-stabilizer-opencv.plugin/Contents/Frameworks
cp opencv-universal.a obs-stabilizer-opencv.plugin/Contents/Frameworks/
```

### Windows Deployment
- **Installer Package**: NSIS installer with dependency checking
- **Visual C++ Runtime**: Bundle required redistributables
- **Side-by-Side Assembly**: Handle Windows DLL conflicts
- **Windows Store**: Consider Windows Store distribution

#### Windows NSIS Script
```nsis
!define PLUGIN_NAME "OBS Stabilizer"
!define VERSION "1.0.0"

Section "OBS Stabilizer" SecOBS
    SetOutPath "$INSTDIR"
    File "obs-stabilizer-opencv.dll"
    
    # Detect OBS installation
    ReadRegStr $0 HKLM "Software\OBS Studio" "InstallPath"
    IfErrors NoOBS
    
    CopyFiles "$INSTDIR\obs-stabilizer-opencv.dll" "$0\obs-plugins\64bit\"
SectionEnd
```

### Linux Deployment
- **AppImage**: Self-contained application format
- **Snap Distribution**: Ubuntu Store deployment
- **Package Managers**: Native deb/rpm packages
- **Flatpak**: Universal Linux application format

#### AppImage Implementation
```bash
# Create AppImage with bundled dependencies
mkdir -p obs-stabilizer.AppDir/usr/lib
cp obs-stabilizer-opencv.so obs-stabilizer.AppDir/usr/bin/
cp opencv-*.so.* obs-stabilizer.AppDir/usr/lib/

# Create AppRun script
cat > obs-stabilizer.AppDir/AppRun << 'EOF'
#!/bin/bash
HERE="$(dirname "$(readlink -f "${0}")")"
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH}"
exec "${HERE}/usr/bin/obs-stabilizer-opencv" "$@"
EOF
```

## Recommendation Matrix

| Strategy | Implementation Complexity | User Experience | Maintenance | Size Impact | Overall Score |
|----------|--------------------------|----------------|-------------|-------------|---------------|
| Static Linking | Medium | Excellent | Low | High (20MB) | **9/10** |
| Bundled Distribution | High | Good | Medium | Medium (8MB) | 7/10 |
| Hybrid Approach | Very High | Good | High | Medium (12MB) | 6/10 |
| System Dependencies | Low | Poor | Low | Minimal (72KB) | 4/10 |

## Recommended Implementation Plan

### Phase 1: Foundation (Weeks 1-2)
1. **Static OpenCV Build Setup**
   - Configure OpenCV static build pipeline
   - Test basic static linking functionality
   - Validate license compliance

2. **Build System Integration**
   - Add CMake configuration options
   - Update CI/CD pipeline for static builds
   - Create testing framework for different strategies

### Phase 2: Implementation (Weeks 3-4)
1. **Static Distribution Implementation**
   - Complete static linking configuration
   - Implement cross-platform build scripts
   - Add automated testing for static builds

2. **Installer Development**
   - Create cross-platform installation scripts
   - Implement fallback mechanisms
   - Add dependency validation

### Phase 3: Validation (Weeks 5-6)
1. **Comprehensive Testing**
   - Cross-platform compatibility testing
   - Performance benchmarking
   - Memory usage validation

2. **Documentation & Release**
   - Update installation documentation
   - Create user migration guide
   - Prepare release packages

### Phase 4: Optimization (Weeks 7-8)
1. **Size Optimization**
   - Analyze and optimize binary size
   - Implement compression techniques
   - Evaluate selective module inclusion

2. **Distribution Strategy**
   - Implement automated release pipeline
   - Add update mechanisms
   - Monitor deployment metrics

## Success Metrics

### Technical Metrics
- **Installation Success Rate**: Target >95%
- **Zero-Dependency Success**: 100% for static builds
- **Cross-Platform Compatibility**: 100% across supported platforms
- **Binary Size**: <25MB for static builds

### User Experience Metrics  
- **Installation Time**: <2 minutes
- **Support Tickets**: 50% reduction in dependency-related issues
- **User Satisfaction**: >4.5/5 rating
- **Adoption Rate**: 20% increase in new users

### Development Metrics
- **Build Time**: <10 minutes for all variants
- **CI/CD Pipeline**: 100% automated testing
- **Release Frequency**: Weekly builds possible
- **Documentation Coverage**: 100% for new deployment options

## Risk Mitigation

### Technical Risks
- **Binary Size Bloat**: Implement size monitoring and optimization
- **Cross-Platform Issues**: Maintain comprehensive test matrix
- **Performance Impact**: Benchmark and profile regularly
- **Memory Usage**: Monitor and optimize memory consumption

### Legal Risks
- **License Compliance**: Regular audit of third-party licenses
- **Patent Issues**: Monitor OpenCV patent landscape
- **Distribution Rights**: Validate distribution permissions

### Maintenance Risks
- **OpenCV Updates**: Implement automated update testing
- **Build Complexity**: Maintain clear documentation
- **Testing Overhead**: Automate as much testing as possible

## Conclusion

Static linking represents the optimal balance of user experience, maintainability, and reliability for resolving Issue #171. While it increases the binary size, it eliminates the primary pain point of complex dependency management while maintaining full functionality.

The phased implementation approach ensures minimal disruption while delivering significant user experience improvements. The recommended strategy positions the OBS Stabilizer plugin for increased adoption and reduced support overhead.

**Next Steps**: Proceed with Phase 1 implementation, starting with OpenCV static build setup and build system integration.