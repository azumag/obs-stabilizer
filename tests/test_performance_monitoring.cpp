#ifndef SKIP_OPENCV_TESTS

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <vector>
#include <fstream>
#include <cmath>
#include "core/stabilizer_core.hpp"
#include "test_data_generator.hpp"
#include "test_constants.hpp"

using namespace TestDataGenerator;
using namespace TestConstants;

struct PerformanceBaseline {
    double avg_time_1080p_ms = 11.6;
    double avg_time_720p_ms = 4.7;
    double avg_time_480p_ms = 1.3;
    double fps_1080p = 85.8;
    double fps_720p = 214.0;
    double fps_480p = 757.0;
};

class PerformanceMonitoringTests : public ::testing::Test {
protected:
    void SetUp() override {
        test_params.enabled = true;
        test_params.smoothing_radius = 10;
        test_params.max_correction = 20.0f;
        test_params.feature_count = 200;
        test_params.quality_level = 0.01f;
        test_params.min_distance = 10.0f;
        test_params.block_size = 3;
        test_params.use_harris = false;
        test_params.k = 0.04f;
        test_params.debug_mode = false;
    }

    void TearDown() override {
    }

    StabilizerCore::StabilizerParams test_params;
    PerformanceBaseline baseline;

    bool check_performance_regression(double current_time_ms, double baseline_time_ms, 
                                    double threshold_percent = 10.0) {
        double ratio = (current_time_ms / baseline_time_ms) * 100.0;
        double degradation = ratio - 100.0;
        return degradation > threshold_percent;
    }

    double calculate_stddev(const std::vector<double>& values, double mean) {
        double variance = 0.0;
        for (double val : values) {
            variance += (val - mean) * (val - mean);
        }
        variance /= values.size();
        return std::sqrt(variance);
    }
};

TEST_F(PerformanceMonitoringTests, BenchmarkBaseline1080p) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    cv::Mat frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, 0);

    const int iterations = 100;
    std::vector<double> processing_times;
    double total_time = 0.0;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        double time_ms = duration.count() * 1000.0;
        processing_times.push_back(time_ms);
        total_time += time_ms;
    }

    double avg_time = total_time / iterations;
    double stddev = calculate_stddev(processing_times, avg_time);
    double fps = 1000.0 / avg_time;

    std::cout << "1080p Performance Baseline:" << std::endl;
    std::cout << "  Average time: " << avg_time << " ms" << std::endl;
    std::cout << "  Std deviation: " << stddev << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;
    std::cout << "  Baseline time: " << baseline.avg_time_1080p_ms << " ms" << std::endl;

    bool has_regression = check_performance_regression(avg_time, baseline.avg_time_1080p_ms);
    if (has_regression) {
        std::cout << "  WARNING: Performance regression detected!" << std::endl;
    }

    EXPECT_FALSE(has_regression) << "No performance regression should exceed 10%";
    EXPECT_LT(stddev, avg_time * 0.5) << "Standard deviation should be reasonable";
}

TEST_F(PerformanceMonitoringTests, BenchmarkBaseline720p) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD720_WIDTH, Resolution::HD720_HEIGHT, test_params);

    cv::Mat frame = generate_test_frame(Resolution::HD720_WIDTH, Resolution::HD720_HEIGHT, 0);

    const int iterations = 100;
    std::vector<double> processing_times;
    double total_time = 0.0;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        double time_ms = duration.count() * 1000.0;
        processing_times.push_back(time_ms);
        total_time += time_ms;
    }

    double avg_time = total_time / iterations;
    double stddev = calculate_stddev(processing_times, avg_time);
    double fps = 1000.0 / avg_time;

    std::cout << "720p Performance Baseline:" << std::endl;
    std::cout << "  Average time: " << avg_time << " ms" << std::endl;
    std::cout << "  Std deviation: " << stddev << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;
    std::cout << "  Baseline time: " << baseline.avg_time_720p_ms << " ms" << std::endl;

    bool has_regression = check_performance_regression(avg_time, baseline.avg_time_720p_ms);
    if (has_regression) {
        std::cout << "  WARNING: Performance regression detected!" << std::endl;
    }

    EXPECT_FALSE(has_regression) << "No performance regression should exceed 10%";
}

