#!/bin/bash

# Crash Test Framework
# Systematic testing of plugin versions to identify crash causes

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
TEST_REPORTS_DIR="$PROJECT_ROOT/tmp/crash-test-reports"
OBS_APP="/Applications/OBS.app/Contents/MacOS/OBS"
OBS_PLUGINS_DIR="/Applications/OBS.app/Contents/PlugIns"
CRASH_LOG_DIR="$HOME/Library/Logs/DiagnosticReports"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
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

log_test() {
    echo -e "${MAGENTA}[TEST]${NC} $1"
}

show_usage() {
    echo "Usage: $0 <version> [options]"
    echo ""
    echo "Test a specific plugin version for crashes:"
    echo "  step0-minimal      - Test baseline minimal plugin"
    echo "  step1-parameters   - Test parameter reading functionality" 
    echo "  step2-frame-access - Test frame data access"
    echo "  step3-opencv-basic - Test OpenCV integration"
    echo "  step4-processing   - Test video processing"
    echo ""
    echo "Options:"
    echo "  --timeout <sec>    - Test timeout in seconds (default: 30)"
    echo "  --ui-test          - Include UI interaction tests"
    echo "  --verbose          - Show detailed logging"
    echo "  --report-only      - Only generate report, don't run tests"
    echo "  --help             - Show this help"
    echo ""
    echo "Example: $0 step1-parameters --ui-test --timeout 60"
}

clean_environment() {
    log_info "Cleaning test environment..."
    
    # Kill any running OBS instances
    pkill -x OBS 2>/dev/null || true
    sleep 2
    
    # Remove all existing stabilizer plugins
    log_info "Removing existing stabilizer plugins..."
    find "$OBS_PLUGINS_DIR" -name "*stabilizer*" -type d -exec sudo rm -rf {} + 2>/dev/null || true
    find "$OBS_PLUGINS_DIR" -name "*test-stabilizer*" -type d -exec sudo rm -rf {} + 2>/dev/null || true
    
    # Clean OBS cache and temp files
    rm -rf "$HOME/Library/Application Support/obs-studio/plugin_config" 2>/dev/null || true
    
    log_success "Environment cleaned"
}

install_test_version() {
    local version="$1"
    local build_dir="$PROJECT_ROOT/tmp/version-builds/$version/build"
    
    log_info "Installing test version: $version"
    
    # Determine plugin name based on version
    local plugin_name
    case "$version" in
        "step0-minimal") plugin_name="test-stabilizer-step0" ;;
        "step1-parameters") plugin_name="test-stabilizer-step1" ;;
        "step2-frame-access") plugin_name="test-stabilizer-step2" ;;
        "step3-opencv-basic") plugin_name="test-stabilizer-step3" ;;
        "step4-processing") plugin_name="test-stabilizer-step4" ;;
        *) log_error "Unknown version: $version"; exit 1 ;;
    esac
    
    local plugin_bundle="${plugin_name}.plugin"
    local source_path="$build_dir/$plugin_bundle"
    local target_path="$OBS_PLUGINS_DIR/$plugin_bundle"
    
    # Check if plugin was built
    if [ ! -d "$source_path" ]; then
        log_error "Plugin not found: $source_path"
        log_error "Please build the plugin first with: scripts/plugin-version-builder.sh $version"
        exit 1
    fi
    
    # Install plugin
    log_info "Installing: $source_path -> $target_path"
    sudo cp -r "$source_path" "$OBS_PLUGINS_DIR/"
    
    # Verify installation
    if [ -d "$target_path" ]; then
        log_success "Plugin installed: $target_path"
        
        # Show plugin binary
        local binary_path="$target_path/Contents/MacOS/$plugin_name"
        if [ -f "$binary_path" ]; then
            log_info "Plugin binary: $binary_path"
            file "$binary_path"
        fi
    else
        log_error "Installation failed: $target_path not found"
        exit 1
    fi
}

create_crash_marker() {
    local timestamp=$(date +%s)
    CRASH_MARKER="/tmp/obs_crash_marker_${timestamp}"
    touch "$CRASH_MARKER"
    log_info "Crash marker created: $CRASH_MARKER"
}

