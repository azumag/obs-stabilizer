#!/bin/bash

# Fully Automated Test and Fix Loop
# Runs tests, auto-fixes issues, and repeats until all tests pass
# NEVER asks for manual intervention - keeps trying automatically

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$(dirname "$SCRIPT_DIR")" && pwd)"

# Configuration
MAX_ITERATIONS=10
ITERATION=0
TMUX_SESSION="obs-stabilizer-auto-fix"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
NC='\033[0m'

# Kill any existing tmux session
tmux kill-session -t "$TMUX_SESSION" 2>/dev/null || true

# Create new tmux session with 3 panes
tmux new-session -d -s "$TMUX_SESSION" -c "$PROJECT_ROOT"
tmux split-window -v -t "$TMUX_SESSION:0" -c "$PROJECT_ROOT"
tmux split-window -h -t "$TMUX_SESSION:0.1" -c "$PROJECT_ROOT"

# Pane 0.0: Main loop output (top-left)
# Pane 0.1: Test results (bottom-left)
# Pane 0.2: Auto-fix output (bottom-right)

log() {
	local msg=$1
	echo -e "${BLUE}[Auto-Fix Loop]${NC} $msg"
	tmux send-keys -t "$TMUX_SESSION:0.0" "echo '$msg'" C-m
}

log_success() {
	local msg=$1
	echo -e "${GREEN}[SUCCESS]${NC} $msg"
	tmux send-keys -t "$TMUX_SESSION:0.0" "echo '$msg'" C-m
}

log_error() {
	local msg=$1
	echo -e "${RED}[ERROR]${NC} $msg"
	tmux send-keys -t "$TMUX_SESSION:0.0" "echo '$msg'" C-m
}

log_warning() {
	local msg=$1
	echo -e "${YELLOW}[WARNING]${NC} $msg"
	tmux send-keys -t "$TMUX_SESSION:0.0" "echo '$msg'" C-m
}

