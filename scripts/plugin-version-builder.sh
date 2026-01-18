#!/bin/bash

# Plugin Version Builder
# Builds specific plugin versions with unique identifiers

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
PLUGIN_VERSIONS_DIR="$PROJECT_ROOT/plugin-versions"
BUILD_DIR="$PROJECT_ROOT/tmp/version-builds"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

show_usage() {
    echo "Usage: $0 <version> [options]"
    echo ""
    echo "Available versions:"
    echo "  step0-minimal      - Baseline minimal plugin (no crashes)"
    echo "  step1-parameters   - Add parameter reading"
    echo "  step2-frame-access - Add frame data access"
    echo "  step3-opencv-basic - Add OpenCV integration"
    echo "  step4-processing   - Add video processing"
    echo ""
    echo "Options:"
    echo "  --clean           - Clean build directory first"
    echo "  --install         - Install plugin after building"
    echo "  --help            - Show this help"
    echo ""
    echo "Example: $0 step1-parameters --clean --install"
}

validate_version() {
    local version="$1"
    if [ ! -d "$PLUGIN_VERSIONS_DIR/$version" ]; then
        log_error "Version '$version' not found in $PLUGIN_VERSIONS_DIR"
        echo ""
        show_usage
        exit 1
    fi
    
    if [ ! -f "$PLUGIN_VERSIONS_DIR/$version/plugin.json" ]; then
        log_error "Plugin metadata file not found: $PLUGIN_VERSIONS_DIR/$version/plugin.json"
        exit 1
    fi
}

load_plugin_config() {
    local version="$1"
    local config_file="$PLUGIN_VERSIONS_DIR/$version/plugin.json"
    
    # Extract values from JSON (basic parsing)
    FILTER_ID=$(grep '"filter_id"' "$config_file" | cut -d'"' -f4)
    FILTER_NAME=$(grep '"filter_name"' "$config_file" | cut -d'"' -f4)
    PLUGIN_NAME=$(grep '"plugin_name"' "$config_file" | cut -d'"' -f4)
    BUNDLE_ID=$(grep '"bundle_id"' "$config_file" | cut -d'"' -f4)
    
    log_info "Loaded config for $version:"
    log_info "  Filter ID: $FILTER_ID"
    log_info "  Filter Name: $FILTER_NAME"
    log_info "  Plugin Name: $PLUGIN_NAME"
    log_info "  Bundle ID: $BUNDLE_ID"
}

create_version_source() {
    local version="$1"
    local build_version_dir="$BUILD_DIR/$version"
    
    log_info "Creating version-specific source code..."
    
    # Create build directory for this version
    mkdir -p "$build_version_dir/src"
    
    # Determine base source file
    case "$version" in
        "step0-minimal")
            BASE_SOURCE="$PROJECT_ROOT/src/minimal_safe_plugin.cpp"
            ;;
        "step1-parameters")
            BASE_SOURCE="$PROJECT_ROOT/src/minimal_step1_plugin.cpp"
            ;;
        "step2-frame-access"|"step3-opencv-basic"|"step4-processing")
            # These will extend step1 - for now use step1 as base
            BASE_SOURCE="$PROJECT_ROOT/src/minimal_step1_plugin.cpp"
            ;;
        *)
            log_error "Unknown version: $version"
            exit 1
            ;;
    esac
    
    if [ ! -f "$BASE_SOURCE" ]; then
        log_error "Base source file not found: $BASE_SOURCE"
        exit 1
    fi
    
    # Copy and modify source file
    local output_source="$build_version_dir/src/${PLUGIN_NAME}.cpp"
    cp "$BASE_SOURCE" "$output_source"
    
    # Update identifiers in source code
    sed -i '' "s/test_stabilizer_minimal_filter/$FILTER_ID/g" "$output_source"
    sed -i '' "s/test_stabilizer_step1_filter/$FILTER_ID/g" "$output_source"
    sed -i '' "s/\"Minimal Safe Filter\"/\"$FILTER_NAME\"/g" "$output_source"
    sed -i '' "s/\"Minimal Step 1 Filter\"/\"$FILTER_NAME\"/g" "$output_source"
    sed -i '' "s/test-stabilizer/$PLUGIN_NAME/g" "$output_source"
    sed -i '' "s/test-stabilizer-step1/$PLUGIN_NAME/g" "$output_source"
    
    log_success "Version-specific source created: $output_source"
    
    # Copy additional required files
    if [ -f "$PROJECT_ROOT/src/obs_module_exports.c" ]; then
        cp "$PROJECT_ROOT/src/obs_module_exports.c" "$build_version_dir/src/"
    fi
    if [ -f "$PROJECT_ROOT/src/plugin-support.c" ]; then
        cp "$PROJECT_ROOT/src/plugin-support.c" "$build_version_dir/src/"
    fi
    if [ -f "$PROJECT_ROOT/src/plugin-support.h" ]; then
        cp "$PROJECT_ROOT/src/plugin-support.h" "$build_version_dir/src/"
    fi
    
    # Copy include directory
    if [ -d "$PROJECT_ROOT/include" ]; then
        cp -r "$PROJECT_ROOT/include" "$build_version_dir/"
    fi
}

