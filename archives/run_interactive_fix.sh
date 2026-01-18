#!/bin/bash

# Interactive Auto-Fix System
# Uses tmux to run opencode and send fix commands

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$(dirname "$SCRIPT_DIR")" && pwd)"

# Configuration
TMUX_SESSION="obs-stabilizer-opencode-fix"
MAX_ITERATIONS=100 # Run indefinitely until user stops
ITERATION=0

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

# Kill any existing tmux session
tmux kill-session -t "$TMUX_SESSION" 2>/dev/null || true

# Create new tmux session with 3 panes
# Pane 0.0: Main control (top)
# Pane 0.1: Test execution (left-bottom)
# Pane 0.2: opencode session (right-bottom)
tmux new-session -d -s "$TMUX_SESSION" -c "$PROJECT_ROOT" -x 200 -y 60
tmux split-window -v -t "$TMUX_SESSION:0" -c "$PROJECT_ROOT" -p 40
tmux split-window -h -t "$TMUX_SESSION:0.1" -c "$PROJECT_ROOT" -p 50

log() {
	echo -e "${BLUE}[CONTROL]${NC} $1"
	# Don't send to tmux - just print to console
}

log_success() {
	echo -e "${GREEN}[SUCCESS]${NC} $1"
	# Don't send to tmux - just print to console
}

log_error() {
	echo -e "${RED}[ERROR]${NC} $1"
	# Don't send to tmux - just print to console
}

log_warning() {
	echo -e "${YELLOW}[WARNING]${NC} $1"
	# Don't send to tmux - just print to console
}

log_fix() {
	echo -e "${MAGENTA}[FIX COMMAND]${NC} $1"
	# Send to opencode pane only
	tmux send-keys -t "$TMUX_SESSION:0.2" "$1" C-m
}

# New function to send status to control pane (only for important updates)
update_control_pane() {
	local msg=$1
	tmux send-keys -t "$TMUX_SESSION:0.0" "echo '$msg'" C-m
	sleep 0.2
}

