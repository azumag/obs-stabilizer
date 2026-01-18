#!/bin/bash

# Plugin Manager
# Manages installation, removal, and listing of OBS stabilizer plugins

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
OBS_PLUGINS_DIR="/Applications/OBS.app/Contents/PlugIns"

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
    echo "OBS Plugin Manager - Manage stabilizer plugin installations"
    echo ""
    echo "Usage: $0 <command> [options]"
    echo ""
    echo "Commands:"
    echo "  list                 - List all installed stabilizer plugins"
    echo "  clean               - Remove all stabilizer plugins"
    echo "  install <version>   - Install specific plugin version"
    echo "  remove <plugin>     - Remove specific plugin by name"
    echo "  status              - Show current plugin status"
    echo "  verify              - Verify plugin installations"
    echo ""
    echo "Examples:"
    echo "  $0 list                           # Show all installed plugins"
    echo "  $0 clean                          # Remove all stabilizer plugins"
    echo "  $0 install step1-parameters       # Install step1 version"
    echo "  $0 remove test-stabilizer-step0   # Remove specific plugin"
    echo "  $0 status                         # Show plugin status"
}

list_installed_plugins() {
    log_info "Scanning for installed stabilizer plugins..."
    
    local found_plugins=()
    
    # Look for stabilizer-related plugins
    if [ -d "$OBS_PLUGINS_DIR" ]; then
        while IFS= read -r -d '' plugin_dir; do
            local plugin_name=$(basename "$plugin_dir")
            found_plugins+=("$plugin_name")
        done < <(find "$OBS_PLUGINS_DIR" -name "*stabilizer*" -type d -print0 2>/dev/null || true)
        
        while IFS= read -r -d '' plugin_dir; do
            local plugin_name=$(basename "$plugin_dir")
            found_plugins+=("$plugin_name")
        done < <(find "$OBS_PLUGINS_DIR" -name "*test-stabilizer*" -type d -print0 2>/dev/null || true)
    fi
    
    if [ ${#found_plugins[@]} -eq 0 ]; then
        log_info "No stabilizer plugins found"
        return 0
    fi
    
    log_info "Found ${#found_plugins[@]} stabilizer plugin(s):"
    echo ""
    
    for plugin in "${found_plugins[@]}"; do
        local plugin_path="$OBS_PLUGINS_DIR/$plugin"
        local binary_path="$plugin_path/Contents/MacOS"
        local info_plist="$plugin_path/Contents/Info.plist"
        
        echo "  📦 $plugin"
        echo "     Path: $plugin_path"
        
        # Check if binary exists
        if [ -d "$binary_path" ]; then
            local binaries=($(ls "$binary_path" 2>/dev/null || true))
            if [ ${#binaries[@]} -gt 0 ]; then
                echo "     Binary: ${binaries[0]}"
                
                # Show binary info
                local binary_file="$binary_path/${binaries[0]}"
                if [ -f "$binary_file" ]; then
                    local file_info=$(file "$binary_file" 2>/dev/null || echo "Unknown format")
                    echo "     Format: $file_info"
                fi
            else
                echo "     Binary: ⚠️  Missing"
            fi
        else
            echo "     Binary: ❌ Directory not found"
        fi
        
        # Check Info.plist
        if [ -f "$info_plist" ]; then
            local bundle_id=$(defaults read "$info_plist" CFBundleIdentifier 2>/dev/null || echo "Unknown")
            local version=$(defaults read "$info_plist" CFBundleVersion 2>/dev/null || echo "Unknown")
            echo "     Bundle ID: $bundle_id"
            echo "     Version: $version"
        else
            echo "     Info.plist: ❌ Missing"
        fi
        
        echo ""
    done
    
    return 0
}

clean_all_plugins() {
    log_info "Removing all stabilizer plugins..."
    
    local removed_count=0
    
    # Remove stabilizer plugins
    if [ -d "$OBS_PLUGINS_DIR" ]; then
        while IFS= read -r -d '' plugin_dir; do
            local plugin_name=$(basename "$plugin_dir")
            log_info "Removing: $plugin_name"
            
            if sudo rm -rf "$plugin_dir"; then
                log_success "Removed: $plugin_name"
                ((removed_count++))
            else
                log_error "Failed to remove: $plugin_name"
            fi
        done < <(find "$OBS_PLUGINS_DIR" -name "*stabilizer*" -type d -print0 2>/dev/null || true)
        
        while IFS= read -r -d '' plugin_dir; do
            local plugin_name=$(basename "$plugin_dir")
            log_info "Removing: $plugin_name"
            
            if sudo rm -rf "$plugin_dir"; then
                log_success "Removed: $plugin_name"
                ((removed_count++))
            else
                log_error "Failed to remove: $plugin_name"
            fi
        done < <(find "$OBS_PLUGINS_DIR" -name "*test-stabilizer*" -type d -print0 2>/dev/null || true)
    fi
    
    if [ $removed_count -eq 0 ]; then
        log_info "No stabilizer plugins found to remove"
    else
        log_success "Removed $removed_count stabilizer plugin(s)"
    fi
    
    # Clean OBS cache
    local obs_cache="$HOME/Library/Application Support/obs-studio/plugin_config"
    if [ -d "$obs_cache" ]; then
        log_info "Cleaning OBS plugin cache..."
        rm -rf "$obs_cache" 2>/dev/null || true
    fi
    
    return 0
}

install_plugin_version() {
    local version="$1"
    
    if [ -z "$version" ]; then
        log_error "Version not specified"
        return 1
    fi
    
    log_info "Installing plugin version: $version"
    
    # Determine plugin name based on version
    local plugin_name
    case "$version" in
        "step0-minimal") plugin_name="test-stabilizer-step0" ;;
        "step1-parameters") plugin_name="test-stabilizer-step1" ;;
        "step2-frame-access") plugin_name="test-stabilizer-step2" ;;
        "step3-opencv-basic") plugin_name="test-stabilizer-step3" ;;
        "step4-processing") plugin_name="test-stabilizer-step4" ;;
        *) 
            log_error "Unknown version: $version"
            log_error "Available versions: step0-minimal, step1-parameters, step2-frame-access, step3-opencv-basic, step4-processing"
            return 1
            ;;
    esac
    
    local build_dir="$PROJECT_ROOT/tmp/version-builds/$version/build"
    local plugin_bundle="${plugin_name}.plugin"
    local source_path="$build_dir/$plugin_bundle"
    local target_path="$OBS_PLUGINS_DIR/$plugin_bundle"
    
    # Check if plugin was built
    if [ ! -d "$source_path" ]; then
        log_error "Plugin not built: $source_path"
        log_error "Please build first with: scripts/plugin-version-builder.sh $version"
        return 1
    fi
    
    # Clean existing installations first
    log_info "Cleaning existing installations..."
    clean_all_plugins
    
    # Install plugin
    log_info "Installing: $source_path -> $target_path"
    if sudo cp -r "$source_path" "$OBS_PLUGINS_DIR/"; then
        log_success "Plugin installed successfully"
        
        # Verify installation
        if [ -d "$target_path" ]; then
            log_success "Installation verified: $target_path"
            
            # Show installed plugin info
            echo ""
            log_info "Installed plugin details:"
            list_installed_plugins
        else
            log_error "Installation verification failed"
            return 1
        fi
    else
        log_error "Installation failed"
        return 1
    fi
    
    return 0
}

