#!/bin/bash

# Repair generated OBS Stabilizer plugin bundles on macOS. Legacy .so/.dylib
# outputs are converted to a standard .plugin bundle for compatibility.
set -euo pipefail

PLUGIN_PATH="${1:-}"
BUNDLE_VERSION="${OBS_STABILIZER_VERSION:-0.1.0}"

find_build_output() {
	local candidate
	for candidate in \
		build/obs-stabilizer.plugin \
		build/Release/obs-stabilizer.plugin \
		build/obs-stabilizer-opencv.so \
		build/obs-stabilizer.so \
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

	if [ -f "$bundle/Contents/Info.plist" ] && [ -x /usr/libexec/PlistBuddy ]; then
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
	local contents_dir
	local binary_path
	local plist_path

	source_dir=$(cd "$(dirname "$source_binary")" && pwd) || return 1
	bundle_path="${OBS_STABILIZER_BUNDLE_PATH:-$source_dir/obs-stabilizer.plugin}"
	contents_dir="$bundle_path/Contents"
	binary_path="$contents_dir/MacOS/obs-stabilizer"
	plist_path="$contents_dir/Info.plist"

	if [ -e "$bundle_path" ]; then
		printf "Error: Refusing to replace existing bundle at %s\n" "$bundle_path" >&2
		printf "Pass that bundle directly, move it aside, or set OBS_STABILIZER_BUNDLE_PATH.\n" >&2
		return 1
	fi

	mkdir -p "$contents_dir/MacOS" "$contents_dir/Resources" || return 1
	cp "$source_binary" "$binary_path" || return 1
	chmod +x "$binary_path" || return 1

	plutil -create xml1 "$plist_path" || return 1
	plutil -insert CFBundleDevelopmentRegion -string English "$plist_path" || return 1
	plutil -insert CFBundleDisplayName -string "OBS Stabilizer" "$plist_path" || return 1
	plutil -insert CFBundleExecutable -string obs-stabilizer "$plist_path" || return 1
	plutil -insert CFBundleGetInfoString -string "OBS Stabilizer $BUNDLE_VERSION" "$plist_path" || return 1
	plutil -insert CFBundleIdentifier -string com.azumag.obs-stabilizer "$plist_path" || return 1
	plutil -insert CFBundleInfoDictionaryVersion -string 6.0 "$plist_path" || return 1
	plutil -insert CFBundleName -string "OBS Stabilizer" "$plist_path" || return 1
	plutil -insert CFBundlePackageType -string BNDL "$plist_path" || return 1
	plutil -insert CFBundleShortVersionString -string "$BUNDLE_VERSION" "$plist_path" || return 1
	plutil -insert CFBundleSignature -string '????' "$plist_path" || return 1
	plutil -insert CFBundleSupportedPlatforms -json '["MacOSX"]' "$plist_path" || return 1
	plutil -insert CFBundleVersion -string "$BUNDLE_VERSION" "$plist_path" || return 1
	plutil -insert CSResourcesFileMapped -bool YES "$plist_path" || return 1
	plutil -insert LSMinimumSystemVersion -string 12.0 "$plist_path" || return 1
	plutil -lint "$plist_path" >/dev/null || return 1

	printf '%s\n' "$bundle_path"
}

if [ -z "$PLUGIN_PATH" ]; then
	PLUGIN_PATH=$(find_build_output || true)
fi

if [ -z "$PLUGIN_PATH" ]; then
	printf "Error: Plugin not found. Build the plugin first or specify its path.\n" >&2
	printf "Usage: %s [plugin-path]\n" "$0" >&2
	printf "Examples:\n" >&2
	printf "  %s build/obs-stabilizer.plugin\n" "$0" >&2
	printf "  %s build/obs-stabilizer-opencv.so\n" "$0" >&2
	exit 1
fi

for command in install_name_tool otool codesign plutil; do
	if ! command -v "$command" >/dev/null 2>&1; then
		printf "Error: Required macOS command not found: %s\n" "$command" >&2
		exit 1
	fi
done

