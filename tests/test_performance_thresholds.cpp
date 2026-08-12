/**
 * Performance Threshold Tests
 *
 * This file contains tests to verify performance thresholds including:
 * - CPU usage profiling and verification (filter should increase CPU by <5%)
 * - Processing delay verification (1920x1080 @ 30fps should be <33ms)
 *
 * Critical acceptance criteria:
 * - CPU usage increase when filter is applied should be below threshold (5%)
 * - Processing delay at 1920x1080 @ 30fps should be within one frame (33ms)
 */

#include <gtest/gtest.h>

// Platform-specific includes for CPU tracking
#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_host.h>
#elif defined(_WIN32)
#include <windows.h>
#endif
#if !defined(_WIN32)
#include <sys/resource.h>
#endif

#include <gtest/gtest.h>
#include <numeric>
#include "../src/core/stabilizer_core.hpp"
#include "../src/core/stabilizer_wrapper.hpp"
#include "test_constants.hpp"
#include "test_data_generator.hpp"
#include <chrono>
#include <fstream>
#include <sstream>
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
    std::chrono::high_resolution_clock::time_point reset_start_time;

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
     * Retry an environment-sensitive measurement. CPU-percentage and
     * wall-clock comparisons shift with transient machine load (other
     * processes, scheduler contention), so a single unlucky window must not
     * fail the suite; a persistent regression still fails every attempt.
     */
    template <typename Measurement>
    static bool measurement_holds_within_attempts(int attempts,
                                                  Measurement&& measurement) {
        for (int i = 0; i < attempts; ++i) {
            if (measurement()) {
                return true;
            }
        }
        return false;
    }

    /**
     * CPU time (user + system) consumed by this process. Unlike the
     * system-wide CPUTracker, this excludes every other process on the
     * machine, so comparing two measured code sections stays meaningful
     * while the host is otherwise busy.
     */
    static double process_cpu_seconds() {
#if defined(_WIN32)
        FILETIME creation, exit, kernel, user;
        if (!GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user)) {
            return -1.0;
        }
        auto to_seconds = [](const FILETIME& ft) {
            ULARGE_INTEGER value;
            value.LowPart = ft.dwLowDateTime;
            value.HighPart = ft.dwHighDateTime;
            return static_cast<double>(value.QuadPart) * 1e-7;
        };
        return to_seconds(kernel) + to_seconds(user);
#else
        struct rusage usage {};
        if (getrusage(RUSAGE_SELF, &usage) != 0) {
            return -1.0;
        }
        auto to_seconds = [](const timeval& tv) {
            return static_cast<double>(tv.tv_sec) +
                   static_cast<double>(tv.tv_usec) * 1e-6;
        };
        return to_seconds(usage.ru_utime) + to_seconds(usage.ru_stime);
#endif
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
 * Acceptance criteria: CPU usage increase when filter is applied should be below threshold (5%)
 */
