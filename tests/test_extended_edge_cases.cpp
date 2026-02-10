/**
 * Extended Edge Case Tests
 *
 * This file contains additional edge case tests to increase test coverage to 80%+.
 * Tests cover edge cases in frame_utils, adaptive stabilization transitions,
 * error handling paths, and boundary conditions.
 */

#include <gtest/gtest.h>
#include "../src/core/stabilizer_core.hpp"
#include "../src/core/feature_detection.hpp"
#include "../src/core/motion_classifier.hpp"
#include "../src/core/adaptive_stabilizer.hpp"
#include "../src/core/frame_utils.hpp"
#include "../src/core/parameter_validation.hpp"
#include "test_constants.hpp"
#include "test_data_generator.hpp"
#include <opencv2/opencv.hpp>
#include <limits>

using namespace TestConstants;

// ============================================================================
// Frame Utils Edge Cases
// ============================================================================

class FrameUtilsEdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * Test: Observed frame with null data pointer
 */
TEST_F(FrameUtilsEdgeCaseTest, ObsToCVWithNullData) {
#ifdef HAVE_OBS_HEADERS
    obs_source_frame frame;
    frame.width = 640;
    frame.height = 480;
    frame.format = VIDEO_FORMAT_BGRA;
    frame.data[0] = nullptr;  // Null data pointer

    cv::Mat result = FRAME_UTILS::Conversion::obs_to_cv(&frame);
    EXPECT_TRUE(result.empty()) << "Null data pointer should return empty mat";
#endif
}

/**
 * Test: Frame with minimum dimensions (1x1)
 */
TEST_F(FrameUtilsEdgeCaseTest, MinDimensionFrame) {
    cv::Mat frame = TestDataGenerator::generate_test_frame(1, 1);
    EXPECT_FALSE(frame.empty());
    EXPECT_EQ(frame.cols, 1);
    EXPECT_EQ(frame.rows, 1);
}

/**
 * Test: Frame with maximum allowed dimensions
 */
TEST_F(FrameUtilsEdgeCaseTest, MaxDimensionFrame) {
    // Test near maximum allowed dimensions
    int width = static_cast<int>(FRAME_UTILS::MAX_FRAME_WIDTH) - 100;
    int height = static_cast<int>(FRAME_UTILS::MAX_FRAME_HEIGHT) - 100;

    cv::Mat frame = TestDataGenerator::generate_test_frame(width, height);
    EXPECT_FALSE(frame.empty());
    EXPECT_EQ(frame.cols, width);
    EXPECT_EQ(frame.rows, height);
}

/**
 * Test: Frame with dimensions exceeding maximum
 */
TEST_F(FrameUtilsEdgeCaseTest, ExceedsMaxDimensionFrame) {
#ifdef HAVE_OBS_HEADERS
    obs_source_frame frame;
    frame.width = FRAME_UTILS::MAX_FRAME_WIDTH + 1;
    frame.height = FRAME_UTILS::MAX_FRAME_HEIGHT + 1;
    frame.format = VIDEO_FORMAT_BGRA;
    frame.data[0] = nullptr;

    cv::Mat result = FRAME_UTILS::Conversion::obs_to_cv(&frame);
    EXPECT_TRUE(result.empty()) << "Frame exceeding max dimensions should return empty mat";
#endif
}

/**
 * Test: Frame with zero width
 */
TEST_F(FrameUtilsEdgeCaseTest, ZeroWidthFrame) {
    cv::Mat frame(0, 480, CV_8UC4);
    EXPECT_TRUE(frame.empty());
}

/**
 * Test: Frame with zero height
 */
TEST_F(FrameUtilsEdgeCaseTest, ZeroHeightFrame) {
    cv::Mat frame(640, 0, CV_8UC4);
    EXPECT_TRUE(frame.empty());
}

/**
 * Test: Color conversion for various formats
 */
