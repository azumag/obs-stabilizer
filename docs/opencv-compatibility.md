# OpenCV Version Compatibility Matrix

## Supported Versions

### ✅ Fully Supported (Tested & Recommended)
- **OpenCV 4.5.x** - Minimum required version
- **OpenCV 4.6.x** - Fully tested and stable
- **OpenCV 4.7.x** - Fully tested and stable
- **OpenCV 4.8.x** - Latest stable, fully supported

### ⚠️ Experimental Support (Limited Testing)
- **OpenCV 5.0.x** - May work but not extensively tested
- **OpenCV 5.1.x** - API changes possible, use with caution

### ❌ Not Supported
- **OpenCV 3.x** - Too old, missing required APIs
- **OpenCV 4.0-4.4** - Missing critical stability improvements
- **OpenCV 6.x+** - Future versions, compatibility unknown

## API Dependencies

### Required OpenCV Modules
1. **core** - Basic matrix operations and data structures
2. **imgproc** - Image processing functions (warpAffine, etc.)
3. **video** - Video processing and optical flow
4. **features2d** - Feature detection and matching

### Critical APIs Used
- `cv::goodFeaturesToTrack()` - Corner detection
- `cv::calcOpticalFlowPyrLK()` - Lucas-Kanade optical flow
- `cv::getAffineTransform()` - Transform matrix calculation
- `cv::warpAffine()` - Image transformation
- `cv::Mat` - Image container class

## Installation Instructions

### macOS (Homebrew)
```bash
# Install latest stable OpenCV 4.x
brew install opencv

# Verify installation
pkg-config --modversion opencv4
```

### Ubuntu/Debian
```bash
# Install OpenCV development packages
sudo apt-get update
sudo apt-get install libopencv-dev libopencv-contrib-dev

# Verify installation
pkg-config --modversion opencv4
```

### Building from Source
```bash
# For maximum compatibility, build OpenCV 4.8.x from source
git clone https://github.com/opencv/opencv.git
cd opencv
git checkout 4.8.1
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DBUILD_EXAMPLES=OFF \
      -DBUILD_TESTS=OFF \
      -DBUILD_DOCS=OFF \
      ..
make -j$(nproc)
sudo make install
```

## Compatibility Testing

### Automated Testing
Run the compatibility test script:
```bash
./scripts/test-opencv-compatibility.sh
```

### Manual Testing
1. **CMake Configuration Test:**
   ```bash
   mkdir build && cd build
   cmake ..
   # Look for OpenCV version messages
   ```

2. **Compile Test:**
   ```bash
   make
   # Check for compilation errors
   ```

3. **Runtime Test:**
   ```bash
   # Test with actual video processing
   # Verify stabilization works correctly
   ```

## Version-Specific Notes

### OpenCV 4.5.x
- **Status:** ✅ Minimum supported version
- **Notes:** All required APIs available and stable
- **Recommendation:** Safe baseline version

### OpenCV 4.6-4.8.x
- **Status:** ✅ Recommended
- **Notes:** Performance improvements and bug fixes
- **Recommendation:** Best choice for production use

### OpenCV 5.x
- **Status:** ⚠️ Experimental
- **Notes:** 
  - Some API changes in headers
  - Backward compatibility maintained for core APIs
  - Extended testing needed
- **Recommendation:** Wait for broader adoption

## Troubleshooting

### Common Issues

#### "OpenCV not found"
```
Solution: Install OpenCV development packages
- macOS: brew install opencv
- Ubuntu: apt-get install libopencv-dev
- Check CMAKE_PREFIX_PATH includes OpenCV installation
```

#### "OpenCV version too old"
```
Error: OpenCV 4.3.0 is too old. Minimum required: 4.5.0
Solution: Upgrade to OpenCV 4.5+
```

#### "Component not found" 
```
Error: Required OpenCV component 'features2d' not found
Solution: Install full OpenCV package with all modules
- Use opencv-contrib-dev on Ubuntu
- Ensure complete Homebrew installation on macOS
```

#### Build warnings on OpenCV 5.x
```
Warning: OpenCV 5.x detected - some APIs may have changed
Solution: This is expected - functionality should still work
Monitor for any runtime issues
```

### Debug Information
Enable detailed OpenCV information during CMake configuration:
```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DOPENCV_DEBUG=ON ..
```

## Contributing

### Testing New OpenCV Versions
1. Run compatibility test script
2. Test core stabilization functionality
3. Check for API deprecation warnings
4. Update compatibility matrix
5. Submit pull request with test results

### Reporting Issues
When reporting OpenCV compatibility issues, include:
- OpenCV version (`opencv_version` command output)
- System information (OS, architecture) 
- CMake configuration output
- Build error messages
- Runtime behavior description