#!/bin/bash

# Plugin Test Orchestrator
# Master script to systematically test all plugin versions and identify crash causes

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
PLUGIN_VERSIONS_DIR="$PROJECT_ROOT/plugin-versions"
TEST_REPORTS_DIR="$PROJECT_ROOT/tmp/crash-test-reports"
COMPREHENSIVE_REPORT_DIR="$PROJECT_ROOT/tmp/comprehensive-test-reports"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
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

log_orchestrator() {
    echo -e "${CYAN}[ORCHESTRATOR]${NC} $1"
}

show_usage() {
    echo "Plugin Test Orchestrator - Systematic testing of all plugin versions"
    echo ""
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  --full              - Run full test suite (build + test all versions)"
    echo "  --test-only         - Only run tests (skip building)"
    echo "  --build-only        - Only build all versions (skip testing)"
    echo "  --version <ver>     - Test specific version only"
    echo "  --ui-tests          - Include UI interaction tests"
    echo "  --timeout <sec>     - Test timeout per version (default: 30)"
    echo "  --parallel          - Run tests in parallel (experimental)"
    echo "  --report-only       - Generate comprehensive report from existing data"
    echo "  --help              - Show this help"
    echo ""
    echo "Examples:"
    echo "  $0 --full --ui-tests                    # Complete test suite with UI tests"
    echo "  $0 --version step1-parameters          # Test only step1 version"
    echo "  $0 --test-only --timeout 60           # Test existing builds with longer timeout"
    echo "  $0 --report-only                      # Generate report from existing test data"
}

get_available_versions() {
    find "$PLUGIN_VERSIONS_DIR" -name "plugin.json" -exec dirname {} \; | \
        xargs -I {} basename {} | \
        sort
}

