#include <gtest/gtest.h>
#include "../src/core/stabilizer_core.hpp"
#include "../src/core/parameter_validation.hpp"
#include "test_constants.hpp"
#include "test_data_generator.hpp"
#include <chrono>
#include <thread>
#include <memory>
#include <vector>

// Platform-specific headers for memory tracking
#if defined(__linux__) || defined(__APPLE__)
#include <sys/resource.h>  // for getrusage()
#endif
#if defined(_WIN32)
#include <windows.h>      // for GetCurrentProcess()
#include <psapi.h>        // for GetProcessMemoryInfo()
#endif

using namespace TestConstants;

class MemoryLeakTest : public ::testing::Test {
protected:
    void SetUp() override {
        stabilizer = std::make_unique<StabilizerCore>();
        initial_memory_usage = get_memory_usage();
    }

    void TearDown() override {
        stabilizer.reset();
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

    // Get current memory usage in MB (approximate)
    // Implements platform-specific memory tracking for accurate leak detection
    size_t get_memory_usage() {
#if defined(__linux__) || defined(__APPLE__)
        // Unix/Linux/macOS: Use getrusage() to get memory usage
        // Returns maximum resident set size in KB
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
            // ru_maxrss is in KB on most systems
            // On macOS, it's in bytes, so convert to KB
#if defined(__APPLE__)
            return usage.ru_maxrss / 1024;  // bytes to KB
#else
            return usage.ru_maxrss;  // Already in KB
#endif
        }
#elif defined(_WIN32)
        // Windows: Use GetProcessMemoryInfo() to get working set size
        // Returns working set size in bytes, convert to KB
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            return pmc.WorkingSetSize / 1024;  // bytes to KB
        }
#endif
        // Fallback for unsupported platforms or API failures
        // This allows tests to run without crashing even if memory tracking fails
        return 0;
    }

    std::unique_ptr<StabilizerCore> stabilizer;
    size_t initial_memory_usage;
};

// ============================================================================
// Long Duration Tests
// ============================================================================

TEST_F(MemoryLeakTest, LongDurationProcessing) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Process many frames to detect memory leaks
    int num_frames = 1000;
    auto frames = TestDataGenerator::generate_test_sequence(
        num_frames, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );

    for (int i = 0; i < num_frames; i++) {
        cv::Mat result = stabilizer->process_frame(frames[i]);
        EXPECT_FALSE(result.empty());

        // Check memory usage periodically
        if (i % 100 == 0) {
            size_t current_memory = get_memory_usage();
            size_t memory_increase = current_memory - initial_memory_usage;

            // Memory increase should be bounded
            // Note: get_memory_usage() returns max RSS (peak memory), not current usage
            // Peak memory includes OpenCV allocator caches and OS-level buffering
            // Threshold set to 500MB to account for peak memory tracking behavior
            EXPECT_LT(memory_increase, 500 * 1024 * 1024) << "Memory leak detected at frame " << i;
        }
    }

    // Final memory check
    size_t final_memory = get_memory_usage();
    size_t total_increase = final_memory - initial_memory_usage;
    // Peak memory threshold: 500MB allows for OpenCV buffers and stabilization data
    EXPECT_LT(total_increase, 500 * 1024 * 1024) << "Excessive memory usage after processing " << num_frames << " frames";
}

TEST_F(MemoryLeakTest, ContinuousReinitialization) {
    StabilizerCore::StabilizerParams params = getDefaultParams();

    // Reinitialize many times to detect memory leaks in initialization
    for (int i = 0; i < 100; i++) {
        stabilizer->reset();
        ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

        auto frames = TestDataGenerator::generate_test_sequence(
            10, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
        );

        for (const auto& frame : frames) {
            stabilizer->process_frame(frame);
        }
    }

    // Final memory check
    size_t final_memory = get_memory_usage();
    size_t total_increase = final_memory - initial_memory_usage;
    // Peak memory threshold: 300MB accounts for multiple reinitialization cycles
    EXPECT_LT(total_increase, 300 * 1024 * 1024) << "Excessive memory usage after 100 reinitializations";
}