create_version_cmake() {
    local version="$1"
    local build_version_dir="$BUILD_DIR/$version"
    
    log_info "Creating version-specific CMake configuration..."
    
    # Create CMakeLists.txt for this version
    cat > "$build_version_dir/CMakeLists.txt" << EOF
cmake_minimum_required(VERSION 3.16)
project($PLUGIN_NAME)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OBS
if(APPLE)
    set(OBS_INCLUDE_DIRS "/Applications/OBS.app/Contents/Resources/include")
    set(OBS_LIBRARY "/Applications/OBS.app/Contents/Frameworks/libobs.framework/Versions/A/libobs")
endif()

if(NOT OBS_INCLUDE_DIRS OR NOT EXISTS "\${OBS_INCLUDE_DIRS}")
    message(FATAL_ERROR "OBS include directories not found")
endif()

if(NOT OBS_LIBRARY OR NOT EXISTS "\${OBS_LIBRARY}")
    message(FATAL_ERROR "OBS library not found")
endif()

# Include directories
include_directories("\${OBS_INCLUDE_DIRS}")
include_directories("include")

# Source files
set(SOURCES
    "src/${PLUGIN_NAME}.cpp"
    "src/obs_module_exports.c"
    "src/plugin-support.c"
)

# Create shared library
add_library(\${PROJECT_NAME} SHARED \${SOURCES})

# Link libraries
target_link_libraries(\${PROJECT_NAME} "\${OBS_LIBRARY}")

# Set bundle properties
set_target_properties(\${PROJECT_NAME} PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION "plugin"
    MACOSX_BUNDLE_BUNDLE_NAME "\${PROJECT_NAME}"
    MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
    MACOSX_BUNDLE_IDENTIFIER "$BUNDLE_ID"
)

# Install rules
install(TARGETS \${PROJECT_NAME}
    BUNDLE DESTINATION "/Applications/OBS.app/Contents/PlugIns"
)
EOF

    # Create Info.plist
    cat > "$build_version_dir/Info.plist.in" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleIdentifier</key>
    <string>$BUNDLE_ID</string>
    <key>CFBundleName</key>
    <string>$PLUGIN_NAME</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0.0</string>
    <key>CFBundleExecutable</key>
    <string>$PLUGIN_NAME</string>
</dict>
</plist>
EOF

    log_success "Version-specific CMake configuration created"
}

build_version() {
    local version="$1"
    local build_version_dir="$BUILD_DIR/$version"
    
    log_info "Building plugin version: $version"
    
    cd "$build_version_dir"
    
    # Create build subdirectory
    mkdir -p build
    cd build
    
    # Configure with CMake
    log_info "Configuring with CMake..."
    if ! cmake ..; then
        log_error "CMake configuration failed"
        exit 1
    fi
    
    # Build
    log_info "Building..."
    if ! make -j$(nproc 2>/dev/null || echo 4); then
        log_error "Build failed"
        exit 1
    fi
    
    log_success "Build completed successfully"
    
    # Verify build output
    local plugin_bundle="${PLUGIN_NAME}.plugin"
    if [ -d "$plugin_bundle" ]; then
        log_success "Plugin bundle created: $plugin_bundle"
        
        # Show bundle contents
        log_info "Bundle contents:"
        find "$plugin_bundle" -type f | head -10
    else
        log_error "Plugin bundle not found: $plugin_bundle"
        exit 1
    fi
}

install_version() {
    local version="$1"
    local build_version_dir="$BUILD_DIR/$version"
    local plugin_bundle="${PLUGIN_NAME}.plugin"
    local install_target="/Applications/OBS.app/Contents/PlugIns/$plugin_bundle"
    
    log_info "Installing plugin version: $version"
    
    cd "$build_version_dir/build"
    
    if [ ! -d "$plugin_bundle" ]; then
        log_error "Plugin bundle not found: $plugin_bundle"
        exit 1
    fi
    
    # Remove existing installation
    if [ -d "$install_target" ]; then
        log_info "Removing existing installation: $install_target"
        sudo rm -rf "$install_target"
    fi
    
    # Install new version
    log_info "Installing to: $install_target"
    sudo cp -r "$plugin_bundle" "/Applications/OBS.app/Contents/PlugIns/"
    
    log_success "Plugin installed successfully"
    
    # Verify installation
    if [ -d "$install_target" ]; then
        log_success "Installation verified: $install_target"
    else
        log_error "Installation verification failed"
        exit 1
    fi
}

# Main script logic
main() {
    local version=""
    local clean_build=false
    local install_after_build=false
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --clean)
                clean_build=true
                shift
                ;;
            --install)
                install_after_build=true
                shift
                ;;
            --help)
                show_usage
                exit 0
                ;;
            *)
                if [ -z "$version" ]; then
                    version="$1"
                else
                    log_error "Unknown option: $1"
                    show_usage
                    exit 1
                fi
                shift
                ;;
        esac
    done
    
    # Validate arguments
    if [ -z "$version" ]; then
        log_error "Version not specified"
        show_usage
        exit 1
    fi
    
    # Validate version exists
    validate_version "$version"
    
    # Load plugin configuration
    load_plugin_config "$version"
    
    # Clean build directory if requested
    if [ "$clean_build" = true ]; then
        log_info "Cleaning build directory..."
        rm -rf "$BUILD_DIR/$version"
    fi
    
    # Create build directory
    mkdir -p "$BUILD_DIR"
    
    # Create version-specific source and build files
    create_version_source "$version"
    create_version_cmake "$version"
    
    # Build the plugin
    build_version "$version"
    
    # Install if requested
    if [ "$install_after_build" = true ]; then
        install_version "$version"
    fi
    
    log_success "Plugin version '$version' build process completed!"
    
    # Show next steps
    echo ""
    log_info "Next steps:"
    if [ "$install_after_build" = false ]; then
        echo "  1. Install with: $0 $version --install"
        echo "  2. Test with crash detection scripts"
    else
        echo "  1. Run crash detection tests"
        echo "  2. Check OBS logs for any issues"
    fi
    echo "  3. Verify filter appears in OBS Studio"
    echo ""
    log_info "Build output location: $BUILD_DIR/$version/build/${PLUGIN_NAME}.plugin"
}

# Run main function
main "$@"