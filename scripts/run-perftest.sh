#!/bin/bash

# Performance Test Runner for OBS Stabilizer
# This script builds and runs the performance verification prototype

set -e

echo "=== OBS Stabilizer Performance Test ==="
echo "Building and running performance verification prototype..."

# Clean previous build
rm -rf tmp/builds/build-perftest

# Configure with CMAKE (try multiple possible cmake locations)
CMAKE_CMD=""
for cmake_path in "/usr/bin/cmake" "/usr/local/bin/cmake" "/opt/homebrew/bin/cmake" "cmake"; do
    if command -v "$cmake_path" &> /dev/null; then
        CMAKE_CMD="$cmake_path"
        echo "Found cmake at: $CMAKE_CMD"
        break
    fi
done

if [ -z "$CMAKE_CMD" ]; then
    echo "ERROR: cmake not found in PATH"
    echo "Please install cmake or add it to your PATH"
    exit 1
fi

# Build the performance test
echo "Configuring build..."
# Copy perftest CMakeLists.txt to build directory and configure from there
mkdir -p tmp/builds/build-perftest
cp src/CMakeLists-perftest.txt tmp/builds/build-perftest/CMakeLists.txt
cp src/performance-test.cpp tmp/builds/build-perftest/
cp src/memory-test.cpp tmp/builds/build-perftest/
$CMAKE_CMD -S tmp/builds/build-perftest -B tmp/builds/build-perftest -DCMAKE_BUILD_TYPE=Release

echo "Building performance test..."
$CMAKE_CMD --build tmp/builds/build-perftest --config Release

# Run the performance test
echo ""
echo "Running performance test..."
echo "This will test stabilization performance across different resolutions and settings..."
echo ""

tmp/builds/build-perftest/perftest

echo ""
echo "Running memory stability test..."
echo "This will test for memory leaks during extended operation..."
echo ""

tmp/builds/build-perftest/memtest

echo ""
echo "=== Performance Testing Complete ==="
echo "Review the results above to verify:"
echo "1. Real-time performance: Processing time should be ≤33.3ms per frame for 30fps"
echo "2. Memory stability: No significant memory growth during extended operation"