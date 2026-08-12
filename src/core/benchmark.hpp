/*
 * OBS Stabilizer Plugin - Performance Benchmarking Framework
 * Comprehensive performance testing infrastructure
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <numeric>

namespace PERF {

/** Metrics captured for one benchmark scenario. */
struct BenchmarkMetrics {
    /** Human-readable scenario identifier. */
    std::string scenario_name = "";
    /** Input frame width in pixels. */
    int resolution_width = 0;
    /** Input frame height in pixels. */
    int resolution_height = 0;
    /** Nominal source frame rate in frames per second. */
    int frame_rate = 0;

    /** Mean processing latency in milliseconds. */
    double avg_processing_time_ms = 0.0;
    /** Minimum observed processing latency in milliseconds. */
    double min_processing_time_ms = 0.0;
    /** Maximum observed processing latency in milliseconds. */
    double max_processing_time_ms = 0.0;
    /** Standard deviation of processing latency in milliseconds. */
    double std_deviation_ms = 0.0;

    /** Peak resident memory observed during the scenario, in bytes. */
    size_t peak_memory_bytes = 0;
    /** Average resident memory observed during the scenario, in bytes. */
    size_t avg_memory_bytes = 0;

    /** Whether the scenario satisfied its configured acceptance criteria. */
    bool passed = false;
    /** Explanation populated when passed is false. */
    std::string failure_reason = "";

    /** Maximum target latency per frame in milliseconds. */
    double target_processing_time_ms = 0.0;
    /** Whether measured latency meets the target frame-time budget. */
    bool meets_realtime_requirement = false;
};

/** Runtime options controlling a benchmark run. */
struct BenchmarkConfig {
    /** Number of measured frames to process. */
    int num_frames;
    /** Enable resident-memory sampling. */
    bool enable_memory_tracking;
    /** Enable detailed profiling hooks. */
    bool enable_profiling;
    /** Number of warm-up frames excluded from measurements. */
    int warmup_frames;
    /** Result serialization format, such as csv or json. */
    std::string output_format;
    /** Destination path for serialized results. */
    std::string output_file;
};

/** Standard workloads supported by the benchmark runner. */
enum class TestScenario {
    STATIC_SCENE,
    SLOW_PAN,
    FAST_SHAKE,
    ZOOM_OPERATION,
    COMPLEX_BACKGROUND,
    EXTENDED_RUN,
    RESOLUTION_480P,
    RESOLUTION_720P,
    RESOLUTION_1080P,
    RESOLUTION_1440P,
    RESOLUTION_4K
};

/** Executes deterministic stabilization benchmark scenarios and stores results. */
class BenchmarkRunner {
public:
    BenchmarkRunner();
    ~BenchmarkRunner();

    /** Replace the configuration used by subsequent benchmark runs. */
    void set_config(const BenchmarkConfig& config);
    /** Return the currently active benchmark configuration. */
    BenchmarkConfig get_config() const;

    /** Execute one benchmark scenario and append its metrics. */
    void run_scenario(TestScenario scenario);
    /** Execute every standard benchmark scenario. */
    void run_all_scenarios();

    /** Return all metrics collected by this runner. */
    std::vector<BenchmarkMetrics> get_results() const;
    /** Save results using the configured/default serialization format. */
    void save_results(const std::string& filename);
    /** Save results as comma-separated values. */
    void save_results_csv(const std::string& filename);
    /** Save results as JSON. */
    void save_results_json(const std::string& filename);

    /** Load reference metrics used for regression comparison. */
    bool load_baseline(const std::string& filename);
    /** Persist current metrics as a regression baseline. */
    bool save_baseline(const std::string& filename);
    /** Compare current results against the loaded baseline. */
    bool compare_against_baseline();

    /** Print a human-readable summary to standard output. */
    void print_summary();

private:
    BenchmarkConfig config_;
    std::vector<BenchmarkMetrics> results_;
    std::map<std::string, BenchmarkMetrics> baselines_;

    double calculate_std_deviation(const std::vector<double>& values);
    std::string scenario_to_string(TestScenario scenario) const;
};

/** Miscellaneous timing, memory, and reporting helpers for benchmarks. */
namespace Utils {
    /** Stopwatch based on std::chrono::high_resolution_clock. */
    class Timer {
    public:
        Timer();
        /** Start or restart elapsed-time measurement. */
        void start();
        /** Stop elapsed-time measurement. */
        void stop();
        /** Return elapsed time in milliseconds. */
        double elapsed_ms() const;

    private:
        std::chrono::high_resolution_clock::time_point start_time_;
        std::chrono::high_resolution_clock::time_point end_time_;
        bool running_;
    };

    /** Return current process memory usage in bytes when supported. */
    size_t get_current_memory_usage();
    /** Return peak process memory usage in bytes when supported. */
    size_t get_peak_memory_usage();

    /** Print a visual separator in benchmark reports. */
    void print_separator();
    /** Print a named floating-point metric with its unit. */
    void print_metric(const std::string& name, double value, const std::string& unit);
    /** Print a named integer metric with its unit. */
    void print_metric(const std::string& name, size_t value, const std::string& unit);
}

} // namespace PERF
