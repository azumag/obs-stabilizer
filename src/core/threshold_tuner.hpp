#pragma once

#include <string>
#include <map>
#include <vector>
#include "core/motion_classifier.hpp"

namespace AdaptiveStabilization {

struct ThresholdConfig {
    double static_threshold;
    double slow_threshold;
    double fast_threshold;
    double variance_threshold;
    double high_freq_threshold;
    double consistency_threshold;
    
    ThresholdConfig()
        : static_threshold(6.0)
        , slow_threshold(15.0)
        , fast_threshold(40.0)
        , variance_threshold(3.0)
        , high_freq_threshold(0.85)
        , consistency_threshold(0.96) {}
    
    ThresholdConfig(double st, double sl, double ft, double vt, double hf, double cs)
        : static_threshold(st)
        , slow_threshold(sl)
        , fast_threshold(ft)
        , variance_threshold(vt)
        , high_freq_threshold(hf)
        , consistency_threshold(cs) {}
    
    std::string to_string() const;
};

struct TestResult {
    std::string test_name;
    bool passed;
    MotionType predicted_type;
    MotionType expected_type;
    MotionMetrics metrics;
    
    TestResult()
        : passed(false)
        , predicted_type(MotionType::Static)
        , expected_type(MotionType::Static) {}
    
    TestResult(const std::string& name, bool pass, MotionType pred, MotionType exp,
               const MotionMetrics& m)
        : test_name(name)
        , passed(pass)
        , predicted_type(pred)
        , expected_type(exp)
        , metrics(m) {}
};

struct TuningReport {
    ThresholdConfig best_config;
    double best_accuracy;
    int total_tests;
    int passed_tests;
    std::vector<TestResult> test_results;
    int iterations;
    
    TuningReport()
        : best_accuracy(0.0)
        , total_tests(0)
        , passed_tests(0)
        , iterations(0) {}
    
    void print_report() const;
};

class ThresholdTuner {
public:
    ThresholdTuner();
    ~ThresholdTuner() = default;
    
    TuningReport tune_thresholds_grid_search(
        size_t steps_per_threshold = 10
    );
    
    TuningReport tune_thresholds_random_search(
        size_t iterations = 1000
    );
    
    void set_search_ranges(
        double static_min, double static_max,
        double slow_min, double slow_max,
        double fast_min, double fast_max,
        double var_min, double var_max,
        double hf_min, double hf_max,
        double cs_min, double cs_max
    );
    
    std::vector<TestResult> evaluate_thresholds(
        const ThresholdConfig& config
    ) const;
    
    double calculate_accuracy(
        const std::vector<TestResult>& results
    ) const;
    
    static ThresholdConfig get_current_thresholds();
    static void apply_thresholds(const ThresholdConfig& config);
    
private:
    struct SearchRange {
        double min_val;
        double max_val;
        
        SearchRange() : min_val(0.0), max_val(100.0) {}
        SearchRange(double min_v, double max_v) : min_val(min_v), max_val(max_v) {}
    };
    
    SearchRange range_static_;
    SearchRange range_slow_;
    SearchRange range_fast_;
    SearchRange range_variance_;
    SearchRange range_hf_;
    SearchRange range_cs_;
    
    void generate_test_cases();
    
    struct TestCase {
        std::string name;
        std::deque<cv::Mat> transforms;
        MotionType expected_type;
        
        TestCase() : expected_type(MotionType::Static) {}
        TestCase(const std::string& n, const std::deque<cv::Mat>& t, MotionType e)
            : name(n), transforms(t), expected_type(e) {}
    };
    
    std::vector<TestCase> test_cases_;
    
    cv::Mat create_transform(double tx, double ty, double angle = 0.0, double scale = 1.0) const;
};

} // namespace AdaptiveStabilization
