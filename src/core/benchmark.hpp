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

// Performance metrics for a single benchmark run
struct BenchmarkMetrics {
    // Test scenario information
    std::string scenario_name;
    int resolution_width;
    int resolution_height;
    int frame_rate;
    
    // Timing metrics (milliseconds)
    double avg_processing_time_ms;
    double min_processing_time_ms;
    double max_processing_time_ms;
    double std_deviation_ms;
    
    // Memory metrics (bytes)
    size_t peak_memory_bytes;
    size_t avg_memory_bytes;
    
    // Status
    bool passed;
    std::string failure_reason;
    
    // Performance target
    double target_processing_time_ms;  // e.g., 33.3ms for 30fps
    bool meets_realtime_requirement;
};

// Benchmark configuration
struct BenchmarkConfig {
    int num_frames;           // Number of frames to process
    bool enable_memory_tracking;  // Track memory usage
    bool enable_profiling;     // Detailed function profiling
    int warmup_frames;        // Frames to skip before measurement
    std::string output_format;  // "csv", "json"
    std::string output_file;    // Output file path
};

// Performance test scenario
enum class TestScenario {
    STATIC_SCENE,           // Baseline static processing
    SLOW_PAN,              // Slow panning motion
    FAST_SHAKE,             // Fast camera shake
    ZOOM_OPERATION,          // Zoom in/out
    COMPLEX_BACKGROUND,       // Complex scene tracking
    EXTENDED_RUN,           // Extended run for memory stability
    RESOLUTION_480P,        // 480x360
    RESOLUTION_720P,        // 1280x720
    RESOLUTION_1080P,       // 1920x1080
    RESOLUTION_1440P,       // 2560x1440
    RESOLUTION_4K            // 3840x2160
};

// Benchmark runner class
class BenchmarkRunner {
public:
    BenchmarkRunner();
    ~BenchmarkRunner();
    
    // Configuration
    void set_config(const BenchmarkConfig& config);
    BenchmarkConfig get_config() const;
    
    // Benchmark execution
    void run_scenario(TestScenario scenario);
    void run_all_scenarios();
    
    // Results
    std::vector<BenchmarkMetrics> get_results() const;
    void save_results(const std::string& filename);
    void save_results_csv(const std::string& filename);
    void save_results_json(const std::string& filename);
    
    // Baseline management
    bool load_baseline(const std::string& filename);
    bool save_baseline(const std::string& filename);
    bool compare_against_baseline();
    
    // Statistics
    void print_summary();
    
private:
    BenchmarkConfig config_;
    std::vector<BenchmarkMetrics> results_;
    std::map<std::string, BenchmarkMetrics> baselines_;
    
    // Internal utilities
    double calculate_std_deviation(const std::vector<double>& values);
    std::string scenario_to_string(TestScenario scenario) const;
};

// Utility functions
namespace Utils {
    // Timing utilities
    class Timer {
    public:
        Timer();
        void start();
        void stop();
        double elapsed_ms() const;
        
    private:
        std::chrono::high_resolution_clock::time_point start_time_;
        std::chrono::high_resolution_clock::time_point end_time_;
        bool running_;
    };
    
    // Memory utilities
    size_t get_current_memory_usage();
    size_t get_peak_memory_usage();
    
    // Reporting utilities
    void print_separator();
    void print_metric(const std::string& name, double value, const std::string& unit);
    void print_metric(const std::string& name, size_t value, const std::string& unit);
}

} // namespace PERF
