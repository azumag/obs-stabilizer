#!/bin/bash

# Build step 1 plugin for testing
echo "Building minimal step 1 OBS plugin..."

# First, modify CMakeLists.txt temporarily
cp CMakeLists.txt CMakeLists.txt.bak
sed -i '' 's/src\/minimal_safe_plugin.cpp/src\/minimal_step1_plugin.cpp/g' CMakeLists.txt

# Clean up previous build
rm -rf build-step1
mkdir build-step1
cd build-step1

# Configure with CMake (minimal version without OpenCV)
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DOBS_DIR=/Applications/OBS.app/Contents/Resources \
    -DOBS_INCLUDE_DIR=/Users/azumag/work/obs-stabilizer/include/obs

# Build
make -j$(sysctl -n hw.ncpu)

if [ $? -eq 0 ]; then
    echo "Plugin built successfully!"
    
    # Backup existing plugin if it exists
    PLUGIN_DIR="$HOME/Library/Application Support/obs-studio/plugins"
    if [ -d "$PLUGIN_DIR/test-stabilizer.plugin" ]; then
        echo "Backing up existing plugin..."
        mv "$PLUGIN_DIR/test-stabilizer.plugin" "$PLUGIN_DIR/test-stabilizer.plugin.bak.$(date +%Y%m%d_%H%M%S)"
    fi
    
    # Install plugin
    echo "Installing plugin..."
    cp -R test-stabilizer.plugin "$PLUGIN_DIR/"
    
    echo "Plugin installed successfully!"
    echo "Restart OBS to load the updated plugin."
    
    # Show plugin info
    echo ""
    echo "Installed plugin info:"
    file "$PLUGIN_DIR/test-stabilizer.plugin/Contents/MacOS/test-stabilizer"
    otool -L "$PLUGIN_DIR/test-stabilizer.plugin/Contents/MacOS/test-stabilizer" | head -5
    
    # Restore CMakeLists.txt
    cd ..
    mv CMakeLists.txt.bak CMakeLists.txt
else
    echo "Build failed!"
    # Restore CMakeLists.txt
    cd ..
    mv CMakeLists.txt.bak CMakeLists.txt
    exit 1
fi