test_basic_loading() {
    local version="$1"
    local timeout="${2:-30}"
    
    log_test "Testing basic plugin loading for $version (timeout: ${timeout}s)"
    
    local log_file="$TEST_REPORTS_DIR/${version}_basic_loading_$(date +%Y%m%d_%H%M%S).log"
    
    # Start OBS with minimal configuration
    log_info "Starting OBS..."
    timeout "$timeout" "$OBS_APP" --disable-shutdown-check --minimize-to-tray 2>&1 | tee "$log_file" &
    local obs_pid=$!
    
    log_info "OBS PID: $obs_pid"
    
    # Monitor for crash
    local crashed=false
    for i in $(seq 1 "$timeout"); do
        if ! kill -0 $obs_pid 2>/dev/null; then
            log_error "CRASH DETECTED after $i seconds!"
            crashed=true
            break
        fi
        
        if [ $((i % 5)) -eq 0 ]; then
            echo -n "[$i/${timeout}s] "
        fi
        
        sleep 1
    done
    
    echo # New line
    
    # Terminate OBS if still running
    if kill -0 $obs_pid 2>/dev/null; then
        log_info "Test completed, terminating OBS..."
        kill $obs_pid 2>/dev/null || true
        sleep 2
        pkill -x OBS 2>/dev/null || true
    fi
    
    # Return crash status
    if [ "$crashed" = true ]; then
        return 1
    else
        return 0
    fi
}

test_ui_interaction() {
    local version="$1" 
    local timeout="${2:-45}"
    
    log_test "Testing UI interaction for $version (timeout: ${timeout}s)"
    
    local log_file="$TEST_REPORTS_DIR/${version}_ui_test_$(date +%Y%m%d_%H%M%S).log"
    local scene_file="$HOME/Library/Application Support/obs-studio/basic/scenes/CrashTest_${version}.json"
    
    # Create test scene with filter pre-applied
    create_test_scene "$version" "$scene_file"
    
    # Start OBS
    log_info "Starting OBS with test scene..."
    timeout "$timeout" "$OBS_APP" --disable-shutdown-check 2>&1 | tee "$log_file" &
    local obs_pid=$!
    
    log_info "OBS PID: $obs_pid"
    
    # Monitor for crash during startup
    local crashed=false
    for i in $(seq 1 "$timeout"); do
        if ! kill -0 $obs_pid 2>/dev/null; then
            log_error "CRASH DETECTED during UI test after $i seconds!"
            crashed=true
            break
        fi
        
        # Check for specific error patterns in log
        if [ -f "$log_file" ]; then
            if grep -q "segmentation fault\|illegal instruction\|bus error" "$log_file" 2>/dev/null; then
                log_error "CRASH PATTERN detected in log!"
                kill $obs_pid 2>/dev/null || true
                crashed=true
                break
            fi
        fi
        
        if [ $((i % 5)) -eq 0 ]; then
            echo -n "[$i/${timeout}s] "
        fi
        
        sleep 1
    done
    
    echo # New line
    
    # Cleanup
    if kill -0 $obs_pid 2>/dev/null; then
        kill $obs_pid 2>/dev/null || true
        sleep 2
        pkill -x OBS 2>/dev/null || true
    fi
    
    # Clean up test scene
    rm -f "$scene_file" 2>/dev/null || true
    
    if [ "$crashed" = true ]; then
        return 1
    else
        return 0
    fi
}

create_test_scene() {
    local version="$1"
    local scene_file="$2"
    
    # Determine filter ID based on version
    local filter_id
    case "$version" in
        "step0-minimal") filter_id="test_stabilizer_step0_minimal" ;;
        "step1-parameters") filter_id="test_stabilizer_step1_params" ;;
        "step2-frame-access") filter_id="test_stabilizer_step2_frame" ;;
        "step3-opencv-basic") filter_id="test_stabilizer_step3_opencv" ;;
        "step4-processing") filter_id="test_stabilizer_step4_processing" ;;
    esac
    
    mkdir -p "$(dirname "$scene_file")"
    
    cat > "$scene_file" << EOF
{
    "current_scene": "CrashTestScene",
    "current_program_scene": "CrashTestScene",
    "scene_order": [{"name": "CrashTestScene"}],
    "sources": [
        {
            "balance": 0.5,
            "enabled": true,
            "flags": 0,
            "id": "macos-avcapture",
            "muted": false,
            "name": "TestVideoSource",
            "settings": {
                "device": "",
                "preset": "AVCaptureSessionPreset640x480"
            },
            "sync": 0,
            "versioned_id": "macos-avcapture",
            "volume": 1.0,
            "filters": [
                {
                    "enabled": true,
                    "id": "$filter_id",
                    "name": "Crash Test Filter",
                    "settings": {
                        "enabled": true
                    }
                }
            ]
        }
    ],
    "transitions": []
}
EOF
    
    log_info "Test scene created: $scene_file"
}

