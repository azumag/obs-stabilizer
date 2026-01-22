#!/bin/bash
# Performance Testing Quick Script
# Quick performance validation for development

set -e

GREEN='\033[0;32m'
NC='\033[0m'

echo "Running quick performance validation..."
echo ""

# Run limited benchmark set (1080p only, fewer frames)
./scripts/run-perf-benchmark.sh \
	--scenario 1080p \
	--frames 100 \
	--warmup 5 \
	--output quick_perf_results.csv

echo ""
echo -e "${GREEN}✓ Quick performance validation complete${NC}"
echo "Results saved to: quick_perf_results.csv"
