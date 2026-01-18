#!/bin/bash

# Dual Pane Test System - Simple Approach
# Left pane: Run tests
# Right pane: Apply fixes

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$(dirname "$SCRIPT_DIR")" && pwd)"

# Configuration
TMUX_SESSION="obs-stabilizer-dual-test"
MAX_ITERATIONS=100
ITERATION=0

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Kill any existing tmux session
tmux kill-session -t "$TMUX_SESSION" 2>/dev/null || true

# Create new tmux session with 2 panes
# Pane 0.0: Test execution (left)
# Pane 0.1: Fix execution (right)
tmux new-session -d -s "$TMUX_SESSION" -c "$PROJECT_ROOT" -x 200 -y 60
tmux split-window -h -t "$TMUX_SESSION:0" -c "$PROJECT_ROOT" -p 50

log() {
	echo -e "${BLUE}[SYSTEM]${NC} $1"
}

log_success() {
	echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_error() {
	echo -e "${RED}[ERROR]${NC} $1"
}

log_warning() {
	echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Function to check if all tests passed
check_tests_passed() {
	local results_file=$1

	if [ ! -f "$results_file" ]; then
		return 1
	fi

	local failed=$(jq -r '.summary.failed' "$results_file" 2>/dev/null || echo "0")
	[ "$failed" -eq 0 ]
}

# Function to send test command to left pane
run_test_in_pane() {
	local iteration=$1

	log "Running tests (iteration $iteration)..."

	# Clear left pane and run tests
	tmux send-keys -t "$TMUX_SESSION:0.0" "clear" C-m
	sleep 0.3
	tmux send-keys -t "$TMUX_SESSION:0.0" "$SCRIPT_DIR/tests/integration/run_integration_tests.sh" C-m

	# Wait for tests to complete
	log "Waiting for tests to complete..."
	sleep 5
}

# Function to send fix command to right pane
run_fix_in_pane() {
	local iteration=$1

	# Find latest results
	local results_dir="$SCRIPT_DIR/tests/integration/results"
	local latest_results=$(ls -t "$results_dir"/results_*.json 2>/dev/null | head -1)

	if [ ! -f "$latest_results" ]; then
		log_error "No test results found"
		return 1
	fi

	# Get failed tests
	local failed_tests=$(jq -r '.tests[] | select(.status == "failed") | .name' "$latest_results")
	local failed_count=$(jq -r '.summary.failed' "$latest_results")

	log "Running fixes (iteration $iteration)..."
	log_warning "$failed_count test(s) failed"

	# Clear right pane
	tmux send-keys -t "$TMUX_SESSION:0.1" "clear" C-m
	sleep 0.3

	# Determine and run fix script
	local fix_command="echo 'No fix needed'"

	if echo "$failed_tests" | grep -q "Build"; then
		fix_command="$SCRIPT_DIR/tests/integration/fix_patterns/fix_build.sh"
	elif echo "$failed_tests" | grep -q "Plugin Loading"; then
		fix_command="$SCRIPT_DIR/tests/integration/fix_patterns/fix_plugin_loading.sh"
	elif echo "$failed_tests" | grep -q "Basic Functionality"; then
		fix_command="$SCRIPT_DIR/tests/integration/fix_patterns/fix_basic_functionality.sh"
	elif echo "$failed_tests" | grep -q "Crash Detection"; then
		fix_command="$SCRIPT_DIR/tests/integration/fix_patterns/fix_crash.sh"
	elif echo "$failed_tests" | grep -q "Pre-flight"; then
		fix_command="$SCRIPT_DIR/tests/integration/fix_patterns/fix_preflight.sh"
	fi

	# Run fix command in right pane
	log "Running: $fix_command"
	tmux send-keys -t "$TMUX_SESSION:0.1" "$fix_command" C-m

	# Wait for fix to complete
	log "Waiting for fix to complete..."
	sleep 3
}

# Function to update test display in left pane
update_test_display() {
	local iteration=$1

	local results_dir="$SCRIPT_DIR/tests/integration/results"
	local latest_results=$(ls -t "$results_dir"/results_*.json 2>/dev/null | head -1)

	if [ -f "$latest_results" ]; then
		log "Updating test display (iteration $iteration)..."

		# Clear left pane and show results
		tmux send-keys -t "$TMUX_SESSION:0.0" "clear" C-m
		sleep 0.3
		tmux send-keys -t "$TMUX_SESSION:0.0" "echo '=== Test Results (Iteration $iteration) ==='" C-m
		sleep 0.3
		tmux send-keys -t "$TMUX_SESSION:0.0" "cat '$latest_results' | jq ." C-m
	fi
}

# Function to run one iteration
run_iteration() {
	ITERATION=$((ITERATION + 1))

	log ""
	log "=========================================="
	log "Iteration $ITERATION"
	log "=========================================="
	log ""

	# Run tests
	run_test_in_pane "$ITERATION"

	# Find latest results
	local results_dir="$SCRIPT_DIR/tests/integration/results"
	local latest_results=$(ls -t "$results_dir"/results_*.json 2>/dev/null | head -1)

	if [ ! -f "$latest_results" ]; then
		log_error "No test results found"
		return 1
	fi

	# Update test display
	update_test_display "$ITERATION"

	# Check if all tests passed
	if check_tests_passed "$latest_results"; then
		log_success "All tests passed!"
		return 0
	fi

	# Tests failed - run fixes
	log_warning "Tests failed. Running fixes..."

	# Run fixes in right pane
	run_fix_in_pane "$ITERATION"

	return 1
}

# Main function
main() {
	log "Starting Dual Pane Test System"
	log "=========================================="
	log "This system will:"
	log "1. Left pane: Run tests"
	log "2. Right pane: Apply fixes"
	log "3. Repeat until all tests pass"
	log "=========================================="
	log ""

	# Initial display in both panes
	tmux send-keys -t "$TMUX_SESSION:0.0" "clear" C-m
	sleep 0.3
	tmux send-keys -t "$TMUX_SESSION:0.0" "echo '=== Test Pane ==='" C-m
	tmux send-keys -t "$TMUX_SESSION:0.0" "echo 'Waiting to start...'" C-m

	tmux send-keys -t "$TMUX_SESSION:0.1" "clear" C-m
	sleep 0.3
	tmux send-keys -t "$TMUX_SESSION:0.1" "echo '=== Fix Pane ==='" C-m
	tmux send-keys -t "$TMUX_SESSION:0.1" "echo 'Waiting for tests...'" C-m

	sleep 2

	# Main loop
	while true; do
		if run_iteration; then
			# Tests passed
			log ""
			log "=========================================="
			log_success "ALL TESTS PASSED!"
			log "=========================================="
			log ""
			log "Total iterations: $ITERATION"
			log ""

			# Update panes with success message
			tmux send-keys -t "$TMUX_SESSION:0.0" "clear" C-m
			sleep 0.3
			tmux send-keys -t "$TMUX_SESSION:0.0" "echo '=== SUCCESS ==='" C-m
			tmux send-keys -t "$TMUX_SESSION:0.0" "echo 'All tests passed after $ITERATION iterations'" C-m

			tmux send-keys -t "$TMUX_SESSION:0.1" "clear" C-m
			sleep 0.3
			tmux send-keys -t "$TMUX_SESSION:0.1" "echo '=== SUCCESS ==='" C-m
			tmux send-keys -t "$TMUX_SESSION:0.1" "echo 'All tests passed!'" C-m

			# Keep session open
			log ""
			log "Session: $TMUX_SESSION"
			log "Attach: tmux attach -t $TMUX_SESSION"
			log "Close: tmux kill-session -t $TMUX_SESSION"
			log ""

			while true; do
				sleep 10
			done

			exit 0
		fi

		# Tests failed, wait before next iteration
		log ""
		log "Waiting 3 seconds before next iteration..."
		log ""

		sleep 3
	done
}

# Run main function
main