TEST_F(FrameUtilsEdgeCaseTest, ColorConversionFormats) {
    cv::Mat bgra_frame = TestDataGenerator::generate_test_frame(640, 480);

    // Test BGRA to BGR conversion
    cv::Mat bgr_result = FRAME_UTILS::ColorConversion::convert_to_bgr(bgra_frame);
    EXPECT_FALSE(bgr_result.empty());
    EXPECT_EQ(bgr_result.type(), CV_8UC3);

    // Test to grayscale
    cv::Mat gray_result = FRAME_UTILS::ColorConversion::convert_to_grayscale(bgra_frame);
    EXPECT_FALSE(gray_result.empty());
    EXPECT_EQ(gray_result.type(), CV_8UC1);
}

/**
 * Test: Validation of invalid Mat
 */
TEST_F(FrameUtilsEdgeCaseTest, ValidateInvalidMat) {
    cv::Mat empty_frame;
    bool valid = FRAME_UTILS::Validation::validate_cv_mat(empty_frame);
    EXPECT_FALSE(valid) << "Empty Mat should be invalid";
}

/**
 * Test: Validation of Mat with zero dimensions
 */
TEST_F(FrameUtilsEdgeCaseTest, ValidateZeroDimensionMat) {
    cv::Mat zero_dim_frame(0, 0, CV_8UC4);
    bool valid = FRAME_UTILS::Validation::validate_cv_mat(zero_dim_frame);
    EXPECT_FALSE(valid) << "Zero dimension Mat should be invalid";
}

/**
 * Test: Validation of Mat with valid dimensions
 */
TEST_F(FrameUtilsEdgeCaseTest, ValidateValidMat) {
    cv::Mat valid_frame = TestDataGenerator::generate_test_frame(640, 480);
    bool valid = FRAME_UTILS::Validation::validate_cv_mat(valid_frame);
    EXPECT_TRUE(valid) << "Valid Mat should pass validation";
}

// ============================================================================
// Parameter Validation Edge Cases
// ============================================================================

class ParameterValidationEdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * Test: Smoothing radius at minimum boundary
 */
TEST_F(ParameterValidationEdgeCaseTest, SmoothingRadiusMinBoundary) {
    StabilizerCore::StabilizerParams params;
    params.smoothing_radius = 1;  // Minimum valid value

    auto validated = VALIDATION::validate_and_clamp_params(params);
    EXPECT_GE(validated.smoothing_radius, 1);
}

/**
 * Test: Smoothing radius at maximum boundary
 */
TEST_F(ParameterValidationEdgeCaseTest, SmoothingRadiusMaxBoundary) {
    StabilizerCore::StabilizerParams params;
    params.smoothing_radius = 1000;  // Exceeds reasonable max

    auto validated = VALIDATION::validate_and_clamp_params(params);
    EXPECT_LE(validated.smoothing_radius, 500);  // Should be clamped
}

/**
 * Test: Feature count at minimum boundary
 */
TEST_F(ParameterValidationEdgeCaseTest, FeatureCountMinBoundary) {
    StabilizerCore::StabilizerParams params;
    params.feature_count = Features::MIN_COUNT - 1;  // Below minimum

    auto validated = VALIDATION::validate_and_clamp_params(params);
    EXPECT_GE(validated.feature_count, Features::MIN_COUNT);
}

/**
 * Test: Feature count at maximum boundary
 */
TEST_F(ParameterValidationEdgeCaseTest, FeatureCountMaxBoundary) {
    StabilizerCore::StabilizerParams params;
    params.feature_count = Features::MAX_COUNT + 100;  // Above maximum

    auto validated = VALIDATION::validate_and_clamp_params(params);
    EXPECT_LE(validated.feature_count, Features::MAX_COUNT);
}

/**
 * Test: Quality level at boundary values
 */
TEST_F(ParameterValidationEdgeCaseTest, QualityLevelBoundaries) {
    StabilizerCore::StabilizerParams params;

    // Test zero quality level
    params.quality_level = 0.0f;
    auto validated1 = VALIDATION::validate_and_clamp_params(params);
    EXPECT_GT(validated1.quality_level, 0.0f);

    // Test quality level > 1.0
    params.quality_level = 1.5f;
    auto validated2 = VALIDATION::validate_and_clamp_params(params);
    EXPECT_LE(validated2.quality_level, 1.0f);
}

