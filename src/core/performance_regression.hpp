/*
 * OBS Stabilizer Plugin - Performance Regression Detection
 * Automatically detects performance degradations
 */

#pragma once

#include "benchmark.hpp"

namespace PERF {

struct RegressionThresholds {
    double processing_time_degradation_percent = 10.0;
    double memory_increase_percent = 20.0;
    double frame_rate_degradation_percent = 5.0;
    double std_deviation_increase_percent = 50.0;
};

struct RegressionReport {
    std::string scenario_name;
    bool regression_detected;
    double processing_time_change;
    double memory_change;
    std::string severity;  // "CRITICAL", "WARNING", "NONE"
    std::string message;
};

class RegressionDetector {
public:
    RegressionDetector(const RegressionThresholds& thresholds = RegressionThresholds());
    
    void set_thresholds(const RegressionThresholds& thresholds);
    RegressionThresholds get_thresholds() const;
    
    bool detect_regression(const BenchmarkMetrics& current,
                         const BenchmarkMetrics& baseline);
    
    void analyze_results(const std::vector<BenchmarkMetrics>& current_results,
                       const std::map<std::string, BenchmarkMetrics>& baseline_results);
    
    std::vector<RegressionReport> get_reports() const;
    
    void print_report();
    void save_report_json(const std::string& filename);
    
    bool has_critical_regressions() const;
    void clear();
    
private:
    RegressionThresholds thresholds_;
    std::vector<RegressionReport> reports_;
    
    RegressionReport analyze_regression(const BenchmarkMetrics& current,
                                     const BenchmarkMetrics& baseline);
    void print_regression_report(const RegressionReport& report);
};

} // namespace PERF
