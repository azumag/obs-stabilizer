#!/bin/bash

# OBS Stabilizer Plugin - Cross-platform Installer Creation Script
# Creates platform-specific installers from release artifacts

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "=== OBS Stabilizer Installer Creation ==="

# Check if we're in a release context
if [ -z "$GITHUB_REF" ]; then
    echo "Warning: Not in GitHub Actions context, using default version"
    VERSION="dev-build"
else
    VERSION="${GITHUB_REF#refs/tags/}"
fi

echo "Creating installers for version: $VERSION"

# Platform detection
case "$(uname -s)" in
    Linux*)
        echo "Creating Linux DEB package..."
        "$SCRIPT_DIR/linux-deb.sh" "$VERSION"
        ;;
    Darwin*)
        echo "Creating macOS PKG installer..."
        "$SCRIPT_DIR/macos-pkg.sh" "$VERSION"
        ;;
    MINGW*|CYGWIN*|MSYS*)
        echo "Creating Windows NSIS installer..."
        "$SCRIPT_DIR/windows-nsis.sh" "$VERSION"
        ;;
    *)
        echo "Unknown platform: $(uname -s)"
        exit 1
        ;;
esac

echo "Installer creation complete for $VERSION"