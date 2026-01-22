# Performance Testing Guide

## Overview

The OBS Stabilizer plugin includes a comprehensive performance testing infrastructure that enables continuous monitoring of performance characteristics, automatic detection of performance regressions, and guidance for optimization efforts.

## Architecture

The performance testing system consists of the following components:

- **Benchmark Runner** (`src/core/benchmark.cpp`): Orchestrates execution of performance test scenarios
- **Regression Detector** (`src/core/performance_regression.cpp`): Automatically detects performance degradations
- **Benchmark Executable** (`tools/performance_benchmark.cpp`): Command-line interface for running benchmarks
- **Test Scenarios**: Predefined test cases for different use cases and resolutions
- **CI/CD Integration**: GitHub Actions workflow for automated performance testing

## Quick Start

### Building the Performance Testing Infrastructure

The performance testing infrastructure is built automatically when you build the project:

```bash
cmake -B build
cmake --build build
```

This will create the `performance_benchmark` executable in the `build/` directory.

### Running Quick Performance Validation

For quick validation during development:

```bash
./scripts/quick-perf.sh
```

This runs a limited benchmark (1080p, 100 frames) and provides immediate feedback.

### Running Full Benchmark Suite

To run the complete benchmark suite:

```bash
./scripts/run-perf-benchmark.sh
```

### Running Regression Detection

To compare current performance against baseline:

```bash
./scripts/run-perf-regression.sh
```

This will:
1. Run benchmarks
2. Compare results against baseline
3. Fail if regression detected (>10% degradation)

## Command-Line Options

### Basic Usage

```bash
./build/performance_benchmark [OPTIONS]
```

### Options

| Option | Description | Default |
|--------|-------------|----------|
| `--scenario <name>` | Run specific scenario (480p, 720p, 1080p, 1440p, 4k) | All scenarios |
| `--frames <num>` | Number of frames to process | 1000 |
| `--warmup <num>` | Warmup frames to skip | 10 |
| `--output <file>` | Output file path | performance_results.csv |
| `--format <fmt>` | Output format: csv or json | csv |
| `--baseline <file>` | Compare against baseline file | None |
| `--save-baseline <file>` | Save results as baseline | None |
| `--no-memory` | Disable memory tracking | Enabled |
| `--help` | Show help message | - |

### Examples

Run specific scenario:
```bash
./build/performance_benchmark --scenario 1080p --frames 500
```

Save results as JSON:
```bash
./build/performance_benchmark --output results.json --format json
```

Compare against baseline:
```bash
./build/performance_benchmark --baseline baseline.json --output results.csv
```

## Test Scenarios

### Resolution Scenarios

| Scenario | Resolution | Target | Use Case |
|----------|------------|--------|----------|
| 480p | 640x480 | 33.33ms/frame (30fps) | Low-end systems |
| 720p | 1280x720 | 16.67ms/frame (60fps) | HD streaming |
| 1080p | 1920x1080 | 33.33ms/frame (30fps) | Full HD |
| 1440p | 2560x1440 | 33.33ms/frame (30fps) | QHD |
| 4K | 3840x2160 | 33.33ms/frame (30fps) | Ultra HD |

### Metrics Collected

For each scenario, the following metrics are collected:

- **Average Processing Time**: Mean time to process a frame
- **Min/Max Processing Time**: Range of processing times
- **Standard Deviation**: Consistency of processing times
- **Peak Memory Usage**: Maximum memory consumed
- **Average Memory Usage**: Average memory consumption
- **Pass/Fail Status**: Whether real-time requirements are met
- **Real-time Capability**: Can maintain target frame rate

## Output Formats

### CSV Format

CSV output is suitable for spreadsheet analysis and quick review:

```csv
scenario_name,resolution_width,resolution_height,frame_rate,
avg_processing_time_ms,min_processing_time_ms,max_processing_time_ms,std_deviation_ms,
peak_memory_bytes,avg_memory_bytes,passed,meets_realtime_requirement,failure_reason

Resolution 1080p,1920,1080,30,10.5,9.8,12.3,0.8,52428800,51118000,true,true,
```

### JSON Format

JSON output is suitable for automated processing and CI/CD integration:

```json
{
  "benchmark_results": [
    {
      "scenario_name": "Resolution 1080p",
      "resolution_width": 1920,
      "resolution_height": 1080,
      "frame_rate": 30,
      "avg_processing_time_ms": 10.5,
      "min_processing_time_ms": 9.8,
      "max_processing_time_ms": 12.3,
      "std_deviation_ms": 0.8,
      "peak_memory_bytes": 52428800,
      "avg_memory_bytes": 51118000,
      "passed": true,
      "meets_realtime_requirement": true,
      "failure_reason": ""
    }
  ]
}
```

## Regression Detection

### Thresholds

The following thresholds are used for regression detection:

| Metric | Degradation Threshold | Severity |
|--------|----------------------|----------|
| Processing Time | +10% | Warning |
| Processing Time | +20% | Critical |
| Memory Usage | +20% | Warning |
| Frame Rate | -5% | Critical |

### Creating a Baseline

To establish a performance baseline:

```bash
./scripts/run-perf-benchmark.sh --output results/baselines/performance_baseline.json --format json
```

### Updating Baseline

If you've made intentional performance changes (e.g., algorithm improvements):

```bash
./scripts/run-perf-benchmark.sh --output new_baseline.json --format json
cp new_baseline.json results/baselines/performance_baseline.json
```

## CI/CD Integration

### GitHub Actions

Performance tests run automatically on:
- Pull requests to main
- Pushes to main
- Manual workflow dispatch

### Quick Tests (PRs)

On pull requests, quick performance tests run:
- 1080p scenario only
- 100 frames
- ~30 seconds total runtime
- Results compared against baseline if available

### Full Benchmarks (Main)

On main branch or manual dispatch, full benchmarks run:
- All resolution scenarios
- 1000 frames each
- ~5-10 minutes total runtime
- Results archived for historical tracking

### Regression Notifications

If a regression is detected:
- CI/CD job fails
- PR comment added with details
- Performance artifacts uploaded for review
- Issue automatically created for tracking

## Interpreting Results

### Understanding the Output

After running benchmarks, you'll see output like:

```
======================================================================
Scenario: Resolution 1080p (1920x1080)
  Avg: 10.50 ms (95.2 fps)
  Min: 9.80 ms, Max: 12.30 ms
  StdDev: 0.80 ms
  Target: <33.33 ms/frame
  Status: ✅ PASS
```

**Key Metrics:**
- **Avg**: Average processing time. Lower is better.
- **Min/Max**: Range of processing times. Consistent times (low spread) indicate stable performance.
- **StdDev**: Standard deviation. Lower is better for consistent performance.
- **FPS Capacity**: How many frames per second the system can handle.
- **Status**: Pass if meets real-time target, Fail otherwise.

### Performance Targets

Based on ARCHITECTURE.md requirements:

| Resolution | Target Time | Real-time Frame Rate |
|------------|--------------|---------------------|
| 480p | <2ms/frame | 60fps+ |
| 720p | <2ms/frame | 60fps+ |
| 1080p | <4ms/frame | 30fps+ |
| 1440p | <8ms/frame | 30fps+ |
| 4K | <15ms/frame | 30fps+ |

### Analyzing Regressions

When a regression is detected:

1. **Review the change**: Look at code changes in the PR
2. **Check the magnitude**: +10% is warning, +20% is critical
3. **Consider intent**: Was this change expected to impact performance?
4. **Verify consistency**: Does regression appear across all platforms or just one?
5. **Investigate**: Use profiling tools to identify bottlenecks

## Performance Optimization

### Identifying Bottlenecks

