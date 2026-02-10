#include <gtest/gtest.h>
#include "../src/core/stabilizer_core.hpp"
#include "../src/core/parameter_validation.hpp"
#include "test_constants.hpp"
#include "test_data_generator.hpp"

using namespace TestConstants;

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        stabilizer = std::make_unique<StabilizerCore>();
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

    std::unique_ptr<StabilizerCore> stabilizer;
};

// ============================================================================
// Sequential Frame Processing Tests
// ============================================================================

TEST_F(IntegrationTest, ProcessLongSequence) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        100, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, "static"
    );

    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
        EXPECT_EQ(result.size(), frame.size());
    }

    // Check performance metrics
    auto metrics = stabilizer->get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 100);
    EXPECT_GT(metrics.avg_processing_time, 0.0);
}

TEST_F(IntegrationTest, ContinuousMotionSequence) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        FrameCount::STANDARD_SEQUENCE,
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT,
        "pan_right"
    );

    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    auto metrics = stabilizer->get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, FrameCount::STANDARD_SEQUENCE);
}

// ============================================================================
// Resolution Change Tests
// ============================================================================

TEST_F(IntegrationTest, ResolutionChangeDuringStream) {
    StabilizerCore::StabilizerParams params = getDefaultParams();

    // Start with VGA resolution
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto vga_frames = TestDataGenerator::generate_test_sequence(
        10, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );

    for (const auto& frame : vga_frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    // Change to HD resolution (requires reinitialization)
    stabilizer->reset();
    ASSERT_TRUE(stabilizer->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));

    auto hd_frames = TestDataGenerator::generate_test_sequence(
        10, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, "static"
    );

    for (const auto& frame : hd_frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    auto metrics = stabilizer->get_performance_metrics();
    EXPECT_GT(metrics.frame_count, 0);
}

TEST_F(IntegrationTest, MultipleReinitializations) {
    StabilizerCore::StabilizerParams params = getDefaultParams();

    for (int i = 0; i < 5; i++) {
        stabilizer->reset();
        ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

        auto frames = TestDataGenerator::generate_test_sequence(
            10, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
        );

        for (const auto& frame : frames) {
            cv::Mat result = stabilizer->process_frame(frame);
            EXPECT_FALSE(result.empty());
        }
    }

    auto metrics = stabilizer->get_performance_metrics();
    EXPECT_GT(metrics.frame_count, 0);
}

// ============================================================================
// Parameter Update During Processing Tests
// ============================================================================

TEST_F(IntegrationTest, UpdateParametersDuringStream) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        30, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );

    // Process first 10 frames with default params
    for (int i = 0; i < 10; i++) {
        stabilizer->process_frame(frames[i]);
    }

    // Update parameters
    params.smoothing_radius = 50;
    params.feature_count = 300;
    stabilizer->update_parameters(params);

    // Process next 10 frames with updated params
    for (int i = 10; i < 20; i++) {
        cv::Mat result = stabilizer->process_frame(frames[i]);
        EXPECT_FALSE(result.empty());
    }

    // Update parameters again
    params.smoothing_radius = 20;
    params.feature_count = 100;
    stabilizer->update_parameters(params);

    // Process remaining frames
    for (int i = 20; i < 30; i++) {
        cv::Mat result = stabilizer->process_frame(frames[i]);
        EXPECT_FALSE(result.empty());
    }

    auto metrics = stabilizer->get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 30);
}

// ============================================================================
// Mixed Motion Type Tests
// ============================================================================

TEST_F(IntegrationTest, MixedMotionSequence) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Create mixed motion sequence
    std::vector<cv::Mat> mixed_frames;

    // Static frames
    auto static_frames = TestDataGenerator::generate_test_sequence(10, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static");
    mixed_frames.insert(mixed_frames.end(), static_frames.begin(), static_frames.end());

    // Pan frames
    auto pan_frames = TestDataGenerator::generate_test_sequence(10, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "pan_right");
    mixed_frames.insert(mixed_frames.end(), pan_frames.begin(), pan_frames.end());

    // Zoom frames
    auto zoom_frames = TestDataGenerator::generate_test_sequence(10, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "zoom_in");
    mixed_frames.insert(mixed_frames.end(), zoom_frames.begin(), zoom_frames.end());

    // Shake frames
    auto shake_frames = TestDataGenerator::generate_test_sequence(10, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake");
    mixed_frames.insert(mixed_frames.end(), shake_frames.begin(), shake_frames.end());

    for (const auto& frame : mixed_frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    auto metrics = stabilizer->get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 40);
}

// ============================================================================
// Performance Metrics Integration Tests
// ============================================================================

TEST_F(IntegrationTest, PerformanceMetricsTracking) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        50, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );

    for (const auto& frame : frames) {
        stabilizer->process_frame(frame);
    }

    auto metrics = stabilizer->get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 50);
    EXPECT_GT(metrics.avg_processing_time, 0.0);
    EXPECT_LT(metrics.avg_processing_time, 1.0); // Should be less than 1 second
}

