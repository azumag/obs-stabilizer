#!/bin/bash

# Quick test runner script for OBS Stabilizer
# Run from project root to execute all integration tests

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "=========================================="
echo "OBS Stabilizer Test System"
echo "=========================================="
echo ""
echo "Choose test mode:"
echo "1) Quick test (run tests once)"
echo "2) Dual opencode (test + fix in separate panes)"
echo "3) 3-pane layout (control + tests + opencode)"
echo "4) Automated auto-fix loop (tmux, no opencode interaction)"
echo ""
read -p "Enter choice [1-4]: " choice

case $choice in
1)
	echo ""
	echo "Running quick test..."
	if [ ! -f "$SCRIPT_DIR/tests/integration/run_integration_tests.sh" ]; then
		echo "ERROR: Integration tests not found"
		exit 1
	fi

	"$SCRIPT_DIR/tests/integration/run_integration_tests.sh"
	EXIT_CODE=$?

	echo ""
	if [ $EXIT_CODE -eq 0 ]; then
		echo "All tests passed!"
	else
		echo "Some tests failed. Check logs in tests/integration/results/"
	fi
	exit $EXIT_CODE
	;;
2)
	echo ""
	echo "Starting dual opencode mode..."
	echo "This will:"
	echo "  - Create a tmux session with 2 panes"
	echo "  - Left pane: Run tests via opencode"
	echo "  - Right pane: Apply fixes via opencode"
	echo "  - Repeat until all tests pass"
	echo "  - Auto-attach to tmux session"
	echo ""
	echo "To detach from tmux: Ctrl+B, D"
	echo "To close tmux session: tmux kill-session -t obs-stabilizer-opencode-dual"
	echo ""

	if [ ! -f "$SCRIPT_DIR/run_dual_opencode.sh" ]; then
		echo "ERROR: Dual opencode script not found"
		exit 1
	fi

	exec "$SCRIPT_DIR/run_dual_opencode.sh"
	;;
3)
	echo ""
	echo "Starting interactive auto-fix with opencode..."
	echo "This will:"
	echo "  - Create a tmux session with 3 panes"
	echo "  - Run tests and send fix commands to opencode"
	echo "  - Repeat until all tests pass"
	echo "  - Auto-attach to tmux session"
	echo ""
	echo "To detach from tmux: Ctrl+B, D"
	echo "To close tmux session: tmux kill-session -t obs-stabilizer-opencode-fix"
	echo ""

	if [ ! -f "$SCRIPT_DIR/run_interactive_fix.sh" ]; then
		echo "ERROR: Interactive fix script not found"
		exit 1
	fi

	exec "$SCRIPT_DIR/run_interactive_fix.sh"
	;;
4)
	echo ""
	echo "Starting automated auto-fix loop..."
	echo "This will:"
	echo "  - Create a tmux session"
	echo "  - Run tests and auto-fix without opencode interaction"
	echo "  - Repeat until all tests pass or max iterations reached"
	echo ""
	echo "Attach to tmux session: tmux attach -t obs-stabilizer-auto-fix"
	echo "Close tmux session: tmux kill-session -t obs-stabilizer-auto-fix"
	echo ""

	if [ ! -f "$SCRIPT_DIR/run_auto_fix.sh" ]; then
		echo "ERROR: Auto-fix script not found"
		exit 1
	fi

	exec "$SCRIPT_DIR/run_auto_fix.sh"
	;;
*)
	echo "Invalid choice. Please enter 1, 2, 3, or 4."
	exit 1
	;;
esac