remove_specific_plugin() {
    local plugin_name="$1"
    
    if [ -z "$plugin_name" ]; then
        log_error "Plugin name not specified"
        return 1
    fi
    
    local plugin_path="$OBS_PLUGINS_DIR/$plugin_name"
    
    if [ ! -d "$plugin_path" ]; then
        log_warning "Plugin not found: $plugin_name"
        return 1
    fi
    
    log_info "Removing plugin: $plugin_name"
    
    if sudo rm -rf "$plugin_path"; then
        log_success "Plugin removed: $plugin_name"
        return 0
    else
        log_error "Failed to remove plugin: $plugin_name"
        return 1
    fi
}

show_plugin_status() {
    log_info "OBS Plugin Status Report"
    echo ""
    
    # Check OBS installation
    if [ -d "/Applications/OBS.app" ]; then
        local obs_version=$(defaults read /Applications/OBS.app/Contents/Info.plist CFBundleShortVersionString 2>/dev/null || echo "Unknown")
        log_success "OBS Studio installed (version: $obs_version)"
    else
        log_error "OBS Studio not found at /Applications/OBS.app"
    fi
    
    # Check plugins directory
    if [ -d "$OBS_PLUGINS_DIR" ]; then
        log_success "OBS plugins directory exists: $OBS_PLUGINS_DIR"
        
        # Count total plugins
        local total_plugins=$(find "$OBS_PLUGINS_DIR" -name "*.plugin" -type d | wc -l | tr -d ' ')
        log_info "Total plugins installed: $total_plugins"
    else
        log_error "OBS plugins directory not found: $OBS_PLUGINS_DIR"
    fi
    
    echo ""
    
    # List stabilizer plugins
    list_installed_plugins
    
    echo ""
    
    # Check for available builds
    local build_dir="$PROJECT_ROOT/tmp/version-builds"
    if [ -d "$build_dir" ]; then
        local available_builds=($(find "$build_dir" -name "*.plugin" -type d 2>/dev/null | wc -l | tr -d ' '))
        log_info "Available built versions: $available_builds"
        
        if [ "$available_builds" -gt 0 ]; then
            echo "  Built versions ready for installation:"
            find "$build_dir" -name "*.plugin" -type d 2>/dev/null | while read plugin_path; do
                local plugin_name=$(basename "$plugin_path")
                local version_dir=$(basename "$(dirname "$(dirname "$plugin_path")")")
                echo "    - $plugin_name (from $version_dir)"
            done
        fi
    else
        log_info "No build directory found - no versions built yet"
    fi
}

