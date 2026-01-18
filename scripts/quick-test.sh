#!/bin/bash

# Quick Test Script
# Demonstrates the complete plugin testing workflow

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[QUICK-TEST]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

show_usage() {
    echo "Quick Test Script - Demonstrate plugin testing workflow"
    echo ""
    echo "Usage: $0 [version]"
    echo ""
    echo "Examples:"
    echo "  $0                      # Test step0-minimal (safest baseline)"
    echo "  $0 step1-parameters     # Test step1 (parameter reading)"
    echo "  $0 step2-frame-access   # Test step2 (frame data access)"
    echo ""
    echo "This script demonstrates the complete workflow:"
    echo "  1. Clean environment"
    echo "  2. Build plugin version" 
    echo "  3. Install plugin"
    echo "  4. Run crash tests"
    echo "  5. Generate report"
}

main() {
    local version="${1:-step0-minimal}"
    
    if [ "$version" = "--help" ] || [ "$version" = "-h" ]; then
        show_usage
        exit 0
    fi
    
    echo "================================================"
    echo "OBS Plugin Quick Test Workflow"
    echo "================================================"
    echo "Version: $version"
    echo "Date: $(date)"
    echo ""
    
    log_info "Starting quick test workflow for version: $version"
    
    # Step 1: Show current status
    echo ""
    log_info "Step 1: Current Plugin Status"
    echo "-----------------------------------"
    "$SCRIPT_DIR/plugin-manager.sh" status
    
    # Step 2: Clean environment
    echo ""
    log_info "Step 2: Clean Environment"  
    echo "-----------------------------------"
    "$SCRIPT_DIR/plugin-manager.sh" clean
    
    # Step 3: Build version
    echo ""
    log_info "Step 3: Build Plugin Version"
    echo "-----------------------------------"
    if "$SCRIPT_DIR/plugin-version-builder.sh" "$version" --clean; then
        log_success "Build completed successfully"
    else
        log_error "Build failed - stopping test"
        exit 1
    fi
    
    # Step 4: Install version
    echo ""
    log_info "Step 4: Install Plugin"
    echo "-----------------------------------"
    if "$SCRIPT_DIR/plugin-manager.sh" install "$version"; then
        log_success "Installation completed successfully"
    else
        log_error "Installation failed - stopping test"
        exit 1
    fi
    
    # Step 5: Verify installation
    echo ""
    log_info "Step 5: Verify Installation"
    echo "-----------------------------------"
    if "$SCRIPT_DIR/plugin-manager.sh" verify; then
        log_success "Verification passed"
    else
        log_warning "Verification found issues"
    fi
    
    # Step 6: Run crash tests
    echo ""
    log_info "Step 6: Run Crash Tests"
    echo "-----------------------------------"
    log_info "Running basic loading test and UI interaction test..."
    
    if "$SCRIPT_DIR/crash-test-framework.sh" "$version" --ui-test --timeout 45; then
        log_success "Crash tests passed - plugin appears stable!"
    else
        log_warning "Crash tests detected issues - see detailed report"
    fi
    
    # Step 7: Generate comprehensive report
    echo ""
    log_info "Step 7: Generate Comprehensive Report"
    echo "-----------------------------------"
    "$SCRIPT_DIR/plugin-test-orchestrator.sh" --report-only
    
    # Summary
    echo ""
    echo "================================================"
    echo "QUICK TEST COMPLETE"
    echo "================================================"
    log_success "Quick test workflow completed for version: $version"
    echo ""
    echo "Next Steps:"
    echo "  • Review test reports in: tmp/crash-test-reports/"
    echo "  • Check comprehensive report in: tmp/comprehensive-test-reports/"
    echo "  • If stable, proceed to test next version"
    echo "  • If crashes found, analyze and fix before proceeding"
    echo ""
    echo "Available Commands:"
    echo "  ./scripts/plugin-manager.sh status          # Check current status"
    echo "  ./scripts/plugin-manager.sh clean           # Clean all plugins"
    echo "  ./scripts/quick-test.sh step1-parameters    # Test next version"
    echo "  ./scripts/plugin-test-orchestrator.sh --full # Test all versions"
}

main "$@"