# Function to start opencode in tmux
start_opencode() {
	log "Starting opencode in tmux pane..."

	tmux send-keys -t "$TMUX_SESSION:0.2" "clear" C-m
	sleep 0.5
	tmux send-keys -t "$TMUX_SESSION:0.2" "echo '=== opencode Session ==='" C-m
	sleep 0.3
	tmux send-keys -t "$TMUX_SESSION:0.2" "echo 'Waiting for fix commands...'" C-m
	sleep 0.3

	# Update control pane
	update_control_pane "opencode session started in pane 0.2"

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

# Function to display test results
display_test_results() {
	local results_file=$1
	local iteration=$2

	if [ -f "$results_file" ]; then
		tmux send-keys -t "$TMUX_SESSION:0.1" "clear" C-m
		tmux send-keys -t "$TMUX_SESSION:0.1" "echo '=== Test Results (Iteration $iteration) ==='" C-m
		tmux send-keys -t "$TMUX_SESSION:0.1" "echo ''" C-m
		tmux send-keys -t "$TMUX_SESSION:0.1" "cat '$results_file' | jq ." C-m
		tmux send-keys -t "$TMUX_SESSION:0.1" "" C-m
	fi
}

# Function to analyze test failure and send fix command to opencode
send_fix_command() {
	local results_file=$1

	# Read test results
	if [ ! -f "$results_file" ]; then
		return 1
	fi

	local failed_tests=$(jq -r '.tests[] | select(.status == "failed") | .name' "$results_file")
	local fix_sent=false

	# Clear opencode pane
	tmux send-keys -t "$TMUX_SESSION:0.2" "clear" C-m
	sleep 0.3

	tmux send-keys -t "$TMUX_SESSION:0.2" "echo 'Fixing...'" C-m
	sleep 0.3

	for test_name in $failed_tests; do
		log_warning "Failed test: $test_name"

		# Determine fix based on test type
		if [[ "$test_name" == *"Build"* ]]; then
			update_control_pane "Status: Running build fix..."
			"$SCRIPT_DIR/tests/integration/fix_patterns/fix_build.sh" >>/tmp/fix_log.txt 2>&1 || true
			fix_sent=true

		elif [[ "$test_name" == *"Plugin Loading"* ]]; then
			update_control_pane "Status: Running plugin fix..."
			"$SCRIPT_DIR/tests/integration/fix_patterns/fix_plugin_loading.sh" >>/tmp/fix_log.txt 2>&1 || true
			if [ -d "$PROJECT_ROOT/build" ]; then
				cd "$PROJECT_ROOT/build"
				ninja >>/tmp/rebuild_log.txt 2>&1 || true
			fi
			fix_sent=true

		elif [[ "$test_name" == *"Basic Functionality"* ]]; then
			update_control_pane "Status: Running functionality fix..."
			"$SCRIPT_DIR/tests/integration/fix_patterns/fix_basic_functionality.sh" >>/tmp/fix_log.txt 2>&1 || true
			if [ -d "$PROJECT_ROOT/build" ]; then
				cd "$PROJECT_ROOT/build"
				ninja >>/tmp/rebuild_log.txt 2>&1 || true
			fi
			fix_sent=true

		elif [[ "$test_name" == *"Crash Detection"* ]]; then
			update_control_pane "Status: Running crash fix..."
			"$SCRIPT_DIR/tests/integration/fix_patterns/fix_crash.sh" >>/tmp/fix_log.txt 2>&1 || true
			if [ -d "$PROJECT_ROOT/build" ]; then
				cd "$PROJECT_ROOT/build"
				ninja >>/tmp/rebuild_log.txt 2>&1 || true
			fi
			fix_sent=true

		elif [[ "$test_name" == *"Pre-flight"* ]]; then
			update_control_pane "Status: Running pre-flight fix..."
			"$SCRIPT_DIR/tests/integration/fix_patterns/fix_preflight.sh" >>/tmp/fix_log.txt 2>&1 || true
			fix_sent=true
		fi
	done

	if [ "$fix_sent" = true ]; then
		update_control_pane "Status: Waiting before next test..."
		sleep 2
	else
		update_control_pane "Status: No auto-fix available"
		sleep 1
	fi

	return 0
}

# Function to run one iteration
run_iteration() {
	ITERATION=$((ITERATION + 1))

	log ""
	log "=========================================="
	log "Iteration $ITERATION"
	log "=========================================="
	log ""

	# Update control pane with iteration number
	update_control_pane "Iteration: $ITERATION"
	update_control_pane "Status: Running tests..."

	# Clear test pane
	tmux send-keys -t "$TMUX_SESSION:0.1" "clear" C-m

	# Run tests
	log "Running tests..."
	tmux send-keys -t "$TMUX_SESSION:0.1" "echo '=== Running Tests (Iteration $ITERATION) ==='" C-m
	tmux send-keys -t "$TMUX_SESSION:0.1" "echo ''" C-m

	# Suppress output during test
	if "$SCRIPT_DIR/tests/integration/run_integration_tests.sh" >/tmp/test_iteration_$ITERATION.log 2>&1; then
		TEST_EXIT_CODE=0
	else
		TEST_EXIT_CODE=1
	fi

	# Find latest results
	local results_dir="$SCRIPT_DIR/tests/integration/results"
	local latest_results=$(ls -t "$results_dir"/results_*.json 2>/dev/null | head -1)

	if [ ! -f "$latest_results" ]; then
		log_error "No test results found"
		update_control_pane "Status: ERROR - No results"
		return 1
	fi

	# Display test results
	display_test_results "$latest_results" "$ITERATION"

	# Check if all tests passed
	if check_tests_passed "$latest_results"; then
		log_success "All tests passed!"
		update_control_pane "Status: SUCCESS - All tests passed"
		return 0
	fi

	# Tests failed - send fix commands to opencode
	log_warning "Tests failed. Sending fix commands to opencode..."

	# Get failed count
	local failed=$(jq -r '.summary.failed' "$latest_results")
	log_warning "$failed test(s) failed"

	update_control_pane "Status: FAILED - $failed test(s) failed"

	# Send fix commands
	send_fix_command "$latest_results"

	return 1
}

# Main function
main() {
	log "Starting Interactive Auto-Fix System"
	log "=========================================="
	log "This system will:"
	log "1. Run integration tests"
	log "2. If tests fail, send fix commands to opencode"
	log "3. Wait for opencode to fix issues"
	log "4. Re-run tests"
	log "5. Repeat until all tests pass"
	log "=========================================="
	log ""

	# Initialize panes
	log "Initializing tmux panes..."

	# Clear all panes
	tmux send-keys -t "$TMUX_SESSION:0.0" "clear" C-m
	sleep 0.3

	tmux send-keys -t "$TMUX_SESSION:0.1" "clear" C-m
	sleep 0.3

	tmux send-keys -t "$TMUX_SESSION:0.2" "clear" C-m
	sleep 0.3

	# Set headers
	tmux send-keys -t "$TMUX_SESSION:0.0" "echo 'Control Panel'" C-m
	tmux send-keys -t "$TMUX_SESSION:0.0" "echo ''" C-m
	tmux send-keys -t "$TMUX_SESSION:0.0" "echo 'Iteration: 0'" C-m
	sleep 0.3

	tmux send-keys -t "$TMUX_SESSION:0.1" "echo 'Test Results'" C-m
	sleep 0.3

	tmux send-keys -t "$TMUX_SESSION:0.2" "echo 'opencode Session'" C-m
	sleep 0.3

	# Start opencode
	start_opencode

	# Initial delay
	sleep 2

	# Main loop - background execution
	# Run in background so we can attach to tmux session
	(
		while true; do
			if run_iteration; then
				# Tests passed
				log ""
				log "=========================================="
				log_success "ALL TESTS PASSED!"
				log "=========================================="
				log ""
				log "Total iterations: $ITERATION"

				# Update control pane with success
				update_control_pane "=== SUCCESS ==="
				update_control_pane "All tests passed after $ITERATION iterations"
				update_control_pane ""
				update_control_pane "To exit tmux: Ctrl+B, D"
				update_control_pane "To close: tmux kill-session -t $TMUX_SESSION"

				# Keep session open
				while true; do
					sleep 10
				done

				exit 0
			fi

			# Tests failed, wait before next iteration
			log ""
			log "Waiting 5 seconds before next iteration..."
			log ""

			sleep 5
		done
	) &

	MAIN_PID=$!

	# Wait a bit for the background process to start
	sleep 3

	# Auto-attach to tmux session
	log ""
	log "Auto-attaching to tmux session..."
	log "To detach from tmux: Ctrl+B, D"
	log ""

	# Attach to tmux session (this will block)
	exec tmux attach -t "$TMUX_SESSION"

	# This will only execute if tmux exits
	log "tmux session ended."
	kill $MAIN_PID 2>/dev/null || true
}

# Run main function
main