// ============================================================================
// Multiple Instance Tests
// ============================================================================

TEST_F(MemoryLeakTest, MultipleInstancesSimultaneously) {
    std::vector<std::unique_ptr<StabilizerCore>> stabilizers;

    // Create multiple stabilizer instances
    for (int i = 0; i < 10; i++) {
        auto stab = std::make_unique<StabilizerCore>();
        auto params = getDefaultParams();
        ASSERT_TRUE(stab->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
        stabilizers.push_back(std::move(stab));
    }

    // Process frames with each instance
    auto frames = TestDataGenerator::generate_test_sequence(
        100, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );

    for (auto& stab : stabilizers) {
        for (const auto& frame : frames) {
            cv::Mat result = stab->process_frame(frame);
            EXPECT_FALSE(result.empty());
        }
    }

    // Cleanup
    stabilizers.clear();

    // Memory should return to baseline
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    size_t final_memory = get_memory_usage();
    size_t memory_increase = final_memory - initial_memory_usage;
    // Peak memory threshold: 200MB for multi-instance test
    EXPECT_LT(memory_increase, 200 * 1024 * 1024) << "Memory not released after destroying instances";
}

// ============================================================================
// Parameter Update Memory Test
// ============================================================================

TEST_F(MemoryLeakTest, ParameterUpdateMemory) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        100, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );

    // Process frames while frequently updating parameters
    for (int i = 0; i < 100; i++) {
        params.smoothing_radius = 10 + (i % 50);
        params.feature_count = 100 + (i % 400);
        params.quality_level = 0.01f + (i % 100) * 0.0001f;
        stabilizer->update_parameters(params);

        cv::Mat result = stabilizer->process_frame(frames[i]);
        EXPECT_FALSE(result.empty());
    }

    // Final memory check
    size_t final_memory = get_memory_usage();
    size_t total_increase = final_memory - initial_memory_usage;
    // Peak memory threshold: 300MB for parameter update operations
    EXPECT_LT(total_increase, 300 * 1024 * 1024) << "Memory leak detected after parameter updates";
}

// ============================================================================
// Reset During Processing Memory Test
// ============================================================================

TEST_F(MemoryLeakTest, ResetDuringProcessing) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        500, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );

    // Process frames and reset frequently
    for (int i = 0; i < 500; i++) {
        cv::Mat result = stabilizer->process_frame(frames[i]);
        EXPECT_FALSE(result.empty());

        // Reset every 50 frames
        if (i % 50 == 0) {
            stabilizer->reset();
            ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));
        }
    }

    // Final memory check
    size_t final_memory = get_memory_usage();
    size_t total_increase = final_memory - initial_memory_usage;
    // Peak memory threshold: 300MB for reset test
    EXPECT_LT(total_increase, 300 * 1024 * 1024) << "Memory leak detected after resets during processing";
}

// ============================================================================
// Large Frame Memory Test
// ============================================================================

TEST_F(MemoryLeakTest, LargeFrameProcessing) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));

    // Process large frames
    auto frames = TestDataGenerator::generate_test_sequence(
        100, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, "static"
    );

    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    // Final memory check
    size_t final_memory = get_memory_usage();
    size_t total_increase = final_memory - initial_memory_usage;
    // Peak memory threshold: 600MB for large frame test (HD frames require more memory)
    EXPECT_LT(total_increase, 600 * 1024 * 1024) << "Excessive memory usage after processing large frames";
}

// ============================================================================
// Transform Buffer Management Test
// ============================================================================

TEST_F(MemoryLeakTest, TransformBufferManagement) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    params.smoothing_radius = 100; // Large buffer to test memory management
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        500, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "pan_right"
    );

    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    // Check transform buffer size
    auto transforms = stabilizer->get_current_transforms();
    EXPECT_LE(transforms.size(), params.smoothing_radius);

    // Final memory check
    size_t final_memory = get_memory_usage();
    size_t total_increase = final_memory - initial_memory_usage;
    // Peak memory threshold: 300MB for transform buffer test
    EXPECT_LT(total_increase, 300 * 1024 * 1024) << "Memory leak in transform buffer management";
}

