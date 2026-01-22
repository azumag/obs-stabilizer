/*
 * OBS Stabilizer Plugin - Performance Benchmark Runner
 * Orchestrates execution of performance test suite
 */

#include "benchmark.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <iostream>
#include <thread>
#include <unistd.h>

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_host.h>
#endif

namespace PERF {

BenchmarkRunner::BenchmarkRunner()
    : config_({1000, true, false, 10, "csv", "performance_results.csv"}) {
}

BenchmarkRunner::~BenchmarkRunner() {
}

void BenchmarkRunner::set_config(const BenchmarkConfig& config) {
    config_ = config;
}

BenchmarkConfig BenchmarkRunner::get_config() const {
    return config_;
}

void BenchmarkRunner::run_scenario(TestScenario scenario) {
    BenchmarkMetrics metrics;
    metrics.scenario_name = scenario_to_string(scenario);
    metrics.passed = false;
    metrics.meets_realtime_requirement = false;
    
    // Setup resolution based on scenario
    switch (scenario) {
        case TestScenario::RESOLUTION_480P:
            metrics.resolution_width = 640;
            metrics.resolution_height = 480;
            metrics.target_processing_time_ms = 33.33; // 30fps
            break;
        case TestScenario::RESOLUTION_720P:
            metrics.resolution_width = 1280;
            metrics.resolution_height = 720;
            metrics.target_processing_time_ms = 16.67; // 60fps
            break;
        case TestScenario::RESOLUTION_1080P:
            metrics.resolution_width = 1920;
            metrics.resolution_height = 1080;
            metrics.target_processing_time_ms = 33.33; // 30fps
            break;
        case TestScenario::RESOLUTION_1440P:
            metrics.resolution_width = 2560;
            metrics.resolution_height = 1440;
            metrics.target_processing_time_ms = 33.33; // 30fps
            break;
        case TestScenario::RESOLUTION_4K:
            metrics.resolution_width = 3840;
            metrics.resolution_height = 2160;
            metrics.target_processing_time_ms = 33.33; // 30fps
            break;
        default:
            metrics.resolution_width = 1920;
            metrics.resolution_height = 1080;
            metrics.target_processing_time_ms = 33.33;
    }
    
    metrics.frame_rate = static_cast<int>(1000.0 / metrics.target_processing_time_ms);
    
    // Initialize metrics collection
    std::vector<double> processing_times;
    size_t peak_memory = 0;
    size_t start_memory = Utils::get_current_memory_usage();
    
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "Running Scenario: " << metrics.scenario_name << std::endl;
    std::cout << "Resolution: " << metrics.resolution_width << "x" << metrics.resolution_height;
    std::cout << " @ " << metrics.frame_rate << " fps" << std::endl;
    std::cout << "Target: <" << metrics.target_processing_time_ms << "ms/frame" << std::endl;
    
    // Run benchmark (placeholder - actual implementation would call stabilizer)
    Utils::Timer timer;
    for (int i = 0; i < config_.num_frames; i++) {
        timer.start();
        
        // TODO: Call actual stabilizer processing here
        // For now, simulate processing time based on resolution
        double simulated_time = (metrics.resolution_width * metrics.resolution_height) / 10000000.0;
        std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(simulated_time));
        
        timer.stop();
        
        // Skip warmup frames
        if (i >= config_.warmup_frames) {
            processing_times.push_back(timer.elapsed_ms());
        }
        
        if (config_.enable_memory_tracking) {
            size_t current_memory = Utils::get_current_memory_usage();
            peak_memory = std::max(peak_memory, current_memory);
        }
        
        // Progress indicator
        if ((i + 1) % 100 == 0) {
            std::cout << "." << std::flush;
        }
    }
    std::cout << std::endl;
    
