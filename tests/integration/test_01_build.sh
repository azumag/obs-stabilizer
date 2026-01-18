#!/bin/bash

# Test 1: Build verification
# Verifies the plugin can be built successfully

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

echo "Testing build process..."

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
	echo "ERROR: Build directory does not exist"
	exit 1
fi

# Clean previous build
echo "Cleaning previous build..."
cd "$BUILD_DIR"
rm -rf * 2>/dev/null || true

# Configure CMake
echo "Configuring CMake..."
if ! cmake -G Ninja "$PROJECT_ROOT" -DCMAKE_BUILD_TYPE=Release 2>&1 | tee /tmp/cmake_config.log; then
	echo "ERROR: CMake configuration failed"
	echo "Check log: /tmp/cmake_config.log"

	# Check for common errors
	if grep -q "OpenCV" /tmp/cmake_config.log; then
		echo "HINT: OpenCV not found. Install with: brew install opencv"
	fi

	exit 1
fi
echo "✓ CMake configured successfully"

# Build
echo "Building plugin..."
if ! ninja 2>&1 | tee /tmp/ninja_build.log; then
	echo "ERROR: Build failed"
	echo "Check log: /tmp/ninja_build.log"
	exit 1
fi
echo "✓ Build completed successfully"

# Verify output files
echo "Verifying output files..."
if [ ! -f "$BUILD_DIR/obs-stabilizer-opencv.so" ]; then
	echo "ERROR: Plugin binary not found"
	exit 1
fi
echo "✓ Plugin binary found: obs-stabilizer-opencv.so"

# Check binary format
BINARY_TYPE=$(file "$BUILD_DIR/obs-stabilizer-opencv.so")
echo "Binary type: $BINARY_TYPE"

if ! echo "$BINARY_TYPE" | grep -q "Mach-O.*dylib\|Mach-O.*bundle\|shared object"; then
	echo "ERROR: Binary is not a shared library"
	exit 1
fi
echo "✓ Binary format correct"

# Check dependencies
echo "Checking dependencies..."
if ! otool -L "$BUILD_DIR/obs-stabilizer-opencv.so" 2>/dev/null | grep -q "opencv"; then
	echo "WARNING: OpenCV not linked properly"
	exit 2 # Warning, not critical
fi
echo "✓ OpenCV linked"

echo "Build verification passed"
exit 0