TEST_F(IntegrationTest, DisabledStabilizerPerformance) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    params.enabled = false;
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        50, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );

    auto start_time = std::chrono::high_resolution_clock::now();
    for (const auto& frame : frames) {
        stabilizer->process_frame(frame);
    }
    auto end_time = std::chrono::high_resolution_clock::now();

    auto disabled_duration = std::chrono::duration<double>(end_time - start_time).count();

    // With stabilizer enabled
    params.enabled = true;
    stabilizer->reset();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    start_time = std::chrono::high_resolution_clock::now();
    for (const auto& frame : frames) {
        stabilizer->process_frame(frame);
    }
    end_time = std::chrono::high_resolution_clock::now();

    auto enabled_duration = std::chrono::duration<double>(end_time - start_time).count();

    // Disabled should be faster
    EXPECT_LT(disabled_duration, enabled_duration);
}

// ============================================================================
// Transform History Tests
// ============================================================================

TEST_F(IntegrationTest, TransformHistoryManagement) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    params.smoothing_radius = 10;
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        20, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "pan_right"
    );

    for (const auto& frame : frames) {
        stabilizer->process_frame(frame);
    }

    auto transforms = stabilizer->get_current_transforms();
    EXPECT_LE(transforms.size(), params.smoothing_radius);
}

// ============================================================================
// Preset Integration Tests
// ============================================================================

TEST_F(IntegrationTest, GamingPresetIntegration) {
    StabilizerCore::StabilizerParams params = StabilizerCore::get_preset_gaming();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        30, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "fast"
    );

    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }
}

TEST_F(IntegrationTest, StreamingPresetIntegration) {
    StabilizerCore::StabilizerParams params = StabilizerCore::get_preset_streaming();
    ASSERT_TRUE(stabilizer->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        30, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, "static"
    );

    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }
}

TEST_F(IntegrationTest, RecordingPresetIntegration) {
    StabilizerCore::StabilizerParams params = StabilizerCore::get_preset_recording();
    ASSERT_TRUE(stabilizer->initialize(Resolution::HD_WIDTH, Resolution::HD_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        30, Resolution::HD_WIDTH, Resolution::HD_HEIGHT, "slow"
    );

    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }
}

// ============================================================================
// Error Recovery Tests
// ============================================================================

TEST_F(IntegrationTest, RecoverFromInvalidFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        10, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );

    // Process valid frames
    for (int i = 0; i < 5; i++) {
        cv::Mat result = stabilizer->process_frame(frames[i]);
        EXPECT_FALSE(result.empty());
    }

    // Process invalid frame (empty)
    cv::Mat empty_frame;
    cv::Mat result = stabilizer->process_frame(empty_frame);
    EXPECT_TRUE(result.empty());

    // Should recover and process valid frames
    for (int i = 5; i < 10; i++) {
        cv::Mat result = stabilizer->process_frame(frames[i]);
        EXPECT_FALSE(result.empty());
    }

    auto metrics = stabilizer->get_performance_metrics();
    EXPECT_GT(metrics.frame_count, 5);
}

TEST_F(IntegrationTest, RecoverFromBadInitialization) {
    StabilizerCore::StabilizerParams params = getDefaultParams();

    // Try to initialize with invalid dimensions
    bool initialized = stabilizer->initialize(0, 0, params);
    EXPECT_FALSE(initialized);

    // Should be able to recover with valid initialization
    initialized = stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params);
    EXPECT_TRUE(initialized);

    auto frames = TestDataGenerator::generate_test_sequence(
        10, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );

    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }
}
