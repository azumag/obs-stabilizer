/**
 * Multi-Source Concurrency Tests
 *
 * This file contains tests to verify that multiple simultaneous stabilizer
 * instances (representing multiple video sources) can operate without
 * crashes, deadlocks, or race conditions.
 *
 * Critical acceptance criteria:
 * - 複数のビデオソースにフィルターを適用してもOBSがクラッシュしない
 */

#include <gtest/gtest.h>
#include "../src/core/stabilizer_core.hpp"
#include "../src/core/stabilizer_wrapper.hpp"
#include "test_constants.hpp"
#include "test_data_generator.hpp"
#include <vector>
#include <memory>
#include <thread>
#include <random>
#include <chrono>

using namespace TestConstants;

// ============================================================================
// Multi-Source Concurrency Test Class
// ============================================================================

class MultiSourceConcurrencyTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}

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
     * Process frames with a single stabilizer instance
     * Returns true if all frames processed successfully
     */
    bool process_single_source(
        StabilizerCore* stabilizer,
        const std::vector<cv::Mat>& frames
    ) {
        for (const auto& frame : frames) {
            cv::Mat result = stabilizer->process_frame(frame);
            if (result.empty()) {
                return false;
            }
        }
        return true;
    }

    /**
     * Process frames with multiple stabilizer instances concurrently
     * Returns true if all instances processed successfully
     */
    bool process_multiple_sources_concurrently(
        std::vector<StabilizerCore*>& stabilizers,
        const std::vector<std::vector<cv::Mat>>& frame_sets
    ) {
        if (stabilizers.size() != frame_sets.size()) {
            return false;
        }

        // Process each source in parallel
        std::vector<std::thread> threads;
        std::vector<bool> results(stabilizers.size(), false);

        for (size_t i = 0; i < stabilizers.size(); i++) {
            threads.emplace_back([&, i]() {
                results[i] = process_single_source(stabilizers[i], frame_sets[i]);
            });
        }

        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }

        // Check all results
        return std::all_of(results.begin(), results.end(), [](bool r) { return r; });
    }
};

// ============================================================================
// Basic Multi-Source Tests
// ============================================================================

/**
 * Test: Two simultaneous stabilizer instances
 * Verifies that OBS can handle two video sources with stabilizer filters
 * without crashing or deadlocking
 */