/**
 * Test: Max correction at boundary values
 */
TEST_F(ParameterValidationEdgeCaseTest, MaxCorrectionBoundaries) {
    StabilizerCore::StabilizerParams params;

    // Test negative max correction
    params.max_correction = -10.0f;
    auto validated1 = VALIDATION::validate_and_clamp_params(params);
    EXPECT_GE(validated1.max_correction, 0.0f);

    // Test very large max correction
    params.max_correction = 1000.0f;
    auto validated2 = VALIDATION::validate_and_clamp_params(params);
    EXPECT_LE(validated2.max_correction, 200.0f);  // Reasonable upper bound
}

/**
 * Test: Invalid smoothing radius (zero)
 */
TEST_F(ParameterValidationEdgeCaseTest, InvalidSmoothingRadiusZero) {
    StabilizerCore::StabilizerParams params;
    params.smoothing_radius = 0;

    auto validated = VALIDATION::validate_and_clamp_params(params);
    EXPECT_GT(validated.smoothing_radius, 0);
}

/**
 * Test: Invalid feature count (zero)
 */
TEST_F(ParameterValidationEdgeCaseTest, InvalidFeatureCountZero) {
    StabilizerCore::StabilizerParams params;
    params.feature_count = 0;

    auto validated = VALIDATION::validate_and_clamp_params(params);
    EXPECT_GT(validated.feature_count, 0);
}

// ============================================================================
// Adaptive Stabilization Transition Tests
// ============================================================================

class AdaptiveStabilizationEdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        stabilizer = std::make_unique<StabilizerCore>();
    }

    void TearDown() override {
        stabilizer.reset();
    }

    std::unique_ptr<StabilizerCore> stabilizer;
};

/**
 * Test: Transition from static to shake motion
 */
TEST_F(AdaptiveStabilizationEdgeCaseTest, StaticToShakeTransition) {
    StabilizerCore::StabilizerParams params = StabilizerCore::get_preset_streaming();
    params.adaptive_enabled = true;
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Process static frames
    auto static_frames = TestDataGenerator::generate_test_sequence(
        20, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );
    for (const auto& frame : static_frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    // Process shake frames (transition)
    auto shake_frames = TestDataGenerator::generate_test_sequence(
        20, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
    );
    for (const auto& frame : shake_frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    // Should not crash or produce empty results during transition
    auto metrics = stabilizer->get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 40);
}

/**
 * Test: Transition from pan to shake motion
 */
TEST_F(AdaptiveStabilizationEdgeCaseTest, PanToShakeTransition) {
    StabilizerCore::StabilizerParams params = StabilizerCore::get_preset_streaming();
    params.adaptive_enabled = true;
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Process pan frames
    auto pan_frames = TestDataGenerator::generate_test_sequence(
        20, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "pan_right"
    );
    for (const auto& frame : pan_frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    // Process shake frames (transition)
    auto shake_frames = TestDataGenerator::generate_test_sequence(
        20, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
    );
    for (const auto& frame : shake_frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    auto metrics = stabilizer->get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 40);
}

/**
 * Test: Rapid motion type changes
 */
TEST_F(AdaptiveStabilizationEdgeCaseTest, RapidMotionTypeChanges) {
    StabilizerCore::StabilizerParams params = StabilizerCore::get_preset_streaming();
    params.adaptive_enabled = true;
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Rapidly switch between motion types
    std::vector<std::string> motion_types = {"static", "shake", "pan_right", "shake", "static", "fast"};

    for (const auto& motion_type : motion_types) {
        auto frames = TestDataGenerator::generate_test_sequence(
            5, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, motion_type
        );
        for (const auto& frame : frames) {
            cv::Mat result = stabilizer->process_frame(frame);
            EXPECT_FALSE(result.empty());
        }
    }

    auto metrics = stabilizer->get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 30);  // 6 types * 5 frames
}

/**
 * Test: Adaptive mode with disabled stabilization
 */
TEST_F(AdaptiveStabilizationEdgeCaseTest, AdaptiveWithDisabledStabilizer) {
    StabilizerCore::StabilizerParams params = StabilizerCore::get_preset_streaming();
    params.adaptive_enabled = true;
    params.enabled = false;  // Disabled but adaptive is enabled
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        20, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
    );

    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    // With disabled stabilizer, frames should pass through
    auto metrics = stabilizer->get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 20);
}