analyze_crash_logs() {
    local version="$1"
    local crash_found=false
    
    log_info "Analyzing crash logs for $version..."
    
    # Find recent crash logs
    local crash_logs=$(find "$CRASH_LOG_DIR" -name "OBS*.crash" -o -name "OBS*.ips" -newer "$CRASH_MARKER" 2>/dev/null || true)
    
    if [ -n "$crash_logs" ]; then
        log_warning "Found crash logs:"
        crash_found=true
        
        for log_file in $crash_logs; do
            local log_basename=$(basename "$log_file")
            log_warning "  - $log_basename"
            
            # Extract key crash information
            echo ""
            echo "=== Crash Analysis: $log_basename ==="
            
            # Exception type
            grep -A2 "Exception Type:" "$log_file" 2>/dev/null || true
            
            # Crashed thread
            echo ""
            grep -A10 "Thread 0 Crashed" "$log_file" 2>/dev/null || true
            
            # Look for plugin-related frames
            echo ""
            grep -E "(stabilizer|test-stabilizer)" "$log_file" 2>/dev/null || true
            
            echo ""
        done
    else
        log_success "No crash logs found - plugin appears stable"
    fi
    
    return $crash_found
}

generate_test_report() {
    local version="$1"
    local basic_result="$2"
    local ui_result="$3"
    local crash_analysis="$4"
    
    local report_file="$TEST_REPORTS_DIR/${version}_crash_test_report_$(date +%Y%m%d_%H%M%S).md"
    
    log_info "Generating test report: $report_file"
    
    cat > "$report_file" << EOF
# Crash Test Report: $version

**Date**: $(date)  
**Version**: $version  
**Test Environment**: macOS $(sw_vers -productVersion), OBS Studio

## Test Results Summary

| Test Type | Status | Details |
|-----------|--------|---------|
| Basic Loading | $([ "$basic_result" -eq 0 ] && echo "✅ PASS" || echo "❌ FAIL") | Plugin loading and initialization |
| UI Interaction | $([ "$ui_result" -eq 0 ] && echo "✅ PASS" || echo "❌ FAIL") | Filter application through UI |
| Crash Analysis | $([ "$crash_analysis" -eq 0 ] && echo "✅ CLEAN" || echo "⚠️ CRASHES DETECTED") | System crash log analysis |

## Overall Assessment

$(if [ "$basic_result" -eq 0 ] && [ "$ui_result" -eq 0 ] && [ "$crash_analysis" -eq 0 ]; then
    echo "**STATUS**: ✅ **STABLE** - No crashes detected"
    echo ""
    echo "The plugin version '$version' appears to be stable and safe for use. All tests passed without crashes."
elif [ "$basic_result" -ne 0 ]; then
    echo "**STATUS**: ❌ **CRITICAL FAILURE** - Basic loading crashes"
    echo ""
    echo "The plugin version '$version' crashes during basic loading. This indicates fundamental issues that must be resolved before proceeding."
elif [ "$ui_result" -ne 0 ]; then
    echo "**STATUS**: ⚠️ **UI CRASH** - Crashes when applied through UI"
    echo ""
    echo "The plugin version '$version' loads successfully but crashes when applied to video sources through the UI. This suggests issues with filter initialization or video processing."
else
    echo "**STATUS**: ⚠️ **INVESTIGATION NEEDED** - Some crashes detected"
    echo ""
    echo "The plugin version '$version' shows some stability issues that require further investigation."
fi)

## Detailed Test Results

### Basic Loading Test
$([ "$basic_result" -eq 0 ] && echo "✅ Plugin loaded successfully without crashes" || echo "❌ Plugin crashed during basic loading")

### UI Interaction Test  
$([ "$ui_result" -eq 0 ] && echo "✅ Filter applied successfully through UI without crashes" || echo "❌ Plugin crashed when filter applied through UI")

### Crash Log Analysis
$([ "$crash_analysis" -eq 0 ] && echo "✅ No system crash logs generated" || echo "⚠️ System crash logs detected - see crash analysis section")

## Recommendations

$(if [ "$basic_result" -eq 0 ] && [ "$ui_result" -eq 0 ] && [ "$crash_analysis" -eq 0 ]; then
    echo "- ✅ This version is ready for the next development step"
    echo "- ✅ Can proceed with adding more functionality"
    echo "- ✅ Safe to use as a baseline for further development"
elif [ "$basic_result" -ne 0 ]; then
    echo "- ❌ **CRITICAL**: Fix basic loading issues before proceeding"
    echo "- ❌ Review plugin initialization and symbol resolution"
    echo "- ❌ Check for missing dependencies or library issues"
    echo "- ❌ Do not proceed to next steps until this is resolved"
elif [ "$ui_result" -ne 0 ]; then
    echo "- ⚠️ **HIGH PRIORITY**: Fix UI application crashes" 
    echo "- ⚠️ Review filter initialization and video processing code"
    echo "- ⚠️ Check for memory access issues in update functions"
    echo "- ⚠️ Consider simplifying functionality until stable"
else
    echo "- ⚠️ Investigate crash logs for root cause analysis"
    echo "- ⚠️ Consider reducing functionality complexity" 
    echo "- ⚠️ Add more defensive programming practices"
fi)

## Test Environment Details

- **OS Version**: $(sw_vers -productVersion)  
- **OBS Version**: $(defaults read /Applications/OBS.app/Contents/Info.plist CFBundleShortVersionString 2>/dev/null || echo "Unknown")
- **Test Date**: $(date)
- **Test Duration**: Basic: 30s, UI: 45s
- **Plugin Location**: $OBS_PLUGINS_DIR

## Log Files

Test logs are available in: $TEST_REPORTS_DIR

EOF

    log_success "Test report generated: $report_file"
}

