#!/bin/bash
set -e

echo "Building OBS Stabilizer Plugin with OpenCV support"

# Clean previous build
echo "Cleaning previous build..."
rm -rf build_opencv
mkdir -p build_opencv
cd build_opencv

# Find OpenCV
OPENCV_DIR=$(brew --prefix opencv 2>/dev/null || echo "/opt/homebrew/opt/opencv")
if [ ! -d "$OPENCV_DIR" ]; then
    echo "Error: OpenCV not found. Please install with: brew install opencv"
    exit 1
fi

echo "Using OpenCV from: $OPENCV_DIR"

# Copy CMakeLists for OpenCV build
cp ../CMakeLists_opencv.txt ../CMakeLists.txt.backup 2>/dev/null || true
cp ../CMakeLists_opencv.txt ../CMakeLists.txt

# Configure with CMake using OpenCV-specific CMakeLists
echo "Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DOpenCV_DIR="$OPENCV_DIR/lib/cmake/opencv4" \
    -DCMAKE_CXX_FLAGS="-I$OPENCV_DIR/include/opencv4" \
    -DCMAKE_SHARED_LINKER_FLAGS="-L$OPENCV_DIR/lib"

# Build
echo "Building plugin..."
make -j$(sysctl -n hw.ncpu)

# Check if build succeeded
if [ ! -f "obs-stabilizer-opencv.so" ]; then
    echo "Error: Build failed - plugin not created"
    exit 1
fi

# Create plugin directory structure
PLUGIN_NAME="obs-stabilizer-opencv"
PLUGIN_DIR="$PLUGIN_NAME.plugin"
rm -rf "$PLUGIN_DIR"
mkdir -p "$PLUGIN_DIR/Contents/MacOS"

# Copy plugin binary
cp obs-stabilizer-opencv.so "$PLUGIN_DIR/Contents/MacOS/$PLUGIN_NAME"

# Sign the plugin
echo "Signing plugin..."
codesign --force --sign - "$PLUGIN_DIR/Contents/MacOS/$PLUGIN_NAME"

# Install to OBS
OBS_PLUGIN_DIR="$HOME/Library/Application Support/obs-studio/plugins"
echo "Installing to: $OBS_PLUGIN_DIR"

# Backup existing if present
if [ -d "$OBS_PLUGIN_DIR/$PLUGIN_NAME.plugin" ]; then
    echo "Backing up existing plugin..."
    mv "$OBS_PLUGIN_DIR/$PLUGIN_NAME.plugin" "$OBS_PLUGIN_DIR/$PLUGIN_NAME.plugin.backup.$(date +%Y%m%d_%H%M%S)"
fi

# Install new plugin
cp -R "$PLUGIN_DIR" "$OBS_PLUGIN_DIR/"

echo "Plugin installed successfully!"
echo "Checking installation..."
ls -la "$OBS_PLUGIN_DIR/$PLUGIN_NAME.plugin/Contents/MacOS/"

echo "Done! Restart OBS to load the new stabilizer plugin."