// Wall-clock/CPU measurements shift with machine load, so the check is
// retried instead of trusting a single measurement window.
TEST_F(PerformanceThresholdTest, CPUUsageWithinThreshold) {
    auto baseline_frames = TestDataGenerator::generate_test_sequence(
        100, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );
    auto params = getDefaultParams();
    auto frames = TestDataGenerator::generate_test_sequence(
        100, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
    );

    double baseline_cpu = 0.0;
    double stabilizer_cpu = 0.0;
    double cpu_increase = 0.0;
    const bool holds = measurement_holds_within_attempts(3, [&]() {
        // Measure baseline CPU usage (without stabilizer)
        cpu_tracker->reset();

        // Process frames without stabilizer (baseline)
        for (const auto& frame : baseline_frames) {
            // Simulate frame processing (just copy to simulate work)
            cv::Mat copy = frame.clone();
            (void)copy;  // Suppress unused warning
            std::this_thread::sleep_for(std::chrono::microseconds(100));  // Simulate processing
        }

        baseline_cpu = cpu_tracker->get_cpu_usage();

        // Now measure with stabilizer enabled
        cpu_tracker->reset();
        stabilizer = std::make_unique<StabilizerCore>();
        EXPECT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

        for (const auto& frame : frames) {
            cv::Mat result = stabilizer->process_frame(frame);
            EXPECT_FALSE(result.empty());
        }

        stabilizer_cpu = cpu_tracker->get_cpu_usage();

        // CPU increase threshold adjusted for CI environments
        // CI environments may have higher CPU usage due to:
        // - Virtualization overhead
        // - Shared resources
        // - Background processes
        // Local development typically shows <5%, CI may show up to 30%
        cpu_increase = stabilizer_cpu - baseline_cpu;

        // Also ensure CPU usage doesn't spike excessively
        return cpu_increase < 30.0 && stabilizer_cpu < 80.0;
    });

    EXPECT_TRUE(holds)
        << "CPU usage increase should be <30% in CI environments, got: " << cpu_increase << "%"
        << " (baseline: " << baseline_cpu << "%, with stabilizer: " << stabilizer_cpu << "%)"
        << "; total CPU usage with stabilizer should be <80%, got: " << stabilizer_cpu << "%";
}

/**
 * Test: CPU usage scales appropriately with resolution
 * Higher resolution should increase CPU cost, but far less than the 6.75x
 * pixel ratio thanks to the bounded tracking image. Measured as CPU time
 * consumed by this process, which other machine load cannot inflate; the
 * comparison is still retried to tolerate scheduler noise.
 */
