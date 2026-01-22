#!/bin/bash
# Performance Regression Detection Script
# Runs performance benchmarks and compares against baseline

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
BASELINE_DIR="results/baselines"
CURRENT_DIR="results/current"
REPORT_DIR="results/reports"
REGRESSION_THRESHOLD=10

# Create directories
mkdir -p "$BASELINE_DIR"
mkdir -p "$CURRENT_DIR"
mkdir -p "$REPORT_DIR"

# Baseline file
BASELINE_FILE="$BASELINE_DIR/performance_baseline.json"

# Check if baseline exists
if [ ! -f "$BASELINE_FILE" ]; then
	echo -e "${YELLOW}Warning: Baseline file not found at $BASELINE_FILE${NC}"
	echo "Creating new baseline from current run..."

	# Run benchmark and save as baseline
	./scripts/run-perf-benchmark.sh \
		--output "$BASELINE_FILE" \
		--format json \
		--save-baseline "$BASELINE_FILE"

	echo -e "${GREEN}✓ New baseline created${NC}"
	exit 0
fi

# Current timestamp
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
CURRENT_FILE="$CURRENT_DIR/performance_$TIMESTAMP.json"

# Run benchmark with baseline comparison
echo -e "${GREEN}Running performance benchmarks...${NC}"
./scripts/run-perf-benchmark.sh \
	--baseline "$BASELINE_FILE" \
	--output "$CURRENT_FILE" \
	--format json

EXIT_CODE=$?

# Check exit code
if [ $EXIT_CODE -ne 0 ]; then
	echo -e "${RED}✗ Performance regressions detected!${NC}"
	echo ""
	echo "Review the output above for details."
	echo ""
	echo "If this is expected (e.g., due to intentional algorithm changes),"
	echo "update the baseline with:"
	echo "  cp $CURRENT_FILE $BASELINE_FILE"
	exit 1
else
	echo -e "${GREEN}✓ No performance regressions detected${NC}"

	# Optionally update baseline for small improvements
	echo ""
	echo "Current results saved to: $CURRENT_FILE"
	echo "Baseline file: $BASELINE_FILE"

	exit 0
fi