// ============================================================================
// Error Handling Path Tests
// ============================================================================

class ErrorHandlingEdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        stabilizer = std::make_unique<StabilizerCore>();
    }

    void TearDown() override {
        stabilizer.reset();
    }

    std::unique_ptr<StabilizerCore> stabilizer;
};

/**
 * Test: Process frame before initialization
 */
TEST_F(ErrorHandlingEdgeCaseTest, ProcessBeforeInitialization) {
    cv::Mat frame = TestDataGenerator::generate_test_frame(640, 480);
    cv::Mat result = stabilizer->process_frame(frame);

    // Should return empty or pass through frame without crashing
    EXPECT_FALSE(result.empty()) << "Process before init should handle gracefully";
}

/**
 * Test: Reinitialize without reset
 */
TEST_F(ErrorHandlingEdgeCaseTest, ReinitializeWithoutReset) {
    auto params = StabilizerCore::get_preset_streaming();

    // First initialization
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Second initialization without reset (may or may not fail)
    // The implementation should handle this gracefully
    bool result = stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params);
    // Either should succeed or fail gracefully without crashing

    auto frames = TestDataGenerator::generate_test_sequence(
        10, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );

    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }
}

/**
 * Test: Update parameters before initialization
 */
TEST_F(ErrorHandlingEdgeCaseTest, UpdateParamsBeforeInitialization) {
    auto params = StabilizerCore::get_preset_streaming();

    // Should handle gracefully without crashing
    stabilizer->update_parameters(params);

    // Now initialize
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        10, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );

    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }
}

/**
 * Test: Get performance metrics before processing
 */
TEST_F(ErrorHandlingEdgeCaseTest, GetMetricsBeforeProcessing) {
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT,
                                       StabilizerCore::get_preset_streaming()));

    auto metrics = stabilizer->get_performance_metrics();

    // Should return zero or valid default values without crashing
    EXPECT_EQ(metrics.frame_count, 0);
    EXPECT_EQ(metrics.avg_processing_time, 0.0);
}

/**
 * Test: Reset without initialization
 */
TEST_F(ErrorHandlingEdgeCaseTest, ResetWithoutInitialization) {
    // Should handle gracefully without crashing
    stabilizer->reset();

    // Now initialize and use normally
    auto params = StabilizerCore::get_preset_streaming();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        10, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );

    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }
}

// ============================================================================
// Feature Detection Edge Cases
// ============================================================================

class FeatureDetectionEdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * Test: Feature detection on blank (uniform color) frame
 */
TEST_F(FeatureDetectionEdgeCaseTest, UniformColorFrame) {
    cv::Mat uniform_frame(640, 480, CV_8UC4, cv::Scalar(128, 128, 128, 255));

    StabilizerCore stabilizer;
    auto params = StabilizerCore::get_preset_streaming();
    ASSERT_TRUE(stabilizer.initialize(640, 480, params));

    // Should handle uniform color frame without crashing
    cv::Mat result = stabilizer.process_frame(uniform_frame);
    EXPECT_FALSE(result.empty());
}

/**
 * Test: Feature detection on very dark frame
 */
