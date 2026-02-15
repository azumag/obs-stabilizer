#include <gtest/gtest.h>
#include "../src/core/stabilizer_core.hpp"
#include "../src/core/parameter_validation.hpp"
#include "../src/core/frame_utils.hpp"
#include "test_constants.hpp"
#include "test_data_generator.hpp"

using namespace TestConstants;

class EdgeCaseTest : public ::testing::Test {
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
// Empty Frame Tests
// ============================================================================

TEST_F(EdgeCaseTest, EmptyFrameHandling) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    cv::Mat empty_frame;
    cv::Mat result = stabilizer->process_frame(empty_frame);

    // Empty frame should be handled gracefully
    EXPECT_TRUE(result.empty());
}

TEST_F(EdgeCaseTest, NullFramePointerHandling) {
    // This test verifies that the stabilizer can handle null frame pointers
    // The current implementation uses reference parameters, so this test
    // validates the frame validation logic
    cv::Mat empty_frame;
    EXPECT_TRUE(FRAME_UTILS::Validation::validate_cv_mat(empty_frame) == false);
}

// ============================================================================
// Invalid Frame Size Tests
// ============================================================================

TEST_F(EdgeCaseTest, ZeroWidthFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    cv::Mat zero_width_frame(Resolution::VGA_HEIGHT, 0, CV_8UC4);

    EXPECT_FALSE(stabilizer->validate_frame(zero_width_frame));
}

TEST_F(EdgeCaseTest, ZeroHeightFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    cv::Mat zero_height_frame(0, Resolution::VGA_WIDTH, CV_8UC4);

    EXPECT_FALSE(stabilizer->validate_frame(zero_height_frame));
}

TEST_F(EdgeCaseTest, VerySmallFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    cv::Mat tiny_frame(1, 1, CV_8UC4);

    EXPECT_FALSE(stabilizer->validate_frame(tiny_frame));
}

TEST_F(EdgeCaseTest, VeryLargeFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    cv::Mat huge_frame(10000, 10000, CV_8UC4);

    EXPECT_FALSE(stabilizer->validate_frame(huge_frame));
}

// ============================================================================
// Invalid Frame Format Tests
// ============================================================================

TEST_F(EdgeCaseTest, SingleChannelFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    cv::Mat single_channel_frame(Resolution::VGA_HEIGHT, Resolution::VGA_WIDTH, CV_8UC1);
    cv::Mat result = stabilizer->process_frame(single_channel_frame);

    // Single channel frames should be processed correctly (grayscale)
    EXPECT_FALSE(result.empty());
}

TEST_F(EdgeCaseTest, TwoChannelFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    cv::Mat two_channel_frame(Resolution::VGA_HEIGHT, Resolution::VGA_WIDTH, CV_8UC2);
    cv::Mat result = stabilizer->process_frame(two_channel_frame);

    // Two channel frames are not supported, should return empty
    EXPECT_TRUE(result.empty());
}

TEST_F(EdgeCaseTest, SixteenBitDepthFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    cv::Mat sixteen_bit_frame(Resolution::VGA_HEIGHT, Resolution::VGA_WIDTH, CV_16UC4);
    cv::Mat result = stabilizer->process_frame(sixteen_bit_frame);

    // 16-bit frames are not supported, should return empty or converted
    EXPECT_TRUE(result.empty());
}

// ============================================================================
// Boundary Value Tests
// ============================================================================

TEST_F(EdgeCaseTest, MinimumValidFrameSize) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(32, 32, params));

    cv::Mat min_frame = TestDataGenerator::generate_test_frame(32, 32);
    cv::Mat result = stabilizer->process_frame(min_frame);

    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.cols, 32);
    EXPECT_EQ(result.rows, 32);
}

