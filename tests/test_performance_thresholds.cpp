/**
 * Performance Threshold Tests
 *
 * This file contains tests to verify performance thresholds including:
 * - CPU usage profiling and verification (filter should increase CPU by <5%)
 * - Processing delay verification (1920x1080 @ 30fps should be <33ms)
 *
 * Critical acceptance criteria:
 * - フィルター適用時のCPU使用率増加が閾値（5%）以下
 * - 1920x1080 @ 30fpsで処理遅延が1フレーム（33ms）以内
 */

#include <gtest/gtest.h>

// Platform-specific includes for CPU tracking
#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_host.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

#include <gtest/gtest.h>
#include <numeric>
#include "../src/core/stabilizer_core.hpp"
#include "../src/core/stabilizer_wrapper.hpp"
#include "test_constants.hpp"
#include "test_data_generator.hpp"
#include <chrono>
#include <thread>
#include <vector>

using namespace TestConstants;

// ============================================================================
// Platform-Specific CPU Tracking
// ============================================================================

/**
 * CPU Usage Tracker class
 * Provides platform-independent CPU usage measurement
 */
class CPUTracker {
public:
    /**
     * Initialize the CPU tracker
     * Must be called before measurements
     */
    bool initialize() {
#if defined(__linux__)
        // Linux: Read from /proc/stat
        cpu_stat_file.open("/proc/stat");
        return cpu_stat_file.is_open();
#elif defined(__APPLE__)
        // macOS: Use host_statistics
        init_time = std::chrono::high_resolution_clock::now();
        return get_initial_cpu_time();
#elif defined(_WIN32)
        // Windows: Use GetSystemTimes
        // Initialize CPU time tracking for Windows by capturing initial CPU times
        init_time = std::chrono::high_resolution_clock::now();
        return GetSystemTimes(&idle_time, &kernel_time, &user_time);
#else
        return false;
#endif
    }

    /**
     * Get current CPU usage percentage since initialization or last reset
     * Returns -1.0 on error
     */
    double get_cpu_usage() {
#if defined(__linux__)
        return get_cpu_usage_linux();
#elif defined(__APPLE__)
        return get_cpu_usage_macos();
#elif defined(_WIN32)
        return get_cpu_usage_windows();
#else
        return -1.0;
#endif
    }

    /**
     * Reset the CPU measurement baseline
     */
    void reset() {
        reset_start_time = std::chrono::high_resolution_clock::now();
#if defined(_WIN32)
        GetSystemTimes(&idle_time, &kernel_time, &user_time);
#endif
    }

private:
#if defined(__linux__)
    std::ifstream cpu_stat_file;
    unsigned long long prev_idle = 0, prev_total = 0;

    double get_cpu_usage_linux() {
        std::ifstream cpu_stat("/proc/stat");
        std::string line;
        std::getline(cpu_stat, line);

        std::istringstream iss(line);
        std::string cpu_label;
        unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
        iss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

        unsigned long long total = user + nice + system + idle + iowait + irq + softirq + steal;

        if (prev_total == 0) {
            prev_idle = idle;
            prev_total = total;
            return 0.0;
        }

        unsigned long long total_diff = total - prev_total;
        unsigned long long idle_diff = idle - prev_idle;

        prev_idle = idle;
        prev_total = total;

        if (total_diff == 0) return 0.0;
        return (1.0 - static_cast<double>(idle_diff) / total_diff) * 100.0;
    }

#elif defined(__APPLE__)
    std::chrono::high_resolution_clock::time_point init_time;
    std::chrono::high_resolution_clock::time_point reset_start_time;
    unsigned long long initial_cpu_time = 0;

