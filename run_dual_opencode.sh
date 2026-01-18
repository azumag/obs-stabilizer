#!/bin/bash

# Interactive Auto-Fix System - Dual opencode approach
# Two panes: one for testing, one for fixing

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$(dirname "$SCRIPT_DIR")" && pwd)"

# Configuration
TMUX_SESSION="obs-stabilizer-opencode-dual"
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
# Pane 0.0: Test opencode (left)
# Pane 0.1: Fix opencode (right)
tmux new-session -d -s "$TMUX_SESSION" -c "$PROJECT_ROOT" -x 200 -y 60
tmux split-window -h -t "$TMUX_SESSION:0" -c "$PROJECT_ROOT" -p 50

# Wait for panes to be created
sleep 1

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

# Function to start opencode in test pane
start_test_opencode() {
	log "Starting opencode in left pane..."

	tmux send-keys -t "$TMUX_SESSION:0.0" "clear" C-m
	sleep 0.5

	# Start opencode
	log "Starting opencode in test pane..."
	tmux send-keys -t "$TMUX_SESSION:0.0" "opencode" C-m
	sleep 1

	# Wait for opencode to load and show initial message
	log "Waiting for opencode to be ready..."
	sleep 2

	# Send initial test command
	log "Sending initial test command..."
	tmux send-keys -t "$TMUX_SESSION:0.0" "Run integration tests" C-m
	sleep 1
}

# Function to start opencode in fix pane
start_fix_opencode() {
	log "Starting opencode in right pane..."

	tmux send-keys -t "$TMUX_SESSION:0.1" "clear" C-m
	sleep 0.5

	# Start opencode
	log "Starting opencode in fix pane..."
	tmux send-keys -t "$TMUX_SESSION:0.1" "opencode" C-m
	sleep 1

	# Wait for opencode to load
	log "Waiting for opencode to be ready..."
	sleep 2

	# Send initial message
	log "Sending initial fix command..."
	tmux send-keys -t "$TMUX_SESSION:0.1" "Ready to fix issues. Waiting for test results..." C-m
	sleep 1
}