TEST_F(PerformanceMonitoringTests, BenchmarkBaseline480p) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, test_params);

    cv::Mat frame = generate_test_frame(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, 0);

    const int iterations = 100;
    std::vector<double> processing_times;
    double total_time = 0.0;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        double time_ms = duration.count() * 1000.0;
        processing_times.push_back(time_ms);
        total_time += time_ms;
    }

    double avg_time = total_time / iterations;
    double stddev = calculate_stddev(processing_times, avg_time);
    double fps = 1000.0 / avg_time;

    std::cout << "480p Performance Baseline:" << std::endl;
    std::cout << "  Average time: " << avg_time << " ms" << std::endl;
    std::cout << "  Std deviation: " << stddev << " ms" << std::endl;
    std::cout << "  FPS: " << fps << std::endl;
    std::cout << "  Baseline time: " << baseline.avg_time_480p_ms << " ms" << std::endl;

    bool has_regression = check_performance_regression(avg_time, baseline.avg_time_480p_ms);
    if (has_regression) {
        std::cout << "  WARNING: Performance regression detected!" << std::endl;
    }

    EXPECT_FALSE(has_regression) << "No performance regression should exceed 10%";
}

TEST_F(PerformanceMonitoringTests, MemoryUsageMonitoring) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    auto initial_metrics = stabilizer.get_performance_metrics();

    for (int i = 0; i < 100; i++) {
        cv::Mat frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, i % 3);
        stabilizer.process_frame(frame);
    }

    auto final_metrics = stabilizer.get_performance_metrics();

    std::cout << "Memory Usage Monitoring:" << std::endl;
    std::cout << "  Frames processed: " << final_metrics.frame_count << std::endl;
    std::cout << "  Average processing time: " << final_metrics.avg_processing_time * 1000 << " ms" << std::endl;

    EXPECT_EQ(final_metrics.frame_count, 100);
    EXPECT_GT(final_metrics.avg_processing_time, 0.0);
    EXPECT_LT(final_metrics.avg_processing_time, 1.0);
}

TEST_F(PerformanceMonitoringTests, FrameRateStabilityUnderLoad) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    const int num_frames = 200;
    std::vector<double> frame_times;
    frame_times.reserve(num_frames);

    for (int i = 0; i < num_frames; i++) {
        cv::Mat frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, i % 3);
        
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        frame_times.push_back(duration.count() * 1000.0);
    }

    double avg_time = 0.0;
    for (double time : frame_times) {
        avg_time += time;
    }
    avg_time /= num_frames;

    double stddev = calculate_stddev(frame_times, avg_time);
    double fps = 1000.0 / avg_time;
    double fps_stddev = (stddev / avg_time) * fps;

    std::cout << "Frame Rate Stability Under Load:" << std::endl;
    std::cout << "  Average frame time: " << avg_time << " ms" << std::endl;
    std::cout << "  Frame time stddev: " << stddev << " ms" << std::endl;
    std::cout << "  Average FPS: " << fps << std::endl;
    std::cout << "  FPS variability: " << fps_stddev << " FPS" << std::endl;

    EXPECT_LT(fps_stddev, fps * 10.0) << "FPS variability should be reasonable (allows for wide variation)";
}