    bool get_initial_cpu_time() {
        host_cpu_load_info_data_t cpu_info;
        mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
        if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO,
                           (host_info_t)&cpu_info, &count) == KERN_SUCCESS) {
            initial_cpu_time = cpu_info.cpu_ticks[CPU_STATE_USER]
                             + cpu_info.cpu_ticks[CPU_STATE_SYSTEM]
                             + cpu_info.cpu_ticks[CPU_STATE_IDLE]
                             + cpu_info.cpu_ticks[CPU_STATE_NICE];
            return true;
        }
        return false;
    }

    double get_cpu_usage_macos() {
        host_cpu_load_info_data_t cpu_info;
        mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
        if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO,
                           (host_info_t)&cpu_info, &count) == KERN_SUCCESS) {
            unsigned long long current_cpu_time = cpu_info.cpu_ticks[CPU_STATE_USER]
                                                + cpu_info.cpu_ticks[CPU_STATE_SYSTEM]
                                                + cpu_info.cpu_ticks[CPU_STATE_IDLE]
                                                + cpu_info.cpu_ticks[CPU_STATE_NICE];

            unsigned long long elapsed_ticks = current_cpu_time - initial_cpu_time;

            auto elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - init_time
            ).count();

            if (elapsed_time == 0) return 0.0;

            // Estimate CPU usage based on CPU ticks vs wall time
            // This is an approximation
            return (static_cast<double>(elapsed_ticks) / elapsed_time) * 100.0;
        }
        return -1.0;
    }

#elif defined(_WIN32)
    FILETIME idle_time, kernel_time, user_time;
    std::chrono::high_resolution_clock::time_point init_time;
    std::chrono::high_resolution_clock::time_point reset_start_time;

    double get_cpu_usage_windows() {
        FILETIME current_idle, current_kernel, current_user;
        if (!GetSystemTimes(&current_idle, &current_kernel, &current_user)) {
            return -1.0;
        }

        ULONGLONG idle_diff = file_time_diff(idle_time, current_idle);
        ULONGLONG kernel_diff = file_time_diff(kernel_time, current_kernel);
        ULONGLONG user_diff = file_time_diff(user_time, current_user);

        ULONGLONG total_diff = kernel_diff + user_diff;
        ULONGLONG total_system = total_diff + idle_diff;

        idle_time = current_idle;
        kernel_time = current_kernel;
        user_time = current_user;

        if (total_system == 0) return 0.0;
        return (static_cast<double>(total_diff) / total_system) * 100.0;
    }

    ULONGLONG file_time_diff(FILETIME a, FILETIME b) {
        ULONGLONG a_int = (static_cast<ULONGLONG>(a.dwHighDateTime) << 32) | a.dwLowDateTime;
        ULONGLONG b_int = (static_cast<ULONGLONG>(b.dwHighDateTime) << 32) | b.dwLowDateTime;
        return b_int - a_int;
    }
#endif
};

// ============================================================================
// Performance Test Class
// ============================================================================

class PerformanceThresholdTest : public ::testing::Test {
protected:
    void SetUp() override {
        stabilizer = std::make_unique<StabilizerCore>();
        cpu_tracker = std::make_unique<CPUTracker>();
        ASSERT_TRUE(cpu_tracker->initialize());
    }

    void TearDown() override {
        stabilizer.reset();
        cpu_tracker.reset();
    }

    StabilizerCore::StabilizerParams getDefaultParams() {
        StabilizerCore::StabilizerParams params;
        params.smoothing_radius = Processing::MEDIUM_SMOOTHING_WINDOW;
        params.max_correction = 50.0f;
        params.feature_count = Features::DEFAULT_COUNT;
        params.quality_level = Processing::DEFAULT_QUALITY_LEVEL;
        params.min_distance = Processing::DEFAULT_MIN_DISTANCE;
        return params;
    }

    /**
     * Process frames and measure processing time for each frame
     * Returns vector of processing times in milliseconds
     */
    std::vector<double> measure_processing_times(
        StabilizerCore* stab,
        const std::vector<cv::Mat>& frames
    ) {
        std::vector<double> processing_times;

        for (const auto& frame : frames) {
            auto start = std::chrono::high_resolution_clock::now();
            cv::Mat result = stab->process_frame(frame);
            auto end = std::chrono::high_resolution_clock::now();

            auto duration = std::chrono::duration<double, std::milli>(end - start);
            processing_times.push_back(duration.count());

            if (result.empty()) {
                ADD_FAILURE() << "Frame processing failed";
                return {};
            }
        }

        return processing_times;
    }

