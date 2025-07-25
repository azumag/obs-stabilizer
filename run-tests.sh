#!/bin/bash

# Test Runner for OBS Stabilizer Plugin
# Builds and runs the unit test suite using Google Test

set -e

echo "=== OBS Stabilizer Test Suite ==="
echo "Building and running unit tests..."

# Navigate to tests directory
cd tests

# Clean previous build
rm -rf build-tests

# Find CMAKE (try multiple possible cmake locations)
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

# Build the test suite
echo "Configuring test build..."
$CMAKE_CMD -S . -B build-tests -DCMAKE_BUILD_TYPE=Debug

echo "Building test suite..."
$CMAKE_CMD --build build-tests

# Run the tests
echo ""
echo "Running unit tests..."
echo "=========================="

./build-tests/stabilizer_tests

echo ""
echo "=== Test Suite Complete ==="
echo "All unit tests have been executed."
echo "Review results above for any failures or issues."