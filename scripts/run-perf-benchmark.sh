#!/bin/bash
# Performance Benchmark Runner Script
# Runs the OBS Stabilizer performance benchmark suite

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default configuration
SCENARIO=""
FRAMES=1000
WARMUP=10
OUTPUT="performance_results.csv"
FORMAT="csv"
BASELINE=""
SAVE_BASELINE=""
ENABLE_MEMORY=true

# Parse command line arguments
while [[ $# -gt 0 ]]; do
	case $1 in
	--scenario)
		SCENARIO="$2"
		shift 2
		;;
	--frames)
		FRAMES="$2"
		shift 2
		;;
	--warmup)
		WARMUP="$2"
		shift 2
		;;
	--output)
		OUTPUT="$2"
		shift 2
		;;
	--format)
		FORMAT="$2"
		shift 2
		;;
	--baseline)
		BASELINE="$2"
		shift 2
		;;
	--save-baseline)
		SAVE_BASELINE="$2"
		shift 2
		;;
	--no-memory)
		ENABLE_MEMORY=false
		shift
		;;
	--help)
		echo "Usage: $0 [OPTIONS]"
		echo ""
		echo "Options:"
		echo "  --scenario <name>      Run specific scenario (480p, 720p, 1080p, 1440p, 4k)"
		echo "  --frames <num>         Number of frames to process (default: 1000)"
		echo "  --warmup <num>         Warmup frames to skip (default: 10)"
		echo "  --output <file>        Output file path (default: performance_results.csv)"
		echo "  --format <fmt>         Output format: csv or json (default: csv)"
		echo "  --baseline <file>      Compare against baseline file"
		echo "  --save-baseline <file> Save results as baseline"
		echo "  --no-memory           Disable memory tracking"
		echo "  --help                 Show this help message"
		echo ""
		echo "Examples:"
		echo "  $0 --scenario 1080p --frames 500"
		echo "  $0 --output results.json --format json"
		echo "  $0 --baseline baseline.json --output results.csv"
		exit 0
		;;
	*)
		echo -e "${RED}Error: Unknown option $1${NC}"
		echo "Use --help for usage information"
		exit 1
		;;
	esac
done

# Check if performance benchmark executable exists
BENCHMARK_BIN="./build/performance_benchmark"
if [ ! -f "$BENCHMARK_BIN" ]; then
	echo -e "${RED}Error: Performance benchmark executable not found at $BENCHMARK_BIN${NC}"
	echo "Please build the project first with: cmake --build build"
	exit 1
fi

# Build command line arguments
CMD_ARGS=""
if [ -n "$SCENARIO" ]; then
	CMD_ARGS="$CMD_ARGS --scenario $SCENARIO"
fi
if [ -n "$FRAMES" ]; then
	CMD_ARGS="$CMD_ARGS --frames $FRAMES"
fi
if [ -n "$WARMUP" ]; then
	CMD_ARGS="$CMD_ARGS --warmup $WARMUP"
fi
if [ -n "$OUTPUT" ]; then
	CMD_ARGS="$CMD_ARGS --output $OUTPUT"
fi
if [ -n "$FORMAT" ]; then
	CMD_ARGS="$CMD_ARGS --format $FORMAT"
fi
if [ -n "$BASELINE" ]; then
	CMD_ARGS="$CMD_ARGS --baseline $BASELINE"
fi
if [ -n "$SAVE_BASELINE" ]; then
	CMD_ARGS="$CMD_ARGS --save-baseline $SAVE_BASELINE"
fi
if [ "$ENABLE_MEMORY" = false ]; then
	CMD_ARGS="$CMD_ARGS --no-memory"
fi

# Run benchmark
echo -e "${GREEN}Running OBS Stabilizer Performance Benchmark...${NC}"
echo ""
$BENCHMARK_BIN $CMD_ARGS
EXIT_CODE=$?

# Check exit code
if [ $EXIT_CODE -eq 0 ]; then
	echo -e "${GREEN}✓ Performance benchmarks completed successfully${NC}"
	if [ -n "$OUTPUT" ]; then
		echo -e "${GREEN}Results saved to: $OUTPUT${NC}"
	fi
	exit 0
else
	echo -e "${RED}✗ Performance benchmarks failed${NC}"
	exit 1
fi