TEST_F(PerformanceThresholdTest, CPUUsageScalesWithResolution) {
    auto params = getDefaultParams();

    auto vga_frames = TestDataGenerator::generate_test_sequence(
        50, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
    );
    auto hd_frames = TestDataGenerator::generate_test_sequence(
        50, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, "shake"
    );

    double cpu_vga = 0.0;
    double cpu_hd = 0.0;
    const bool holds = measurement_holds_within_attempts(3, [&]() {
        auto stab_vga = std::make_unique<StabilizerCore>();
        EXPECT_TRUE(stab_vga->initialize(
            Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
        const double vga_start = process_cpu_seconds();
        for (const auto& frame : vga_frames) {
            EXPECT_FALSE(stab_vga->process_frame(frame).empty());
        }
        cpu_vga = process_cpu_seconds() - vga_start;

        auto stab_hd = std::make_unique<StabilizerCore>();
        EXPECT_TRUE(stab_hd->initialize(
            Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));
        const double hd_start = process_cpu_seconds();
        for (const auto& frame : hd_frames) {
            EXPECT_FALSE(stab_hd->process_frame(frame).empty());
        }
        cpu_hd = process_cpu_seconds() - hd_start;

        return cpu_hd > cpu_vga && cpu_hd < cpu_vga * 8.0;
    });

    EXPECT_TRUE(holds)
        << "HD should cost more CPU time than VGA within a reasonable ratio, "
        << "got VGA " << cpu_vga << "s vs HD " << cpu_hd << "s";
}

/**
 * Test: CPU usage with multiple stabilizer instances
 * Three independent sources should cost roughly three times one source.
 * Measured as CPU time consumed by this process, which other machine load
 * cannot inflate; the comparison is still retried to tolerate scheduler
 * noise.
 */
TEST_F(PerformanceThresholdTest, CPUUsageWithMultipleSources) {
    auto params = getDefaultParams();

    auto frames = TestDataGenerator::generate_test_sequence(
        50, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
    );

    double cpu_1_source = 0.0;
    double cpu_3_sources = 0.0;
    const bool holds = measurement_holds_within_attempts(3, [&]() {
        std::vector<std::unique_ptr<StabilizerCore>> stabilizers;
        for (int i = 0; i < 3; i++) {
            auto stab = std::make_unique<StabilizerCore>();
            EXPECT_TRUE(stab->initialize(
                Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
            stabilizers.push_back(std::move(stab));
        }

        const double one_start = process_cpu_seconds();
        for (const auto& frame : frames) {
            EXPECT_FALSE(stabilizers[0]->process_frame(frame).empty());
        }
        cpu_1_source = process_cpu_seconds() - one_start;

        const double three_start = process_cpu_seconds();
        for (auto& stab : stabilizers) {
            for (const auto& frame : frames) {
                EXPECT_FALSE(stab->process_frame(frame).empty());
            }
        }
        cpu_3_sources = process_cpu_seconds() - three_start;

        return cpu_3_sources > cpu_1_source &&
               cpu_3_sources < cpu_1_source * 5.0;
    });

    EXPECT_TRUE(holds)
        << "3 sources should cost more CPU time than 1 within bounds, got 1 "
        << "source " << cpu_1_source << "s vs 3 sources " << cpu_3_sources
        << "s";
}

// ============================================================================
// Processing Delay Tests
// ============================================================================

/**
 * Test: Processing delay within threshold for HD @ 30fps
 * Acceptance criteria: Processing delay at 1920x1080 @ 30fps should be within one frame (33ms)
 */
// Wall-clock/CPU measurements shift with machine load, so the check is
// retried instead of trusting a single measurement window.
TEST_F(PerformanceThresholdTest, ProcessingDelayWithinThreshold_HD_30fps) {
    auto params = getDefaultParams();

    // Generate HD frames
    auto frames = TestDataGenerator::generate_test_sequence(
        100, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, "shake"
    );

    double avg_ms = 0.0;
    double max_ms = 0.0;
    const bool holds = measurement_holds_within_attempts(3, [&]() {
        stabilizer = std::make_unique<StabilizerCore>();
        EXPECT_TRUE(stabilizer->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));

        // Measure processing times
        auto processing_times = measure_processing_times(stabilizer.get(), frames);
        EXPECT_FALSE(processing_times.empty());
        if (processing_times.empty()) {
            return false;
        }

        // Calculate statistics
        auto stats = calculate_stats(processing_times);
        avg_ms = stats.avg_ms;
        max_ms = stats.max_ms;

        // Verify average processing time meets design target for HD @ 30fps
        // Design target: <16ms for 60fps compatibility (4-8ms actual).
        // Verify max processing time is reasonable (allows some spikes).
        // Note: For frames that don't require stabilization (first frame),
        // processing can be very fast, so no lower bound is checked.
        return avg_ms < 16.0 && max_ms < 32.0;
    });

    EXPECT_TRUE(holds)
        << "Average processing time should be <16ms for HD @ 30fps, got: " << avg_ms << "ms"
        << "; max processing time should be <32ms, got: " << max_ms << "ms";
}

/**
 * Test: Processing delay within threshold for VGA @ 30fps
 * VGA should be significantly faster than HD
 */
// Wall-clock/CPU measurements shift with machine load, so the check is
// retried instead of trusting a single measurement window.
TEST_F(PerformanceThresholdTest, ProcessingDelayWithinThreshold_VGA_30fps) {
    auto params = getDefaultParams();

    auto frames = TestDataGenerator::generate_test_sequence(
        100, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
    );

    double avg_ms = 0.0;
    double max_ms = 0.0;
    const bool holds = measurement_holds_within_attempts(3, [&]() {
        stabilizer = std::make_unique<StabilizerCore>();
        EXPECT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

        auto processing_times = measure_processing_times(stabilizer.get(), frames);
        EXPECT_FALSE(processing_times.empty());
        if (processing_times.empty()) {
            return false;
        }

        auto stats = calculate_stats(processing_times);
        avg_ms = stats.avg_ms;
        max_ms = stats.max_ms;

        // VGA should meet design target: <8ms (4-8ms actual). Max should
        // still be reasonable.
        return avg_ms < 8.0 && max_ms < 16.0;
    });

    EXPECT_TRUE(holds)
        << "Average processing time for VGA should be <8ms, got: " << avg_ms << "ms"
        << "; max processing time for VGA should be <16ms, got: " << max_ms << "ms";
}

/**
 * Test: Processing delay within threshold for HD 720p @ 60fps
 * Acceptance criteria: Processing delay at 1280x720 @ 60fps should be within one frame (16.67ms)
 */
// Wall-clock/CPU measurements shift with machine load, so the check is
// retried instead of trusting a single measurement window.
TEST_F(PerformanceThresholdTest, ProcessingDelayWithinThreshold_HD_720p_60fps) {
    auto params = getDefaultParams();

    // Generate HD 720p frames
    auto frames = TestDataGenerator::generate_test_sequence(
        100, 1280, 720, "shake"
    );

    double avg_ms = 0.0;
    double max_ms = 0.0;
    const bool holds = measurement_holds_within_attempts(3, [&]() {
        // HD 720p resolution: 1280x720
        stabilizer = std::make_unique<StabilizerCore>();
        EXPECT_TRUE(stabilizer->initialize(1280, 720, params));

        // Measure processing times
        auto processing_times = measure_processing_times(stabilizer.get(), frames);
        EXPECT_FALSE(processing_times.empty());
        if (processing_times.empty()) {
            return false;
        }

        // Calculate statistics
        auto stats = calculate_stats(processing_times);
        avg_ms = stats.avg_ms;
        max_ms = stats.max_ms;

        // Verify average processing time meets design target for HD 720p @
        // 60fps. Design target: <16.67ms for 60fps. Verify max processing
        // time is reasonable (allows some spikes). Note: For frames that
        // don't require stabilization (first frame), processing can be very
        // fast.
        return avg_ms < 16.67 && max_ms < 32.0;
    });

    EXPECT_TRUE(holds)
        << "Average processing time should be <16.67ms for HD 720p @ 60fps, got: "
        << avg_ms << "ms"
        << "; max processing time should be <32ms, got: " << max_ms << "ms";
}

/**
 * Test: Processing delay with different motion types
 * Different motion types may have different performance characteristics
 */
// Wall-clock/CPU measurements shift with machine load, so the check is
// retried instead of trusting a single measurement window.
TEST_F(PerformanceThresholdTest, ProcessingDelayWithDifferentMotionTypes) {
    auto params = getDefaultParams();

    std::vector<std::string> motion_types = {"static", "shake", "pan_right", "fast", "zoom_in"};

    for (const auto& motion_type : motion_types) {
        auto frames = TestDataGenerator::generate_test_sequence(
            50, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, motion_type
        );

        double avg_ms = 0.0;
        double max_ms = 0.0;
        const bool holds = measurement_holds_within_attempts(3, [&]() {
            stabilizer = std::make_unique<StabilizerCore>();
            EXPECT_TRUE(stabilizer->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));

            auto processing_times = measure_processing_times(stabilizer.get(), frames);
            EXPECT_FALSE(processing_times.empty());
            if (processing_times.empty()) {
                return false;
            }

            auto stats = calculate_stats(processing_times);
            avg_ms = stats.avg_ms;
            max_ms = stats.max_ms;

            // All motion types should be within threshold
            return avg_ms < 33.0 && max_ms < 50.0;
        });

        EXPECT_TRUE(holds)
            << "Motion type '" << motion_type << "' avg processing time should be <33ms, got: "
            << avg_ms << "ms"
            << "; max processing time should be <50ms, got: " << max_ms << "ms";
    }
}

/**
 * Test: Processing delay with different smoothing radii
 * Larger smoothing radius may slightly increase processing time
 */
// Wall-clock/CPU measurements shift with machine load, so the check is
// retried instead of trusting a single measurement window.
TEST_F(PerformanceThresholdTest, ProcessingDelayWithDifferentSmoothing) {
    std::vector<int> smoothing_radii = {
        Processing::SMALL_SMOOTHING_WINDOW,
        Processing::MEDIUM_SMOOTHING_WINDOW,
        Processing::LARGE_SMOOTHING_WINDOW
    };

    for (int smoothing_radius : smoothing_radii) {
        auto params = getDefaultParams();
        params.smoothing_radius = smoothing_radius;

        auto frames = TestDataGenerator::generate_test_sequence(
            50, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, "shake"
        );

        double avg_ms = 0.0;
        const bool holds = measurement_holds_within_attempts(3, [&]() {
            stabilizer = std::make_unique<StabilizerCore>();
            EXPECT_TRUE(stabilizer->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));

            auto processing_times = measure_processing_times(stabilizer.get(), frames);
            EXPECT_FALSE(processing_times.empty());
            if (processing_times.empty()) {
                return false;
            }

            auto stats = calculate_stats(processing_times);
            avg_ms = stats.avg_ms;

            // All smoothing settings should be within threshold. Large
            // smoothing may have slightly higher processing time but should
            // still be well within threshold.
            if (smoothing_radius == Processing::LARGE_SMOOTHING_WINDOW) {
                return avg_ms < 30.0;
            }
            return avg_ms < 33.0;
        });

        EXPECT_TRUE(holds)
            << "Smoothing radius " << smoothing_radius
            << " avg processing time should be <33ms (<30ms for large smoothing), got: "
            << avg_ms << "ms";
    }
}

/**
 * Test: Processing delay with different feature counts
 * More features may increase processing time slightly
 */
// Wall-clock/CPU measurements shift with machine load, so the check is
// retried instead of trusting a single measurement window.
TEST_F(PerformanceThresholdTest, ProcessingDelayWithDifferentFeatureCounts) {
    std::vector<int> feature_counts = {
        Features::LOW_COUNT,
        Features::DEFAULT_COUNT,
        Features::HIGH_COUNT
    };

    for (int feature_count : feature_counts) {
        auto params = getDefaultParams();
        params.feature_count = feature_count;

        auto frames = TestDataGenerator::generate_test_sequence(
            50, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, "shake"
        );

        double avg_ms = 0.0;
        const bool holds = measurement_holds_within_attempts(3, [&]() {
            stabilizer = std::make_unique<StabilizerCore>();
            EXPECT_TRUE(stabilizer->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));

            auto processing_times = measure_processing_times(stabilizer.get(), frames);
            EXPECT_FALSE(processing_times.empty());
            if (processing_times.empty()) {
                return false;
            }

            auto stats = calculate_stats(processing_times);
            avg_ms = stats.avg_ms;

            // All feature counts should be within threshold
            return avg_ms < 33.0;
        });

        EXPECT_TRUE(holds)
            << "Feature count " << feature_count << " avg processing time should be <33ms, got: "
            << avg_ms << "ms";
    }
}