TEST_F(FeatureDetectionEdgeCaseTest, VeryDarkFrame) {
    cv::Mat dark_frame(640, 480, CV_8UC4, cv::Scalar(10, 10, 10, 255));

    StabilizerCore stabilizer;
    auto params = StabilizerCore::get_preset_streaming();
    ASSERT_TRUE(stabilizer.initialize(640, 480, params));

    cv::Mat result = stabilizer.process_frame(dark_frame);
    EXPECT_FALSE(result.empty());
}

/**
 * Test: Feature detection on very bright frame
 */
TEST_F(FeatureDetectionEdgeCaseTest, VeryBrightFrame) {
    cv::Mat bright_frame(640, 480, CV_8UC4, cv::Scalar(245, 245, 245, 255));

    StabilizerCore stabilizer;
    auto params = StabilizerCore::get_preset_streaming();
    ASSERT_TRUE(stabilizer.initialize(640, 480, params));

    cv::Mat result = stabilizer.process_frame(bright_frame);
    EXPECT_FALSE(result.empty());
}

/**
 * Test: Feature detection on noisy frame
 */
TEST_F(FeatureDetectionEdgeCaseTest, NoisyFrame) {
    cv::Mat noisy_frame(640, 480, CV_8UC4);
    cv::randu(noisy_frame, cv::Scalar(0, 0, 0, 0), cv::Scalar(255, 255, 255, 255));

    StabilizerCore stabilizer;
    auto params = StabilizerCore::get_preset_streaming();
    ASSERT_TRUE(stabilizer.initialize(640, 480, params));

    cv::Mat result = stabilizer.process_frame(noisy_frame);
    EXPECT_FALSE(result.empty());
}

// ============================================================================
// Boundary Condition Tests
// ============================================================================

class BoundaryConditionTest : public ::testing::Test {
protected:
    void SetUp() override {
        stabilizer = std::make_unique<StabilizerCore>();
    }

    void TearDown() override {
        stabilizer.reset();
    }

    std::unique_ptr<StabilizerCore> stabilizer;
};

/**
 * Test: Process single frame
 */
TEST_F(BoundaryConditionTest, SingleFrame) {
    auto params = StabilizerCore::get_preset_streaming();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    cv::Mat frame = TestDataGenerator::generate_test_frame(640, 480);
    cv::Mat result = stabilizer->process_frame(frame);
    EXPECT_FALSE(result.empty());

    auto metrics = stabilizer->get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 1);
}

/**
 * Test: Process two frames (minimum for optical flow)
 */
TEST_F(BoundaryConditionTest, TwoFrames) {
    auto params = StabilizerCore::get_preset_streaming();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        2, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );

    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    auto metrics = stabilizer->get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 2);
}

/**
 * Test: Smoothing radius equals number of frames
 */
TEST_F(BoundaryConditionTest, SmoothingRadiusEqualsFrameCount) {
    auto params = StabilizerCore::get_preset_streaming();
    params.smoothing_radius = 10;  // Exactly 10 frames
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        10, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );

    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    auto metrics = stabilizer->get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 10);
}

/**
 * Test: Smoothing radius larger than frame count
 */
TEST_F(BoundaryConditionTest, SmoothingRadiusLargerThanFrameCount) {
    auto params = StabilizerCore::get_preset_streaming();
    params.smoothing_radius = 100;  // More than we'll process
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        10, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );

    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    auto metrics = stabilizer->get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 10);
}

/**
 * Test: Maximum correction at zero
 */
TEST_F(BoundaryConditionTest, ZeroMaxCorrection) {
    auto params = StabilizerCore::get_preset_streaming();
    params.max_correction = 0.0f;  // No correction allowed
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        20, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
    );

    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    // Should still process frames without crashing
    auto metrics = stabilizer->get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 20);
}

/**
 * Test: Very large max correction
 */
TEST_F(BoundaryConditionTest, VeryLargeMaxCorrection) {
    auto params = StabilizerCore::get_preset_streaming();
    params.max_correction = 1000.0f;  // Very large
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        20, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "shake"
    );

    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }

    auto metrics = stabilizer->get_performance_metrics();
    EXPECT_EQ(metrics.frame_count, 20);
}
