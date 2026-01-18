# Integration Test Scripts for OBS Stabilizer

This directory contains automated integration tests with auto-fix capabilities for the OBS Stabilizer plugin.

## Quick Start

```bash
# Run all integration tests with auto-fix
./run_integration_tests.sh

# View latest results
cat results/results_*.json | jq .

# Check logs
cat results/test_*.log
```

## Test Suite

| Test | Description | Auto-Fix Available |
|------|-------------|-------------------|
| Pre-flight | Environment checks | Yes |
| Build | Compilation & linking | Yes |
| Plugin Loading | OBS integration | Yes |
| Basic Functionality | Runtime behavior | Yes |
| Crash Detection | Stability testing | Yes |

## See README.md for detailed documentation