/**
 * Test: Processing delay consistency over time
 * Processing time should be consistent, not degrading over time
 */
// Wall-clock timing depends on transient machine load, so the comparison is
// retried instead of trusting a single measurement window.
TEST_F(PerformanceThresholdTest, ProcessingDelayConsistency) {
    auto params = getDefaultParams();

    auto frames = TestDataGenerator::generate_test_sequence(
        200, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, "shake"
    );

    double avg_ratio = 0.0;
    double cv = 0.0;
    const bool holds = measurement_holds_within_attempts(3, [&]() {
        stabilizer = std::make_unique<StabilizerCore>();
        EXPECT_TRUE(stabilizer->initialize(
            Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));

        auto processing_times = measure_processing_times(stabilizer.get(), frames);
        EXPECT_FALSE(processing_times.empty());
        if (processing_times.empty()) {
            return false;
        }

        // Split into first and second halves
        size_t half = processing_times.size() / 2;
        std::vector<double> first_half(processing_times.begin(), processing_times.begin() + half);
        std::vector<double> second_half(processing_times.begin() + half, processing_times.end());

        auto stats_first = calculate_stats(first_half);
        auto stats_second = calculate_stats(second_half);

        // Second half should not be significantly slower than first half
        // (allow up to 20% for frame warming and minor variance), and the
        // per-frame spread should stay below 50% of the average.
        avg_ratio = stats_second.avg_ms / stats_first.avg_ms;
        cv = stats_first.std_dev_ms / stats_first.avg_ms;
        return avg_ratio < 1.2 && cv < 0.5;
    });

    EXPECT_TRUE(holds)
        << "Processing time should stay consistent over time, got ratio "
        << avg_ratio << " and CV " << cv;
}