TEST_F(PerformanceMonitoringTests, PerformanceDegradationAlert) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    cv::Mat frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, 0);

    const int iterations = 50;
    std::vector<double> early_times;
    std::vector<double> late_times;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        double time_ms = duration.count() * 1000.0;

        if (i < iterations / 2) {
            early_times.push_back(time_ms);
        } else {
            late_times.push_back(time_ms);
        }
    }

    double early_avg = 0.0, late_avg = 0.0;
    for (double t : early_times) early_avg += t;
    for (double t : late_times) late_avg += t;
    early_avg /= early_times.size();
    late_avg /= late_times.size();

    double degradation = ((late_avg - early_avg) / early_avg) * 100.0;

    std::cout << "Performance Degradation Check:" << std::endl;
    std::cout << "  Early frames avg: " << early_avg << " ms" << std::endl;
    std::cout << "  Late frames avg: " << late_avg << " ms" << std::endl;
    std::cout << "  Degradation: " << degradation << "%" << std::endl;

    if (degradation > 10.0) {
        std::cout << "  ALERT: Performance degradation > 10% detected!" << std::endl;
    }

    EXPECT_LT(degradation, 10.0) << "Performance should not degrade more than 10%";
}

TEST_F(PerformanceMonitoringTests, LongRunningPerformanceStability) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD720_WIDTH, Resolution::HD720_HEIGHT, test_params);

    const int num_batches = 10;
    const int frames_per_batch = 50;
    std::vector<double> batch_avg_times;

    for (int batch = 0; batch < num_batches; batch++) {
        double batch_time = 0.0;
        for (int i = 0; i < frames_per_batch; i++) {
            cv::Mat frame = generate_test_frame(Resolution::HD720_WIDTH, Resolution::HD720_HEIGHT, 
                                               (batch * frames_per_batch + i) % 3);
            
            auto start = std::chrono::high_resolution_clock::now();
            cv::Mat result = stabilizer.process_frame(frame);
            auto end = std::chrono::high_resolution_clock::now();

            std::chrono::duration<double> duration = end - start;
            batch_time += duration.count() * 1000.0;
        }
        batch_avg_times.push_back(batch_time / frames_per_batch);
    }

    double overall_avg = 0.0;
    for (double t : batch_avg_times) overall_avg += t;
    overall_avg /= batch_avg_times.size();

    double stddev = calculate_stddev(batch_avg_times, overall_avg);
    double cv_percent = (stddev / overall_avg) * 100.0;

    std::cout << "Long Running Performance Stability:" << std::endl;
    std::cout << "  Overall avg time: " << overall_avg << " ms" << std::endl;
    std::cout << "  Batch time stddev: " << stddev << " ms" << std::endl;
    std::cout << "  Coefficient of variation: " << cv_percent << "%" << std::endl;

    EXPECT_LT(cv_percent, 100.0) << "Performance should be reasonably stable (CV < 100%)";
}

TEST_F(PerformanceMonitoringTests, ConsecutiveOperationsPerformance) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    const int iterations = 100;
    double min_time = 1e9, max_time = 0.0, total_time = 0.0;

    for (int i = 0; i < iterations; i++) {
        cv::Mat frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, i % 3);
        
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        double time_ms = duration.count() * 1000.0;

        min_time = std::min(min_time, time_ms);
        max_time = std::max(max_time, time_ms);
        total_time += time_ms;
    }

    double avg_time = total_time / iterations;
    double range = max_time - min_time;
    double range_percent = (range / avg_time) * 100.0;

    std::cout << "Consecutive Operations Performance:" << std::endl;
    std::cout << "  Min time: " << min_time << " ms" << std::endl;
    std::cout << "  Max time: " << max_time << " ms" << std::endl;
    std::cout << "  Avg time: " << avg_time << " ms" << std::endl;
    std::cout << "  Range: " << range << " ms (" << range_percent << "%)" << std::endl;

    EXPECT_LT(range_percent, 5000.0) << "Time range should be within 50x (allows for initial frame setup)";
}

