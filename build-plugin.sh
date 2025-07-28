#!/bin/bash

# OBS Stabilizer Plugin Build Script for macOS
# Usage: ./build-plugin.sh

set -e

echo "=== OBS Stabilizer Plugin Build Script ==="
echo "Building OBS Stabilizer Plugin for macOS..."

# Clean build directory - use consolidated tmp location
echo "Cleaning build directory..."
rm -rf tmp/builds/build
mkdir -p tmp/builds/build

# Configure with CMake
echo "Configuring with CMake..."
cd tmp/builds/build
cmake ../../.. -DCMAKE_BUILD_TYPE=RelWithDebInfo

# Build the plugin
echo "Building plugin..."
make -j$(sysctl -n hw.ncpu)

# Check if plugin bundle was created
if [ -d "obs-stabilizer.plugin" ]; then
    echo "✅ Plugin bundle created successfully: obs-stabilizer.plugin"
    
    # Show plugin structure
    echo "Plugin structure:"
    find obs-stabilizer.plugin -type f | head -10
    
    echo ""
    echo "🎯 Next steps:"
    echo "1. Remove old files from OBS plugin directory:"
    echo "   sudo rm -f '/Applications/OBS.app/Contents/PlugIns/obs-stabilizer'"
    echo "   sudo rm -f '/Applications/OBS.app/Contents/PlugIns/obs-stabilizer-0.1.0'"
    echo ""
    echo "2. Copy the plugin bundle to OBS:"
    echo "   sudo cp -r tmp/builds/build/obs-stabilizer.plugin '/Applications/OBS.app/Contents/PlugIns/'"
    echo ""
    echo "3. Set correct permissions:"
    echo "   sudo chmod -R 755 '/Applications/OBS.app/Contents/PlugIns/obs-stabilizer.plugin'"
    echo ""
    echo "4. Restart OBS Studio"
    echo ""
    echo "The 'Stabilizer' filter should now appear in Video Filters menu."
    
else
    echo "❌ Plugin bundle was not created. Check for build errors above."
    
    # Check if regular library was created instead
    if [ -f "obs-stabilizer" ] || [ -f "libobs-stabilizer.dylib" ]; then
        echo ""
        echo "⚠️  A regular library/executable was created instead of plugin bundle."
        echo "This happens when OBS headers are not found properly."
        echo "Please check the CMake configuration output above."
    fi
fi

echo ""
echo "Build process completed."