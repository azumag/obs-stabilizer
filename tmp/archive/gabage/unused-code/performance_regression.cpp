/*
 * OBS Stabilizer Plugin - Performance Regression Detection
 * Automatically detects performance degradations
 */

#include "benchmark.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iomanip>

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
    RegressionDetector(const RegressionThresholds& thresholds = RegressionThresholds())
        : thresholds_(thresholds) {
    }
    
    void set_thresholds(const RegressionThresholds& thresholds) {
        thresholds_ = thresholds;
    }
    
    RegressionThresholds get_thresholds() const {
        return thresholds_;
    }
    
    bool detect_regression(const BenchmarkMetrics& current,
                         const BenchmarkMetrics& baseline) {
        RegressionReport report = analyze_regression(current, baseline);
        reports_.push_back(report);
        return report.regression_detected;
    }
    
    void analyze_results(const std::vector<BenchmarkMetrics>& current_results,
                       const std::map<std::string, BenchmarkMetrics>& baseline_results) {
        for (const auto& current : current_results) {
            auto it = baseline_results.find(current.scenario_name);
            if (it != baseline_results.end()) {
                const auto& baseline = it->second;
                detect_regression(current, baseline);
            } else {
                std::cout << "Warning: No baseline found for scenario '" 
                          << current.scenario_name << "'" << std::endl;
            }
        }
    }
    
    std::vector<RegressionReport> get_reports() const {
        return reports_;
    }
    
    void print_report() {
        if (reports_.empty()) {
            std::cout << "\nNo regression analysis available.\n";
            return;
        }
        
        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║           Performance Regression Detection Report                ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
        
        int critical_count = 0;
        int warning_count = 0;
        int ok_count = 0;
        
        for (const auto& report : reports_) {
            print_regression_report(report);
            if (report.severity == "CRITICAL") critical_count++;
            else if (report.severity == "WARNING") warning_count++;
            else ok_count++;
        }
        
        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║                      Summary                                    ║\n";
        std::cout << "╠════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║  Scenarios Analyzed: " << std::setw(44) << reports_.size() << " ║\n";
        std::cout << "║  ✅ No Regression:  " << std::setw(44) << ok_count << " ║\n";
        std::cout << "║  ⚠️  Warnings:       " << std::setw(44) << warning_count << " ║\n";
        std::cout << "║  ❌ Critical:       " << std::setw(44) << critical_count << " ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";
        
        bool has_regressions = (critical_count > 0);
        if (has_regressions) {
            std::cout << "❌ PERFORMANCE REGRESSIONS DETECTED!\n";
            std::cout << "Review critical items above and investigate before merging.\n";
        } else if (warning_count > 0) {
            std::cout << "⚠️  Performance warnings detected.\n";
            std::cout << "Review warnings but not blocking.\n";
        } else {
            std::cout << "✅ No performance regressions detected.\n";
        }
        std::cout << "\n";
    }
    
    void save_report_json(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file " << filename << " for writing" << std::endl;
            return;
        }
        
        file << "{\n";
        file << "  \"regression_report\": {\n";
        file << "    \"thresholds\": {\n";
        file << "      \"processing_time_degradation_percent\": " << thresholds_.processing_time_degradation_percent << ",\n";
        file << "      \"memory_increase_percent\": " << thresholds_.memory_increase_percent << ",\n";
        file << "      \"frame_rate_degradation_percent\": " << thresholds_.frame_rate_degradation_percent << ",\n";
        file << "      \"std_deviation_increase_percent\": " << thresholds_.std_deviation_increase_percent << "\n";
        file << "    },\n";
        file << "    \"scenarios\": [\n";
        
        for (size_t i = 0; i < reports_.size(); i++) {
            const auto& report = reports_[i];
            file << "      {\n";
            file << "        \"scenario_name\": \"" << report.scenario_name << "\",\n";
            file << "        \"regression_detected\": " << (report.regression_detected ? "true" : "false") << ",\n";
            file << "        \"processing_time_change\": " << std::fixed << std::setprecision(2) 
                 << report.processing_time_change << ",\n";
            file << "        \"memory_change\": " << report.memory_change << ",\n";
            file << "        \"severity\": \"" << report.severity << "\",\n";
            file << "        \"message\": \"" << report.message << "\"\n";
            file << "      }" << (i < reports_.size() - 1 ? "," : "") << "\n";
        }
        
        file << "    ]\n";
        file << "  }\n";
        file << "}\n";
        
        file.close();
        std::cout << "Regression report saved to " << filename << std::endl;
    }
    
    bool has_critical_regressions() const {
        for (const auto& report : reports_) {
            if (report.severity == "CRITICAL") {
                return true;
            }
        }
        return false;
    }
    
    void clear() {
        reports_.clear();
    }
    
private:
    RegressionThresholds thresholds_;
    std::vector<RegressionReport> reports_;
    
    RegressionReport analyze_regression(const BenchmarkMetrics& current,
                                     const BenchmarkMetrics& baseline) {
        RegressionReport report;
        report.scenario_name = current.scenario_name;
        report.regression_detected = false;
        
        // Calculate processing time change
        double time_change = 0.0;
        if (baseline.avg_processing_time_ms > 0) {
            time_change = ((current.avg_processing_time_ms - baseline.avg_processing_time_ms) / 
                          baseline.avg_processing_time_ms) * 100.0;
        }
        report.processing_time_change = time_change;
        
        // Calculate memory change
        double memory_change = 0.0;
        if (baseline.peak_memory_bytes > 0) {
            memory_change = ((current.peak_memory_bytes - baseline.peak_memory_bytes) / 
                           baseline.peak_memory_bytes) * 100.0;
        }
        report.memory_change = memory_change;
        
        // Determine severity
        report.severity = "NONE";
        report.message = "Performance stable or improved";
        
        bool time_regression = time_change > thresholds_.processing_time_degradation_percent;
        bool memory_regression = memory_change > thresholds_.memory_increase_percent;
        
        if (time_regression) {
            if (time_change > thresholds_.processing_time_degradation_percent * 2) {
                report.severity = "CRITICAL";
                report.message = "Severe processing time degradation: " + 
                               std::to_string(time_change) + "%";
                report.regression_detected = true;
            } else {
                report.severity = "WARNING";
                report.message = "Processing time degradation: " + 
                               std::to_string(time_change) + "%";
            }
        }
        
        if (memory_regression) {
            if (report.severity != "CRITICAL") {
                report.severity = "WARNING";
                report.message = "Memory usage increased: " + std::to_string(memory_change) + "%";
            }
        }
        
        return report;
    }
    
    void print_regression_report(const RegressionReport& report) {
        std::cout << "\n";
        std::cout << "Scenario: " << report.scenario_name << "\n";
        
        if (report.severity == "CRITICAL") {
            std::cout << "  ❌ " << report.severity << ": " << report.message << "\n";
        } else if (report.severity == "WARNING") {
            std::cout << "  ⚠️  " << report.severity << ": " << report.message << "\n";
        } else {
            std::cout << "  ✅ " << report.message << "\n";
        }
        
        std::cout << "  Processing Time Change: " << std::fixed << std::setprecision(2) 
                  << report.processing_time_change << "%\n";
        std::cout << "  Memory Change: " << report.memory_change << "%\n";
    }
};

} // namespace PERF