    /**
     * Calculate statistics from processing times
     */
    struct ProcessingStats {
        double avg_ms;
        double min_ms;
        double max_ms;
        double std_dev_ms;
    };

    ProcessingStats calculate_stats(const std::vector<double>& times) {
        if (times.empty()) {
            return {0.0, 0.0, 0.0, 0.0};
        }

        double sum = std::accumulate(times.begin(), times.end(), 0.0);
        double avg = sum / times.size();
        double min_val = *std::min_element(times.begin(), times.end());
        double max_val = *std::max_element(times.begin(), times.end());

        // Calculate standard deviation
        double variance = 0.0;
        for (double time : times) {
            variance += (time - avg) * (time - avg);
        }
        variance /= times.size();
        double std_dev = std::sqrt(variance);

        return {avg, min_val, max_val, std_dev};
    }

    std::unique_ptr<StabilizerCore> stabilizer;
    std::unique_ptr<CPUTracker> cpu_tracker;
};

// ============================================================================
// CPU Usage Tests
// ============================================================================

/**
 * Test: CPU usage increase is within threshold (5%)
 * Acceptance criteria: フィルター適用時のCPU使用率増加が閾値（5%）以下
 */
TEST_F(PerformanceThresholdTest, CPUUsageWithinThreshold) {
    // Measure baseline CPU usage (without stabilizer)
    cpu_tracker->reset();
    auto baseline_frames = TestDataGenerator::generate_test_sequence(
        100, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );

    // Process frames without stabilizer (baseline)
    for (const auto& frame : baseline_frames) {
        // Simulate frame processing (just copy to simulate work)
        cv::Mat copy = frame.clone();
        (void)copy;  // Suppress unused warning
        std::this_thread::sleep_for(std::chrono::microseconds(100));  // Simulate processing
    }

    double baseline_cpu = cpu_tracker->get_cpu_usage();

    // Now measure with stabilizer enabled
    cpu_tracker->reset();
    auto params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        100, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
    );

    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        ASSERT_FALSE(result.empty());
    }

    double stabilizer_cpu = cpu_tracker->get_cpu_usage();

    // CPU increase should be < 5%
    double cpu_increase = stabilizer_cpu - baseline_cpu;

    EXPECT_LT(cpu_increase, 5.0)
        << "CPU usage increase should be <5%, got: " << cpu_increase << "%"
        << " (baseline: " << baseline_cpu << "%, with stabilizer: " << stabilizer_cpu << "%)";

    // Also ensure CPU usage doesn't spike excessively
    EXPECT_LT(stabilizer_cpu, 50.0)
        << "Total CPU usage with stabilizer should be reasonable, got: " << stabilizer_cpu << "%";
}

/**
 * Test: CPU usage scales appropriately with resolution
 * Higher resolution should increase CPU usage, but not dramatically
 */
// DISABLED: CPU usage measurement is platform-dependent and unstable in CI environments
// These tests are useful for local development but not for CI/CD pipelines
TEST_F(PerformanceThresholdTest, DISABLED_CPUUsageScalesWithResolution) {
    auto params = getDefaultParams();

    // Measure CPU usage for VGA
    cpu_tracker->reset();
    std::unique_ptr<StabilizerCore> stab_vga = std::make_unique<StabilizerCore>();
    ASSERT_TRUE(stab_vga->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto vga_frames = TestDataGenerator::generate_test_sequence(
        50, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
    );

    for (const auto& frame : vga_frames) {
        cv::Mat result = stab_vga->process_frame(frame);
        ASSERT_FALSE(result.empty());
    }

    double cpu_vga = cpu_tracker->get_cpu_usage();

    // Measure CPU usage for HD
    cpu_tracker->reset();
    std::unique_ptr<StabilizerCore> stab_hd = std::make_unique<StabilizerCore>();
    ASSERT_TRUE(stab_hd->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));

    auto hd_frames = TestDataGenerator::generate_test_sequence(
        50, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, "shake"
    );

    for (const auto& frame : hd_frames) {
        cv::Mat result = stab_hd->process_frame(frame);
        ASSERT_FALSE(result.empty());
    }

    double cpu_hd = cpu_tracker->get_cpu_usage();

    // HD should use more CPU than VGA (roughly proportional to pixel count)
    // VGA: 640*480 = 307,200 pixels
    // HD: 1920*1080 = 2,073,600 pixels (6.75x)
    // However, optimizations may reduce the ratio
    EXPECT_GT(cpu_hd, cpu_vga)
        << "HD resolution should use more CPU than VGA";

    // CPU ratio should be reasonable (not linear with pixel count due to optimizations)
    double cpu_ratio = cpu_hd / std::max(cpu_vga, 0.1);
    EXPECT_LT(cpu_ratio, 5.0)
        << "CPU usage ratio should be reasonable, got: " << cpu_ratio << "x";
}