    // Calculate statistics
    if (!processing_times.empty()) {
        double sum = std::accumulate(processing_times.begin(), processing_times.end(), 0.0);
        metrics.avg_processing_time_ms = sum / processing_times.size();
        metrics.min_processing_time_ms = *std::min_element(processing_times.begin(), processing_times.end());
        metrics.max_processing_time_ms = *std::max_element(processing_times.begin(), processing_times.end());
        metrics.std_deviation_ms = calculate_std_deviation(processing_times);
        
        metrics.peak_memory_bytes = peak_memory;
        metrics.avg_memory_bytes = (peak_memory + start_memory) / 2;
        
        // Determine pass/fail
        metrics.meets_realtime_requirement = metrics.avg_processing_time_ms < metrics.target_processing_time_ms;
        metrics.passed = metrics.meets_realtime_requirement;
        
        if (!metrics.passed) {
            metrics.failure_reason = "Processing time exceeds real-time requirement";
        }
    } else {
        metrics.passed = false;
        metrics.failure_reason = "No frames processed";
    }
    
    results_.push_back(metrics);
}

void BenchmarkRunner::run_all_scenarios() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "OBS Stabilizer Performance Benchmark Suite" << std::endl;
    std::cout << "Configuration: " << config_.num_frames << " frames, " 
              << config_.warmup_frames << " warmup frames" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    // Run all test scenarios
    run_scenario(TestScenario::RESOLUTION_480P);
    run_scenario(TestScenario::RESOLUTION_720P);
    run_scenario(TestScenario::RESOLUTION_1080P);
    run_scenario(TestScenario::RESOLUTION_1440P);
    run_scenario(TestScenario::RESOLUTION_4K);
    
    // Save results
    if (!config_.output_file.empty()) {
        if (config_.output_format == "json") {
            save_results_json(config_.output_file);
        } else {
            save_results_csv(config_.output_file);
        }
    }
    
    print_summary();
}

std::vector<BenchmarkMetrics> BenchmarkRunner::get_results() const {
    return results_;
}

void BenchmarkRunner::save_results(const std::string& filename) {
    if (config_.output_format == "json") {
        save_results_json(filename);
    } else {
        save_results_csv(filename);
    }
}

void BenchmarkRunner::save_results_csv(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << " for writing" << std::endl;
        return;
    }
    
    // Write CSV header
    file << "scenario_name,resolution_width,resolution_height,frame_rate,"
         << "avg_processing_time_ms,min_processing_time_ms,max_processing_time_ms,std_deviation_ms,"
         << "peak_memory_bytes,avg_memory_bytes,passed,meets_realtime_requirement,failure_reason\n";
    
    // Write data rows
    for (const auto& metrics : results_) {
        file << metrics.scenario_name << ","
             << metrics.resolution_width << ","
             << metrics.resolution_height << ","
             << metrics.frame_rate << ","
             << std::fixed << std::setprecision(2)
             << metrics.avg_processing_time_ms << ","
             << metrics.min_processing_time_ms << ","
             << metrics.max_processing_time_ms << ","
             << metrics.std_deviation_ms << ","
             << metrics.peak_memory_bytes << ","
             << metrics.avg_memory_bytes << ","
             << (metrics.passed ? "true" : "false") << ","
             << (metrics.meets_realtime_requirement ? "true" : "false") << ","
             << "\"" << metrics.failure_reason << "\"\n";
    }
    
    file.close();
    std::cout << "\nResults saved to " << filename << std::endl;
}

void BenchmarkRunner::save_results_json(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << " for writing" << std::endl;
        return;
    }
    
    file << "{\n";
    file << "  \"benchmark_results\": [\n";
    
    for (size_t i = 0; i < results_.size(); i++) {
        const auto& metrics = results_[i];
        file << "    {\n";
        file << "      \"scenario_name\": \"" << metrics.scenario_name << "\",\n";
        file << "      \"resolution_width\": " << metrics.resolution_width << ",\n";
        file << "      \"resolution_height\": " << metrics.resolution_height << ",\n";
        file << "      \"frame_rate\": " << metrics.frame_rate << ",\n";
        file << "      \"avg_processing_time_ms\": " << std::fixed << std::setprecision(2) 
             << metrics.avg_processing_time_ms << ",\n";
        file << "      \"min_processing_time_ms\": " << metrics.min_processing_time_ms << ",\n";
        file << "      \"max_processing_time_ms\": " << metrics.max_processing_time_ms << ",\n";
        file << "      \"std_deviation_ms\": " << metrics.std_deviation_ms << ",\n";
        file << "      \"peak_memory_bytes\": " << metrics.peak_memory_bytes << ",\n";
        file << "      \"avg_memory_bytes\": " << metrics.avg_memory_bytes << ",\n";
        file << "      \"passed\": " << (metrics.passed ? "true" : "false") << ",\n";
        file << "      \"meets_realtime_requirement\": " << (metrics.meets_realtime_requirement ? "true" : "false") << ",\n";
        file << "      \"failure_reason\": \"" << metrics.failure_reason << "\"\n";
        file << "    }" << (i < results_.size() - 1 ? "," : "") << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";
    
    file.close();
    std::cout << "\nResults saved to " << filename << std::endl;
}