TEST_F(MultiSourceConcurrencyTest, TwoSimultaneousSources) {
    // Create two stabilizer instances
    std::unique_ptr<StabilizerCore> stabilizer1 = std::make_unique<StabilizerCore>();
    std::unique_ptr<StabilizerCore> stabilizer2 = std::make_unique<StabilizerCore>();

    auto params = getDefaultParams();

    // Initialize both instances
    ASSERT_TRUE(stabilizer1->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    ASSERT_TRUE(stabilizer2->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Generate frame sequences for each source
    auto frames1 = TestDataGenerator::generate_test_sequence(
        50, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
    );
    auto frames2 = TestDataGenerator::generate_test_sequence(
        50, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "pan_right"
    );

    // Process both sources
    std::vector<StabilizerCore*> stabilizers = { stabilizer1.get(), stabilizer2.get() };
    std::vector<std::vector<cv::Mat>> frame_sets = { frames1, frames2 };

    bool success = process_multiple_sources_concurrently(stabilizers, frame_sets);
    EXPECT_TRUE(success) << "Two simultaneous sources should process successfully without crashes";

    // Verify both processed all frames
    auto metrics1 = stabilizer1->get_performance_metrics();
    auto metrics2 = stabilizer2->get_performance_metrics();
    EXPECT_EQ(metrics1.frame_count, 50);
    EXPECT_EQ(metrics2.frame_count, 50);
}

/**
 * Test: Five simultaneous stabilizer instances
 * Tests with a moderate number of simultaneous sources
 */
TEST_F(MultiSourceConcurrencyTest, FiveSimultaneousSources) {
    std::vector<std::unique_ptr<StabilizerCore>> stabilizers;
    std::vector<std::vector<cv::Mat>> frame_sets;
    auto params = getDefaultParams();

    // Create five stabilizer instances
    for (int i = 0; i < 5; i++) {
        auto stab = std::make_unique<StabilizerCore>();
        ASSERT_TRUE(stab->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
        stabilizers.push_back(std::move(stab));

        // Generate unique frame sequence for each source
        std::string motion_type = (i % 2 == 0) ? "shake" : "pan_right";
        auto frames = TestDataGenerator::generate_test_sequence(
            50, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, motion_type
        );
        frame_sets.push_back(frames);
    }

    // Convert to pointers for threading
    std::vector<StabilizerCore*> stabilizer_ptrs;
    for (const auto& stab : stabilizers) {
        stabilizer_ptrs.push_back(stab.get());
    }

    // Process all sources concurrently
    bool success = process_multiple_sources_concurrently(stabilizer_ptrs, frame_sets);
    EXPECT_TRUE(success) << "Five simultaneous sources should process successfully";

    // Verify all processed correctly
    for (const auto& stab : stabilizers) {
        auto metrics = stab->get_performance_metrics();
        EXPECT_EQ(metrics.frame_count, 50);
    }
}

/**
 * Test: Ten simultaneous stabilizer instances
 * Tests with a larger number of simultaneous sources (stress test)
 */
TEST_F(MultiSourceConcurrencyTest, TenSimultaneousSources) {
    std::vector<std::unique_ptr<StabilizerCore>> stabilizers;
    std::vector<std::vector<cv::Mat>> frame_sets;
    auto params = getDefaultParams();

    // Create ten stabilizer instances
    for (int i = 0; i < 10; i++) {
        auto stab = std::make_unique<StabilizerCore>();
        ASSERT_TRUE(stab->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
        stabilizers.push_back(std::move(stab));

        // Generate frame sequence
        std::vector<std::string> motion_types = {"static", "shake", "pan_right", "pan_left", "zoom_in"};
        std::string motion_type = motion_types[i % motion_types.size()];
        auto frames = TestDataGenerator::generate_test_sequence(
            30, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, motion_type
        );
        frame_sets.push_back(frames);
    }

    // Convert to pointers
    std::vector<StabilizerCore*> stabilizer_ptrs;
    for (const auto& stab : stabilizers) {
        stabilizer_ptrs.push_back(stab.get());
    }

    // Process all sources concurrently
    bool success = process_multiple_sources_concurrently(stabilizer_ptrs, frame_sets);
    EXPECT_TRUE(success) << "Ten simultaneous sources should process successfully without crashes";

    // Verify all processed correctly
    for (const auto& stab : stabilizers) {
        auto metrics = stab->get_performance_metrics();
        EXPECT_EQ(metrics.frame_count, 30);
    }
}

// ============================================================================
// Mixed Resolution Tests
// ============================================================================

/**
 * Test: Multiple sources with different resolutions
 * Verifies that sources with different resolutions can coexist
 */
TEST_F(MultiSourceConcurrencyTest, MixedResolutionSources) {
    std::vector<std::unique_ptr<StabilizerCore>> stabilizers;
    std::vector<std::vector<cv::Mat>> frame_sets;

    // Create sources with different resolutions
    std::vector<std::pair<int, int>> resolutions = {
        {Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT},
        {Resolution::HD_WIDTH, Resolution::HD_HEIGHT},
        {1280, 720},  // 720p
        {640, 360}    // 360p
    };

    for (const auto& [width, height] : resolutions) {
        auto stab = std::make_unique<StabilizerCore>();
        auto params = getDefaultParams();
        ASSERT_TRUE(stab->initialize(width, height, params));
        stabilizers.push_back(std::move(stab));

        auto frames = TestDataGenerator::generate_test_sequence(
            30, width, height, "shake"
        );
        frame_sets.push_back(frames);
    }

    // Convert to pointers
    std::vector<StabilizerCore*> stabilizer_ptrs;
    for (const auto& stab : stabilizers) {
        stabilizer_ptrs.push_back(stab.get());
    }

    // Process all sources concurrently
    bool success = process_multiple_sources_concurrently(stabilizer_ptrs, frame_sets);
    EXPECT_TRUE(success) << "Mixed resolution sources should process successfully";

    // Verify all processed correctly
    for (const auto& stab : stabilizers) {
        auto metrics = stab->get_performance_metrics();
        EXPECT_EQ(metrics.frame_count, 30);
    }
}

// ============================================================================
// Parameter Variation Tests
// ============================================================================

/**
 * Test: Multiple sources with different parameters
 * Verifies that sources with different stabilization parameters can coexist
 */
TEST_F(MultiSourceConcurrencyTest, DifferentParametersPerSource) {
    std::vector<std::unique_ptr<StabilizerCore>> stabilizers;
    std::vector<std::vector<cv::Mat>> frame_sets;

    // Create sources with different parameters
    std::vector<StabilizerCore::StabilizerParams> param_sets = {
        StabilizerCore::get_preset_gaming(),
        StabilizerCore::get_preset_streaming(),
        StabilizerCore::get_preset_recording(),
        getDefaultParams()
    };

    for (const auto& params : param_sets) {
        auto stab = std::make_unique<StabilizerCore>();
        ASSERT_TRUE(stab->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
        stabilizers.push_back(std::move(stab));

        auto frames = TestDataGenerator::generate_test_sequence(
            30, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
        );
        frame_sets.push_back(frames);
    }

    // Convert to pointers
    std::vector<StabilizerCore*> stabilizer_ptrs;
    for (const auto& stab : stabilizers) {
        stabilizer_ptrs.push_back(stab.get());
    }

    // Process all sources concurrently
    bool success = process_multiple_sources_concurrently(stabilizer_ptrs, frame_sets);
    EXPECT_TRUE(success) << "Sources with different parameters should process successfully";
}

// ============================================================================
// Dynamic Add/Remove Tests
// ============================================================================

/**
 * Test: Add and remove sources dynamically
 * Simulates adding/removing video sources in OBS
 */
TEST_F(MultiSourceConcurrencyTest, DynamicAddRemoveSources) {
    std::vector<std::unique_ptr<StabilizerCore>> stabilizers;
    std::vector<std::vector<cv::Mat>> frame_sets;
    auto params = getDefaultParams();

    // Start with 3 sources
    for (int i = 0; i < 3; i++) {
        auto stab = std::make_unique<StabilizerCore>();
        ASSERT_TRUE(stab->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
        stabilizers.push_back(std::move(stab));

        auto frames = TestDataGenerator::generate_test_sequence(
            20, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
        );
        frame_sets.push_back(frames);
    }

    // Convert to pointers
    std::vector<StabilizerCore*> stabilizer_ptrs;
    for (const auto& stab : stabilizers) {
        stabilizer_ptrs.push_back(stab.get());
    }

    // Process initial sources
    bool success = process_multiple_sources_concurrently(stabilizer_ptrs, frame_sets);
    EXPECT_TRUE(success) << "Initial sources should process successfully";

    // Add 2 more sources
    for (int i = 0; i < 2; i++) {
        auto stab = std::make_unique<StabilizerCore>();
        ASSERT_TRUE(stab->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
        stabilizers.push_back(std::move(stab));

        auto frames = TestDataGenerator::generate_test_sequence(
            20, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
        );
        frame_sets.push_back(frames);
    }

    // Process all 5 sources
    stabilizer_ptrs.clear();
    for (const auto& stab : stabilizers) {
        stabilizer_ptrs.push_back(stab.get());
    }
    success = process_multiple_sources_concurrently(stabilizer_ptrs, frame_sets);
    EXPECT_TRUE(success) << "5 sources after adding should process successfully";

    // Remove 2 sources (delete them)
    stabilizers.pop_back();
    stabilizers.pop_back();
    frame_sets.pop_back();
    frame_sets.pop_back();

    // Process remaining 3 sources
    stabilizer_ptrs.clear();
    for (const auto& stab : stabilizers) {
        stabilizer_ptrs.push_back(stab.get());
    }
    success = process_multiple_sources_concurrently(stabilizer_ptrs, frame_sets);
    EXPECT_TRUE(success) << "3 sources after removing should process successfully";
}

// ============================================================================
// Memory and Resource Tests
// ============================================================================

/**
 * Test: Memory usage with multiple simultaneous sources
 * Verifies that memory usage scales reasonably with number of sources
 */
TEST_F(MultiSourceConcurrencyTest, MemoryUsageWithMultipleSources) {
    // Platform-specific memory tracking
#if defined(__linux__) || defined(__APPLE__)
    #include <sys/resource.h>
    size_t get_memory_usage() {
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
#if defined(__APPLE__)
            return usage.ru_maxrss / 1024;  // bytes to KB
#else
            return usage.ru_maxrss;
#endif
        }
        return 0;
    }
#elif defined(_WIN32)
    #include <windows.h>
    #include <psapi.h>
    size_t get_memory_usage() {
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            return pmc.WorkingSetSize / 1024;
        }
        return 0;
    }
#else
    size_t get_memory_usage() { return 0; }
#endif

    size_t initial_memory = get_memory_usage();

    // Create multiple sources
    std::vector<std::unique_ptr<StabilizerCore>> stabilizers;
    auto params = getDefaultParams();

    for (int i = 0; i < 5; i++) {
        auto stab = std::make_unique<StabilizerCore>();
        ASSERT_TRUE(stab->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));
        stabilizers.push_back(std::move(stab));
    }

    // Process frames with each source
    for (auto& stab : stabilizers) {
        auto frames = TestDataGenerator::generate_test_sequence(
            50, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, "static"
        );
        for (const auto& frame : frames) {
            cv::Mat result = stab->process_frame(frame);
            ASSERT_FALSE(result.empty());
        }
    }

    size_t final_memory = get_memory_usage();
    size_t memory_increase = final_memory - initial_memory;

    // Memory increase should be reasonable (less than 2GB for 5 HD sources)
    // Note: This is a generous threshold accounting for OpenCV buffers
    EXPECT_LT(memory_increase, 2 * 1024 * 1024)
        << "Memory increase for 5 HD sources should be reasonable";
}

// ============================================================================
// Stability and Stress Tests
// ============================================================================

/**
 * Test: Rapid start/stop of multiple sources
 * Tests stability when sources are rapidly added and removed
 */
TEST_F(MultiSourceConcurrencyTest, RapidStartStopMultipleSources) {
    auto params = getDefaultParams();

    // Perform multiple cycles of adding and removing sources
    for (int cycle = 0; cycle < 5; cycle++) {
        std::vector<std::unique_ptr<StabilizerCore>> stabilizers;

        // Add 5 sources
        for (int i = 0; i < 5; i++) {
            auto stab = std::make_unique<StabilizerCore>();
            ASSERT_TRUE(stab->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
            stabilizers.push_back(std::move(stab));

            // Process a few frames
            auto frames = TestDataGenerator::generate_test_sequence(
                10, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
            );
            for (const auto& frame : frames) {
                cv::Mat result = stab->process_frame(frame);
                ASSERT_FALSE(result.empty());
            }
        }

        // Remove all sources (they go out of scope and are destroyed)
    }

    // If we reach here without crashing, the test passes
    SUCCEED() << "Rapid start/stop of multiple sources completed without crashes";
}

/**
 * Test: Long-running multiple sources
 * Tests that multiple sources can run for extended periods without issues
 */
TEST_F(MultiSourceConcurrencyTest, LongRunningMultipleSources) {
    std::vector<std::unique_ptr<StabilizerCore>> stabilizers;
    std::vector<std::vector<cv::Mat>> frame_sets;
    auto params = getDefaultParams();

    // Create 3 sources
    for (int i = 0; i < 3; i++) {
        auto stab = std::make_unique<StabilizerCore>();
        ASSERT_TRUE(stab->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
        stabilizers.push_back(std::move(stab));

        // Generate longer sequence
        auto frames = TestDataGenerator::generate_test_sequence(
            200, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
        );
        frame_sets.push_back(frames);
    }

    // Convert to pointers
    std::vector<StabilizerCore*> stabilizer_ptrs;
    for (const auto& stab : stabilizers) {
        stabilizer_ptrs.push_back(stab.get());
    }

    // Process all sources concurrently
    bool success = process_multiple_sources_concurrently(stabilizer_ptrs, frame_sets);
    EXPECT_TRUE(success) << "Long-running multiple sources should process successfully";

    // Verify all processed correctly
    for (const auto& stab : stabilizers) {
        auto metrics = stab->get_performance_metrics();
        EXPECT_EQ(metrics.frame_count, 200);
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

/**
 * Test: One source with issues doesn't affect others
 * Verifies that a problematic source doesn't crash the entire system
 */
TEST_F(MultiSourceConcurrencyTest, IsolatedSourceFailure) {
    std::unique_ptr<StabilizerCore> stabilizer1 = std::make_unique<StabilizerCore>();
    std::unique_ptr<StabilizerCore> stabilizer2 = std::make_unique<StabilizerCore>();
    std::unique_ptr<StabilizerCore> stabilizer3 = std::make_unique<StabilizerCore>();

    auto params = getDefaultParams();

    // Initialize all three
    ASSERT_TRUE(stabilizer1->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    ASSERT_TRUE(stabilizer2->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
    ASSERT_TRUE(stabilizer3->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Generate frames for each
    auto frames1 = TestDataGenerator::generate_test_sequence(
        20, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );
    auto frames2 = TestDataGenerator::generate_test_sequence(
        20, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
    );
    auto frames3 = TestDataGenerator::generate_test_sequence(
        20, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "pan_right"
    );

    // Process normally with sources 1 and 2
    for (const auto& frame : frames1) {
        cv::Mat result = stabilizer1->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    for (const auto& frame : frames2) {
        cv::Mat result = stabilizer2->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    // Introduce empty frames to source 3 (simulating problematic input)
    for (size_t i = 0; i < frames3.size(); i++) {
        cv::Mat frame = (i == 10) ? cv::Mat() : frames3[i];  // One empty frame
        cv::Mat result = stabilizer3->process_frame(frame);
        // Empty frame should produce empty result (not crash)
    }

    // Continue processing sources 1 and 2 (should still work)
    for (const auto& frame : frames1) {
        cv::Mat result = stabilizer1->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    for (const auto& frame : frames2) {
        cv::Mat result = stabilizer2->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    // Sources 1 and 2 should have processed all frames
    auto metrics1 = stabilizer1->get_performance_metrics();
    auto metrics2 = stabilizer2->get_performance_metrics();
    EXPECT_EQ(metrics1.frame_count, 40);  // 2 batches of 20
    EXPECT_EQ(metrics2.frame_count, 40);
}
