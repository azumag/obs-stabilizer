#!/bin/bash

# OBS Stabilizer - Local Integration Test Suite with Auto-Fix
# This script runs integration tests and automatically fixes common issues

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
TEST_RESULTS_DIR="$SCRIPT_DIR/results"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULTS_FILE="$TEST_RESULTS_DIR/results_$TIMESTAMP.json"
LOG_FILE="$TEST_RESULTS_DIR/test_$TIMESTAMP.log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test tracking
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
FIXED_TESTS=0

# Create results directory
mkdir -p "$TEST_RESULTS_DIR"

# Logging functions
log_info() {
	echo -e "${BLUE}[INFO]${NC} $1" | tee -a "$LOG_FILE"
}

log_success() {
	echo -e "${GREEN}[SUCCESS]${NC} $1" | tee -a "$LOG_FILE"
}

log_error() {
	echo -e "${RED}[ERROR]${NC} $1" | tee -a "$LOG_FILE"
}

log_warning() {
	echo -e "${YELLOW}[WARNING]${NC} $1" | tee -a "$LOG_FILE"
}

# Initialize results JSON
init_results() {
	cat >"$RESULTS_FILE" <<EOF
{
  "timestamp": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")",
  "project_root": "$PROJECT_ROOT",
  "tests": [],
  "summary": {
    "total": 0,
    "passed": 0,
    "failed": 0,
    "fixed": 0
  }
}
EOF
}

# Add test result to JSON
add_result() {
	local test_name=$1
	local status=$2
	local message=$3
	local fix_attempted=$4
	local fix_successful=$5

	TOTAL_TESTS=$((TOTAL_TESTS + 1))

	if [ "$status" = "passed" ]; then
		PASSED_TESTS=$((PASSED_TESTS + 1))
	elif [ "$status" = "failed" ]; then
		FAILED_TESTS=$((FAILED_TESTS + 1))
	fi

	if [ "$fix_successful" = "true" ]; then
		FIXED_TESTS=$((FIXED_TESTS + 1))
	fi

	# Update JSON file
	temp_json=$(mktemp)
	jq --arg name "$test_name" \
		--arg status "$status" \
		--arg msg "$message" \
		--arg fix_attempted "$fix_attempted" \
		--arg fix_successful "$fix_successful" \
		'.tests += [{
           "name": $name,
           "status": $status,
           "message": $msg,
           "fix_attempted": ($fix_attempted == "true"),
           "fix_successful": ($fix_successful == "true")
       }]' "$RESULTS_FILE" >"$temp_json"
	mv "$temp_json" "$RESULTS_FILE"
}

# Update summary in JSON
update_summary() {
	local temp_json=$(mktemp)
	jq --arg total "$TOTAL_TESTS" \
		--arg passed "$PASSED_TESTS" \
		--arg failed "$FAILED_TESTS" \
		--arg fixed "$FIXED_TESTS" \
		'.summary.total = ($total | tonumber) |
        .summary.passed = ($passed | tonumber) |
        .summary.failed = ($failed | tonumber) |
        .summary.fixed = ($fixed | tonumber)' "$RESULTS_FILE" >"$temp_json"
	mv "$temp_json" "$RESULTS_FILE"
}

# Run a test with auto-fix capability
run_test() {
	local test_name=$1
	local test_script=$2
	local fix_script="${3:-}"

	log_info "Running test: $test_name"
	echo "----------------------------------------" | tee -a "$LOG_FILE"

	if [ ! -f "$test_script" ]; then
		log_error "Test script not found: $test_script"
		add_result "$test_name" "failed" "Test script not found" "false" "false"
		update_summary
		return 1
	fi

	# Run the test
	if bash "$test_script" >>"$LOG_FILE" 2>&1; then
		log_success "Test passed: $test_name"
		add_result "$test_name" "passed" "Test completed successfully" "false" "false"
		update_summary
		return 0
	else
		log_error "Test failed: $test_name"

		# Try to fix if fix script is available
		if [ -n "$fix_script" ] && [ -f "$fix_script" ]; then
			log_info "Attempting automatic fix..."

			if bash "$fix_script" >>"$LOG_FILE" 2>&1; then
				log_info "Fix applied, re-running test..."

				# Re-run the test
				if bash "$test_script" >>"$LOG_FILE" 2>&1; then
					log_success "Test passed after fix: $test_name"
					add_result "$test_name" "passed" "Fixed and passed" "true" "true"
					update_summary
					return 0
				else
					log_error "Test still failed after fix: $test_name"
					add_result "$test_name" "failed" "Fix applied but test still failed" "true" "false"
					update_summary
					return 1
				fi
			else
				log_error "Fix failed: $test_name"
				add_result "$test_name" "failed" "Auto-fix failed" "true" "false"
				update_summary
				return 1
			fi
		else
			add_result "$test_name" "failed" "No fix available" "false" "false"
			update_summary
			return 1
		fi
	fi
}

# Print summary
print_summary() {
	update_summary

	echo ""
	echo "=========================================="
	echo "Integration Test Summary"
	echo "=========================================="
	echo "Total Tests: $TOTAL_TESTS"
	echo -e "${GREEN}Passed: $PASSED_TESTS${NC}"
	echo -e "${RED}Failed: $FAILED_TESTS${NC}"
	echo -e "${YELLOW}Auto-Fixed: $FIXED_TESTS${NC}"
	echo "=========================================="
	echo ""
	echo "Results saved to: $RESULTS_FILE"
	echo "Log saved to: $LOG_FILE"
	echo ""

	if [ $FAILED_TESTS -gt 0 ]; then
		return 1
	else
		return 0
	fi
}

# Main execution
main() {
	echo "=========================================="
	echo "OBS Stabilizer - Integration Test Suite"
	echo "=========================================="
	echo "Started at: $(date)"
	echo ""

	init_results

	# Test 1: Pre-flight checks
	run_test "Pre-flight Checks" \
		"$SCRIPT_DIR/test_00_preflight.sh" \
		"$SCRIPT_DIR/fix_patterns/fix_preflight.sh"

	# Test 2: Build verification
	run_test "Build Verification" \
		"$SCRIPT_DIR/test_01_build.sh" \
		"$SCRIPT_DIR/fix_patterns/fix_build.sh"

	# Test 3: Plugin loading
	run_test "Plugin Loading" \
		"$SCRIPT_DIR/test_02_plugin_loading.sh" \
		"$SCRIPT_DIR/fix_patterns/fix_plugin_loading.sh"

	# Test 4: Basic functionality
	run_test "Basic Functionality" \
		"$SCRIPT_DIR/test_03_basic_functionality.sh" \
		"$SCRIPT_DIR/fix_patterns/fix_basic_functionality.sh"

	# Test 5: Crash detection
	run_test "Crash Detection" \
		"$SCRIPT_DIR/test_04_crash_detection.sh" \
		"$SCRIPT_DIR/fix_patterns/fix_crash.sh"

	# Test 6: Cleanup
	run_test "Cleanup" \
		"$SCRIPT_DIR/test_99_cleanup.sh" \
		""

	print_summary
}

# Run main
main
exit $?