bool BenchmarkRunner::load_baseline(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Warning: Cannot open baseline file " << filename << std::endl;
        return false;
    }
    
    // Simple JSON parsing for baseline loading
    std::string line;
    std::string current_scenario;
    bool in_results = false;
    
    while (std::getline(file, line)) {
        if (line.find("\"benchmark_results\"") != std::string::npos) {
            in_results = true;
            continue;
        }
        
        if (in_results) {
            if (line.find("scenario_name") != std::string::npos) {
                size_t start = line.find("\"") + 1;
                size_t end = line.find("\"", start);
                current_scenario = line.substr(start, end - start);
            }
            
            if (line.find("avg_processing_time_ms") != std::string::npos) {
                double value = std::stod(line.substr(line.find(":") + 1));
                BenchmarkMetrics metrics;
                metrics.scenario_name = current_scenario;
                metrics.avg_processing_time_ms = value;
                baselines_[current_scenario] = metrics;
            }
        }
    }
    
    file.close();
    std::cout << "Loaded " << baselines_.size() << " baselines from " << filename << std::endl;
    return true;
}

bool BenchmarkRunner::save_baseline(const std::string& filename) {
    save_results_json(filename);
    std::cout << "Baseline saved to " << filename << std::endl;
    return true;
}

bool BenchmarkRunner::compare_against_baseline() {
    if (baselines_.empty()) {
        std::cout << "Warning: No baselines loaded for comparison" << std::endl;
        return true;
    }
    
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "Baseline Comparison" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    bool all_passed = true;
    const double degradation_threshold = 0.10; // 10% degradation threshold
    
    for (auto& metrics : results_) {
        auto it = baselines_.find(metrics.scenario_name);
        if (it != baselines_.end()) {
            const auto& baseline = it->second;
            double baseline_time = baseline.avg_processing_time_ms;
            double current_time = metrics.avg_processing_time_ms;
            double degradation = (current_time - baseline_time) / baseline_time;
            
            std::cout << "\nScenario: " << metrics.scenario_name << std::endl;
            std::cout << "  Baseline: " << baseline_time << " ms" << std::endl;
            std::cout << "  Current:  " << current_time << " ms" << std::endl;
            std::cout << "  Change:   " << std::fixed << std::setprecision(2) 
                      << (degradation * 100.0) << "%";
            
            if (degradation > degradation_threshold) {
                std::cout << " ⚠️  REGRESSION DETECTED" << std::endl;
                metrics.passed = false;
                metrics.failure_reason = "Performance regression detected: " + 
                                         std::to_string(degradation * 100.0) + "% degradation";
                all_passed = false;
            } else if (degradation < -degradation_threshold) {
                std::cout << " ✅ IMPROVEMENT" << std::endl;
            } else {
                std::cout << " ✅ STABLE" << std::endl;
            }
        }
    }
    
    return all_passed;
}

