#!/bin/bash

# Create DEB package for OBS Stabilizer Plugin (Linux)

set -e

VERSION="${1:-dev-build}"
PACKAGE_NAME="obs-stabilizer"
ARCH="amd64"

echo "Creating DEB package for $PACKAGE_NAME version $VERSION"

# Create package directory structure
PKG_DIR="build/deb-package"
mkdir -p "$PKG_DIR/DEBIAN"
mkdir -p "$PKG_DIR/usr/lib/obs-plugins"
mkdir -p "$PKG_DIR/usr/share/obs/obs-plugins/$PACKAGE_NAME"
mkdir -p "$PKG_DIR/usr/share/doc/$PACKAGE_NAME"

# Copy and verify plugin binary
if ls build/src/*.so 1> /dev/null 2>&1; then
    for so_file in build/src/*.so; do
        # Verify binary is valid ELF and not corrupted
        if file "$so_file" | grep -q "ELF.*shared object"; then
            echo "Verified: $so_file is a valid shared library"
            cp "$so_file" "$PKG_DIR/usr/lib/obs-plugins/"
        else
            echo "ERROR: $so_file is not a valid ELF shared object"
            exit 1
        fi
        
        # Check for required symbols
        if objdump -T "$so_file" | grep -q "obs_module_load"; then
            echo "Verified: $so_file contains required OBS symbols"
        else
            echo "WARNING: $so_file may not be a valid OBS plugin"
        fi
    done
else
    echo "ERROR: No .so files found in build/src/"
    exit 1
fi

# Copy documentation
cp README.md "$PKG_DIR/usr/share/doc/$PACKAGE_NAME/"
cp LICENSE "$PKG_DIR/usr/share/doc/$PACKAGE_NAME/"

# Create control file
cat > "$PKG_DIR/DEBIAN/control" << EOF
Package: $PACKAGE_NAME
Version: $VERSION
Section: video
Priority: optional
Architecture: $ARCH
Depends: obs-studio (>= 30.0), libopencv-core4.5d | libopencv-core4.6 | libopencv-core4.8
Maintainer: azumag <azumag@users.noreply.github.com>
Description: Real-time video stabilization plugin for OBS Studio
 OBS Stabilizer provides real-time video stabilization for OBS Studio
 using advanced computer vision algorithms. Features include:
 - Real-time stabilization with minimal latency
 - Configurable smoothing and quality settings  
 - Cross-platform support (Windows, macOS, Linux)
 - Integration with OBS Studio properties panel
Homepage: https://github.com/azumag/obs-stabilizer
EOF

# Create postinst script for plugin activation
cat > "$PKG_DIR/DEBIAN/postinst" << 'EOF'
#!/bin/bash
set -e

echo "OBS Stabilizer plugin installed successfully"
echo "Please restart OBS Studio to activate the plugin"

# Set proper permissions
chmod 755 /usr/lib/obs-plugins/*.so 2>/dev/null || true

exit 0
EOF

chmod 755 "$PKG_DIR/DEBIAN/postinst"

# Build the package
PACKAGE_FILE="${PACKAGE_NAME}_${VERSION}_${ARCH}.deb"
dpkg-deb --build "$PKG_DIR" "$PACKAGE_FILE"

echo "DEB package created: $PACKAGE_FILE"
echo "Install with: sudo dpkg -i $PACKAGE_FILE"