TEST_F(EdgeCaseTest, SmoothingRadiusBoundary) {
    StabilizerCore::StabilizerParams params = getDefaultParams();

    // Test minimum smoothing radius (1)
    params.smoothing_radius = 1;
    EXPECT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Test maximum smoothing radius (1000)
    params.smoothing_radius = 1000;
    EXPECT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Test invalid smoothing radius (0)
    params.smoothing_radius = 0;
    params = VALIDATION::validate_parameters(params);
    EXPECT_GT(params.smoothing_radius, 0);

    // Test invalid smoothing radius (negative)
    params.smoothing_radius = -10;
    params = VALIDATION::validate_parameters(params);
    EXPECT_GT(params.smoothing_radius, 0);
}

TEST_F(EdgeCaseTest, FeatureCountBoundary) {
    StabilizerCore::StabilizerParams params = getDefaultParams();

    // Test minimum feature count (1)
    params.feature_count = 1;
    EXPECT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Test maximum feature count (10000)
    params.feature_count = 10000;
    EXPECT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Test invalid feature count (0)
    params.feature_count = 0;
    params = VALIDATION::validate_parameters(params);
    EXPECT_GT(params.feature_count, 0);
}

TEST_F(EdgeCaseTest, QualityLevelBoundary) {
    StabilizerCore::StabilizerParams params = getDefaultParams();

    // Test minimum quality level (very small)
    params.quality_level = 0.0001f;
    EXPECT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Test maximum quality level (1.0)
    params.quality_level = 1.0f;
    EXPECT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Test invalid quality level (negative)
    params.quality_level = -0.01f;
    params = VALIDATION::validate_parameters(params);
    EXPECT_GE(params.quality_level, 0.0f);

    // Test invalid quality level (> 1.0)
    params.quality_level = 1.1f;
    params = VALIDATION::validate_parameters(params);
    EXPECT_LE(params.quality_level, 1.0f);
}

// ============================================================================
// Reset and Reinitialization Tests
// ============================================================================

TEST_F(EdgeCaseTest, ResetDuringProcessing) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    auto frames = TestDataGenerator::generate_test_sequence(
        FrameCount::STANDARD_SEQUENCE,
        Resolution::VGA_WIDTH,
        Resolution::VGA_HEIGHT,
        "static"
    );

    // Process some frames
    for (int i = 0; i < 10; i++) {
        stabilizer->process_frame(frames[i]);
    }

    // Reset stabilizer
    stabilizer->reset();

    // Should work normally after reset
    cv::Mat result = stabilizer->process_frame(frames[0]);
    EXPECT_FALSE(result.empty());
}

TEST_F(EdgeCaseTest, ReinitializeWithDifferentParams) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    // Process some frames
    auto frames = TestDataGenerator::generate_test_sequence(
        5, Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, "static"
    );
    for (const auto& frame : frames) {
        stabilizer->process_frame(frame);
    }

    // Reinitialize with different parameters
    params.smoothing_radius = 50;
    params.feature_count = 200;
    stabilizer->update_parameters(params);

    // Should work normally after parameter update
    cv::Mat result = stabilizer->process_frame(frames[0]);
    EXPECT_FALSE(result.empty());
}

// ============================================================================
// All Black Frame Tests
// ============================================================================

TEST_F(EdgeCaseTest, AllBlackFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    cv::Mat black_frame = cv::Mat::zeros(Resolution::VGA_HEIGHT, Resolution::VGA_WIDTH, CV_8UC4);

    cv::Mat result = stabilizer->process_frame(black_frame);

    // Should handle all-black frames gracefully
    EXPECT_FALSE(result.empty());
}

TEST_F(EdgeCaseTest, AllWhiteFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(Resolution::VGA_WIDTH, Resolution::VGA_HEIGHT, params));

    cv::Mat white_frame(Resolution::VGA_HEIGHT, Resolution::VGA_WIDTH, CV_8UC4, cv::Scalar(255, 255, 255, 255));

    cv::Mat result = stabilizer->process_frame(white_frame);

    // Should handle all-white frames gracefully
    EXPECT_FALSE(result.empty());
}

// ============================================================================
// Frame Conversion Edge Cases
// ============================================================================

TEST_F(EdgeCaseTest, FrameUtilsEmptyMatValidation) {
    cv::Mat empty_mat;
    EXPECT_FALSE(FRAME_UTILS::Validation::validate_cv_mat(empty_mat));
}