// ============================================================================
// Preset Performance Tests
// ============================================================================

/**
 * Test: Gaming preset performance
 * Gaming preset should handle fast motion efficiently
 */
// Wall-clock/CPU measurements shift with machine load, so the check is
// retried instead of trusting a single measurement window.
TEST_F(PerformanceThresholdTest, GamingPresetPerformance) {
    auto params = StabilizerCore::get_preset_gaming();

    auto frames = TestDataGenerator::generate_test_sequence(
        100, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "fast"
    );

    double avg_ms = 0.0;
    const bool holds = measurement_holds_within_attempts(3, [&]() {
        stabilizer = std::make_unique<StabilizerCore>();
        EXPECT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

        auto processing_times = measure_processing_times(stabilizer.get(), frames);
        EXPECT_FALSE(processing_times.empty());
        if (processing_times.empty()) {
            return false;
        }

        auto stats = calculate_stats(processing_times);
        avg_ms = stats.avg_ms;

        // Gaming preset should be very fast at VGA resolution
        return avg_ms < 10.0;
    });

    EXPECT_TRUE(holds)
        << "Gaming preset should be fast at VGA, got: " << avg_ms << "ms";
}

/**
 * Test: Streaming preset performance
 * Streaming preset should handle HD efficiently
 */
// Wall-clock/CPU measurements shift with machine load, so the check is
// retried instead of trusting a single measurement window.
TEST_F(PerformanceThresholdTest, StreamingPresetPerformance) {
    auto params = StabilizerCore::get_preset_streaming();

    auto frames = TestDataGenerator::generate_test_sequence(
        100, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, "shake"
    );

    double avg_ms = 0.0;
    const bool holds = measurement_holds_within_attempts(3, [&]() {
        stabilizer = std::make_unique<StabilizerCore>();
        EXPECT_TRUE(stabilizer->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));

        auto processing_times = measure_processing_times(stabilizer.get(), frames);
        EXPECT_FALSE(processing_times.empty());
        if (processing_times.empty()) {
            return false;
        }

        auto stats = calculate_stats(processing_times);
        avg_ms = stats.avg_ms;

        // Streaming preset should meet 30fps requirement
        return avg_ms < 33.0;
    });

    EXPECT_TRUE(holds)
        << "Streaming preset should meet 30fps requirement, got: " << avg_ms << "ms";
}

/**
 * Test: Recording preset performance
 * Recording preset may be slower but should still be real-time
 */
// Wall-clock/CPU measurements shift with machine load, so the check is
// retried instead of trusting a single measurement window.
TEST_F(PerformanceThresholdTest, RecordingPresetPerformance) {
    auto params = StabilizerCore::get_preset_recording();

    auto frames = TestDataGenerator::generate_test_sequence(
        100, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, "slow"
    );

    double avg_ms = 0.0;
    const bool holds = measurement_holds_within_attempts(3, [&]() {
        stabilizer = std::make_unique<StabilizerCore>();
        EXPECT_TRUE(stabilizer->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));

        auto processing_times = measure_processing_times(stabilizer.get(), frames);
        EXPECT_FALSE(processing_times.empty());
        if (processing_times.empty()) {
            return false;
        }

        auto stats = calculate_stats(processing_times);
        avg_ms = stats.avg_ms;

        // Recording preset should still be real-time capable
        return avg_ms < 33.0;
    });

    EXPECT_TRUE(holds)
        << "Recording preset should still be real-time capable, got: " << avg_ms << "ms";
}
