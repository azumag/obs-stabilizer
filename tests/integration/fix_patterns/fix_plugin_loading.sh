#!/bin/bash

# Fix script for plugin loading issues
# Automatically resolves common OBS plugin loading problems

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$(dirname "$SCRIPT_DIR")/..")/.."
BUILD_DIR="$PROJECT_ROOT/build"

echo "Attempting to fix plugin loading issues..."

FIXES_APPLIED=0

# Fix 1: Kill OBS processes
if pgrep -x "OBS" >/dev/null; then
	echo "Fixing: Killing OBS processes..."
	pkill -9 OBS 2>/dev/null || true
	sleep 1
	FIXES_APPLIED=$((FIXES_APPLIED + 1))
fi

# Fix 2: Remove plugin and reinstall
OBS_PLUGINS_DIR="$HOME/Library/Application Support/obs-studio/plugins"
PLUGIN_DIR="$OBS_PLUGINS_DIR/obs-stabilizer"

if [ -d "$PLUGIN_DIR" ]; then
	echo "Fixing: Removing old plugin installation..."
	rm -rf "$PLUGIN_DIR"
	sleep 1
	FIXES_APPLIED=$((FIXES_APPLIED + 1))
fi

# Fix 3: Reinstall plugin
if [ -f "$BUILD_DIR/libobs-stabilizer-opencv.so" ]; then
	echo "Fixing: Reinstalling plugin..."
	mkdir -p "$PLUGIN_DIR/bin/64bit"
	cp "$BUILD_DIR/libobs-stabilizer-opencv.so" "$PLUGIN_DIR/bin/64bit/"
	echo "✓ Plugin reinstalled"
	FIXES_APPLIED=$((FIXES_APPLIED + 1))
fi

# Fix 4: Fix plugin binary permissions
if [ -f "$PLUGIN_DIR/bin/64bit/libobs-stabilizer-opencv.so" ]; then
	echo "Fixing: Setting executable permissions..."
	chmod +x "$PLUGIN_DIR/bin/64bit/libobs-stabilizer-opencv.so"
	FIXES_APPLIED=$((FIXES_APPLIED + 1))
fi

# Fix 5: Fix dynamic library paths
if [ -f "$PLUGIN_DIR/bin/64bit/libobs-stabilizer-opencv.so" ]; then
	echo "Fixing: Updating library paths..."

	# Check and fix OpenCV library paths
	if otool -L "$PLUGIN_DIR/bin/64bit/libobs-stabilizer-opencv.so" | grep -q "opencv"; then
		# Use install_name_tool to update paths
		otool -L "$PLUGIN_DIR/bin/64bit/libobs-stabilizer-opencv.so" | grep opencv | while read line; do
			lib_path=$(echo "$line" | awk '{print $1}')
			if [[ "$lib_path" != "@rpath/"* ]] && [[ "$lib_path" != "/opt/homebrew/"* ]] && [[ "$lib_path" != "/usr/local/lib/"* ]]; then
				# Update to use rpath
				lib_name=$(basename "$lib_path")
				install_name_tool -change "$lib_path" "@rpath/$lib_name" "$PLUGIN_DIR/bin/64bit/libobs-stabilizer-opencv.so" 2>/dev/null || true
			fi
		done
	fi

	# Add missing rpath entries
	install_name_tool -add_rpath "@loader_path/../lib" "$PLUGIN_DIR/bin/64bit/libobs-stabilizer-opencv.so" 2>/dev/null || true
	install_name_tool -add_rpath "/opt/homebrew/lib" "$PLUGIN_DIR/bin/64bit/libobs-stabilizer-opencv.so" 2>/dev/null || true
	install_name_tool -add_rpath "/usr/local/lib" "$PLUGIN_DIR/bin/64bit/libobs-stabilizer-opencv.so" 2>/dev/null || true

	echo "✓ Library paths updated"
	FIXES_APPLIED=$((FIXES_APPLIED + 1))
fi

# Fix 6: Reset OBS configuration
OBS_CONFIG_DIR="$HOME/Library/Application Support/obs-studio"
if [ -f "$OBS_CONFIG_DIR/global.ini" ]; then
	echo "Fixing: Resetting OBS safe mode settings..."
	sed -i '' '/^LastCrash=/d' "$OBS_CONFIG_DIR/global.ini" 2>/dev/null || true
	sed -i '' '/^SafeMode=/d' "$OBS_CONFIG_DIR/global.ini" 2>/dev/null || true
	sed -i '' '/^DisableShutdownCheck=/d' "$OBS_CONFIG_DIR/global.ini" 2>/dev/null || true

	# Add safe mode prevention
	if ! grep -q "DisableShutdownCheck" "$OBS_CONFIG_DIR/global.ini"; then
		echo "" >>"$OBS_CONFIG_DIR/global.ini"
		echo "[General]" >>"$OBS_CONFIG_DIR/global.ini"
		echo "DisableShutdownCheck=true" >>"$OBS_CONFIG_DIR/global.ini"
	fi

	echo "✓ OBS settings reset"
	FIXES_APPLIED=$((FIXES_APPLIED + 1))
fi

# Fix 7: Clear crash markers
rm -f "$OBS_CONFIG_DIR/.crashed" 2>/dev/null || true
rm -f "$OBS_CONFIG_DIR/.safe_mode" 2>/dev/null || true
rm -f "$OBS_CONFIG_DIR/.unclean_shutdown" 2>/dev/null || true

# Fix 8: Clear recent crash logs
CRASH_LOG_DIR="$HOME/Library/Logs/DiagnosticReports"
find "$CRASH_LOG_DIR" -name "OBS*.crash" -mmin -5 -delete 2>/dev/null || true

# Fix 9: Close crash reporter windows
osascript -e 'tell application "Problem Reporter" to quit' 2>/dev/null || true
pkill -f "ReportCrash" 2>/dev/null || true
pkill -f "Problem Reporter" 2>/dev/null || true

echo ""
echo "Plugin loading fixes complete. Applied $FIXES_APPLIED fixes."

if [ $FIXES_APPLIED -eq 0 ]; then
	echo "No fixes were needed. The issue may be in the plugin code."
	exit 1
else
	exit 0
fi