If performance doesn't meet targets:

1. **Review Metrics**: Look at avg/min/max times
2. **Check Consistency**: High std_deviation suggests variable processing
3. **Memory Profile**: High memory usage may cause caching issues
4. **Algorithm Review**: Consider algorithmic improvements

### Optimization Strategies

Common optimization approaches:

- **Reduce Feature Count**: Fewer tracking points = faster processing
- **Smaller ROI**: Track only region of interest
- **Optimized Algorithms**: Use faster algorithms for specific cases
- **Parallel Processing**: Utilize multi-core CPUs
- **SIMD Acceleration**: Use platform-specific SIMD instructions

### Testing Optimizations

When making performance improvements:

1. Run baseline before changes
2. Make changes incrementally
3. Test after each change
4. Compare against baseline
5. Document improvements

## Troubleshooting

### Benchmark Fails to Build

**Problem**: Build errors when building performance_benchmark

**Solutions**:
- Ensure OpenCV is installed: `brew install opencv` (macOS) or `sudo apt install libopencv-dev` (Linux)
- Check CMake configuration: `cmake -B build -DENABLE_PERFORMANCE_TESTS=ON`
- Verify C++17 compiler support

### Inconsistent Results

**Problem**: Benchmark results vary between runs

**Solutions**:
- Close other applications to reduce system load
- Run benchmarks multiple times and average
- Use consistent frame counts and warmup periods
- Set CPU governor to fixed frequency (Linux)

### Memory Tracking Not Working

**Problem**: Memory metrics show 0

**Solutions**:
- macOS: Memory tracking works natively
- Linux: Ensure `/proc/self/status` is readable
- Windows: Memory tracking may be limited

### Baseline Comparison Fails

**Problem**: Baseline comparison shows errors

**Solutions**:
- Verify baseline file exists and is valid JSON
- Check scenario names match between baseline and current
- Ensure same resolution settings
- Review baseline creation timestamp

## Best Practices

### Development Workflow

1. **Quick Validation**: Run `./scripts/quick-perf.sh` after changes
2. **Pre-Commit Testing**: Run full benchmarks before major PRs
3. **Baseline Updates**: Update baselines only after intentional changes
4. **Review Regressions**: Investigate all performance regressions
5. **Track History**: Archive results for trend analysis

### Continuous Monitoring

- **Daily Builds**: Run benchmarks on main branch daily
- **PR Validation**: Quick tests on all pull requests
- **Release Testing**: Full benchmarks before releases
- **Platform Coverage**: Test on all supported platforms

### Documentation

- **Record Baselines**: Document baseline values in PRs
- **Explain Changes**: Comment on performance impacts in code
- **Track Improvements**: Document optimization efforts
- **Share Results**: Publish performance reports for users

## Advanced Usage

### Custom Scenarios

To create custom test scenarios, modify `src/core/benchmark.cpp`:

```cpp
enum class TestScenario {
    // ... existing scenarios ...
    CUSTOM_SCENE  // Add your custom scenario
};
```

### Custom Thresholds

To adjust regression thresholds, modify `performance_regression.cpp`:

```cpp
struct RegressionThresholds {
    double processing_time_degradation_percent = 15.0;  // Adjust as needed
    // ...
};
```

### Integration with Other Tools

The JSON output can be integrated with:
- Grafana dashboards
- Performance monitoring systems
- Custom analysis scripts
- CI/CD notification systems

## Support

For issues or questions:
- Check `docs/performance-testing-architecture.md` for technical details
- Review GitHub issues for similar problems
- Open new issue with benchmark results attached
- Contact maintainers via GitHub discussions

## References

- **Architecture**: `docs/performance-testing-architecture.md`
- **Existing Performance Test**: `tools/performance-test.cpp`
- **Core Implementation**: `src/core/stabilizer_core.cpp`
- **README Performance Section**: `README.md` - Performance Characteristics