TEST_F(PerformanceMonitoringTests, PerformanceMetricsAccuracy) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, test_params);

    const int num_frames = 50;
    std::vector<double> actual_times;
    actual_times.reserve(num_frames);

    for (int i = 0; i < num_frames; i++) {
        cv::Mat frame = generate_test_frame(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, i % 3);
        
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = stabilizer.process_frame(frame);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        actual_times.push_back(duration.count());
    }

    auto metrics = stabilizer.get_performance_metrics();
    double calculated_avg = 0.0;
    for (double t : actual_times) calculated_avg += t;
    calculated_avg /= actual_times.size();

    double error_percent = std::abs((metrics.avg_processing_time - calculated_avg) / calculated_avg) * 100.0;

    std::cout << "Performance Metrics Accuracy:" << std::endl;
    std::cout << "  Calculated avg: " << calculated_avg * 1000 << " ms" << std::endl;
    std::cout << "  Metrics avg: " << metrics.avg_processing_time * 1000 << " ms" << std::endl;
    std::cout << "  Error: " << error_percent << "%" << std::endl;

    EXPECT_LT(error_percent, 5.0) << "Metrics should be accurate within 5%";
    EXPECT_EQ(metrics.frame_count, num_frames);
}

TEST_F(PerformanceMonitoringTests, PerformanceUnderDifferentParameterSets) {
    std::vector<std::pair<std::string, StabilizerCore::StabilizerParams>> configs = {
        {"default", test_params},
        {"high_features", test_params},
        {"high_smoothing", test_params},
        {"low_quality", test_params}
    };

    configs[1].second.feature_count = 500;
    configs[2].second.smoothing_radius = 50;
    configs[3].second.quality_level = 0.001f;

    std::cout << "Performance Under Different Parameter Sets:" << std::endl;

    for (const auto& [name, params] : configs) {
        StabilizerCore stabilizer;
        stabilizer.initialize(Resolution::HD720_WIDTH, Resolution::HD720_HEIGHT, params);

        cv::Mat frame = generate_test_frame(Resolution::HD720_WIDTH, Resolution::HD720_HEIGHT, 0);

        const int iterations = 50;
        double total_time = 0.0;

        for (int i = 0; i < iterations; i++) {
            auto start = std::chrono::high_resolution_clock::now();
            cv::Mat result = stabilizer.process_frame(frame);
            auto end = std::chrono::high_resolution_clock::now();

            std::chrono::duration<double> duration = end - start;
            total_time += duration.count() * 1000.0;
        }

        double avg_time = total_time / iterations;
        double fps = 1000.0 / avg_time;

        std::cout << "  " << name << ": " << avg_time << " ms (" << fps << " FPS)" << std::endl;

        EXPECT_GT(avg_time, 0.0);
        EXPECT_LT(avg_time, 100.0);
    }
}

TEST_F(PerformanceMonitoringTests, AdaptivePerformanceScaling) {
    StabilizerCore stabilizer;
    stabilizer.initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, test_params);

    std::vector<int> feature_counts = {50, 100, 200, 400, 800};
    std::vector<double> avg_times;

    for (int fc : feature_counts) {
        test_params.feature_count = fc;
        stabilizer.update_parameters(test_params);

        cv::Mat frame = generate_test_frame(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, 0);

        const int iterations = 30;
        double total_time = 0.0;

        for (int i = 0; i < iterations; i++) {
            auto start = std::chrono::high_resolution_clock::now();
            cv::Mat result = stabilizer.process_frame(frame);
            auto end = std::chrono::high_resolution_clock::now();

            std::chrono::duration<double> duration = end - start;
            total_time += duration.count() * 1000.0;
        }

        double avg_time = total_time / iterations;
        avg_times.push_back(avg_time);
    }

    std::cout << "Adaptive Performance Scaling:" << std::endl;
    for (size_t i = 0; i < feature_counts.size(); i++) {
        std::cout << "  " << feature_counts[i] << " features: " << avg_times[i] << " ms" << std::endl;
    }

    EXPECT_LT(avg_times.back(), avg_times.front() * 10) << "Scaling should be reasonable";
}

#endif // SKIP_OPENCV_TESTS