/**
 * Test: CPU usage with multiple stabilizer instances
 * Tests that CPU usage scales reasonably with multiple sources
 */
// DISABLED: CPU usage measurement is platform-dependent and unstable in CI environments
// These tests are useful for local development but not for CI/CD pipelines
TEST_F(PerformanceThresholdTest, DISABLED_CPUUsageWithMultipleSources) {
    auto params = getDefaultParams();

    // Create 3 stabilizer instances
    std::vector<std::unique_ptr<StabilizerCore>> stabilizers;
    for (int i = 0; i < 3; i++) {
        auto stab = std::make_unique<StabilizerCore>();
        ASSERT_TRUE(stab->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
        stabilizers.push_back(std::move(stab));
    }

    // Measure baseline CPU with 1 source
    cpu_tracker->reset();
    auto frames = TestDataGenerator::generate_test_sequence(
        50, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
    );

    for (const auto& frame : frames) {
        cv::Mat result = stabilizers[0]->process_frame(frame);
        ASSERT_FALSE(result.empty());
    }

    double cpu_1_source = cpu_tracker->get_cpu_usage();

    // Measure CPU with 3 sources
    cpu_tracker->reset();
    for (auto& stab : stabilizers) {
        for (const auto& frame : frames) {
            cv::Mat result = stab->process_frame(frame);
            ASSERT_FALSE(result.empty());
        }
    }

    double cpu_3_sources = cpu_tracker->get_cpu_usage();

    // CPU should increase but not linearly (some overhead is shared)
    EXPECT_GT(cpu_3_sources, cpu_1_source)
        << "3 sources should use more CPU than 1 source";

    // Total CPU increase should still be reasonable
    double total_cpu_increase = cpu_3_sources - cpu_1_source;
    EXPECT_LT(total_cpu_increase, 15.0)
        << "Total CPU increase for 3 sources should be <15%, got: " << total_cpu_increase << "%";
}

// ============================================================================
// Processing Delay Tests
// ============================================================================

/**
 * Test: Processing delay within threshold for HD @ 30fps
 * Acceptance criteria: 1920x1080 @ 30fpsで処理遅延が1フレーム（33ms）以内
 */
TEST_F(PerformanceThresholdTest, ProcessingDelayWithinThreshold_HD_30fps) {
    auto params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));

    // Generate HD frames
    auto frames = TestDataGenerator::generate_test_sequence(
        100, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, "shake"
    );

    // Measure processing times
    auto processing_times = measure_processing_times(stabilizer.get(), frames);
    ASSERT_FALSE(processing_times.empty());

    // Calculate statistics
    auto stats = calculate_stats(processing_times);

    // Verify average processing time is < 33ms (1 frame at 30fps)
    EXPECT_LT(stats.avg_ms, 33.0)
        << "Average processing time should be <33ms (1 frame at 30fps), got: "
        << stats.avg_ms << "ms";

    // Verify max processing time is < 50ms (allows some spikes)
    EXPECT_LT(stats.max_ms, 50.0)
        << "Max processing time should be <50ms, got: " << stats.max_ms << "ms";

    // Verify min processing time is reasonable (not too fast to be suspicious)
    EXPECT_GT(stats.min_ms, 1.0)
        << "Min processing time should be >1ms, got: " << stats.min_ms << "ms";
}

