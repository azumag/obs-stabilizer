#!/bin/bash

# Test 4: Crash detection
# Monitors OBS for crashes during plugin usage

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
LOG_DIR="$SCRIPT_DIR/results"
TEST_LOG="$LOG_DIR/crash_detection_$(date +%Y%m%d_%H%M%S).log"
CRASH_LOG_DIR="$HOME/Library/Logs/DiagnosticReports"

echo "Testing crash detection..."

# Create marker file for crash detection
MARKER_FILE="/tmp/obs_crash_test_marker_$(date +%s)"
touch "$MARKER_FILE"

# Count crashes before test
CRASHES_BEFORE=$(find "$CRASH_LOG_DIR" -name "OBS*.crash" -newer "$MARKER_FILE" 2>/dev/null | wc -l | tr -d ' ')

# Start OBS
echo "Starting OBS..."
OBS_APP="/Applications/OBS.app/Contents/MacOS/OBS"
"$OBS_APP" \
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
	echo "ERROR: OBS crashed immediately on startup"

	# Check for crash logs
	CRASH_COUNT=$(find "$CRASH_LOG_DIR" -name "OBS*.crash" -newer "$MARKER_FILE" 2>/dev/null | wc -l | tr -d ' ')

	if [ "$CRASH_COUNT" -gt 0 ]; then
		echo "Crash logs found:"
		find "$CRASH_LOG_DIR" -name "OBS*.crash" -newer "$MARKER_FILE" 2>/dev/null | while read crash_file; do
			echo "  - $crash_file"
			grep -A5 "Exception Type:" "$crash_file" | head -10
		done
	fi

	rm -f "$MARKER_FILE"
	exit 1
fi
echo "✓ OBS started successfully"

# Monitor for crashes over time
echo "Monitoring for crashes over 30 seconds..."
MONITOR_DURATION=30
START_TIME=$(date +%s)
CRASH_DETECTED=0

while [ $(($(date +%s) - START_TIME)) -lt $MONITOR_DURATION ]; do
	if ! ps -p $OBS_PID >/dev/null; then
		echo "ERROR: OBS crashed during monitoring at $(date)"
		CRASH_DETECTED=1
		break
	fi
	sleep 1
done

if [ $CRASH_DETECTED -eq 1 ]; then
	# Check for crash logs
	CRASH_COUNT=$(find "$CRASH_LOG_DIR" -name "OBS*.crash" -newer "$MARKER_FILE" 2>/dev/null | wc -l | tr -d ' ')

	if [ "$CRASH_COUNT" -gt 0 ]; then
		echo "Crash logs found:"
		find "$CRASH_LOG_DIR" -name "OBS*.crash" -newer "$MARKER_FILE" 2>/dev/null | while read crash_file; do
			echo "  - $crash_file"

			# Extract crash information
			echo "    Exception Type:"
			grep -A2 "Exception Type:" "$crash_file" | head -3 | sed 's/^/      /'

			echo "    Thread info:"
			grep -A5 "Thread.*Crashed:" "$crash_file" | head -6 | sed 's/^/      /'
		done
	fi

	rm -f "$MARKER_FILE"
	exit 1
fi

echo "✓ No crashes detected during monitoring"

# Check for new crash logs
CRASH_COUNT=$(find "$CRASH_LOG_DIR" -name "OBS*.crash" -newer "$MARKER_FILE" 2>/dev/null | wc -l | tr -d ' ')

if [ "$CRASH_COUNT" -gt 0 ]; then
	echo "WARNING: Crash logs were created (OBS may have restarted)"

	find "$CRASH_LOG_DIR" -name "OBS*.crash" -newer "$MARKER_FILE" 2>/dev/null | while read crash_file; do
		echo "  - $crash_file"
	done

	rm -f "$MARKER_FILE"
	exit 1
else
	echo "✓ No crash logs generated"
fi

# Stop OBS
echo "Stopping OBS..."
kill $OBS_PID 2>/dev/null || true
sleep 2

if ps -p $OBS_PID >/dev/null 2>&1; then
	kill -9 $OBS_PID 2>/dev/null || true
fi

# Cleanup
rm -f "$MARKER_FILE"

echo "Crash detection test completed"
exit 0
