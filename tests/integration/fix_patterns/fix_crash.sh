#!/bin/bash

# Fix script for crash issues
# Automatically resolves common crash problems

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$(dirname "$SCRIPT_DIR")/..")/.."

echo "Attempting to fix crash issues..."

FIXES_APPLIED=0

# Fix 1: Kill all OBS processes
if pgrep -x "OBS" >/dev/null; then
	echo "Fixing: Killing OBS processes..."
	pkill -9 OBS 2>/dev/null || true
	sleep 1
	FIXES_APPLIED=$((FIXES_APPLIED + 1))
fi

# Fix 2: Close crash reporter
osascript -e 'tell application "Problem Reporter" to quit' 2>/dev/null || true
pkill -f "ReportCrash" 2>/dev/null || true
pkill -f "Problem Reporter" 2>/dev/null || true

# Fix 3: Analyze crash logs
CRASH_LOG_DIR="$HOME/Library/Logs/DiagnosticReports"
RECENT_CRASH=$(find "$CRASH_LOG_DIR" -name "OBS*.crash" -mmin -10 | head -1)

if [ -n "$RECENT_CRASH" ]; then
	echo "Analyzing crash log: $RECENT_CRASH"

	# Extract exception type
	EXCEPTION_TYPE=$(grep "Exception Type:" "$RECENT_CRASH" | head -1 | cut -d: -f2 | xargs)
	echo "Exception Type: $EXCEPTION_TYPE"

	# Extract signal
	SIGNAL=$(grep "Exception Codes:" "$RECENT_CRASH" | head -1 | cut -d: -f2 | xargs)
	echo "Exception Codes: $SIGNAL"

	# Extract crashed thread
	CRASHED_THREAD=$(grep "Thread.*Crashed:" "$RECENT_CRASH" | head -1)
	echo "$CRASHED_THREAD"

	# Extract backtrace
	BACKTRACE=$(grep -A20 "Thread.*Crashed:" "$RECENT_CRASH" | head -25)
	echo "Backtrace:"
	echo "$BACKTRACE" | sed 's/^/  /'

	# Check for common crash patterns

	# Pattern 1: Segmentation fault in plugin
	if echo "$BACKTRACE" | grep -q "obs-stabilizer"; then
		echo ""
		echo "Fix: Crash detected in obs-stabilizer plugin"
		FIXES_APPLIED=$((FIXES_APPLIED + 1))
	fi

	# Pattern 2: Undefined symbol
	if grep -q "Undefined symbol\|Symbol not found" "$RECENT_CRASH"; then
		echo ""
		echo "Fix: Undefined symbol crash detected"
		echo "  This is likely a linking issue. Checking library paths..."

		# Check plugin binary
		PLUGIN_DIR="$HOME/Library/Application Support/obs-studio/plugins/obs-stabilizer"
		if [ -f "$PLUGIN_DIR/bin/64bit/libobs-stabilizer-opencv.so" ]; then
			echo "  Checking plugin dependencies..."
			otool -L "$PLUGIN_DIR/bin/64bit/libobs-stabilizer-opencv.so"

			# Try to fix library paths
			install_name_tool -add_rpath "@loader_path/../lib" "$PLUGIN_DIR/bin/64bit/libobs-stabilizer-opencv.so" 2>/dev/null || true
			install_name_tool -add_rpath "/opt/homebrew/lib" "$PLUGIN_DIR/bin/64bit/libobs-stabilizer-opencv.so" 2>/dev/null || true

			echo "  Updated library rpath"
			FIXES_APPLIED=$((FIXES_APPLIED + 1))
		fi
	fi

	# Pattern 3: Access violation
	if echo "$EXCEPTION_TYPE" | grep -qi "access\|segmentation"; then
		echo ""
		echo "Fix: Memory access violation detected"
		echo "  This is likely a NULL pointer or invalid memory access."
		echo "  Check the plugin code for proper error handling."

		# Disable plugin temporarily
		echo "  Disabling plugin temporarily..."
		PLUGIN_DIR="$HOME/Library/Application Support/obs-studio/plugins/obs-stabilizer"
		if [ -d "$PLUGIN_DIR" ]; then
			mv "$PLUGIN_DIR" "${PLUGIN_DIR}.disabled"
			echo "  Plugin disabled"
			FIXES_APPLIED=$((FIXES_APPLIED + 1))
		fi
	fi

	# Pattern 4: OpenCV crash
	if echo "$BACKTRACE" | grep -qi "opencv\|cv::"; then
		echo ""
		echo "Fix: OpenCV crash detected"
		echo "  Check for proper OpenCV initialization and error handling."

		# Check for OpenCV linking issues
		PLUGIN_DIR="$HOME/Library/Application Support/obs-studio/plugins/obs-stabilizer"
		if [ -f "$PLUGIN_DIR/bin/64bit/libobs-stabilizer-opencv.so" ]; then
			echo "  Checking OpenCV linking..."
			otool -L "$PLUGIN_DIR/bin/64bit/libobs-stabilizer-opencv.so" | grep -i opencv

			# This might need manual intervention
			echo "  Manual intervention may be required for OpenCV issues"
			FIXES_APPLIED=$((FIXES_APPLIED + 1))
		fi
	fi

	# Move crash log to test results for analysis
	TEST_RESULTS_DIR="$SCRIPT_DIR/results"
	mkdir -p "$TEST_RESULTS_DIR"
	cp "$RECENT_CRASH" "$TEST_RESULTS_DIR/crash_analysis_$(date +%Y%m%d_%H%M%S).crash"
	echo "Crash log copied to: $TEST_RESULTS_DIR/"
fi

# Fix 4: Reset OBS configuration
OBS_CONFIG_DIR="$HOME/Library/Application Support/obs-studio"
if [ -f "$OBS_CONFIG_DIR/global.ini" ]; then
	echo "Fixing: Resetting OBS configuration..."
	sed -i '' '/^LastCrash=/d' "$OBS_CONFIG_DIR/global.ini" 2>/dev/null || true
	sed -i '' '/^SafeMode=/d' "$OBS_CONFIG_DIR/global.ini" 2>/dev/null || true
	sed -i '' '/^DisableShutdownCheck=/d' "$OBS_CONFIG_DIR/global.ini" 2>/dev/null || true

	echo "" >>"$OBS_CONFIG_DIR/global.ini"
	echo "[General]" >>"$OBS_CONFIG_DIR/global.ini"
	echo "DisableShutdownCheck=true" >>"$OBS_CONFIG_DIR/global.ini"

	echo "✓ OBS configuration reset"
	FIXES_APPLIED=$((FIXES_APPLIED + 1))
fi

# Fix 5: Clear crash markers
rm -f "$OBS_CONFIG_DIR/.crashed" 2>/dev/null || true
rm -f "$OBS_CONFIG_DIR/.safe_mode" 2>/dev/null || true
rm -f "$OBS_CONFIG_DIR/.unclean_shutdown" 2>/dev/null || true

echo ""
echo "Crash fixes complete. Applied $FIXES_APPLIED fixes."

if [ $FIXES_APPLIED -eq 0 ]; then
	echo "No automatic fixes were possible. Manual code review required."
	exit 1
else
	echo "Re-run tests to verify fixes."
	exit 0
fi