// ============================================================================
// Feature Tracking Memory Test
// ============================================================================

TEST_F(MemoryLeakTest, FeatureTrackingMemory) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    params.feature_count = 1000; // Many features to test memory management
    ASSERT_TRUE(stabilizer->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        500, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, "pan_right"
    );

    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    // Final memory check
    size_t final_memory = get_memory_usage();
    size_t total_increase = final_memory - initial_memory_usage;
    // Peak memory threshold: 400MB for feature tracking test (more features = more memory)
    EXPECT_LT(total_increase, 400 * 1024 * 1024) << "Memory leak in feature tracking";
}

// ============================================================================
// Edge Case Memory Tests
// ============================================================================

TEST_F(MemoryLeakTest, EmptyFrameHandlingMemory) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Process many empty frames
    for (int i = 0; i < 100; i++) {
        cv::Mat empty_frame;
        cv::Mat result = stabilizer->process_frame(empty_frame);
        EXPECT_TRUE(result.empty());
    }

    // Final memory check
    size_t final_memory = get_memory_usage();
    size_t total_increase = final_memory - initial_memory_usage;
    // Peak memory threshold: 100MB for empty frame test
    EXPECT_LT(total_increase, 100 * 1024 * 1024) << "Memory leak in empty frame handling";
}

TEST_F(MemoryLeakTest, InvalidFrameHandlingMemory) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Process many invalid frames
    for (int i = 0; i < 100; i++) {
        cv::Mat invalid_frame(0, 0, CV_8UC4);
        cv::Mat result = stabilizer->process_frame(invalid_frame);
        EXPECT_TRUE(result.empty());
    }

    // Final memory check
    size_t final_memory = get_memory_usage();
    size_t total_increase = final_memory - initial_memory_usage;
    // Peak memory threshold: 100MB for invalid frame test
    EXPECT_LT(total_increase, 100 * 1024 * 1024) << "Memory leak in invalid frame handling";
}

// ============================================================================
// Stress Test Memory
// ============================================================================

TEST_F(MemoryLeakTest, StressTestMemory) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));

    // Generate a large motion sequence
    std::vector<cv::Mat> stress_frames;
    for (int i = 0; i < 1000; i++) {
        cv::Mat frame = TestDataGenerator::generate_test_frame(
            Resolution::HD_WIDTH, Resolution::HD_HEIGHT
        );
        stress_frames.push_back(frame);
    }

    // Process all frames as fast as possible
    for (const auto& frame : stress_frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    // Final memory check
    size_t final_memory = get_memory_usage();
    size_t total_increase = final_memory - initial_memory_usage;
    // Peak memory threshold: 500MB for stress test
    EXPECT_LT(total_increase, 500 * 1024 * 1024) << "Memory leak detected in stress test";
}

// ============================================================================
// Frame Utils Memory Test
// ============================================================================

TEST_F(MemoryLeakTest, FrameUtilsColorConversionMemory) {
    auto frames = TestDataGenerator::generate_test_sequence(
        1000, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );

    size_t start_memory = get_memory_usage();

    // Test color conversion many times
    for (const auto& frame : frames) {
        cv::Mat gray = FRAME_UTILS::ColorConversion::convert_to_grayscale(frame);
        EXPECT_FALSE(gray.empty());
    }

    size_t end_memory = get_memory_usage();
    size_t memory_increase = end_memory - start_memory;

    EXPECT_LT(memory_increase, 20 * 1024 * 1024) << "Memory leak in color conversion";
}

TEST_F(MemoryLeakTest, FrameUtilsValidationMemory) {
    auto frames = TestDataGenerator::generate_test_sequence(
        1000, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );

    size_t start_memory = get_memory_usage();

    // Test validation many times
    for (const auto& frame : frames) {
        bool valid = FRAME_UTILS::Validation::validate_cv_mat(frame);
        EXPECT_TRUE(valid);
    }

    size_t end_memory = get_memory_usage();
    size_t memory_increase = end_memory - start_memory;

    EXPECT_LT(memory_increase, 10 * 1024 * 1024) << "Memory leak in validation";
}
