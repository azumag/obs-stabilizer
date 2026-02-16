#!/bin/bash

# Fix OBS Stabilizer Plugin Loading Issues for macOS
set -e

# Handle both .plugin bundle format and .so format
PLUGIN_PATH="${1}"

# If no argument provided, search for the plugin in build directory
if [ -z "$PLUGIN_PATH" ]; then
	# Try to find the plugin in common build locations
	PLUGIN_PATH=""

	# Check for .so format (CMake build output)
	if [ -f "build/obs-stabilizer-opencv.so" ]; then
		PLUGIN_PATH="build/obs-stabilizer-opencv.so"
	elif [ -f "../build/obs-stabilizer-opencv.so" ]; then
		PLUGIN_PATH="../build/obs-stabilizer-opencv.so"
	fi

	# Check for .plugin bundle format
	if [ -z "$PLUGIN_PATH" ]; then
		if [ -d "obs-stabilizer.plugin" ]; then
			PLUGIN_PATH="obs-stabilizer.plugin"
		elif [ -d "../obs-stabilizer.plugin" ]; then
			PLUGIN_PATH="../obs-stabilizer.plugin"
		fi
	fi

	if [ -z "$PLUGIN_PATH" ]; then
		printf "Error: Plugin not found. Please specify the path.\n" >&2
		printf "Usage: %s <plugin-path>\n" "$0" >&2
		printf "       %s\n" "$0" >&2
		printf "\nSearched paths:\n" >&2
		printf "  - build/obs-stabilizer-opencv.so\n" >&2
		printf "  - obs-stabilizer.plugin/\n" >&2
		exit 1
	fi
fi

# Determine if it's a .so file or .plugin bundle
if [ -f "$PLUGIN_PATH" ] && [[ "$PLUGIN_PATH" == *.so ]]; then
	# .so format (CMake build output)
	BINARY_PATH="$PLUGIN_PATH"
	PLUGIN_NAME=$(basename "$PLUGIN_PATH")
elif [ -d "$PLUGIN_PATH" ] && [[ "$PLUGIN_PATH" == *.plugin ]]; then
	# .plugin bundle format
	BINARY_PATH="$PLUGIN_PATH/Contents/MacOS/obs-stabilizer"
	PLUGIN_NAME=$(basename "$PLUGIN_PATH")
else
	printf "Error: Invalid plugin format. Expected .so or .plugin bundle.\n" >&2
	exit 1
fi

# Check prerequisites
if ! command -v install_name_tool >/dev/null 2>&1; then
	printf "Error: Xcode Command Line Tools not installed\n" >&2
	printf "Run: xcode-select --install\n" >&2
	exit 1
fi

if [ ! -f "$BINARY_PATH" ]; then
	printf "Error: Plugin binary not found at %s\n" "$BINARY_PATH" >&2
	exit 1
fi

printf "Fixing plugin: %s\n" "$PLUGIN_NAME"
printf "Binary path: %s\n\n" "$BINARY_PATH"

# Fix install name
if [[ "$PLUGIN_NAME" == *.so ]]; then
	# For .so format, use the actual filename
	install_name_tool -id "@loader_path/$(basename "$BINARY_PATH")" "$BINARY_PATH"
else
	# For .plugin bundle format
	install_name_tool -id "@loader_path/../MacOS/obs-stabilizer" "$BINARY_PATH"
fi

# Fix OpenCV dependencies
otool -L "$BINARY_PATH" | grep opencv | awk '{print $1}' | while IFS= read -r lib; do
	if [[ "$lib" == /opt/homebrew/* ]] || [[ "$lib" == /usr/local/* ]] || [[ "$lib" == /opt/local/* ]]; then
		lib_name=$(basename "$lib")
		install_name_tool -change "$lib" "@rpath/$lib_name" "$BINARY_PATH"
	fi
done

# Add OpenCV rpath entries for different package managers
install_name_tool -add_rpath "/opt/homebrew/opt/opencv/lib" "$BINARY_PATH" 2>/dev/null || true
install_name_tool -add_rpath "/usr/local/lib" "$BINARY_PATH" 2>/dev/null || true
install_name_tool -add_rpath "/opt/local/lib" "$BINARY_PATH" 2>/dev/null || true

# Add OBS framework rpath if using OBS.app
if [ -d "/Applications/OBS.app/Contents/Frameworks" ]; then
	install_name_tool -add_rpath "/Applications/OBS.app/Contents/Frameworks" "$BINARY_PATH" 2>/dev/null || true
fi

# Sign the plugin
codesign --force --sign - "$BINARY_PATH"

# Verify fixes
printf "\nVerifying fixes...\n"
otool -L "$BINARY_PATH" | head -5

# Provide installation instructions
if [[ "$PLUGIN_NAME" == *.so ]]; then
	printf "\n✅ Plugin fixed. Install with:\n"
	printf "  cp %s ~/.config/obs-studio/plugins/\n" "$PLUGIN_PATH"
	printf "\nOr for system-wide installation:\n"
	printf "  sudo cp %s /Library/Application\\ Support/obs-studio/plugins/\n" "$PLUGIN_PATH"
else
	printf "\n✅ Plugin fixed. Install with:\n"
	printf "  cp -r %s ~/Library/Application\\ Support/obs-studio/plugins/\n" "$PLUGIN_PATH"
fi

printf "\nRestart OBS and enable the plugin in Settings > Filters\n"