if [ -f "$PLUGIN_PATH" ] && [[ "$PLUGIN_PATH" == *.so || "$PLUGIN_PATH" == *.dylib ]]; then
	printf "Creating macOS .plugin bundle from %s\n" "$PLUGIN_PATH"
	PLUGIN_PATH=$(create_plugin_bundle "$PLUGIN_PATH")
elif ! { [ -d "$PLUGIN_PATH" ] && [[ "$PLUGIN_PATH" == *.plugin ]]; }; then
	printf "Error: Invalid plugin format. Expected a .so/.dylib file or .plugin bundle.\n" >&2
	exit 1
fi

PLIST_PATH="$PLUGIN_PATH/Contents/Info.plist"
if [ ! -f "$PLIST_PATH" ]; then
	printf "Error: Info.plist not found at %s\n" "$PLIST_PATH" >&2
	exit 1
fi
plutil -lint "$PLIST_PATH" >/dev/null

BINARY_PATH=$(find_bundle_binary "$PLUGIN_PATH" || true)
if [ -z "$BINARY_PATH" ]; then
	printf "Error: No plugin binary found under %s/Contents/MacOS\n" "$PLUGIN_PATH" >&2
	exit 1
fi

printf "Fixing plugin bundle: %s\n" "$PLUGIN_PATH"
printf "Binary path: %s\n\n" "$BINARY_PATH"

FRAMEWORKS_DIR="$PLUGIN_PATH/Contents/Frameworks"
HAS_BUNDLED_DEPENDENCIES=0
if [ -d "$FRAMEWORKS_DIR" ] && [ -n "$(find "$FRAMEWORKS_DIR" -maxdepth 1 -type f -print -quit)" ]; then
	HAS_BUNDLED_DEPENDENCIES=1
fi

while IFS= read -r lib; do
	[ -n "$lib" ] || continue
	lib_name=$(basename "$lib")
	if [ -f "$FRAMEWORKS_DIR/$lib_name" ]; then
		install_name_tool -change "$lib" "@loader_path/../Frameworks/$lib_name" "$BINARY_PATH"
	elif [[ "$lib" == /opt/homebrew/* ]] || [[ "$lib" == /usr/local/* ]] || [[ "$lib" == /opt/local/* ]]; then
		install_name_tool -change "$lib" "@rpath/$lib_name" "$BINARY_PATH"
	fi
done < <(otool -L "$BINARY_PATH" | awk 'NR > 1 && /opencv/ {print $1}')

# OBS modules are loaded with dlopen(), so plugin-local dependencies must be
# resolved relative to the module itself, not relative to the OBS executable.
install_name_tool -delete_rpath "@executable_path/../Frameworks" "$BINARY_PATH" 2>/dev/null || true
install_name_tool -add_rpath "@loader_path/../Frameworks" "$BINARY_PATH" 2>/dev/null || true

if [ "$HAS_BUNDLED_DEPENDENCIES" -eq 0 ]; then
	for rpath in \
		/opt/homebrew/opt/opencv@4/lib \
		/opt/homebrew/opt/opencv/lib \
		/opt/homebrew/lib \
		/usr/local/opt/opencv@4/lib \
		/usr/local/opt/opencv/lib \
		/usr/local/lib \
		/opt/local/lib; do
		[ -d "$rpath" ] || continue
		install_name_tool -add_rpath "$rpath" "$BINARY_PATH" 2>/dev/null || true
	done
fi

codesign --force --deep --sign - "$PLUGIN_PATH"
codesign --verify --deep --strict "$PLUGIN_PATH"

printf "\nVerifying fixes...\n"
otool -L "$BINARY_PATH" | head -10

USER_PLUGIN_DIR="$HOME/Library/Application Support/obs-studio/plugins"
printf "\nPlugin bundle is ready: %s\n" "$PLUGIN_PATH"
printf "Move any existing obs-stabilizer.plugin aside, then install with:\n"
printf "  mkdir -p '%s'\n" "$USER_PLUGIN_DIR"
printf "  cp -R '%s' '%s/'\n" "$PLUGIN_PATH" "$USER_PLUGIN_DIR"
printf "\nRestart OBS and enable the plugin in Filters.\n"
