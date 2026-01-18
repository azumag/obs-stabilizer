#!/bin/bash

# Auto-fix loop with tmux integration
# Runs tests, auto-fixes issues, and repeats until all tests pass

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$(dirname "$SCRIPT_DIR")" && pwd)"

# Configuration
MAX_ITERATIONS=10
ITERATION=0
AUTO_FIX_ENABLED=true
TMUX_SESSION="obs-stabilizer-fix"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Kill any existing tmux session
tmux kill-session -t "$TMUX_SESSION" 2>/dev/null || true

# Create new tmux session
tmux new-session -d -s "$TMUX_SESSION" -c "$PROJECT_ROOT"

log() {
	echo -e "${BLUE}[Auto-Fix Loop]${NC} $1"
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

# Function to count iterations
run_iteration() {
	ITERATION=$((ITERATION + 1))

	echo ""
	echo "=========================================="
	echo "Iteration $ITERATION / $MAX_ITERATIONS"
	echo "=========================================="
	echo ""

	# Run tests
	log "Running tests (iteration $ITERATION)..."
	"$SCRIPT_DIR/tests/integration/run_integration_tests.sh"

	TEST_EXIT_CODE=$?

	# Find latest results
	local results_dir="$SCRIPT_DIR/tests/integration/results"
	local latest_results=$(ls -t "$results_dir"/results_*.json 2>/dev/null | head -1)

	if [ ! -f "$latest_results" ]; then
		log_error "No test results found"
		return 1
	fi

	# Check if all tests passed
	if check_tests_passed "$latest_results"; then
		log_success "All tests passed!"
		return 0
	fi

	# Tests failed - analyze and fix
	log_warning "Tests failed. Analyzing..."

	# Display test summary in tmux pane
	tmux send-keys -t "$TMUX_SESSION:0.0" "clear" C-m
	tmux send-keys -t "$TMUX_SESSION:0.0" "cat '$latest_results' | jq ." C-m
	sleep 2

	# Split tmux pane for auto-fix output
	tmux split-pane -v -t "$TMUX_SESSION:0"

	# Apply auto-fix
	if [ "$AUTO_FIX_ENABLED" = true ]; then
		log "Applying auto-fix..."

		# Display auto-fix in bottom pane
		tmux send-keys -t "$TMUX_SESSION:0.1" "echo '=== Running Auto-Fix ==='" C-m
		tmux send-keys -t "$TMUX_SESSION:0.1" "cd '$PROJECT_ROOT'" C-m
		tmux send-keys -t "$TMUX_SESSION:0.1" "python3 '$SCRIPT_DIR/tests/integration/auto_fix.py' 2>&1 | tee /tmp/autofix_output.log" C-m

		# Wait for auto-fix to complete
		sleep 5

		# Check if fixes were applied
		if grep -q "Fixes Applied" /tmp/autofix_output.log 2>/dev/null; then
			log "Auto-fix applied. Waiting 2 seconds..."
			sleep 2
		else
			log_warning "No fixes were applied or auto-fix failed"
		fi

		# Close bottom pane
		tmux kill-pane -t "$TMUX_SESSION:0.1"
	else
		log_warning "Auto-fix disabled"
		return 1
	fi

	return 1
}

# Main loop
log "Starting auto-fix loop (max $MAX_ITERATIONS iterations)"

while [ $ITERATION -lt $MAX_ITERATIONS ]; do
	if run_iteration; then
		# Tests passed
		log_success "All tests passed after $ITERATION iteration(s)"

		# Display final results in tmux
		tmux send-keys -t "$TMUX_SESSION:0.0" "clear" C-m
		tmux send-keys -t "$TMUX_SESSION:0.0" "echo '=== All Tests Passed ==='" C-m
		tmux send-keys -t "$TMUX_SESSION:0.0" "echo 'Iterations: $ITERATION'" C-m
		tmux send-keys -t "$TMUX_SESSION:0.0" "echo ''" C-m
		tmux send-keys -t "$TMUX_SESSION:0.0" "cat '$SCRIPT_DIR/tests/integration/results/results_*.json' | jq ." C-m

		# Keep tmux session open for review
		log "Results displayed in tmux session: $TMUX_SESSION"
		log "Attach with: tmux attach -t $TMUX_SESSION"
		log "Close with: tmux kill-session -t $TMUX_SESSION"

		exit 0
	fi

	# Tests failed, continue loop
	log "Iteration $ITERATION complete. Tests failed, continuing..."
	echo ""

	# Wait a bit before next iteration
	sleep 2
done

# Max iterations reached
log_error "Maximum iterations ($MAX_ITERATIONS) reached without all tests passing"

# Display final status in tmux
tmux send-keys -t "$TMUX_SESSION:0.0" "clear" C-m
tmux send-keys -t "$TMUX_SESSION:0.0" "echo '=== Auto-Fix Loop Failed ==='" C-m
tmux send-keys -t "$TMUX_SESSION:0.0" "echo 'Iterations: $MAX_ITERATIONS / $MAX_ITERATIONS'" C-m
tmux send-keys -t "$TMUX_SESSION:0.0" "echo 'Status: FAILED - Max iterations reached'" C-m
tmux send-keys -t "$TMUX_SESSION:0.0" "echo ''" C-m
tmux send-keys -t "$TMUX_SESSION:0.0" "cat '$SCRIPT_DIR/tests/integration/results/results_*.json' | jq ." C-m

log "Results displayed in tmux session: $TMUX_SESSION"
log "Attach with: tmux attach -t $TMUX_SESSION"
log "Close with: tmux kill-session -t $TMUX_SESSION"

exit 1