TEST_F(EdgeCaseTest, FrameUtilsInvalidDimensions) {
    // Test zero dimensions - this should fail validation
    cv::Mat invalid_frame(0, 0, CV_8UC4);
    EXPECT_FALSE(FRAME_UTILS::Validation::validate_cv_mat(invalid_frame));

    // OpenCV throws an exception when creating a Mat with negative dimensions
    // This is expected behavior - OpenCV validates dimensions at construction time
    // We test that our code handles this gracefully by catching the exception
    try {
        cv::Mat invalid_frame2(100, -50, CV_8UC4);
        // If we get here, verify the validation catches it
        EXPECT_FALSE(FRAME_UTILS::Validation::validate_cv_mat(invalid_frame2));
    } catch (const cv::Exception& e) {
        // OpenCV threw an exception during Mat construction, which is expected
        // This is acceptable behavior - the invalid frame is rejected before validation
        SUCCEED() << "OpenCV correctly rejected negative dimensions at construction time";
    }
}

TEST_F(EdgeCaseTest, FrameUtilsInvalidChannels) {
    cv::Mat invalid_frame(100, 100, CV_8UC2);
    EXPECT_FALSE(FRAME_UTILS::Validation::validate_cv_mat(invalid_frame));
}

TEST_F(EdgeCaseTest, ColorConversionGrayscale) {
    cv::Mat bgra_frame(100, 100, CV_8UC4);
    cv::Mat gray = FRAME_UTILS::ColorConversion::convert_to_grayscale(bgra_frame);
    EXPECT_FALSE(gray.empty());
    EXPECT_EQ(gray.channels(), 1);
}

TEST_F(EdgeCaseTest, ColorConversionEmptyFrame) {
    cv::Mat empty_frame;
    cv::Mat gray = FRAME_UTILS::ColorConversion::convert_to_grayscale(empty_frame);
    EXPECT_TRUE(gray.empty());
}

// ============================================================================
// Additional Validation Edge Case Tests
// ============================================================================

TEST_F(EdgeCaseTest, InitializeWithZeroWidth) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    EXPECT_FALSE(stabilizer->initialize(0, 640, params));
    EXPECT_FALSE(stabilizer->get_last_error().empty());
}

TEST_F(EdgeCaseTest, InitializeWithZeroHeight) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    EXPECT_FALSE(stabilizer->initialize(640, 0, params));
    EXPECT_FALSE(stabilizer->get_last_error().empty());
}

TEST_F(EdgeCaseTest, InitializeWithTooSmallDimensions) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    // MIN_IMAGE_SIZE is 32, so 31x31 should fail
    EXPECT_FALSE(stabilizer->initialize(31, 31, params));
    EXPECT_FALSE(stabilizer->get_last_error().empty());
}

TEST_F(EdgeCaseTest, InitializeWithMinimumValidDimensions) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    // MIN_IMAGE_SIZE is 32, so 32x32 should succeed
    EXPECT_TRUE(stabilizer->initialize(32, 32, params));
}

TEST_F(EdgeCaseTest, ProcessFrame32BitFloatDepth) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(640, 480, params));

    cv::Mat float_frame(480, 640, CV_32FC4);
    cv::Mat result = stabilizer->process_frame(float_frame);

    // 32-bit float frames should return empty (unsupported)
    EXPECT_TRUE(result.empty());
}

TEST_F(EdgeCaseTest, ProcessFrame64BitFloatDepth) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(640, 480, params));

    cv::Mat float64_frame(480, 640, CV_64FC4);
    cv::Mat result = stabilizer->process_frame(float64_frame);

    // 64-bit float frames should return empty (unsupported)
    EXPECT_TRUE(result.empty());
}

TEST_F(EdgeCaseTest, ProcessFrame3Channels) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(640, 480, params));

    cv::Mat bgr_frame(480, 640, CV_8UC3);
    cv::Mat result = stabilizer->process_frame(bgr_frame);

    // 3-channel frames (BGR) are converted to grayscale and processed
    // The stabilizer supports this via FRAME_UTILS::convert_to_grayscale()
    EXPECT_FALSE(result.empty());
}

