#!/bin/bash

# Fix script for pre-flight issues
# Automatically resolves common environment setup problems

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
OBS_PLUGINS_DIR="$HOME/Library/Application Support/obs-studio/plugins"

echo "Attempting to fix pre-flight issues..."

FIXES_APPLIED=0

# Fix 1: Remove problematic plugins
if [ -d "$OBS_PLUGINS_DIR/obs-stabilizer-minimal.plugin" ]; then
	echo "Fixing: Removing problematic obs-stabilizer-minimal.plugin..."
	rm -rf "$OBS_PLUGINS_DIR/obs-stabilizer-minimal.plugin"
	echo "✓ Removed problematic plugin"
	FIXES_APPLIED=$((FIXES_APPLIED + 1))
fi

# Fix 2: Remove any test plugins from previous runs
for plugin_dir in "$OBS_PLUGINS_DIR"/*.plugin; do
	if [ -d "$plugin_dir" ]; then
		plugin_name=$(basename "$plugin_dir")
		if [[ "$plugin_name" =~ (test|stabilizer|minimal|opencv) ]]; then
			echo "Fixing: Removing test plugin: $plugin_name..."
			rm -rf "$plugin_dir"
			echo "✓ Removed $plugin_name"
			FIXES_APPLIED=$((FIXES_APPLIED + 1))
		fi
	fi
done

# Fix 3: Reset OBS safe mode settings
OBS_CONFIG_DIR="$HOME/Library/Application Support/obs-studio"
if [ -f "$OBS_CONFIG_DIR/global.ini" ]; then
	echo "Fixing: Resetting OBS safe mode settings..."
	sed -i '' '/^LastCrash=/d' "$OBS_CONFIG_DIR/global.ini" 2>/dev/null || true
	sed -i '' '/^SafeMode=/d' "$OBS_CONFIG_DIR/global.ini" 2>/dev/null || true
	sed -i '' '/^DisableShutdownCheck=/d' "$OBS_CONFIG_DIR/global.ini" 2>/dev/null || true

	# Add safe mode prevention
	echo "" >>"$OBS_CONFIG_DIR/global.ini"
	echo "[General]" >>"$OBS_CONFIG_DIR/global.ini"
	echo "DisableShutdownCheck=true" >>"$OBS_CONFIG_DIR/global.ini"
	echo "✓ Reset OBS safe mode settings"
	FIXES_APPLIED=$((FIXES_APPLIED + 1))
fi

# Fix 4: Clear crash markers
rm -f "$OBS_CONFIG_DIR/.crashed" 2>/dev/null || true
rm -f "$OBS_CONFIG_DIR/.safe_mode" 2>/dev/null || true
rm -f "$OBS_CONFIG_DIR/.unclean_shutdown" 2>/dev/null || true

# Fix 5: Kill any stuck OBS processes
if pgrep -x "OBS" >/dev/null; then
	echo "Fixing: Killing stuck OBS processes..."
	pkill -9 OBS 2>/dev/null || true
	sleep 1
	echo "✓ Killed stuck OBS processes"
	FIXES_APPLIED=$((FIXES_APPLIED + 1))
fi

# Fix 6: Close crash reporter windows
osascript -e 'tell application "Problem Reporter" to quit' 2>/dev/null || true
pkill -f "ReportCrash" 2>/dev/null || true
pkill -f "Problem Reporter" 2>/dev/null || true

# Fix 7: Create build directory if needed
if [ ! -d "$PROJECT_ROOT/build" ]; then
	echo "Fixing: Creating build directory..."
	mkdir -p "$PROJECT_ROOT/build"
	echo "✓ Created build directory"
	FIXES_APPLIED=$((FIXES_APPLIED + 1))
fi

echo ""
echo "Pre-flight fixes complete. Applied $FIXES_APPLIED fixes."

if [ $FIXES_APPLIED -eq 0 ]; then
	echo "No fixes were needed."
	exit 0
else
	exit 0
fi