void BenchmarkRunner::print_summary() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "Benchmark Summary" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    for (const auto& metrics : results_) {
        std::cout << "\n" << metrics.scenario_name << " (" 
                  << metrics.resolution_width << "x" << metrics.resolution_height << ")\n";
        std::cout << "  Avg: " << std::fixed << std::setprecision(2) 
                  << metrics.avg_processing_time_ms << " ms ("
                  << (1000.0 / metrics.avg_processing_time_ms) << " fps)\n";
        std::cout << "  Min: " << metrics.min_processing_time_ms 
                  << " ms, Max: " << metrics.max_processing_time_ms << " ms\n";
        std::cout << "  StdDev: " << metrics.std_deviation_ms << " ms\n";
        std::cout << "  Target: <" << metrics.target_processing_time_ms << " ms/frame\n";
        std::cout << "  Status: " << (metrics.meets_realtime_requirement ? "✅ PASS" : "❌ FAIL");
        
        if (!metrics.meets_realtime_requirement) {
            std::cout << " (" << metrics.failure_reason << ")";
        }
        
        std::cout << "\n";
        
        if (metrics.meets_realtime_requirement) {
            passed++;
        } else {
            failed++;
        }
    }
    
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "Total: " << results_.size() << " scenarios, "
              << passed << " passed, " << failed << " failed\n";
    std::cout << std::string(70, '=') << std::endl;
}

double BenchmarkRunner::calculate_std_deviation(const std::vector<double>& values) {
    if (values.empty()) return 0.0;
    
    double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    double sq_sum = 0.0;
    
    for (double value : values) {
        sq_sum += (value - mean) * (value - mean);
    }
    
    return std::sqrt(sq_sum / values.size());
}

std::string BenchmarkRunner::scenario_to_string(TestScenario scenario) const {
    switch (scenario) {
        case TestScenario::STATIC_SCENE: return "Static Scene";
        case TestScenario::SLOW_PAN: return "Slow Pan";
        case TestScenario::FAST_SHAKE: return "Fast Shake";
        case TestScenario::ZOOM_OPERATION: return "Zoom Operation";
        case TestScenario::COMPLEX_BACKGROUND: return "Complex Background";
        case TestScenario::EXTENDED_RUN: return "Extended Run";
        case TestScenario::RESOLUTION_480P: return "Resolution 480p";
        case TestScenario::RESOLUTION_720P: return "Resolution 720p";
        case TestScenario::RESOLUTION_1080P: return "Resolution 1080p";
        case TestScenario::RESOLUTION_1440P: return "Resolution 1440p";
        case TestScenario::RESOLUTION_4K: return "Resolution 4K";
        default: return "Unknown";
    }
}

namespace Utils {

Timer::Timer() : running_(false) {
}

void Timer::start() {
    start_time_ = std::chrono::high_resolution_clock::now();
    running_ = true;
}

void Timer::stop() {
    end_time_ = std::chrono::high_resolution_clock::now();
    running_ = false;
}

double Timer::elapsed_ms() const {
    auto end = running_ ? std::chrono::high_resolution_clock::now() : end_time_;
    return std::chrono::duration<double, std::milli>(end - start_time_).count();
}

size_t get_current_memory_usage() {
#ifdef __APPLE__
    struct mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS) {
        return info.resident_size;
    }
#elif defined(__linux__)
    FILE* file = fopen("/proc/self/status", "r");
    if (file) {
        char line[128];
        while (fgets(line, sizeof(line), file)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                size_t memory_kb;
                sscanf(line, "VmRSS: %zu kB", &memory_kb);
                fclose(file);
                return memory_kb * 1024;
            }
        }
        fclose(file);
    }
#endif
    return 0;
}

size_t get_peak_memory_usage() {
#ifdef __APPLE__
    struct mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self_, MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS) {
        return info.resident_size_max;
    }
#endif
    return 0;
}

void print_separator() {
    std::cout << std::string(70, '=') << std::endl;
}

void print_metric(const std::string& name, double value, const std::string& unit) {
    std::cout << std::left << std::setw(30) << name << ": " 
              << std::fixed << std::setprecision(2) << value << " " << unit << std::endl;
}

void print_metric(const std::string& name, size_t value, const std::string& unit) {
    std::cout << std::left << std::setw(30) << name << ": " << value << " " << unit << std::endl;
}

} // namespace Utils

} // namespace PERF
