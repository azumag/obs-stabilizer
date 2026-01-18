#!/bin/bash

# Build minimal safe plugin 
echo "Building minimal safe OBS plugin..."

# Clean any previous builds
rm -rf build-safe
mkdir -p build-safe

# Use the main CMakeLists.txt but build only the safe plugin
cd build-safe

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES=arm64

# Build
make -j4

# Check if build succeeded
if [ -f "test-stabilizer.plugin/Contents/MacOS/test-stabilizer" ]; then
    echo "Plugin built successfully!"
    
    # Backup existing plugin if present
    if [ -d "$HOME/Library/Application Support/obs-studio/plugins/test-stabilizer.plugin" ]; then
        echo "Backing up existing plugin..."
        mv "$HOME/Library/Application Support/obs-studio/plugins/test-stabilizer.plugin" \
           "$HOME/Library/Application Support/obs-studio/plugins/test-stabilizer.plugin.backup.$(date +%Y%m%d_%H%M%S)"
    fi
    
    # Install to user plugins
    cp -R "test-stabilizer.plugin" "$HOME/Library/Application Support/obs-studio/plugins/"
    
    echo "Plugin installed successfully!"
    echo "Restart OBS to load the updated plugin."
    
    # Verify installation
    echo ""
    echo "Installed plugin info:"
    file "$HOME/Library/Application Support/obs-studio/plugins/test-stabilizer.plugin/Contents/MacOS/test-stabilizer"
    otool -L "$HOME/Library/Application Support/obs-studio/plugins/test-stabilizer.plugin/Contents/MacOS/test-stabilizer" | head -5
else
    echo "Plugin build failed!"
    exit 1
fi