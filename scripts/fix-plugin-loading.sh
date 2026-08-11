#!/bin/bash

# Fix OBS Stabilizer plugin loading issues on macOS.
# Raw .so/.dylib build outputs are converted into a standard .plugin bundle.
set -euo pipefail

PLUGIN_PATH="${1:-}"

find_build_output() {
	local candidate
	for candidate in \
		build/obs-stabilizer.plugin \
		build/obs-stabilizer-opencv.so \
		build/obs-stabilizer.so \
		build/Release/obs-stabilizer.plugin \
		build/Release/obs-stabilizer-opencv.so \
		build/Release/obs-stabilizer.so \
		obs-stabilizer.plugin; do
		if [ -f "$candidate" ] || [ -d "$candidate" ]; then
			printf '%s\n' "$candidate"
			return 0
		fi
	done

	candidate=$(find build -maxdepth 5 \
		\( -type d -name 'obs-stabilizer*.plugin' \
		-o -type f \( -name 'obs-stabilizer*.so' -o -name 'obs-stabilizer*.dylib' \) \) \
		-print -quit 2>/dev/null || true)
	[ -n "$candidate" ] && printf '%s\n' "$candidate"
}

find_bundle_binary() {
	local bundle="$1"
	local macos_dir="$bundle/Contents/MacOS"
	local executable_name=""
	local candidate

	[ -d "$macos_dir" ] || return 1

	if [ -f "$bundle/Contents/Info.plist" ] && command -v /usr/libexec/PlistBuddy >/dev/null 2>&1; then
		executable_name=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' \
			"$bundle/Contents/Info.plist" 2>/dev/null || true)
		if [ -n "$executable_name" ] && [ -f "$macos_dir/$executable_name" ]; then
			printf '%s\n' "$macos_dir/$executable_name"
			return 0
		fi
	fi

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

create_plugin_bundle() {
	local source_binary="$1"
	local source_dir
	local bundle_path
	local macos_dir
	local binary_path
	local plist_path

	source_dir=$(cd "$(dirname "$source_binary")" && pwd)
	bundle_path="${OBS_STABILIZER_BUNDLE_PATH:-$source_dir/obs-stabilizer.plugin}"
	macos_dir="$bundle_path/Contents/MacOS"
	binary_path="$macos_dir/obs-stabilizer"
	plist_path="$bundle_path/Contents/Info.plist"

	rm -rf "$bundle_path"
	mkdir -p "$macos_dir"
	cp "$source_binary" "$binary_path"
	chmod +x "$binary_path"

	cat >"$plist_path" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key>
	<string>en</string>
	<key>CFBundleDisplayName</key>
	<string>OBS Stabilizer</string>
	<key>CFBundleExecutable</key>
	<string>obs-stabilizer</string>
	<key>CFBundleIdentifier</key>
	<string>com.azumag.obs-stabilizer</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleName</key>
	<string>OBS Stabilizer</string>
	<key>CFBundlePackageType</key>
	<string>BNDL</string>
	<key>CFBundleShortVersionString</key>
	<string>1.0</string>
	<key>CFBundleVersion</key>
	<string>1</string>
	<key>CFBundleSupportedPlatforms</key>
	<array>
		<string>MacOSX</string>
	</array>
</dict>
</plist>
PLIST

	printf '%s\n' "$bundle_path"
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
	printf "Creating macOS .plugin bundle from %s\n" "$PLUGIN_PATH"
	PLUGIN_PATH=$(create_plugin_bundle "$PLUGIN_PATH")
elif ! { [ -d "$PLUGIN_PATH" ] && [[ "$PLUGIN_PATH" == *.plugin ]]; }; then
	printf "Error: Invalid plugin format. Expected a .so/.dylib file or .plugin bundle.\n" >&2
	exit 1
fi

BINARY_PATH=$(find_bundle_binary "$PLUGIN_PATH" || true)
if [ -z "$BINARY_PATH" ]; then
	printf "Error: No plugin binary found under %s/Contents/MacOS\n" "$PLUGIN_PATH" >&2
	exit 1
fi

for command in install_name_tool otool codesign; do
	if ! command -v "$command" >/dev/null 2>&1; then
		printf "Error: Required Xcode command not found: %s\n" "$command" >&2
		printf "Run: xcode-select --install\n" >&2
		exit 1
	fi
done

if command -v plutil >/dev/null 2>&1; then
	plutil -lint "$PLUGIN_PATH/Contents/Info.plist" >/dev/null
fi

printf "Fixing plugin bundle: %s\n" "$PLUGIN_PATH"
printf "Binary path: %s\n\n" "$BINARY_PATH"

while IFS= read -r lib; do
	[ -n "$lib" ] || continue
	if [[ "$lib" == /opt/homebrew/* ]] || [[ "$lib" == /usr/local/* ]] || [[ "$lib" == /opt/local/* ]]; then
		lib_name=$(basename "$lib")
		install_name_tool -change "$lib" "@rpath/$lib_name" "$BINARY_PATH"
	fi
done < <(otool -L "$BINARY_PATH" | awk '/opencv/ {print $1}')

for rpath in \
	/opt/homebrew/opt/opencv@4/lib \
	/opt/homebrew/opt/opencv/lib \
	/usr/local/opt/opencv@4/lib \
	/usr/local/opt/opencv/lib \
	/usr/local/lib \
	/opt/local/lib; do
	[ -d "$rpath" ] || continue
	install_name_tool -add_rpath "$rpath" "$BINARY_PATH" 2>/dev/null || true
done

if [ -d "/Applications/OBS.app/Contents/Frameworks" ]; then
	install_name_tool -add_rpath "/Applications/OBS.app/Contents/Frameworks" "$BINARY_PATH" 2>/dev/null || true
fi

codesign --force --deep --sign - "$PLUGIN_PATH"

printf "\nVerifying fixes...\n"
otool -L "$BINARY_PATH" | head -10
codesign --verify --deep --strict "$PLUGIN_PATH"

USER_PLUGIN_DIR="$HOME/Library/Application Support/obs-studio/plugins"
printf "\nPlugin bundle is ready: %s\n" "$PLUGIN_PATH"
printf "Install with:\n"
printf "  mkdir -p '%s'\n" "$USER_PLUGIN_DIR"
printf "  rm -rf '%s/obs-stabilizer.plugin'\n" "$USER_PLUGIN_DIR"
printf "  cp -R '%s' '%s/'\n" "$PLUGIN_PATH" "$USER_PLUGIN_DIR"
printf "\nRestart OBS, add/open a source, and choose Filters to enable OBS Stabilizer.\n"
