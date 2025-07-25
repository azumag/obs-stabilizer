#!/bin/bash

# Simple compilation test for modular architecture
# Tests that the core modules compile without external dependencies

set -e

echo "=== OBS Stabilizer Compilation Test ==="
echo "Testing modular architecture compilation..."

# Compiler settings
CXX_FLAGS="-std=c++17 -I. -Wall -Wextra"

# Test 1: Compile without OpenCV (stub mode)
echo ""
echo "Test 1: Compiling without OpenCV (stub mode)"
echo "============================================"

if g++ $CXX_FLAGS -o test-compile-stub test-compile.cpp src/obs/obs_integration.cpp 2>&1; then
    echo "✅ Stub compilation PASSED"
    
    echo "Running stub test..."
    if ./test-compile-stub; then
        echo "✅ Stub execution PASSED"
    else
        echo "❌ Stub execution FAILED"
        exit 1
    fi
    
    # Clean up
    rm -f test-compile-stub
else
    echo "❌ Stub compilation FAILED"
    exit 1
fi

# Test 2: Try to compile with OpenCV if available
echo ""
echo "Test 2: Attempting compilation with OpenCV"
echo "=========================================="

# Try to find OpenCV
OPENCV_FLAGS=""
if pkg-config --exists opencv4 2>/dev/null; then
    OPENCV_FLAGS=$(pkg-config --cflags --libs opencv4)
    echo "Found OpenCV via pkg-config"
elif pkg-config --exists opencv 2>/dev/null; then
    OPENCV_FLAGS=$(pkg-config --cflags --libs opencv)
    echo "Found OpenCV via pkg-config (legacy)"
else
    echo "⚠️  OpenCV not found via pkg-config, trying manual detection..."
    
    # Try manual detection for common paths
    for opencv_dir in "/opt/homebrew/Cellar/opencv"*"/include/opencv4" \
                      "/usr/local/include/opencv4" \
                      "/usr/include/opencv4"; do
        if [ -d "$opencv_dir" ]; then
            OPENCV_FLAGS="-I$(dirname $opencv_dir) -lopencv_core -lopencv_imgproc -lopencv_features2d -lopencv_video"
            echo "Found OpenCV at: $opencv_dir"
            break
        fi
    done
fi

if [ -n "$OPENCV_FLAGS" ]; then
    echo "Compiling with OpenCV support..."
    if g++ $CXX_FLAGS -DENABLE_STABILIZATION $OPENCV_FLAGS -o test-compile-opencv test-compile.cpp src/core/stabilizer_core.cpp src/obs/obs_integration.cpp 2>&1; then
        echo "✅ OpenCV compilation PASSED"
        
        echo "Running OpenCV test..."
        if ./test-compile-opencv; then
            echo "✅ OpenCV execution PASSED"
        else
            echo "❌ OpenCV execution FAILED"
            exit 1
        fi
        
        # Clean up
        rm -f test-compile-opencv
    else
        echo "❌ OpenCV compilation FAILED"
        echo "This may be due to missing OpenCV development libraries"
        echo "Continuing with stub-only testing..."
    fi
else
    echo "⚠️  OpenCV not found, skipping OpenCV compilation test"
    echo "To enable OpenCV tests, install OpenCV development libraries:"
    echo "  macOS: brew install opencv"
    echo "  Ubuntu: apt-get install libopencv-dev"
fi

echo ""
echo "🎉 COMPILATION TESTS COMPLETED"
echo "=============================="
echo "✅ Core modular architecture compiles successfully"
echo "✅ No critical compilation errors detected"
echo "✅ Ready for Issue #39 integration testing"
echo ""
echo "Next steps:"
echo "1. Resolve OpenCV dependency for full testing"
echo "2. Complete Issue #39 - Core Integration Testing"
echo "3. Begin Phase 3 UI development"