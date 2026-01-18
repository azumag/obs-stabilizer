#!/bin/bash

# Test 2: Plugin loading
# Verifies the plugin loads correctly in OBS Studio

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$(dirname "$SCRIPT_DIR")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
OBS_APP="/Applications/OBS.app/Contents/MacOS/OBS"
OBS_PLUGINS_DIR="$HOME/Library/Application Support/obs-studio/plugins"
LOG_DIR="$SCRIPT_DIR/results"
TEST_LOG="$LOG_DIR/obs_loading_$(date +%Y%m%d_%H%M%S).log"

echo "Testing plugin loading in OBS..."

# Check if plugin was built
if [ ! -f "$BUILD_DIR/obs-stabilizer-opencv.so" ]; then
	echo "ERROR: Plugin binary not found. Run build test first."
	exit 1
fi
echo "✓ Plugin binary found"

# Create plugin directory structure
PLUGIN_DIR="$OBS_PLUGINS_DIR/obs-stabilizer"
mkdir -p "$PLUGIN_DIR/bin/64bit"

# Copy plugin to OBS plugins directory
echo "Installing plugin..."
cp "$BUILD_DIR/obs-stabilizer-opencv.so" "$PLUGIN_DIR/bin/64bit/"
echo "✓ Plugin installed to OBS plugins directory"

# Start OBS in headless mode (macOS with display)
echo "Starting OBS Studio..."
"$OBS_APP" \
	--startreplaybuffer \
	--collection "Test Collection" \
	--profile "Test Profile" \
	--minimize-to-tray \
	2>&1 | tee "$TEST_LOG" &
OBS_PID=$!

echo "OBS PID: $OBS_PID"

# Wait for OBS to initialize
sleep 5

# Check if OBS is still running
if ! ps -p $OBS_PID >/dev/null; then
	echo "ERROR: OBS crashed on startup"

	# Check for crash logs
	CRASH_LOG_DIR="$HOME/Library/Logs/DiagnosticReports"
	RECENT_CRASH=$(find "$CRASH_LOG_DIR" -name "OBS*.crash" -mmin -2 | head -1)

	if [ -n "$RECENT_CRASH" ]; then
		echo "Crash log found: $RECENT_CRASH"
		echo "Exception info:"
		grep -A5 "Exception Type:" "$RECENT_CRASH" | head -10
	fi

	exit 1
fi
echo "✓ OBS started successfully"

# Check OBS logs for plugin loading
echo "Checking plugin loading..."
OBS_LOG_DIR="$HOME/Library/Application Support/obs-studio/logs"
LATEST_OBS_LOG=$(ls -t "$OBS_LOG_DIR"/obs-*.log | head -1)

if [ ! -f "$LATEST_OBS_LOG" ]; then
	echo "WARNING: OBS log file not found"
else
	# Check for plugin loading
	if grep -q "obs-stabilizer" "$LATEST_OBS_LOG"; then
		echo "✓ Plugin mentioned in OBS logs"

		# Check for loading errors
		if grep -q "Failed to load.*obs-stabilizer" "$LATEST_OBS_LOG"; then
			echo "ERROR: Plugin failed to load"
			grep "obs-stabilizer" "$LATEST_OBS_LOG"
			exit 1
		elif grep -q "OpenCV Stabilizer plugin loaded successfully" "$LATEST_OBS_LOG"; then
			echo "✓ Plugin loaded successfully"
		elif grep -q "obs_module_load.*obs-stabilizer" "$LATEST_OBS_LOG"; then
			echo "✓ Plugin module loaded"
		else
			echo "WARNING: Plugin loading status unclear"
		fi
	else
		echo "WARNING: Plugin not found in OBS logs"
	fi
fi

# Check if filter is registered
echo "Checking filter registration..."
if grep -q "stabilizer_opencv_filter\|opencv-stabilizer" "$LATEST_OBS_LOG" 2>/dev/null; then
	echo "✓ Filter registered"
else
	echo "WARNING: Filter registration not confirmed in logs"
fi

# Check for OBS crashes
CRASH_COUNT=$(find "$HOME/Library/Logs/DiagnosticReports" -name "OBS*.crash" -mmin -2 | wc -l | tr -d ' ')
if [ "$CRASH_COUNT" -gt 0 ]; then
	echo "ERROR: OBS crashed during plugin loading ($CRASH_COUNT crashes)"
	exit 1
fi
echo "✓ No crashes detected"

# Stop OBS
echo "Stopping OBS..."
kill $OBS_PID 2>/dev/null || true
sleep 2

# Force kill if needed
if ps -p $OBS_PID >/dev/null 2>&1; then
	kill -9 $OBS_PID 2>/dev/null || true
fi

echo "Plugin loading test completed"
exit 0
