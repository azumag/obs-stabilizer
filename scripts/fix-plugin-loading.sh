#!/bin/bash

# Fix OBS Stabilizer Plugin Loading Issues for macOS
set -e

readonly PLUGIN_PATH="${1:-obs-stabilizer.plugin}"
readonly BINARY_PATH="$PLUGIN_PATH/Contents/MacOS/obs-stabilizer"

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

# Fix install name
install_name_tool -id "@loader_path/../MacOS/obs-stabilizer" "$BINARY_PATH"

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

# Sign the plugin
codesign --force --sign - "$BINARY_PATH"

# Verify fixes
printf "Verifying fixes...\n"
otool -L "$BINARY_PATH" | head -5

printf "✅ Plugin fixed. Install with: cp -r %s ~/Library/Application\\ Support/obs-studio/plugins/\n" "$PLUGIN_PATH"