verify_plugin_installations() {
    log_info "Verifying plugin installations..."
    echo ""
    
    local issues_found=0
    
    # Find all stabilizer plugins
    if [ -d "$OBS_PLUGINS_DIR" ]; then
        while IFS= read -r -d '' plugin_dir; do
            local plugin_name=$(basename "$plugin_dir")
            log_info "Verifying: $plugin_name"
            
            # Check structure
            local binary_dir="$plugin_dir/Contents/MacOS"
            local info_plist="$plugin_dir/Contents/Info.plist"
            
            if [ ! -d "$binary_dir" ]; then
                log_error "Missing MacOS directory: $plugin_name"
                ((issues_found++))
                continue
            fi
            
            if [ ! -f "$info_plist" ]; then
                log_error "Missing Info.plist: $plugin_name"
                ((issues_found++))
                continue
            fi
            
            # Check binary
            local binaries=($(ls "$binary_dir" 2>/dev/null || true))
            if [ ${#binaries[@]} -eq 0 ]; then
                log_error "No binary found: $plugin_name"
                ((issues_found++))
                continue
            fi
            
            local binary_path="$binary_dir/${binaries[0]}"
            if [ ! -f "$binary_path" ]; then
                log_error "Binary file missing: $plugin_name"
                ((issues_found++))
                continue
            fi
            
            # Check if binary is valid Mach-O
            if ! file "$binary_path" | grep -q "Mach-O.*shared library"; then
                log_error "Invalid binary format: $plugin_name"
                ((issues_found++))
                continue
            fi
            
            # Check code signature (if present)
            if command -v codesign >/dev/null 2>&1; then
                if ! codesign -v "$binary_path" 2>/dev/null; then
                    log_warning "Code signature invalid: $plugin_name"
                fi
            fi
            
            log_success "✅ $plugin_name - Valid"
            
        done < <(find "$OBS_PLUGINS_DIR" -name "*stabilizer*" -o -name "*test-stabilizer*" -type d -print0 2>/dev/null || true)
    fi
    
    echo ""
    if [ $issues_found -eq 0 ]; then
        log_success "All plugin installations verified successfully"
    else
        log_warning "$issues_found issue(s) found in plugin installations"
    fi
    
    return $issues_found
}

# Main script logic
main() {
    local command="$1"
    
    if [ -z "$command" ]; then
        show_usage
        exit 1
    fi
    
    case "$command" in
        list)
            list_installed_plugins
            ;;
        clean)
            clean_all_plugins
            ;;
        install)
            if [ -z "$2" ]; then
                log_error "Version not specified"
                show_usage
                exit 1
            fi
            install_plugin_version "$2"
            ;;
        remove)
            if [ -z "$2" ]; then
                log_error "Plugin name not specified"
                show_usage
                exit 1
            fi
            remove_specific_plugin "$2"
            ;;
        status)
            show_plugin_status
            ;;
        verify)
            verify_plugin_installations
            ;;
        *)
            log_error "Unknown command: $command"
            show_usage
            exit 1
            ;;
    esac
}

# Run main function
main "$@"