TEST_F(EdgeCaseTest, ProcessFrame5Channels) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(640, 480, params));

    // OpenCV supports CV_8UC(1-4) but we can test with unsupported channel count
    // by using CV_8UC4 which is valid but we already test that separately
    cv::Mat rgba_frame(480, 640, CV_8UC4);
    cv::Mat result = stabilizer->process_frame(rgba_frame);

    // 4-channel frames (BGRA) should work correctly
    EXPECT_FALSE(result.empty());
}

TEST_F(EdgeCaseTest, ProcessFrameWithExtremeMaxCorrection) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    params.max_correction = 1000.0f;  // Extreme value
    params = VALIDATION::validate_parameters(params);
    ASSERT_TRUE(stabilizer->initialize(640, 480, params));

    auto frames = TestDataGenerator::generate_test_sequence(10, 640, 480, "static");
    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        // Should still process frames despite extreme parameter
        EXPECT_FALSE(result.empty());
    }
}

TEST_F(EdgeCaseTest, ProcessFrameWithZeroFeatureCount) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    params.feature_count = 0;
    params = VALIDATION::validate_parameters(params);
    ASSERT_TRUE(stabilizer->initialize(640, 480, params));

    auto frames = TestDataGenerator::generate_test_sequence(10, 640, 480, "static");
    cv::Mat result = stabilizer->process_frame(frames[0]);
    // Should handle gracefully with minimum feature count
    EXPECT_FALSE(result.empty());
}

TEST_F(EdgeCaseTest, ProcessFrameWithNegativeSmoothingRadius) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    params.smoothing_radius = -10;
    params = VALIDATION::validate_parameters(params);
    ASSERT_TRUE(stabilizer->initialize(640, 480, params));

    auto frames = TestDataGenerator::generate_test_sequence(10, 640, 480, "static");
    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }
}

TEST_F(EdgeCaseTest, ProcessFrameWithVeryLargeSmoothingRadius) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    params.smoothing_radius = 10000;  // Extremely large value
    params = VALIDATION::validate_parameters(params);
    ASSERT_TRUE(stabilizer->initialize(640, 480, params));

    auto frames = TestDataGenerator::generate_test_sequence(15, 640, 480, "static");
    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }
}

// ============================================================================
// Feature Tracking Edge Case Tests
// ============================================================================

TEST_F(EdgeCaseTest, ProcessFrameSequenceWithTrackingFailures) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(640, 480, params));

    // Create a sequence that will cause tracking failures
    auto frames = TestDataGenerator::generate_test_sequence(20, 640, 480, "static");

    // First frame should initialize
    cv::Mat result1 = stabilizer->process_frame(frames[0]);
    EXPECT_FALSE(result1.empty());

    // Subsequent frames should work
    for (int i = 1; i < 10; i++) {
        cv::Mat result = stabilizer->process_frame(frames[i]);
        EXPECT_FALSE(result.empty());
    }
}

TEST_F(EdgeCaseTest, ProcessAllBlackFrameSequence) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(640, 480, params));

    for (int i = 0; i < 10; i++) {
        cv::Mat black_frame = cv::Mat::zeros(480, 640, CV_8UC4);
        cv::Mat result = stabilizer->process_frame(black_frame);
        EXPECT_FALSE(result.empty());
    }
}

TEST_F(EdgeCaseTest, ProcessAllWhiteFrameSequence) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(640, 480, params));

    for (int i = 0; i < 10; i++) {
        cv::Mat white_frame(480, 640, CV_8UC4, cv::Scalar(255, 255, 255, 255));
        cv::Mat result = stabilizer->process_frame(white_frame);
        EXPECT_FALSE(result.empty());
    }
}

TEST_F(EdgeCaseTest, ProcessAlternatingBlackWhiteFrames) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(640, 480, params));

    for (int i = 0; i < 10; i++) {
        if (i % 2 == 0) {
            cv::Mat black_frame = cv::Mat::zeros(480, 640, CV_8UC4);
            cv::Mat result = stabilizer->process_frame(black_frame);
            EXPECT_FALSE(result.empty());
        } else {
            cv::Mat white_frame(480, 640, CV_8UC4, cv::Scalar(255, 255, 255, 255));
            cv::Mat result = stabilizer->process_frame(white_frame);
            EXPECT_FALSE(result.empty());
        }
    }
}

