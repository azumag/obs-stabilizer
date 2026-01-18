#!/bin/bash

# Test 99: Cleanup
# Cleans up after all tests

set -e

echo "Cleaning up test environment..."

# Kill OBS
if pgrep -x "OBS" >/dev/null; then
	echo "Stopping OBS..."
	pkill -9 OBS 2>/dev/null || true
	sleep 1
fi
echo "✓ OBS stopped"

# Close crash reporter
osascript -e 'tell application "Problem Reporter" to quit' 2>/dev/null || true
pkill -f "ReportCrash" 2>/dev/null || true
pkill -f "Problem Reporter" 2>/dev/null || true

# Reset OBS configuration
OBS_CONFIG_DIR="$HOME/Library/Application Support/obs-studio"
if [ -f "$OBS_CONFIG_DIR/global.ini" ]; then
	echo "Resetting OBS configuration..."
	sed -i '' '/^LastCrash=/d' "$OBS_CONFIG_DIR/global.ini" 2>/dev/null || true
	sed -i '' '/^SafeMode=/d' "$OBS_CONFIG_DIR/global.ini" 2>/dev/null || true
	sed -i '' '/^DisableShutdownCheck=/d' "$OBS_CONFIG_DIR/global.ini" 2>/dev/null || true

	# Restore safe mode prevention
	echo "" >>"$OBS_CONFIG_DIR/global.ini"
	echo "[General]" >>"$OBS_CONFIG_DIR/global.ini"
	echo "DisableShutdownCheck=true" >>"$OBS_CONFIG_DIR/global.ini"

	echo "✓ OBS configuration reset"
fi

# Clear crash markers
rm -f "$OBS_CONFIG_DIR/.crashed" 2>/dev/null || true
rm -f "$OBS_CONFIG_DIR/.safe_mode" 2>/dev/null || true
rm -f "$OBS_CONFIG_DIR/.unclean_shutdown" 2>/dev/null || true

# Remove test plugin (optional - comment out if you want to keep it)
# PLUGIN_DIR="$OBS_CONFIG_DIR/plugins/obs-stabilizer"
# if [ -d "$PLUGIN_DIR" ]; then
#     echo "Removing test plugin..."
#     rm -rf "$PLUGIN_DIR"
#     echo "✓ Test plugin removed"
# fi

# Cleanup temporary files
rm -f /tmp/obs_*_marker_* 2>/dev/null || true

echo "Cleanup completed"
exit 0
