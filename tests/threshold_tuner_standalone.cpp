#include <iostream>
#include <iomanip>
#include <vector>
#include <deque>
#include <cmath>
#include <random>
#include <sstream>
#include <algorithm>
#include <map>

// Simulating the motion classifier namespace for standalone execution
namespace AdaptiveStabilization {

enum class MotionType {
    Static,
    SlowMotion,
    FastMotion,
    CameraShake,
    PanZoom
};

struct MotionMetrics {
    double mean_magnitude;
    double variance_magnitude;
    double directional_variance;
    double high_frequency_ratio;
    double consistency_score;
    size_t transform_count;
    
    MotionMetrics() 
        : mean_magnitude(0.0)
        , variance_magnitude(0.0)
        , directional_variance(0.0)
        , high_frequency_ratio(0.0)
        , consistency_score(0.0)
        , transform_count(0) {}
};

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
    
    std::string to_string() const {
        std::ostringstream oss;
        oss << "ThresholdConfig {\n"
            << "  static_threshold: " << static_threshold << "\n"
            << "  slow_threshold: " << slow_threshold << "\n"
            << "  fast_threshold: " << fast_threshold << "\n"
            << "  variance_threshold: " << variance_threshold << "\n"
            << "  high_freq_threshold: " << high_freq_threshold << "\n"
            << "  consistency_threshold: " << consistency_threshold << "\n"
            << "}";
        return oss.str();
    }
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
    
    void print_report() const {
        std::cout << "\n=== Threshold Tuning Report ===\n";
        std::cout << "Best Configuration:\n" << best_config.to_string() << "\n";
        std::cout << "\nResults:\n";
        std::cout << "  Total Tests: " << total_tests << "\n";
        std::cout << "  Passed: " << passed_tests << "\n";
        std::cout << "  Failed: " << (total_tests - passed_tests) << "\n";
        std::cout << "  Accuracy: " << std::fixed << std::setprecision(2) 
                  << (best_accuracy * 100.0) << "%\n";
        std::cout << "  Iterations: " << iterations << "\n";
        
        if (!test_results.empty()) {
            std::cout << "\nTest Results:\n";
            for (const auto& result : test_results) {
                std::cout << "  " << (result.passed ? "[PASS]" : "[FAIL]") << " "
                          << result.test_name;
                if (!result.passed) {
                    std::cout << " (Expected: " 
                              << motion_type_to_string(result.expected_type)
                              << ", Got: "
                              << motion_type_to_string(result.predicted_type)
                              << ")";
                }
                std::cout << "\n";
            }
        }
        std::cout << "==============================\n\n";
    }
    
    static std::string motion_type_to_string(MotionType type) {
        switch (type) {
            case MotionType::Static: return "Static";
            case MotionType::SlowMotion: return "SlowMotion";
            case MotionType::FastMotion: return "FastMotion";
            case MotionType::CameraShake: return "CameraShake";
            case MotionType::PanZoom: return "PanZoom";
            default: return "Unknown";
        }
    }
};

class ThresholdTuner {
public:
    ThresholdTuner() {
        set_search_ranges(
            1.0, 20.0,    // static
            10.0, 30.0,   // slow
            20.0, 60.0,   // fast
            1.0, 15.0,    // variance
            0.5, 0.95,    // high_freq
            0.5, 0.99     // consistency
        );
        generate_test_cases();
    }
    