# Main script logic
main() {
    local version=""
    local timeout=30
    local ui_test=false
    local verbose=false
    local report_only=false
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --timeout)
                timeout="$2"
                shift 2
                ;;
            --ui-test)
                ui_test=true
                shift
                ;;
            --verbose)
                verbose=true
                shift
                ;;
            --report-only)
                report_only=true
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
    
    # Create reports directory
    mkdir -p "$TEST_REPORTS_DIR"
    
    if [ "$report_only" = false ]; then
        # Prepare environment
        clean_environment
        install_test_version "$version"
        create_crash_marker
        
        # Run basic loading test
        log_info "Starting crash test sequence for version: $version"
        
        local basic_result=0
        if ! test_basic_loading "$version" "$timeout"; then
            basic_result=1
        fi
        
        # Run UI test if requested and basic test passed
        local ui_result=0
        if [ "$ui_test" = true ]; then
            if [ "$basic_result" -eq 0 ]; then
                if ! test_ui_interaction "$version" $((timeout + 15)); then
                    ui_result=1
                fi
            else
                log_warning "Skipping UI test due to basic loading failure"
                ui_result=1
            fi
        fi
        
        # Analyze crash logs
        local crash_analysis=0
        if ! analyze_crash_logs "$version"; then
            crash_analysis=1
        fi
        
        # Generate report
        generate_test_report "$version" "$basic_result" "$ui_result" "$crash_analysis"
        
        # Show results
        echo ""
        echo "=========================================="
        echo "CRASH TEST RESULTS: $version"
        echo "=========================================="
        
        if [ "$basic_result" -eq 0 ] && [ "$ui_result" -eq 0 ] && [ "$crash_analysis" -eq 0 ]; then
            log_success "ALL TESTS PASSED - Plugin is stable"
        elif [ "$basic_result" -ne 0 ]; then
            log_error "CRITICAL FAILURE - Basic loading crashes"
        elif [ "$ui_result" -ne 0 ]; then
            log_error "UI CRASH - Filter application fails"
        else
            log_warning "MIXED RESULTS - Some issues detected"
        fi
    else
        log_info "Report-only mode - skipping actual tests"
    fi
    
    # Clean up
    rm -f "$CRASH_MARKER" 2>/dev/null || true
    
    log_info "Crash test framework completed for version: $version"
    log_info "Reports available in: $TEST_REPORTS_DIR"
}

# Run main function
main "$@"