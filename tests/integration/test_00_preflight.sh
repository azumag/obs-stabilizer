#!/bin/bash

# Test 0: Pre-flight checks
# Verifies environment setup before running tests

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "Running pre-flight checks..."

# Check 1: OBS is installed
if [ ! -d "/Applications/OBS.app" ]; then
	echo "ERROR: OBS Studio is not installed"
	echo "Please install OBS Studio from https://obsproject.com/"
	exit 1
fi
echo "✓ OBS Studio found"

# Check 2: Plugin directory exists
OBS_PLUGINS_DIR="$HOME/Library/Application Support/obs-studio/plugins"
if [ ! -d "$OBS_PLUGINS_DIR" ]; then
	echo "ERROR: OBS plugins directory not found: $OBS_PLUGINS_DIR"
	exit 1
fi
echo "✓ OBS plugins directory exists"

# Check 3: CMake is available
if ! command -v cmake &>/dev/null; then
	echo "ERROR: CMake is not installed"
	exit 1
fi
echo "✓ CMake is available"

# Check 4: Build tools
if ! command -v ninja &>/dev/null && ! command -v make &>/dev/null; then
	echo "ERROR: No build tool found (ninja or make required)"
	exit 1
fi
echo "✓ Build tool available"

# Check 5: jq is available (for JSON results)
if ! command -v jq &>/dev/null; then
	echo "WARNING: jq is not installed. JSON results will be limited."
	echo "Install with: brew install jq"
else
	echo "✓ jq is available"
fi

# Check 6: OBS is not running
if pgrep -x "OBS" >/dev/null; then
	echo "ERROR: OBS is currently running. Please close OBS before running tests."
	exit 1
fi
echo "✓ OBS is not running"

# Check 7: Check for existing problematic plugins
PROBLEMATIC_PLUGIN="$OBS_PLUGINS_DIR/obs-stabilizer-minimal.plugin"
if [ -d "$PROBLEMATIC_PLUGIN" ]; then
	echo "WARNING: Problematic plugin detected: $PROBLEMATIC_PLUGIN"
	echo "This plugin may cause OBS crashes. It will be removed automatically."
	exit 2 # Exit with code 2 to indicate fix is needed
fi
echo "✓ No problematic plugins detected"

# Check 8: Check for proper CMakeLists.txt
if [ ! -f "$PROJECT_ROOT/CMakeLists.txt" ]; then
	echo "ERROR: CMakeLists.txt not found at project root"
	exit 1
fi
echo "✓ CMakeLists.txt found"

# Check 9: Check source files
if [ ! -d "$PROJECT_ROOT/src" ]; then
	echo "ERROR: src directory not found"
	exit 1
fi
echo "✓ Source directory exists"

echo "All pre-flight checks passed"
exit 0