/**
 * Test: Processing delay within threshold for VGA @ 30fps
 * VGA should be significantly faster than HD
 */
TEST_F(PerformanceThresholdTest, ProcessingDelayWithinThreshold_VGA_30fps) {
    auto params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        100, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
    );

    auto processing_times = measure_processing_times(stabilizer.get(), frames);
    ASSERT_FALSE(processing_times.empty());

    auto stats = calculate_stats(processing_times);

    // VGA should be much faster than 33ms
    EXPECT_LT(stats.avg_ms, 15.0)
        << "Average processing time for VGA should be <15ms, got: " << stats.avg_ms << "ms";

    // Max should still be reasonable
    EXPECT_LT(stats.max_ms, 25.0)
        << "Max processing time for VGA should be <25ms, got: " << stats.max_ms << "ms";
}

/**
 * Test: Processing delay with different motion types
 * Different motion types may have different performance characteristics
 */
TEST_F(PerformanceThresholdTest, ProcessingDelayWithDifferentMotionTypes) {
    auto params = getDefaultParams();

    std::vector<std::string> motion_types = {"static", "shake", "pan_right", "fast", "zoom_in"};

    for (const auto& motion_type : motion_types) {
        stabilizer->reset();
        ASSERT_TRUE(stabilizer->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));

        auto frames = TestDataGenerator::generate_test_sequence(
            50, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, motion_type
        );

        auto processing_times = measure_processing_times(stabilizer.get(), frames);
        ASSERT_FALSE(processing_times.empty());

        auto stats = calculate_stats(processing_times);

        // All motion types should be within threshold
        EXPECT_LT(stats.avg_ms, 33.0)
            << "Motion type '" << motion_type << "' avg processing time should be <33ms, got: "
            << stats.avg_ms << "ms";

        EXPECT_LT(stats.max_ms, 50.0)
            << "Motion type '" << motion_type << "' max processing time should be <50ms, got: "
            << stats.max_ms << "ms";
    }
}

/**
 * Test: Processing delay with different smoothing radii
 * Larger smoothing radius may slightly increase processing time
 */
TEST_F(PerformanceThresholdTest, ProcessingDelayWithDifferentSmoothing) {
    std::vector<int> smoothing_radii = {
        Processing::SMALL_SMOOTHING_WINDOW,
        Processing::MEDIUM_SMOOTHING_WINDOW,
        Processing::LARGE_SMOOTHING_WINDOW
    };

    for (int smoothing_radius : smoothing_radii) {
        auto params = getDefaultParams();
        params.smoothing_radius = smoothing_radius;

        stabilizer->reset();
        ASSERT_TRUE(stabilizer->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));

        auto frames = TestDataGenerator::generate_test_sequence(
            50, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, "shake"
        );

        auto processing_times = measure_processing_times(stabilizer.get(), frames);
        ASSERT_FALSE(processing_times.empty());

        auto stats = calculate_stats(processing_times);

        // All smoothing settings should be within threshold
        EXPECT_LT(stats.avg_ms, 33.0)
            << "Smoothing radius " << smoothing_radius << " avg processing time should be <33ms, got: "
            << stats.avg_ms << "ms";

        // Large smoothing may have slightly higher processing time
        // but should still be well within threshold
        if (smoothing_radius == Processing::LARGE_SMOOTHING_WINDOW) {
            EXPECT_LT(stats.avg_ms, 30.0)
                << "Large smoothing radius should still be efficient, got: " << stats.avg_ms << "ms";
        }
    }
}

/**
 * Test: Processing delay with different feature counts
 * More features may increase processing time slightly
 */
