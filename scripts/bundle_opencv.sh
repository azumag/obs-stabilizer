#!/bin/bash

# Script to bundle OpenCV libraries with the plugin for macOS deployment
# This creates a self-contained .plugin bundle that can be distributed without
# requiring OpenCV to be installed separately

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
PLUGIN_NAME="obs-stabilizer-opencv.so"
PLUGIN_FILE="${BUILD_DIR}/${PLUGIN_NAME}"

if [ ! -f "${PLUGIN_FILE}" ]; then
	echo "Error: Plugin file not found at ${PLUGIN_FILE}"
	echo "Please build the plugin first using: mkdir build && cd build && cmake .. && make"
	exit 1
fi

echo "Analyzing plugin dependencies..."

# Get list of OpenCV dependencies (handle various installation paths)
OPENCV_LIBS=$(otool -L "${PLUGIN_FILE}" | grep 'libopencv.*\.dylib' | awk '{print $1}' | grep -E '^/' | sort -u)

if [ -z "$OPENCV_LIBS" ]; then
	echo "No OpenCV dependencies found."
	echo "Plugin may already be bundled or statically linked."
	OTOOL_OUTPUT=$(otool -L "${PLUGIN_FILE}" | grep -E 'libopencv|@loader_path' || true)
	if [ -n "$OTOOl_OUTPUT" ]; then
		echo "Current OpenCV dependencies:"
		echo "$OTOOl_OUTPUT"
	fi
	exit 0
fi

echo "Found OpenCV dependencies to bundle:"
echo "$OPENCV_LIBS"

# Create Frameworks directory
FRAMEWORKS_DIR="${BUILD_DIR}/Frameworks"
mkdir -p "${FRAMEWORKS_DIR}"

# Copy OpenCV libraries to Frameworks directory
echo ""
echo "Copying OpenCV libraries to bundle..."
for lib in $OPENCV_LIBS; do
	lib_name=$(basename "$lib")
	echo "  Copying $lib_name"
	cp "$lib" "${FRAMEWORKS_DIR}/"
done

# Update plugin to use bundled libraries
echo ""
echo "Updating plugin to use bundled libraries..."
for lib in $OPENCV_LIBS; do
	lib_name=$(basename "$lib")
	echo "  Updating reference to $lib_name"
	install_name_tool -change "$lib" "@loader_path/Frameworks/$lib_name" "${PLUGIN_FILE}"
done

# Verify the changes
echo ""
echo "Verifying plugin dependencies after bundling..."
echo "Plugin dependencies:"
otool -L "${PLUGIN_FILE}" | grep -E "(opencv|@loader_path)"

echo ""
echo "=========================================="
echo "OpenCV libraries bundled successfully!"
echo "=========================================="
echo ""
echo "The plugin now references OpenCV libraries from the Frameworks directory:"
echo "  ${FRAMEWORKS_DIR}"
echo ""
echo "To deploy:"
echo "1. Copy the plugin (${PLUGIN_FILE}) to OBS plugins directory"
echo "2. Copy the Frameworks directory to the same location as the plugin"
echo "3. The Frameworks directory should be at: <OBS plugins dir>/Frameworks/"
echo ""
echo "Example:"
echo "  cp build/obs-stabilizer-opencv.so ~/.config/obs-studio/plugins/obs-stabilizer-opencv/bin/"
echo "  cp -r build/Frameworks ~/.config/obs-studio/plugins/obs-stabilizer-opencv/bin/"
