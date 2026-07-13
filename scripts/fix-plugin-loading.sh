#!/bin/bash

# Fix OBS Stabilizer Plugin Loading Issues for macOS
set -euo pipefail

PLUGIN_PATH="${1:-}"

find_build_output() {
	local candidate
	for candidate in \
		build/obs-stabilizer-opencv.so \
		build/obs-stabilizer.so \
		build/obs-stabilizer.plugin \
		build/Release/obs-stabilizer-opencv.so \
		build/Release/obs-stabilizer.so \
		build/Release/obs-stabilizer.plugin \
		obs-stabilizer.plugin; do
		if [ -f "$candidate" ] || [ -d "$candidate" ]; then
			printf '%s\n' "$candidate"
			return 0
		fi
	done

	candidate=$(find build -maxdepth 5 \
		\( -type f \( -name 'obs-stabilizer*.so' -o -name 'obs-stabilizer*.dylib' \) \
		-o -type d -name 'obs-stabilizer*.plugin' \) \
		-print -quit 2>/dev/null || true)
	[ -n "$candidate" ] && printf '%s\n' "$candidate"
}

find_bundle_binary() {
	local bundle="$1"
	local macos_dir="$bundle/Contents/MacOS"
	local candidate

	[ -d "$macos_dir" ] || return 1

	for candidate in \
		"$macos_dir/obs-stabilizer" \
		"$macos_dir/obs-stabilizer-opencv" \
		"$macos_dir/obs-stabilizer-opencv.so" \
		"$macos_dir/obs-stabilizer.so"; do
		if [ -f "$candidate" ]; then
			printf '%s\n' "$candidate"
			return 0
		fi
	done

	candidate=$(find "$macos_dir" -maxdepth 1 -type f -perm -111 -print -quit 2>/dev/null || true)
	if [ -z "$candidate" ]; then
		candidate=$(find "$macos_dir" -maxdepth 1 -type f -print -quit 2>/dev/null || true)
	fi
	[ -n "$candidate" ] && printf '%s\n' "$candidate"
}

if [ -z "$PLUGIN_PATH" ]; then
	PLUGIN_PATH=$(find_build_output || true)
fi

if [ -z "$PLUGIN_PATH" ]; then
	printf "Error: Plugin not found. Build the plugin first or specify its path.\n" >&2
	printf "Usage: %s [plugin-path]\n" "$0" >&2
	printf "Examples:\n" >&2
	printf "  %s build/obs-stabilizer-opencv.so\n" "$0" >&2
	printf "  %s build/obs-stabilizer.plugin\n" "$0" >&2
	exit 1
fi

if [ -f "$PLUGIN_PATH" ] && [[ "$PLUGIN_PATH" == *.so || "$PLUGIN_PATH" == *.dylib ]]; then
	BINARY_PATH="$PLUGIN_PATH"
	PLUGIN_NAME=$(basename "$PLUGIN_PATH")
elif [ -d "$PLUGIN_PATH" ] && [[ "$PLUGIN_PATH" == *.plugin ]]; then
	BINARY_PATH=$(find_bundle_binary "$PLUGIN_PATH" || true)
	PLUGIN_NAME=$(basename "$PLUGIN_PATH")
	if [ -z "$BINARY_PATH" ]; then
		printf "Error: No plugin binary found under %s/Contents/MacOS\n" "$PLUGIN_PATH" >&2
		exit 1
	fi
else
	printf "Error: Invalid plugin format. Expected a .so/.dylib file or .plugin bundle.\n" >&2
	exit 1
fi

if ! command -v install_name_tool >/dev/null 2>&1; then
	printf "Error: Xcode Command Line Tools not installed\n" >&2
	printf "Run: xcode-select --install\n" >&2
	exit 1
fi

for command in otool codesign; do
	if ! command -v "$command" >/dev/null 2>&1; then
		printf "Error: Required command not found: %s\n" "$command" >&2
		exit 1
	fi
done

printf "Fixing plugin: %s\n" "$PLUGIN_NAME"
printf "Binary path: %s\n\n" "$BINARY_PATH"

install_name_tool -id "@loader_path/$(basename "$BINARY_PATH")" "$BINARY_PATH"

otool -L "$BINARY_PATH" | grep opencv | awk '{print $1}' | while IFS= read -r lib; do
	if [[ "$lib" == /opt/homebrew/* ]] || [[ "$lib" == /usr/local/* ]] || [[ "$lib" == /opt/local/* ]]; then
		lib_name=$(basename "$lib")
		install_name_tool -change "$lib" "@rpath/$lib_name" "$BINARY_PATH"
	fi
done

install_name_tool -add_rpath "/opt/homebrew/opt/opencv/lib" "$BINARY_PATH" 2>/dev/null || true
install_name_tool -add_rpath "/usr/local/lib" "$BINARY_PATH" 2>/dev/null || true
install_name_tool -add_rpath "/opt/local/lib" "$BINARY_PATH" 2>/dev/null || true

if [ -d "/Applications/OBS.app/Contents/Frameworks" ]; then
	install_name_tool -add_rpath "/Applications/OBS.app/Contents/Frameworks" "$BINARY_PATH" 2>/dev/null || true
fi

if [[ "$PLUGIN_PATH" == *.plugin ]]; then
	codesign --force --deep --sign - "$PLUGIN_PATH"
else
	codesign --force --sign - "$BINARY_PATH"
fi

printf "\nVerifying fixes...\n"
otool -L "$BINARY_PATH" | head -5

USER_PLUGIN_DIR="$HOME/Library/Application Support/obs-studio/plugins"
printf "\nPlugin fixed. Install with:\n"
printf "  mkdir -p '%s'\n" "$USER_PLUGIN_DIR"
if [[ "$PLUGIN_PATH" == *.plugin ]]; then
	printf "  cp -R '%s' '%s/'\n" "$PLUGIN_PATH" "$USER_PLUGIN_DIR"
else
	printf "  cp '%s' '%s/'\n" "$PLUGIN_PATH" "$USER_PLUGIN_DIR"
fi

printf "\nRestart OBS and enable the plugin in Filters.\n"