# Function to start opencode in fix pane
start_fix_opencode() {
	log "Starting fix opencode in right pane..."

	tmux send-keys -t "$TMUX_SESSION:0.1" "clear" C-m
	sleep 0.3

	# Start opencode and send initial message
	tmux send-keys -t "$TMUX_SESSION:0.1" "opencode" C-m
	sleep 1

	tmux send-keys -t "$TMUX_SESSION:0.1" 'echo "Fix opencode ready"' Enter
	sleep 0.3

	sleep 1
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

# Function to send test command to test opencode
send_test_command() {
	local iteration=$1

	log "Sending test command to test opencode (iteration $iteration)..."

	tmux send-keys -t "$TMUX_SESSION:0.0" "clear" C-m
	sleep 0.3
	tmux send-keys -t "$TMUX_SESSION:0.0" 'echo "Test opencode"' C-m
	tmux send-keys -t "$TMUX_SESSION:0.0" 'echo "============="' C-m
	tmux send-keys -t "$TMUX_SESSION:0.0" 'echo "Running tests (iteration '$iteration')..."' C-m
	tmux send-keys -t "$TMUX_SESSION:0.0" 'echo ""' C-m

	# Run test command
	tmux send-keys -t "$TMUX_SESSION:0.0" "$SCRIPT_DIR/tests/integration/run_integration_tests.sh" C-m

	# Wait for tests to complete
	log "Waiting for tests to complete..."
	sleep 5
}

# Function to send fix command to fix opencode
send_fix_command() {
	local iteration=$1

	log "Sending fix command to fix opencode (iteration $iteration)..."

	# Find latest results and get failed tests
	local results_dir="$SCRIPT_DIR/tests/integration/results"
	local latest_results=$(ls -t "$results_dir"/results_*.json 2>/dev/null | head -1)

	if [ ! -f "$latest_results" ]; then
		log_error "No test results found"
		return 1
	fi

	local failed_tests=$(jq -r '.tests[] | select(.status == "failed") | .name' "$latest_results")
	local failed_count=$(jq -r '.summary.failed' "$latest_results")

	log_warning "$failed_count test(s) failed"
	log_warning "Failed: $failed_tests"

	# Clear fix pane and show message
	tmux send-keys -t "$TMUX_SESSION:0.1" "clear" C-m
	sleep 0.3
	tmux send-keys -t "$TMUX_SESSION:0.1" 'echo "Fix opencode"' C-m
	tmux send-keys -t "$TMUX_SESSION:0.1" 'echo "============="' C-m
	tmux send-keys -t "$TMUX_SESSION:0.1" 'echo "Fixing issues (iteration '$iteration')..."' C-m
	tmux send-keys -t "$TMUX_SESSION:0.1" 'echo ""' C-m

	# Send fix command based on failed tests
	local fix_command="echo No fix needed"

	if echo "$failed_tests" | grep -q "Build"; then
		fix_command="$SCRIPT_DIR/tests/integration/fix_patterns/fix_build.sh && cd build && ninja"
	elif echo "$failed_tests" | grep -q "Plugin Loading"; then
		fix_command="$SCRIPT_DIR/tests/integration/fix_patterns/fix_plugin_loading.sh && cd build && ninja"
	elif echo "$failed_tests" | grep -q "Basic Functionality"; then
		fix_command="$SCRIPT_DIR/tests/integration/fix_patterns/fix_basic_functionality.sh && cd build && ninja"
	elif echo "$failed_tests" | grep -q "Crash Detection"; then
		fix_command="$SCRIPT_DIR/tests/integration/fix_patterns/fix_crash.sh && cd build && ninja"
	elif echo "$failed_tests" | grep -q "Pre-flight"; then
		fix_command="$SCRIPT_DIR/tests/integration/fix_patterns/fix_preflight.sh"
	fi

	tmux send-keys -t "$TMUX_SESSION:0.1" "$fix_command" C-m

	# Wait for fix to complete
	log "Waiting for fix to complete..."
	sleep 3
}

# Function to update test results display
update_test_display() {
	local iteration=$1

	local results_dir="$SCRIPT_DIR/tests/integration/results"
	local latest_results=$(ls -t "$results_dir"/results_*.json 2>/dev/null | head -1)

	if [ -f "$latest_results" ]; then
		log "Updating test results display (iteration $iteration)..."

		tmux send-keys -t "$TMUX_SESSION:0.0" 'echo ""' C-m
		tmux send-keys -t "$TMUX_SESSION:0.0" 'cat "'$latest_results'" | jq .' C-m
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

	# Send test command to test opencode
	send_test_command "$ITERATION"

	# Find latest results
	local results_dir="$SCRIPT_DIR/tests/integration/results"
	local latest_results=$(ls -t "$results_dir"/results_*.json 2>/dev/null | head -1)

	if [ ! -f "$latest_results" ]; then
		log_error "No test results found"
		return 1
	fi

	# Update display with test results
	update_test_display "$ITERATION"

	# Check if all tests passed
	if check_tests_passed "$latest_results"; then
		log_success "All tests passed!"
		return 0
	fi

	# Tests failed - send fix command
	log_warning "Tests failed. Sending fix command..."

	# Send fix command to fix opencode
	send_fix_command "$ITERATION"

	return 1
}

# Main function
main() {
	log "Starting Interactive Auto-Fix System (Dual opencode)"
	log "=========================================="
	log "This system will:"
	log "1. Left pane: Run tests via opencode"
	log "2. Right pane: Apply fixes via opencode"
	log "3. Repeat until all tests pass"
	log "=========================================="
	log ""

	# Start opencode instances
	start_test_opencode
	start_fix_opencode

	# Initial delay
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
			log "Session: $TMUX_SESSION"
			log "Attach: tmux attach -t $TMUX_SESSION"
			log "Close: tmux kill-session -t $TMUX_SESSION"
			log ""

			# Update panes with success message
			tmux send-keys -t "$TMUX_SESSION:0.0" 'echo ""' C-m
			tmux send-keys -t "$TMUX_SESSION:0.0" 'echo "=== SUCCESS ==="' C-m
			tmux send-keys -t "$TMUX_SESSION:0.0" 'echo "All tests passed!"' C-m
			tmux send-keys -t "$TMUX_SESSION:0.0" 'echo ""' C-m

			tmux send-keys -t "$TMUX_SESSION:0.1" 'echo ""' C-m
			tmux send-keys -t "$TMUX_SESSION:0.1" 'echo "=== SUCCESS ==="' C-m
			tmux send-keys -t "$TMUX_SESSION:0.1" 'echo "All tests passed!"' C-m
			tmux send-keys -t "$TMUX_SESSION:0.1" 'echo ""' C-m

			# Keep session open
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
