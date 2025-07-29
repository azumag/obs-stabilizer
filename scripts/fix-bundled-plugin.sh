#!/bin/bash

# Script to replace the bundled OBS stabilizer plugin with our fixed version
# This fixes the C++ name mangling issue that prevents the bundled plugin from loading

BUNDLED_PLUGIN="/Applications/OBS.app/Contents/PlugIns/obs-stabilizer.plugin"
OUR_PLUGIN="/Users/azumag/Library/Application Support/obs-studio/plugins/obs-stabilizer.plugin"
BACKUP_SUFFIX=".backup.$(date +%Y%m%d_%H%M%S)"

echo "OBS Stabilizer Plugin Fix Script"
echo "================================"
echo "This script will replace the bundled OBS stabilizer plugin with our fixed version."
echo "The bundled plugin has C++ name mangling issues that prevent it from loading."
echo ""

# Check if bundled plugin exists
if [ ! -d "$BUNDLED_PLUGIN" ]; then
    echo "Error: Bundled plugin not found at $BUNDLED_PLUGIN"
    exit 1
fi

# Check if our plugin exists
if [ ! -d "$OUR_PLUGIN" ]; then
    echo "Error: Our fixed plugin not found at $OUR_PLUGIN"
    exit 1
fi

echo "Found bundled plugin: $BUNDLED_PLUGIN"
echo "Found our fixed plugin: $OUR_PLUGIN"
echo ""

# Show the difference in symbols
echo "Comparing plugin symbols:"
echo "Bundled plugin symbols:"
nm -g "$BUNDLED_PLUGIN/Contents/MacOS/obs-stabilizer" | grep -E "obs_module_" | head -5
echo ""
echo "Our fixed plugin symbols:"
nm -g "$OUR_PLUGIN/Contents/MacOS/obs-stabilizer" | grep -E "obs_module_" | head -5
echo ""

echo "The bundled plugin has C++ mangled symbols (starts with __Z) which prevent OBS from loading it."
echo "Our fixed plugin has proper C linkage symbols (starts with _obs_module) which OBS can load."
echo ""

echo "This script needs administrator privileges to replace the bundled plugin."
echo "Please run the following commands manually:"
echo ""
echo "sudo mv '$BUNDLED_PLUGIN' '${BUNDLED_PLUGIN}${BACKUP_SUFFIX}'"
echo "sudo cp -r '$OUR_PLUGIN' '$BUNDLED_PLUGIN'"
echo "sudo chmod -R 755 '$BUNDLED_PLUGIN'"
echo ""
echo "After running these commands, restart OBS and the stabilizer plugin should load properly."