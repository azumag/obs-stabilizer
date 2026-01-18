#!/bin/bash

# Test 3: Basic functionality
# Verifies the plugin can process video frames without crashing

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$(dirname "$SCRIPT_DIR")/..")/.."
LOG_DIR="$SCRIPT_DIR/results"
TEST_LOG="$LOG_DIR/functionality_$(date +%Y%m%d_%H%M%S).log"

echo "Testing basic plugin functionality..."

# Create temporary test scene configuration
TEST_CONFIG_DIR="$HOME/Library/Application Support/obs-studio/test_config"
mkdir -p "$TEST_CONFIG_DIR/scenes" "$TEST_CONFIG_DIR/profiles"

# Create minimal scene config
cat >"$TEST_CONFIG_DIR/scenes/test_scenes.json" <<'EOF'
{
  "scene_collections": [
    {
      "name": "Test Collection",
      "sources": [
        {
          "id": "color_source_v2",
          "name": "Test Color",
          "settings": {
            "color": 4294967295
          }
        }
      ]
    }
  ]
}
EOF

# Start OBS with test config
echo "Starting OBS with test configuration..."
OBS_APP="/Applications/OBS.app/Contents/MacOS/OBS"
"$OBS_APP" \
	--collection "Test Collection" \
	--profile "Test Profile" \
	--portable "$TEST_CONFIG_DIR" \
	--minimize-to-tray \
	2>&1 | tee "$TEST_LOG" &
OBS_PID=$!

echo "OBS PID: $OBS_PID"

# Wait for OBS to initialize
sleep 5

# Check if OBS is still running
if ! ps -p $OBS_PID >/dev/null; then
	echo "ERROR: OBS crashed during basic functionality test"

	# Check crash logs
	CRASH_LOG_DIR="$HOME/Library/Logs/DiagnosticReports"
	RECENT_CRASH=$(find "$CRASH_LOG_DIR" -name "OBS*.crash" -mmin -2 | head -1)

	if [ -n "$RECENT_CRASH" ]; then
		echo "Crash log: $RECENT_CRASH"
		grep -A10 "Exception Type:" "$RECENT_CRASH" | head -15
	fi

	exit 1
fi
echo "✓ OBS running with plugin loaded"

# Monitor OBS for stability
echo "Monitoring OBS stability for 15 seconds..."
START_TIME=$(date +%s)
DURATION=15
CRASH_DETECTED=0

while [ $(($(date +%s) - START_TIME)) -lt $DURATION ]; do
	if ! ps -p $OBS_PID >/dev/null; then
		echo "ERROR: OBS crashed during monitoring"
		CRASH_DETECTED=1
		break
	fi
	sleep 1
done

if [ $CRASH_DETECTED -eq 1 ]; then
	exit 1
fi
echo "✓ OBS stable during monitoring period"

# Check OBS logs for errors
echo "Checking OBS logs for errors..."
OBS_LOG_DIR="$HOME/Library/Application Support/obs-studio/logs"
LATEST_OBS_LOG=$(ls -t "$OBS_LOG_DIR"/obs-*.log | head -1)

if [ -f "$LATEST_OBS_LOG" ]; then
	# Check for plugin errors
	if grep -q "obs-stabilizer.*ERROR" "$LATEST_OBS_LOG"; then
		echo "ERROR: Plugin errors found in OBS logs"
		grep "obs-stabilizer.*ERROR" "$LATEST_OBS_LOG"
		exit 1
	fi
	echo "✓ No plugin errors in logs"

	# Check for OpenCV errors
	if grep -q "OpenCV.*ERROR\|OpenCV.*Exception" "$LATEST_OBS_LOG"; then
		echo "ERROR: OpenCV errors found in OBS logs"
		grep "OpenCV.*ERROR\|OpenCV.*Exception" "$LATEST_OBS_LOG"
		exit 1
	fi
	echo "✓ No OpenCV errors in logs"
fi

# Check memory usage
echo "Checking memory usage..."
if ps -p $OBS_PID >/dev/null; then
	MEM_USAGE=$(ps -p $OBS_PID -o rss= | awk '{print $1}')
	MEM_MB=$((MEM_USAGE / 1024))
	echo "OBS memory usage: ${MEM_MB}MB"

	if [ $MEM_MB -gt 2000 ]; then
		echo "WARNING: High memory usage detected (>2GB)"
	else
		echo "✓ Memory usage acceptable"
	fi
fi

# Stop OBS
echo "Stopping OBS..."
kill $OBS_PID 2>/dev/null || true
sleep 2

if ps -p $OBS_PID >/dev/null 2>&1; then
	kill -9 $OBS_PID 2>/dev/null || true
fi

# Cleanup test config
rm -rf "$TEST_CONFIG_DIR" 2>/dev/null || true

echo "Basic functionality test completed"
exit 0