log_fix() {
	local msg=$1
	echo -e "${MAGENTA}[FIX]${NC} $msg"
	tmux send-keys -t "$TMUX_SESSION:0.2" "echo '$msg'" C-m
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

# Function to display test results in tmux
display_test_results() {
	local results_file=$1

	if [ -f "$results_file" ]; then
		tmux send-keys -t "$TMUX_SESSION:0.1" "clear" C-m
		tmux send-keys -t "$TMUX_SESSION:0.1" "echo '=== Test Results ==='" C-m
		tmux send-keys -t "$TMUX_SESSION:0.1" "cat '$results_file' | jq ." C-m
		tmux send-keys -t "$TMUX_SESSION:0.1" "" C-m
	fi
}

# Function to run auto-fix
run_auto_fix() {
	log_fix "Running auto-fix..."

	tmux send-keys -t "$TMUX_SESSION:0.2" "clear" C-m
	tmux send-keys -t "$TMUX_SESSION:0.2" "echo '=== Auto-Fix ===' && echo ''" C-m

	# Run enhanced auto-fix
	python3 "$SCRIPT_DIR/tests/integration/auto_fix_enhanced.py" 2>&1 | while IFS= read -r line; do
		log_fix "$line"
	done

	# Check if fixes were applied
	local fix_summary="$PROJECT_ROOT/last_fix_summary.json"
	if [ -f "$fix_summary" ]; then
		local success=$(jq -r '.success' "$fix_summary")
		local fixes=$(jq -r '.fixes_applied' "$fix_summary")

		if [ "$success" = "true" ]; then
			log_fix "Fixes applied successfully"
			return 0
		else
			log_fix "No fixes were needed or possible"
			return 1
		fi
	else
		return 1
	fi
}

# Function to run one iteration
run_iteration() {
	ITERATION=$((ITERATION + 1))

	log ""
	log "=========================================="
	log "Iteration $ITERATION / $MAX_ITERATIONS"
	log "=========================================="
	log ""

	# Clear previous results in panes
	tmux send-keys -t "$TMUX_SESSION:0.1" "clear" C-m
	tmux send-keys -t "$TMUX_SESSION:0.2" "clear" C-m

	# Run tests
	log "Running tests (iteration $ITERATION)..."
	tmux send-keys -t "$TMUX_SESSION:0.1" "echo 'Running tests... (iteration $ITERATION)' && echo ''" C-m

	# Suppress normal output, only capture results
	if "$SCRIPT_DIR/tests/integration/run_integration_tests.sh" >/tmp/test_output_$ITERATION.log 2>&1; then
		TEST_EXIT_CODE=0
	else
		TEST_EXIT_CODE=1
	fi

	# Find latest results
	local results_dir="$SCRIPT_DIR/tests/integration/results"
	local latest_results=$(ls -t "$results_dir"/results_*.json 2>/dev/null | head -1)

	if [ ! -f "$latest_results" ]; then
		log_error "No test results found"
		return 1
	fi

	# Display test results
	display_test_results "$latest_results"

	# Check if all tests passed
	if check_tests_passed "$latest_results"; then
		log_success "All tests passed!"
		return 0
	fi

	# Tests failed - analyze and fix
	log_warning "Tests failed. Analyzing..."

	# Get failed test count
	local failed=$(jq -r '.summary.failed' "$latest_results")
	log_warning "$failed test(s) failed"

	# Apply auto-fix
	if run_auto_fix; then
		log "Auto-fix applied. Waiting 2 seconds..."
		sleep 2
	else
		log_warning "No fixes were applied"
		log "Continuing to next iteration..."
		sleep 1
	fi

	return 1
}

# Main loop
log "Starting fully automated test and fix loop"
log "Maximum iterations: $MAX_ITERATIONS"
log "NEVER asks for manual intervention"
log ""

# Initialize main pane
tmux send-keys -t "$TMUX_SESSION:0.0" "clear" C-m
tmux send-keys -t "$TMUX_SESSION:0.0" "echo '=== Auto-Fix Loop ===' && echo ''" C-m

# Initialize other panes
tmux send-keys -t "$TMUX_SESSION:0.1" "clear" C-m
tmux send-keys -t "$TMUX_SESSION:0.1" "echo '=== Test Results ==='" C-m
tmux send-keys -t "$TMUX_SESSION:0.2" "clear" C-m
tmux send-keys -t "$TMUX_SESSION:0.2" "echo '=== Auto-Fix ==='" C-m

while [ $ITERATION -lt $MAX_ITERATIONS ]; do
	if run_iteration; then
		# Tests passed
		log_success "All tests passed after $ITERATION iteration(s)"

		# Display final results
		tmux send-keys -t "$TMUX_SESSION:0.0" "clear" C-m
		tmux send-keys -t "$TMUX_SESSION:0.0" "echo '=========================================='" C-m
		tmux send-keys -t "$TMUX_SESSION:0.0" "echo 'SUCCESS: All Tests Passed!'" C-m
		tmux send-keys -t "$TMUX_SESSION:0.0" "echo '=========================================='" C-m
		tmux send-keys -t "$TMUX_SESSION:0.0" "echo ''" C-m
		tmux send-keys -t "$TMUX_SESSION:0.0" "echo 'Iterations: $ITERATION'" C-m
		tmux send-keys -t "$TMUX_SESSION:0.0" "echo ''" C-m
		tmux send-keys -t "$TMUX_SESSION:0.0" "echo 'Session: $TMUX_SESSION'" C-m
		tmux send-keys -t "$TMUX_SESSION:0.0" "echo 'Attach: tmux attach -t $TMUX_SESSION'" C-m
		tmux send-keys -t "$TMUX_SESSION:0.0" "echo 'Close: tmux kill-session -t $TMUX_SESSION'" C-m
		tmux send-keys -t "$TMUX_SESSION:0.0" "echo ''" C-m
		tmux send-keys -t "$TMUX_SESSION:0.0" "cat '$SCRIPT_DIR/tests/integration/results/results_*.json' | jq ." C-m

		# Keep tmux session open for review
		log ""
		log "Results displayed in tmux session: $TMUX_SESSION"
		log "Attach to view results: tmux attach -t $TMUX_SESSION"
		log "Close when done: tmux kill-session -t $TMUX_SESSION"

		# Wait in main pane
		tmux send-keys -t "$TMUX_SESSION:0.0" "echo '' && echo 'Press Enter to close tmux session...'" C-m
		tmux send-keys -t "$TMUX_SESSION:0.0" "read && tmux kill-session -t $TMUX_SESSION" C-m

		exit 0
	fi

	# Tests failed, continue loop
	log ""
	log "Iteration $ITERATION complete. Continuing to next iteration..."
	log ""

	# Short delay before next iteration
	sleep 3
done

# Max iterations reached
log_error ""
log_error "=========================================="
log_error "FAILED: Maximum iterations reached"
log_error "=========================================="
log_error "Iterations: $MAX_ITERATIONS / $MAX_ITERATIONS"
log_error "Status: Could not fix all issues"
log_error ""

# Display final status
tmux send-keys -t "$TMUX_SESSION:0.0" "clear" C-m
tmux send-keys -t "$TMUX_SESSION:0.0" "echo '=========================================='" C-m
tmux send-keys -t "$TMUX_SESSION:0.0" "echo 'FAILED: Maximum Iterations Reached'" C-m
tmux send-keys -t "$TMUX_SESSION:0.0" "echo '=========================================='" C-m
tmux send-keys -t "$TMUX_SESSION:0.0" "echo ''" C-m
tmux send-keys -t "$TMUX_SESSION:0.0" "echo 'Iterations: $MAX_ITERATIONS / $MAX_ITERATIONS'" C-m
tmux send-keys -t "$TMUX_SESSION:0.0" "echo 'Status: Could not fix all issues'" C-m
tmux send-keys -t "$TMUX_SESSION:0.0" "echo ''" C-m
tmux send-keys -t "$TMUX_SESSION:0.0" "echo 'Session: $TMUX_SESSION'" C-m
tmux send-keys -t "$TMUX_SESSION:0.0" "echo 'Attach: tmux attach -t $TMUX_SESSION'" C-m
tmux send-keys -t "$TMUX_SESSION:0.0" "echo 'Close: tmux kill-session -t $TMUX_SESSION'" C-m
tmux send-keys -t "$TMUX_SESSION:0.0" "echo ''" C-m
tmux send-keys -t "$TMUX_SESSION:0.0" "cat '$SCRIPT_DIR/tests/integration/results/results_*.json' | jq ." C-m

log ""
log "Results displayed in tmux session: $TMUX_SESSION"
log "Attach to view results: tmux attach -t $TMUX_SESSION"
log "Close when done: tmux kill-session -t $TMUX_SESSION"

exit 1
