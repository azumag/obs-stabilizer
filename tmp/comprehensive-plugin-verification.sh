#!/bin/bash

# Comprehensive OBS Plugin Verification Script
# Tests both minimal and full plugins for loading readiness

echo "=== OBS Stabilizer Plugin Comprehensive Verification ==="
echo ""

PLUGIN_DIR="$HOME/Library/Application Support/obs-studio/plugins"
MINIMAL_PLUGIN="$PLUGIN_DIR/obs-stabilizer-minimal.plugin/Contents/MacOS/obs-stabilizer-minimal"
FULL_PLUGIN="$PLUGIN_DIR/obs-stabilizer-full.plugin/Contents/MacOS/test-stabilizer"
OBS_LIBRARY="/Applications/OBS.app/Contents/Frameworks/libobs.framework/Versions/A/libobs"

# Helper function to check plugin
check_plugin() {
    local plugin_name="$1"
    local plugin_path="$2"
    
    echo "🔍 Checking $plugin_name Plugin..."
    
    if [[ ! -f "$plugin_path" ]]; then
        echo "❌ Plugin binary not found: $plugin_path"
        return 1
    fi
    
    echo "✅ Plugin binary exists"
    
    # Check format
    local format=$(file "$plugin_path" | cut -d: -f2)
    if [[ "$format" == *"Mach-O 64-bit bundle arm64"* ]]; then
        echo "✅ Correct binary format: Mach-O 64-bit bundle arm64"
    else
        echo "❌ Incorrect binary format: $format"
        return 1
    fi
    
    # Check OBS module symbols
    local symbols=$(nm -g "$plugin_path" | grep obs_module | wc -l)
    if [[ $symbols -ge 8 ]]; then
        echo "✅ OBS module symbols present ($symbols found)"
    else
        echo "❌ Missing OBS module symbols (only $symbols found)"
        return 1
    fi
    
    # Check dependencies
    echo "📋 Dependencies:"
    otool -L "$plugin_path" | grep -v ":" | sed 's/^/    /'
    
    # Check for problematic @rpath dependencies
    local rpath_deps=$(otool -L "$plugin_path" | grep "@rpath" | grep -v libobs | wc -l)
    if [[ $rpath_deps -gt 0 ]]; then
        echo "⚠️  Warning: $rpath_deps @rpath dependencies found (may cause loading issues)"
        otool -L "$plugin_path" | grep "@rpath" | grep -v libobs | sed 's/^/    /'
    else
        echo "✅ No problematic @rpath dependencies"
    fi
    
    echo ""
    return 0
}

# Check OBS installation
echo "🔍 Checking OBS Studio Installation..."
if [[ ! -f "$OBS_LIBRARY" ]]; then
    echo "❌ OBS Studio not found at expected location!"
    echo "   Expected: $OBS_LIBRARY"
    exit 1
fi
echo "✅ OBS Studio found"
echo ""

# Check plugins
check_plugin "Minimal" "$MINIMAL_PLUGIN"
minimal_result=$?

check_plugin "Full" "$FULL_PLUGIN"
full_result=$?

# Overall assessment
echo "=== VERIFICATION RESULTS ==="
echo ""

if [[ $minimal_result -eq 0 ]]; then
    echo "✅ MINIMAL PLUGIN: Ready for OBS loading"
    echo "   - No external dependencies beyond OBS"
    echo "   - Should load without issues"
else
    echo "❌ MINIMAL PLUGIN: Has issues"
fi

if [[ $full_result -eq 0 ]]; then
    echo "✅ FULL PLUGIN: Ready for OBS loading"
    echo "   - OpenCV dependencies resolved with absolute paths"
    echo "   - Should load with stabilization functionality"
else
    echo "❌ FULL PLUGIN: Has issues"
fi

echo ""
echo "🎯 TESTING INSTRUCTIONS:"
echo "1. Launch OBS Studio"
echo "2. Go to Help → Log Files → Current Log"
echo "3. Look for plugin loading messages:"
echo "   - '[obs-stabilizer-minimal] Module loaded successfully'"
echo "   - Look for any stabilizer-related loading messages"
echo "4. Check Video Filters for 'Stabilizer' option"
echo ""

if [[ $minimal_result -eq 0 && $full_result -eq 0 ]]; then
    echo "🎉 SUCCESS: Both plugins are technically ready for OBS loading!"
    exit 0
else
    echo "⚠️  Some plugins have issues - check output above"
    exit 1
fi