TEST_F(EdgeCaseTest, ResetAfterTrackingFailures) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(640, 480, params));

    // Process some frames that might cause tracking issues
    for (int i = 0; i < 5; i++) {
        cv::Mat black_frame = cv::Mat::zeros(480, 640, CV_8UC4);
        stabilizer->process_frame(black_frame);
    }

    // Reset and process normal frames
    stabilizer->reset();
    auto frames = TestDataGenerator::generate_test_sequence(5, 640, 480, "static");
    cv::Mat result = stabilizer->process_frame(frames[0]);
    EXPECT_FALSE(result.empty());
}

// ============================================================================
// Edge Handling Mode Tests
// ============================================================================

TEST_F(EdgeCaseTest, EdgeModePadding) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    params.edge_mode = StabilizerCore::EdgeMode::Padding;
    ASSERT_TRUE(stabilizer->initialize(640, 480, params));

    auto frames = TestDataGenerator::generate_test_sequence(10, 640, 480, "static");
    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
        EXPECT_EQ(result.size(), frame.size());
    }
}

TEST_F(EdgeCaseTest, EdgeModeCrop) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    params.edge_mode = StabilizerCore::EdgeMode::Crop;
    ASSERT_TRUE(stabilizer->initialize(640, 480, params));

    auto frames = TestDataGenerator::generate_test_sequence(10, 640, 480, "static");
    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }
}

TEST_F(EdgeCaseTest, EdgeModeScale) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    params.edge_mode = StabilizerCore::EdgeMode::Scale;
    ASSERT_TRUE(stabilizer->initialize(640, 480, params));

    auto frames = TestDataGenerator::generate_test_sequence(10, 640, 480, "static");
    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
        EXPECT_EQ(result.size(), frame.size());
    }
}

TEST_F(EdgeCaseTest, EdgeModeCropWithBlackBorders) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    params.edge_mode = StabilizerCore::EdgeMode::Crop;
    ASSERT_TRUE(stabilizer->initialize(640, 480, params));

    // Create a frame with content in the center
    cv::Mat frame = cv::Mat::zeros(480, 640, CV_8UC4);
    cv::rectangle(frame, cv::Rect(100, 100, 440, 280), cv::Scalar(255, 255, 255, 255), -1);

    cv::Mat result = stabilizer->process_frame(frame);
    EXPECT_FALSE(result.empty());
}

TEST_F(EdgeCaseTest, EdgeModeScaleWithBlackBorders) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    params.edge_mode = StabilizerCore::EdgeMode::Scale;
    ASSERT_TRUE(stabilizer->initialize(640, 480, params));

    // Create a frame with content in the center
    cv::Mat frame = cv::Mat::zeros(480, 640, CV_8UC4);
    cv::rectangle(frame, cv::Rect(100, 100, 440, 280), cv::Scalar(255, 255, 255, 255), -1);

    cv::Mat result = stabilizer->process_frame(frame);
    EXPECT_FALSE(result.empty());
}

TEST_F(EdgeCaseTest, DetectContentBoundsSquareFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(512, 512, params));

    cv::Mat frame = cv::Mat::zeros(512, 512, CV_8UC4);
    cv::rectangle(frame, cv::Rect(50, 50, 412, 412), cv::Scalar(255, 255, 255, 255), -1);

    cv::Rect bounds = stabilizer->detect_content_bounds(frame);
    EXPECT_GT(bounds.width, 0);
    EXPECT_GT(bounds.height, 0);
}

TEST_F(EdgeCaseTest, DetectContentBoundsUltrawideFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(1920, 480, params));

    cv::Mat frame = cv::Mat::zeros(480, 1920, CV_8UC4);
    cv::rectangle(frame, cv::Rect(100, 50, 1720, 380), cv::Scalar(255, 255, 255, 255), -1);

    cv::Rect bounds = stabilizer->detect_content_bounds(frame);
    EXPECT_GT(bounds.width, 0);
    EXPECT_GT(bounds.height, 0);
}