build_all_versions() {
    log_orchestrator "Building all plugin versions..."
    
    local versions=($(get_available_versions))
    local build_results=()
    
    for version in "${versions[@]}"; do
        log_info "Building version: $version"
        
        if "$SCRIPT_DIR/plugin-version-builder.sh" "$version" --clean; then
            log_success "Build successful: $version"
            build_results+=("$version:SUCCESS")
        else
            log_error "Build failed: $version"
            build_results+=("$version:FAILED")
        fi
        echo ""
    done
    
    # Summary
    log_orchestrator "Build Summary:"
    for result in "${build_results[@]}"; do
        local ver=${result%:*}
        local status=${result#*:}
        if [ "$status" = "SUCCESS" ]; then
            echo -e "  ✅ $ver"
        else
            echo -e "  ❌ $ver"
        fi
    done
    
    return 0
}

test_all_versions() {
    local ui_tests="$1"
    local timeout="$2"
    local parallel="$3"
    
    log_orchestrator "Testing all plugin versions..."
    log_info "UI Tests: $([ "$ui_tests" = true ] && echo "enabled" || echo "disabled")"
    log_info "Timeout: ${timeout}s per version"
    log_info "Parallel: $([ "$parallel" = true ] && echo "enabled" || echo "disabled")"
    
    local versions=($(get_available_versions))
    local test_results=()
    
    # Create test results directory
    mkdir -p "$TEST_REPORTS_DIR"
    
    for version in "${versions[@]}"; do
        log_test "Testing version: $version"
        
        local ui_flag=""
        if [ "$ui_tests" = true ]; then
            ui_flag="--ui-test"
        fi
        
        if "$SCRIPT_DIR/crash-test-framework.sh" "$version" $ui_flag --timeout "$timeout"; then
            log_success "Test passed: $version"
            test_results+=("$version:PASS")
        else
            log_error "Test failed: $version"
            test_results+=("$version:FAIL") 
        fi
        echo ""
        
        # Brief pause between tests to ensure clean environment
        sleep 3
    done
    
    # Summary
    log_orchestrator "Test Summary:"
    for result in "${test_results[@]}"; do
        local ver=${result%:*}
        local status=${result#*:}
        if [ "$status" = "PASS" ]; then
            echo -e "  ✅ $ver - STABLE"
        else
            echo -e "  ❌ $ver - CRASHES DETECTED"
        fi
    done
    
    return 0
}

test_specific_version() {
    local version="$1"
    local ui_tests="$2"
    local timeout="$3"
    
    log_orchestrator "Testing specific version: $version"
    
    # Validate version exists
    if [ ! -d "$PLUGIN_VERSIONS_DIR/$version" ]; then
        log_error "Version not found: $version"
        echo "Available versions:"
        get_available_versions | sed 's/^/  - /'
        exit 1
    fi
    
    # Build version first
    log_info "Building version: $version"
    if ! "$SCRIPT_DIR/plugin-version-builder.sh" "$version" --clean; then
        log_error "Build failed for version: $version"
        exit 1
    fi
    
    # Test version
    local ui_flag=""
    if [ "$ui_tests" = true ]; then
        ui_flag="--ui-test"
    fi
    
    if "$SCRIPT_DIR/crash-test-framework.sh" "$version" $ui_flag --timeout "$timeout"; then
        log_success "Test passed for version: $version"
        return 0
    else
        log_error "Test failed for version: $version"
        return 1
    fi
}

generate_comprehensive_report() {
    log_orchestrator "Generating comprehensive test report..."
    
    local timestamp=$(date +%Y%m%d_%H%M%S)
    local report_file="$COMPREHENSIVE_REPORT_DIR/comprehensive_crash_analysis_${timestamp}.md"
    
    mkdir -p "$COMPREHENSIVE_REPORT_DIR"
    
    # Collect all individual test reports
    local test_reports=($(find "$TEST_REPORTS_DIR" -name "*_crash_test_report_*.md" | sort))
    
    if [ ${#test_reports[@]} -eq 0 ]; then
        log_warning "No test reports found in $TEST_REPORTS_DIR"
        return 1
    fi
    
    cat > "$report_file" << EOF
# Comprehensive Plugin Crash Analysis Report

**Generated**: $(date)  
**Test Framework**: OBS Plugin Test Orchestrator  
**Environment**: macOS $(sw_vers -productVersion), OBS Studio

## Executive Summary

This report provides a comprehensive analysis of crash testing across all plugin versions, designed to systematically identify which functionality additions cause crashes in the OBS Stabilizer plugin.

### Testing Methodology

1. **Version Isolation**: Each plugin version is built and tested independently
2. **Clean Environment**: All previous plugin installations are removed before each test
3. **Systematic Testing**: Tests progress from minimal functionality to full processing
4. **Crash Detection**: Multiple layers of crash detection and analysis
5. **Detailed Reporting**: Comprehensive logging and crash cause analysis

### Plugin Version Progression

| Version | Functionality Added | Expected Behavior |
|---------|-------------------|-------------------|
| step0-minimal | Baseline (no crashes) | Pass-through only, no parameter updates |
| step1-parameters | Parameter reading | Read settings, update filter data |
| step2-frame-access | Frame data access | Access and validate frame data |
| step3-opencv-basic | OpenCV integration | Convert frames to OpenCV Mat |
| step4-processing | Video processing | Full stabilization processing |

## Individual Test Results

EOF

    # Process each test report
    for report in "${test_reports[@]}"; do
        local version=$(basename "$report" | sed 's/_crash_test_report_.*\.md$//')
        
        echo "### $version" >> "$report_file"
        echo "" >> "$report_file"
        
        # Extract key information from individual reports
        if grep -q "✅ STABLE" "$report" 2>/dev/null; then
            echo "**Status**: ✅ **STABLE** - No crashes detected" >> "$report_file"
        elif grep -q "CRITICAL FAILURE" "$report" 2>/dev/null; then
            echo "**Status**: ❌ **CRITICAL FAILURE** - Basic loading crashes" >> "$report_file"
        elif grep -q "UI CRASH" "$report" 2>/dev/null; then
            echo "**Status**: ⚠️ **UI CRASH** - Crashes when applied through UI" >> "$report_file"
        else
            echo "**Status**: ⚠️ **ISSUES DETECTED** - Some stability problems" >> "$report_file"
        fi
        
        # Extract test results
        echo "" >> "$report_file"
        if grep -A 10 "Test Results Summary" "$report" >/dev/null 2>&1; then
            grep -A 5 "| Test Type" "$report" >> "$report_file" 2>/dev/null || true
        fi
        
        echo "" >> "$report_file"
        echo "---" >> "$report_file"
        echo "" >> "$report_file"
    done
    
    # Add crash progression analysis
    cat >> "$report_file" << 'EOF'
## Crash Progression Analysis

This section analyzes the progression of stability across plugin versions to identify exactly where crashes are introduced.

### Stability Progression
EOF

    # Analyze progression
    local versions=($(get_available_versions))
    echo "" >> "$report_file"
    
    for version in "${versions[@]}"; do
        local version_reports=($(find "$TEST_REPORTS_DIR" -name "${version}_crash_test_report_*.md" | head -1))
        
        if [ ${#version_reports[@]} -gt 0 ]; then
            local report="${version_reports[0]}"
            
            echo "#### $version" >> "$report_file"
            
            if grep -q "✅ STABLE" "$report" 2>/dev/null; then
                echo "- ✅ **STABLE**: This version is safe and can be used as a baseline" >> "$report_file"
                echo "- 🔄 **NEXT STEP**: Can proceed to add more functionality" >> "$report_file"
            elif grep -q "CRITICAL FAILURE" "$report" 2>/dev/null; then
                echo "- ❌ **CRITICAL**: Crashes during basic loading - fundamental issues" >> "$report_file"
                echo "- 🛑 **BLOCKER**: Must fix before proceeding to next versions" >> "$report_file"
            elif grep -q "UI CRASH" "$report" 2>/dev/null; then
                echo "- ⚠️ **UI ISSUES**: Loads but crashes when applied to video sources" >> "$report_file"
                echo "- 🔍 **INVESTIGATION**: Focus on filter initialization and video processing code" >> "$report_file"
            fi
            
            echo "" >> "$report_file"
        fi
    done
    
    # Add recommendations
    cat >> "$report_file" << 'EOF'
## Development Recommendations

### Safe Development Path

1. **Start with Stable Version**: Use the last stable version as baseline
2. **Incremental Changes**: Add one small functionality at a time  
3. **Test After Each Change**: Run crash tests immediately after each modification
4. **Fix Before Proceeding**: Don't add new functionality until current version is stable

### Crash Investigation Priority

1. **Critical Failures**: Fix basic loading crashes first (highest priority)
2. **UI Crashes**: Focus on filter initialization and video processing (medium priority)  
3. **Optimization**: Only optimize after achieving stability (lowest priority)

### Code Review Areas

Based on crash patterns, focus review on:
- Plugin initialization and symbol resolution
- Memory management in filter data structures
- Video frame processing and OpenCV integration
- Parameter validation and error handling

## Testing Instructions

### Reproduce Issues
```bash
# Test specific version
./scripts/plugin-test-orchestrator.sh --version step1-parameters

# Full test suite with UI
./scripts/plugin-test-orchestrator.sh --full --ui-tests

# Generate updated report
./scripts/plugin-test-orchestrator.sh --report-only
```

### Add New Functionality
1. Create new version in `plugin-versions/`
2. Build with `scripts/plugin-version-builder.sh <version>`
3. Test with `scripts/crash-test-framework.sh <version> --ui-test`
4. Generate report with `--report-only`

## Conclusion

This systematic approach provides clear visibility into which code changes cause crashes, enabling targeted debugging and safe incremental development.

**Next Steps**:
1. Review individual test reports for detailed crash analysis
2. Focus development on the last known stable version
3. Add minimal functionality increments with immediate testing
4. Use this framework continuously during development

---

**Report generated by**: OBS Plugin Test Orchestrator  
**Test data location**: `tmp/crash-test-reports/`  
**Framework location**: `scripts/plugin-test-orchestrator.sh`
EOF

    log_success "Comprehensive report generated: $report_file"
    
    # Show summary
    echo ""
    log_orchestrator "Report Summary:"
    echo "  📋 Report file: $report_file"
    echo "  📊 Test reports analyzed: ${#test_reports[@]}"
    echo "  🔍 Plugin versions covered: ${#versions[@]}"
    echo ""
    
    return 0
}

# Main script logic
main() {
    local full_test=false
    local test_only=false
    local build_only=false
    local specific_version=""
    local ui_tests=false
    local timeout=30
    local parallel=false
    local report_only=false
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --full)
                full_test=true
                shift
                ;;
            --test-only)
                test_only=true
                shift
                ;;
            --build-only)
                build_only=true
                shift
                ;;
            --version)
                specific_version="$2"
                shift 2
                ;;
            --ui-tests)
                ui_tests=true
                shift
                ;;
            --timeout)
                timeout="$2"
                shift 2
                ;;
            --parallel)
                parallel=true
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
                log_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
    
    # Validate conflicting options
    local mode_count=0
    [ "$full_test" = true ] && ((mode_count++))
    [ "$test_only" = true ] && ((mode_count++))
    [ "$build_only" = true ] && ((mode_count++))
    [ -n "$specific_version" ] && ((mode_count++))
    [ "$report_only" = true ] && ((mode_count++))
    
    if [ $mode_count -gt 1 ]; then
        log_error "Conflicting options specified"
        show_usage
        exit 1
    fi
    
    if [ $mode_count -eq 0 ]; then
        log_error "No operation mode specified"
        show_usage
        exit 1
    fi
    
    # Create directories
    mkdir -p "$TEST_REPORTS_DIR"
    mkdir -p "$COMPREHENSIVE_REPORT_DIR"
    
    log_orchestrator "OBS Plugin Test Orchestrator Starting..."
    log_info "Available versions: $(get_available_versions | tr '\n' ' ')"
    
    # Execute based on mode
    if [ "$report_only" = true ]; then
        generate_comprehensive_report
        
    elif [ "$full_test" = true ]; then
        log_orchestrator "Running full test suite..."
        build_all_versions
        test_all_versions "$ui_tests" "$timeout" "$parallel"
        generate_comprehensive_report
        
    elif [ "$build_only" = true ]; then
        build_all_versions
        
    elif [ "$test_only" = true ]; then
        test_all_versions "$ui_tests" "$timeout" "$parallel"
        generate_comprehensive_report
        
    elif [ -n "$specific_version" ]; then
        test_specific_version "$specific_version" "$ui_tests" "$timeout"
        generate_comprehensive_report
    fi
    
    log_orchestrator "Plugin Test Orchestrator completed successfully!"
    
    # Show final summary
    echo ""
    echo "=========================================="
    echo "TEST ORCHESTRATION COMPLETE"
    echo "=========================================="
    log_info "Individual reports: $TEST_REPORTS_DIR"
    log_info "Comprehensive reports: $COMPREHENSIVE_REPORT_DIR"
    echo ""
    log_success "Use the comprehensive report to identify crash causes and plan next development steps"
}

# Run main function
main "$@"