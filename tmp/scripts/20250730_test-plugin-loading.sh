#!/bin/bash
# Test script to launch OBS with plugin debugging enabled

echo "Testing OBS Stabilizer Plugin Loading"
echo "======================================"

echo "1. Plugin location and structure:"
ls -la ~/Library/Application\ Support/obs-studio/plugins/obs-stabilizer.plugin/Contents/MacOS/
echo

echo "2. Plugin dependencies:"
otool -L ~/Library/Application\ Support/obs-studio/plugins/obs-stabilizer.plugin/Contents/MacOS/obs-stabilizer
echo

echo "3. Plugin symbols:"
nm ~/Library/Application\ Support/obs-studio/plugins/obs-stabilizer.plugin/Contents/MacOS/obs-stabilizer | grep obs_module
echo

echo "4. Code signature:"
codesign -vvv ~/Library/Application\ Support/obs-studio/plugins/obs-stabilizer.plugin/Contents/MacOS/obs-stabilizer
echo

echo "5. Launch OBS with verbose logging (will check logs after):"
echo "Please check ~/Library/Application Support/obs-studio/logs/ for the latest log file"

# Alternative: Try setting environment variables for plugin loading
export OBS_PLUGIN_PATH="$HOME/Library/Application Support/obs-studio/plugins"
export DYLD_PRINT_LIBRARIES=1  # This might help debug library loading issues

/Applications/OBS.app/Contents/MacOS/OBS &
OBS_PID=$!

echo "OBS launched with PID: $OBS_PID"
echo "Waiting 10 seconds for startup..."
sleep 10

echo "Checking latest log file..."
latest_log=$(ls -t ~/Library/Application\ Support/obs-studio/logs/*.txt | head -1)
echo "Latest log: $latest_log"

if [ -f "$latest_log" ]; then
    echo "Checking for obs-stabilizer mentions:"
    grep -i "stabilizer" "$latest_log" || echo "No stabilizer mentions found"
    echo
    echo "Loaded modules section:"
    sed -n '/Loaded Modules:/,/^-*$/p' "$latest_log"
fi

# Kill OBS
kill $OBS_PID 2>/dev/null
echo "Test complete."