    TuningReport tune_thresholds_grid_search(size_t steps_per_threshold = 5) {
        TuningReport report;
        report.total_tests = static_cast<int>(test_cases_.size());
        report.iterations = 0;
        
        std::cout << "Starting grid search with " << steps_per_threshold 
                  << " steps per threshold...\n";
        
        double best_accuracy = 0.0;
        ThresholdConfig best_config;
        
        // Use reduced steps for the grid search to make it manageable
        size_t actual_steps = std::min(steps_per_threshold, static_cast<size_t>(5));
        
        for (size_t i = 0; i < actual_steps; ++i) {
            double static_t = range_static_.min_val + 
                             (range_static_.max_val - range_static_.min_val) * i / (actual_steps - 1);
            
            for (size_t j = 0; j < actual_steps; ++j) {
                double slow_t = range_slow_.min_val + 
                               (range_slow_.max_val - range_slow_.min_val) * j / (actual_steps - 1);
                
                // Ensure slow > static
                if (slow_t <= static_t) continue;
                
                for (size_t k = 0; k < actual_steps; ++k) {
                    double fast_t = range_fast_.min_val + 
                                   (range_fast_.max_val - range_fast_.min_val) * k / (actual_steps - 1);
                    
                    // Ensure fast > slow
                    if (fast_t <= slow_t) continue;
                    
                    for (size_t l = 0; l < actual_steps; ++l) {
                        double var_t = range_variance_.min_val + 
                                      (range_variance_.max_val - range_variance_.min_val) * l / (actual_steps - 1);
                        
                        for (size_t m = 0; m < actual_steps; ++m) {
                            double hf_t = range_hf_.min_val + 
                                         (range_hf_.max_val - range_hf_.min_val) * m / (actual_steps - 1);
                            
                            for (size_t n = 0; n < actual_steps; ++n) {
                                double cs_t = range_cs_.min_val + 
                                             (range_cs_.max_val - range_cs_.min_val) * n / (actual_steps - 1);
                                
                                ThresholdConfig config(static_t, slow_t, fast_t, var_t, hf_t, cs_t);
                                
                                auto results = evaluate_thresholds(config);
                                double accuracy = calculate_accuracy(results);
                                
                                report.iterations++;
                                
                                if (accuracy > best_accuracy) {
                                    best_accuracy = accuracy;
                                    best_config = config;
                                    report.test_results = results;
                                    report.passed_tests = 0;
                                    for (const auto& r : results) {
                                        if (r.passed) report.passed_tests++;
                                    }
                                    
                                    std::cout << "New best accuracy: " << (accuracy * 100.0) 
                                              << "% (iterations: " << report.iterations << ")\n";
                                    
                                    // If we achieve 100% accuracy, we can stop
                                    if (accuracy >= 1.0) {
                                        report.best_accuracy = best_accuracy;
                                        report.best_config = best_config;
                                        return report;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        report.best_accuracy = best_accuracy;
        report.best_config = best_config;
        
        return report;
    }
    
    TuningReport tune_thresholds_random_search(size_t iterations = 1000) {
        TuningReport report;
        report.total_tests = static_cast<int>(test_cases_.size());
        report.iterations = 0;
        
        std::cout << "Starting random search with " << iterations << " iterations...\n";
        
        std::random_device rd;
        std::mt19937 gen(rd());
        
        double best_accuracy = 0.0;
        ThresholdConfig best_config;
        
        for (size_t iter = 0; iter < iterations; ++iter) {
            std::uniform_real_distribution<> dist_static(range_static_.min_val, range_static_.max_val);
            std::uniform_real_distribution<> dist_slow(range_slow_.min_val, range_slow_.max_val);
            std::uniform_real_distribution<> dist_fast(range_fast_.min_val, range_fast_.max_val);
            std::uniform_real_distribution<> dist_var(range_variance_.min_val, range_variance_.max_val);
            std::uniform_real_distribution<> dist_hf(range_hf_.min_val, range_hf_.max_val);
            std::uniform_real_distribution<> dist_cs(range_cs_.min_val, range_cs_.max_val);
            
            double static_t = dist_static(gen);
            double slow_t = dist_slow(gen);
            double fast_t = dist_fast(gen);
            double var_t = dist_var(gen);
            double hf_t = dist_hf(gen);
            double cs_t = dist_cs(gen);
            
            // Ensure ordering: static < slow < fast
            if (slow_t <= static_t) slow_t = static_t + 1.0;
            if (fast_t <= slow_t) fast_t = slow_t + 1.0;
            
            ThresholdConfig config(static_t, slow_t, fast_t, var_t, hf_t, cs_t);
            
            auto results = evaluate_thresholds(config);
            double accuracy = calculate_accuracy(results);
            
            report.iterations++;
            
            if (accuracy > best_accuracy) {
                best_accuracy = accuracy;
                best_config = config;
                report.test_results = results;
                report.passed_tests = 0;
                for (const auto& r : results) {
                    if (r.passed) report.passed_tests++;
                }
                
                std::cout << "Iteration " << iter << ": New best accuracy: " << (accuracy * 100.0) 
                          << "%\n";
                
                // If we achieve 100% accuracy, we can stop
                if (accuracy >= 1.0) {
                    report.best_accuracy = best_accuracy;
                    report.best_config = best_config;
                    return report;
                }
            }
            
            // Print progress every 100 iterations
            if (iter % 100 == 0 && iter > 0) {
                std::cout << "Iteration " << iter << ": Current best accuracy: " 
                          << (best_accuracy * 100.0) << "%\n";
            }
        }
        
        report.best_accuracy = best_accuracy;
        report.best_config = best_config;
        
        return report;
    }
    
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
    
    struct TestCase {
        std::string name;
        std::vector<std::vector<double>> transforms;  // 2x3 affine transform matrices
        MotionType expected_type;
        
        TestCase() : expected_type(MotionType::Static) {}
        TestCase(const std::string& n, const std::vector<std::vector<double>>& t, MotionType e)
            : name(n), transforms(t), expected_type(e) {}
    };
    
    std::vector<TestCase> test_cases_;
    
    void set_search_ranges(
        double static_min, double static_max,
        double slow_min, double slow_max,
        double fast_min, double fast_max,
        double var_min, double var_max,
        double hf_min, double hf_max,
        double cs_min, double cs_max
    ) {
        range_static_ = SearchRange(static_min, static_max);
        range_slow_ = SearchRange(slow_min, slow_max);
        range_fast_ = SearchRange(fast_min, fast_max);
        range_variance_ = SearchRange(var_min, var_max);
        range_hf_ = SearchRange(hf_min, hf_max);
        range_cs_ = SearchRange(cs_min, cs_max);
    }
    
    void generate_test_cases() {
        test_cases_.clear();
        
        auto add_test = [&](const std::string& name, const std::vector<std::vector<double>>& transforms, MotionType expected) {
            test_cases_.emplace_back(name, transforms, expected);
        };
        
        // Test 1: Static Motion
        {
            std::vector<std::vector<double>> transforms;
            for (int i = 0; i < 30; ++i) {
                transforms.push_back(create_transform(0.1, 0.1));
            }
            add_test("Static", transforms, MotionType::Static);
        }
        
        // Test 2: Slow Motion
        {
            std::vector<std::vector<double>> transforms;
            for (int i = 0; i < 30; ++i) {
                double tx = 6.0 + std::sin(i * 0.3) * 3.0 + (i % 4) * 0.8;
                double ty = 6.0 + std::cos(i * 0.25) * 2.5 + (i % 3) * 1.2;
                transforms.push_back(create_transform(tx, ty));
            }
            add_test("SlowMotion", transforms, MotionType::SlowMotion);
        }
        
        // Test 3: Fast Motion
        {
            std::vector<std::vector<double>> transforms;
            for (int i = 0; i < 30; ++i) {
                double tx = 20.0 + (i * 0.5);
                double ty = 20.0 + (i * 0.4);
                transforms.push_back(create_transform(tx, ty));
            }
            add_test("FastMotion", transforms, MotionType::FastMotion);
        }
        
        // Test 4: PanZoom
        {
            std::vector<std::vector<double>> transforms;
            for (int i = 0; i < 30; ++i) {
                transforms.push_back(create_transform(5.0 + i * 0.2, 2.0 + i * 0.1));
            }
            add_test("PanZoom", transforms, MotionType::PanZoom);
        }
        
        // Test 5: CameraShake
        {
            std::vector<std::vector<double>> transforms;
            for (int i = 0; i < 30; ++i) {
                double jitter = ((i % 2 == 0) ? 1.0 : -1.0) * (10.0 + (i % 3) * 8.0);
                double jitter2 = ((i % 3 == 0) ? 1.0 : -1.0) * (9.0 + (i % 5) * 7.0);
                transforms.push_back(create_transform(jitter + jitter2, jitter - jitter2));
            }
            add_test("CameraShake", transforms, MotionType::CameraShake);
        }
        
        // Test 6: Sensitivity Test (should be Static)
        {
            std::vector<std::vector<double>> transforms;
            for (int i = 0; i < 30; ++i) {
                transforms.push_back(create_transform(3.0 + (i % 5) * 0.5, 3.0 + (i % 7) * 0.3));
            }
            add_test("SensitivityNormal", transforms, MotionType::Static);
        }
    }
    
    std::vector<double> create_transform(double tx, double ty, double angle = 0.0, double scale = 1.0) const {
        // 2x3 affine transform matrix flattened
        std::vector<double> transform(6);
        transform[0] = scale * std::cos(angle);
        transform[1] = scale * std::sin(angle);
        transform[2] = tx;
        transform[3] = -scale * std::sin(angle);
        transform[4] = scale * std::cos(angle);
        transform[5] = ty;
        return transform;
    }
    
    double calculate_magnitude(const std::vector<double>& transform) const {
        double translation_x = transform[2];
        double translation_y = transform[5];
        double scale_x = transform[0];
        double scale_y = transform[4];
        double rotation = std::atan2(transform[1], transform[0]);
        
        double translation_magnitude = std::sqrt(translation_x * translation_x + 
                                             translation_y * translation_y);
        double scale_deviation = std::abs(scale_x - 1.0) + std::abs(scale_y - 1.0);
        double rotation_deviation = std::abs(rotation);
        
        return translation_magnitude + scale_deviation * 100.0 + rotation_deviation * 200.0;
    }
    
    MotionMetrics calculate_metrics(const std::vector<std::vector<double>>& transforms) const {
        MotionMetrics metrics;
        metrics.transform_count = transforms.size();
        
        if (transforms.empty()) {
            return metrics;
        }
        
        // Calculate mean magnitude
        double sum = 0.0;
        for (const auto& t : transforms) {
            sum += calculate_magnitude(t);
        }
        metrics.mean_magnitude = sum / static_cast<double>(transforms.size());
        
        // Calculate variance magnitude
        if (transforms.size() >= 2) {
            double sum_sq_diff = 0.0;
            for (const auto& t : transforms) {
                double mag = calculate_magnitude(t);
                double diff = mag - metrics.mean_magnitude;
                sum_sq_diff += diff * diff;
            }
            metrics.variance_magnitude = sum_sq_diff / static_cast<double>(transforms.size());
        }
        
        // Calculate directional variance
        double sum_dx = 0.0, sum_dy = 0.0;
        for (const auto& t : transforms) {
            sum_dx += t[2];
            sum_dy += t[5];
        }
        double mean_dx = sum_dx / static_cast<double>(transforms.size());
        double mean_dy = sum_dy / static_cast<double>(transforms.size());
        
        double var_dx = 0.0, var_dy = 0.0;
        for (const auto& t : transforms) {
            double diff_dx = t[2] - mean_dx;
            double diff_dy = t[5] - mean_dy;
            var_dx += diff_dx * diff_dx;
            var_dy += diff_dy * diff_dy;
        }
        var_dx /= static_cast<double>(transforms.size());
        var_dy /= static_cast<double>(transforms.size());
        
        metrics.directional_variance = std::sqrt(var_dx + var_dy);
        
        // Calculate consistency score
        if (transforms.size() >= 2) {
            double dot_sum = 0.0;
            double mag_sum = 0.0;
            
            for (size_t i = 1; i < transforms.size(); ++i) {
                const auto& t_prev = transforms[i - 1];
                const auto& t_curr = transforms[i];
                
                double dx_prev = t_prev[2];
                double dy_prev = t_prev[5];
                double dx_curr = t_curr[2];
                double dy_curr = t_curr[5];
                
                double dot = dx_prev * dx_curr + dy_prev * dy_curr;
                double mag_prev = std::sqrt(dx_prev * dx_prev + dy_prev * dy_prev);
                double mag_curr = std::sqrt(dx_curr * dx_curr + dy_curr * dy_curr);
                
                if (mag_prev > 0.001 && mag_curr > 0.001) {
                    dot_sum += dot / (mag_prev * mag_curr);
                    mag_sum += 1.0;
                }
            }
            
            metrics.consistency_score = mag_sum > 0.0 ? dot_sum / mag_sum : 0.0;
        }
        
        // Calculate frequency metrics
        if (transforms.size() >= 6) {
            double low_freq_energy = 0.0;
            double high_freq_energy = 0.0;
            
            for (size_t i = 2; i < transforms.size(); ++i) {
                double mag = calculate_magnitude(transforms[i]);
                double mag_prev_1 = calculate_magnitude(transforms[i - 1]);
                double mag_prev_2 = calculate_magnitude(transforms[i - 2]);
                
                double diff_1 = mag - mag_prev_1;
                double diff_2 = mag_prev_1 - mag_prev_2;
                
                double high_freq = std::abs(diff_1 - diff_2);
                double low_freq = std::abs(mag - mag_prev_2) * 0.5;
                
                high_freq_energy += high_freq;
                low_freq_energy += low_freq;
            }
            
            double total_energy = high_freq_energy + low_freq_energy;
            metrics.high_frequency_ratio = total_energy > 0.001 ? high_freq_energy / total_energy : 0.0;
        }
        
        return metrics;
    }
    
    MotionType classify_from_metrics(const MotionMetrics& metrics, const ThresholdConfig& config) const {
        if (metrics.mean_magnitude < config.static_threshold &&
            metrics.variance_magnitude < config.variance_threshold) {
            return MotionType::Static;
        }
        
        if (metrics.high_frequency_ratio > config.high_freq_threshold) {
            return MotionType::CameraShake;
        }
        
        if (metrics.consistency_score > config.consistency_threshold &&
            metrics.directional_variance < 2.0 &&
            metrics.mean_magnitude > config.static_threshold) {
            return MotionType::PanZoom;
        }
        
        if (metrics.mean_magnitude >= config.slow_threshold &&
            metrics.mean_magnitude < config.fast_threshold) {
            return MotionType::FastMotion;
        }
        
        if (metrics.mean_magnitude >= config.static_threshold &&
            metrics.mean_magnitude < config.slow_threshold) {
            return MotionType::SlowMotion;
        }
        
        return MotionType::SlowMotion;
    }
    
    std::vector<TestResult> evaluate_thresholds(const ThresholdConfig& config) const {
        std::vector<TestResult> results;
        
        for (const auto& test_case : test_cases_) {
            MotionMetrics metrics = calculate_metrics(test_case.transforms);
            MotionType predicted = classify_from_metrics(metrics, config);
            
            bool passed = (predicted == test_case.expected_type);
            
            results.emplace_back(
                test_case.name,
                passed,
                predicted,
                test_case.expected_type,
                metrics
            );
        }
        
        return results;
    }
    
    double calculate_accuracy(const std::vector<TestResult>& results) const {
        if (results.empty()) return 0.0;
        
        int passed_count = 0;
        for (const auto& result : results) {
            if (result.passed) passed_count++;
        }
        
        return static_cast<double>(passed_count) / static_cast<double>(results.size());
    }
};

} // namespace AdaptiveStabilization

int main() {
    using namespace AdaptiveStabilization;
    
    std::cout << "=== MotionClassifier Threshold Tuner ===\n\n";
    
    // Create tuner
    ThresholdTuner tuner;
    
    // Try grid search first
    std::cout << "--- Grid Search ---\n";
    auto grid_report = tuner.tune_thresholds_grid_search(5);
    grid_report.print_report();
    
    // If grid search didn't achieve 100%, try random search
    if (grid_report.best_accuracy < 1.0) {
        std::cout << "\n--- Random Search ---\n";
        auto random_report = tuner.tune_thresholds_random_search(5000);
        random_report.print_report();
    }
    
    std::cout << "\n=== Threshold Tuning Complete ===\n";
    
    return 0;
}