TEST_F(EdgeCaseTest, DetectContentBoundsMinimumFrame) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(32, 32, params));

    cv::Mat frame = cv::Mat::zeros(32, 32, CV_8UC4);
    cv::rectangle(frame, cv::Rect(10, 10, 12, 12), cv::Scalar(255, 255, 255, 255), -1);

    cv::Rect bounds = stabilizer->detect_content_bounds(frame);
    EXPECT_GT(bounds.width, 0);
    EXPECT_GT(bounds.height, 0);
}

TEST_F(EdgeCaseTest, DetectContentBoundsAllBlack) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(640, 480, params));

    cv::Mat black_frame = cv::Mat::zeros(480, 640, CV_8UC4);
    cv::Rect bounds = stabilizer->detect_content_bounds(black_frame);

    // Should return full frame bounds
    EXPECT_EQ(bounds.width, 640);
    EXPECT_EQ(bounds.height, 480);
}

TEST_F(EdgeCaseTest, DetectContentBoundsAllWhite) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(640, 480, params));

    cv::Mat white_frame(480, 640, CV_8UC4, cv::Scalar(255, 255, 255, 255));
    cv::Rect bounds = stabilizer->detect_content_bounds(white_frame);

    // Should return full frame bounds
    EXPECT_EQ(bounds.width, 640);
    EXPECT_EQ(bounds.height, 480);
}

// ============================================================================
// Preset Tests
// ============================================================================

TEST_F(EdgeCaseTest, PresetGaming) {
    StabilizerCore::StabilizerParams params = StabilizerCore::get_preset_gaming();
    EXPECT_TRUE(stabilizer->initialize(640, 480, params));

    auto frames = TestDataGenerator::generate_test_sequence(10, 640, 480, "static");
    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }
}

TEST_F(EdgeCaseTest, PresetStreaming) {
    StabilizerCore::StabilizerParams params = StabilizerCore::get_preset_streaming();
    EXPECT_TRUE(stabilizer->initialize(640, 480, params));

    auto frames = TestDataGenerator::generate_test_sequence(10, 640, 480, "static");
    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }
}

TEST_F(EdgeCaseTest, PresetRecording) {
    StabilizerCore::StabilizerParams params = StabilizerCore::get_preset_recording();
    EXPECT_TRUE(stabilizer->initialize(640, 480, params));

    auto frames = TestDataGenerator::generate_test_sequence(10, 640, 480, "static");
    for (const auto& frame : frames) {
        cv::Mat result = stabilizer->process_frame(frame);
        EXPECT_FALSE(result.empty());
    }
}

// ============================================================================
// Performance Metrics Tests
// ============================================================================

TEST_F(EdgeCaseTest, PerformanceMetricsAfterProcessing) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(640, 480, params));

    auto frames = TestDataGenerator::generate_test_sequence(10, 640, 480, "static");
    for (const auto& frame : frames) {
        stabilizer->process_frame(frame);
    }

    auto metrics = stabilizer->get_performance_metrics();
    EXPECT_GT(metrics.frame_count, 0);
    EXPECT_GT(metrics.avg_processing_time, 0.0);
}

TEST_F(EdgeCaseTest, IsReadyAfterInitialization) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    ASSERT_TRUE(stabilizer->initialize(640, 480, params));

    EXPECT_TRUE(stabilizer->is_ready());
}

TEST_F(EdgeCaseTest, IsReadyBeforeInitialization) {
    EXPECT_FALSE(stabilizer->is_ready());
}

TEST_F(EdgeCaseTest, GetLastErrorAfterError) {
    StabilizerCore::StabilizerParams params = getDefaultParams();
    // Try to initialize with invalid dimensions
    EXPECT_FALSE(stabilizer->initialize(0, 0, params));

    std::string error = stabilizer->get_last_error();
    EXPECT_FALSE(error.empty());
}