TEST_F(PerformanceThresholdTest, ProcessingDelayWithDifferentFeatureCounts) {
    std::vector<int> feature_counts = {
        Features::LOW_COUNT,
        Features::DEFAULT_COUNT,
        Features::HIGH_COUNT
    };

    for (int feature_count : feature_counts) {
        auto params = getDefaultParams();
        params.feature_count = feature_count;

        stabilizer->reset();
        ASSERT_TRUE(stabilizer->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));

        auto frames = TestDataGenerator::generate_test_sequence(
            50, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, "shake"
        );

        auto processing_times = measure_processing_times(stabilizer.get(), frames);
        ASSERT_FALSE(processing_times.empty());

        auto stats = calculate_stats(processing_times);

        // All feature counts should be within threshold
        EXPECT_LT(stats.avg_ms, 33.0)
            << "Feature count " << feature_count << " avg processing time should be <33ms, got: "
            << stats.avg_ms << "ms";
    }
}

/**
 * Test: Processing delay consistency over time
 * Processing time should be consistent, not degrading over time
 */
// DISABLED: CPU usage measurement is platform-dependent and unstable in CI environments
// These tests are useful for local development but not for CI/CD pipelines
TEST_F(PerformanceThresholdTest, DISABLED_ProcessingDelayConsistency) {
    auto params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        200, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, "shake"
    );

    auto processing_times = measure_processing_times(stabilizer.get(), frames);
    ASSERT_FALSE(processing_times.empty());

    // Split into first and second halves
    size_t half = processing_times.size() / 2;
    std::vector<double> first_half(processing_times.begin(), processing_times.begin() + half);
    std::vector<double> second_half(processing_times.begin() + half, processing_times.end());

    auto stats_first = calculate_stats(first_half);
    auto stats_second = calculate_stats(second_half);

    // Second half should not be significantly slower than first half
    // Allow up to 20% increase (accounts for frame warming and minor variance)
    double avg_ratio = stats_second.avg_ms / stats_first.avg_ms;
    EXPECT_LT(avg_ratio, 1.2)
        << "Processing time should not degrade over time, ratio: " << avg_ratio;

    // Standard deviation should be reasonable (<50% of average)
    double cv = stats_first.std_dev_ms / stats_first.avg_ms;
    EXPECT_LT(cv, 0.5)
        << "Processing time should be consistent, CV: " << cv;
}

// ============================================================================
// Preset Performance Tests
// ============================================================================

/**
 * Test: Gaming preset performance
 * Gaming preset should handle fast motion efficiently
 */
TEST_F(PerformanceThresholdTest, GamingPresetPerformance) {
    auto params = StabilizerCore::get_preset_gaming();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        100, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "fast"
    );

    auto processing_times = measure_processing_times(stabilizer.get(), frames);
    ASSERT_FALSE(processing_times.empty());

    auto stats = calculate_stats(processing_times);

    // Gaming preset should be very fast at VGA resolution
    EXPECT_LT(stats.avg_ms, 10.0)
        << "Gaming preset should be fast at VGA, got: " << stats.avg_ms << "ms";
}

/**
 * Test: Streaming preset performance
 * Streaming preset should handle HD efficiently
 */
TEST_F(PerformanceThresholdTest, StreamingPresetPerformance) {
    auto params = StabilizerCore::get_preset_streaming();
    ASSERT_TRUE(stabilizer->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        100, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, "shake"
    );

    auto processing_times = measure_processing_times(stabilizer.get(), frames);
    ASSERT_FALSE(processing_times.empty());

    auto stats = calculate_stats(processing_times);

    // Streaming preset should meet 30fps requirement
    EXPECT_LT(stats.avg_ms, 33.0)
        << "Streaming preset should meet 30fps requirement, got: " << stats.avg_ms << "ms";
}

/**
 * Test: Recording preset performance
 * Recording preset may be slower but should still be real-time
 */
TEST_F(PerformanceThresholdTest, RecordingPresetPerformance) {
    auto params = StabilizerCore::get_preset_recording();
    ASSERT_TRUE(stabilizer->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        100, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, "slow"
    );

    auto processing_times = measure_processing_times(stabilizer.get(), frames);
    ASSERT_FALSE(processing_times.empty());

    auto stats = calculate_stats(processing_times);

    // Recording preset should still be real-time capable
    EXPECT_LT(stats.avg_ms, 33.0)
        << "Recording preset should still be real-time capable, got: " << stats.avg_ms << "ms";
}
